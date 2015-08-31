#include <cstdlib>
#include <sstream>
#include <string>
#include "bcv_io.h"
#include "hog.h"
#include "bcv_bbox.h"
#include "bcv_detectionutils.h"

#include <linear.h> // liblinear
#include <gflags/gflags.h>

using namespace std;
using namespace bcv;

DEFINE_string(input, "/home/vasiliy/data/INRIAPerson/train_64x128_H96/pos.lst","input data");
DEFINE_string(output, "out.jpg","output image");
DEFINE_int32(cell_size, 8, "HOG cell size");
DEFINE_int32(num_orientations, 9, "HOG num orientations");
DEFINE_double(svm_eps, 1e-3, "SVM epsilon");
DEFINE_string(svm_model_fname, "svm_hog_model.dat", "SVM model data");
DEFINE_double(svm_detection_threshold, 0.99999, "svm detection threshold");
DEFINE_double(stride_init, 16, "detection window stride");
DEFINE_double(scaling, 0.9, "scaling factor for multiscale");
DEFINE_double(nms_overlap_threshold, 0.8, "nonmaximum suppression overlap threshold");

void compute_descriptors(const vector<string>& files);
void create_svm_problem(problem& svm_problem, int nexamples, int dim);
void destroy_svm_problem(problem& svm_problem);
void hog_to_feature_node(const vector<float>& data, feature_node* out);
 
void train_svm_model(const vector<string>& pos_files, 
                     const vector<string>& neg_files); 
void test_svm_model(const vector<string>& files, double prob);

vector<bbox_scored> multiscale_detect_in_image(const model* svm_model, double svm_threshold, 
                                const vector<float>& img, int rows, int cols);

vector<bbox_scored> detect_in_image(const model* svm_model, double svm_threshold, int stride, 
                            const vector<float>& img, int rows, int cols);

// window size for training 
const int IMG_ROWS = 160;
const int IMG_COLS = 96;

int main(int argc, char** argv) {
    gflags::SetUsageMessage("HOG example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    string base_path("/home/vasiliy/data/INRIAPerson/");
    string img_path( base_path + "96X160H96/");
    string fname_pos( base_path + "train_64x128_H96/pos.lst");
    string fname_neg( base_path + "train_64x128_H96/neg.lst");
    vector<string> pos_files;
    vector<string> neg_files;

    if ( !file_exists(FLAGS_svm_model_fname.c_str()) ) {
        // compute descriptors for positive and negative training set images
        pos_files = read_file_lines( fname_pos.c_str() );
        neg_files = read_file_lines( fname_neg.c_str() );
        for (auto& f : pos_files) { f = img_path + f; }
        for (auto& f : neg_files) { f = img_path + f; }
        compute_descriptors(pos_files);
        compute_descriptors(neg_files);
        train_svm_model(pos_files, neg_files);
    }
    fname_pos = string( base_path + "Test/pos.lst");
    fname_neg = string( base_path + "Test/neg.lst");
    // TODO: REMOVE
    fname_pos = string( base_path + "96X160H96/train/pos2/temp.lst"); // for testing
    //

    pos_files = read_file_lines( fname_pos.c_str() );
    neg_files = read_file_lines( fname_neg.c_str() );
    
    for (auto& f : pos_files) { f = base_path + "96X160H96/train/pos2/"+ f; }
    
    //for (auto& f : pos_files) { f = base_path + f; }
    for (auto& f : neg_files) { f = base_path + f; }
    test_svm_model(pos_files, FLAGS_svm_detection_threshold);
}

void test_svm_model(const vector<string>& files, double svm_threshold) {
    // load SVM model:
    model* svm_model = load_model( FLAGS_svm_model_fname.c_str() );
    if (svm_model == NULL) { printf("Could not load svm model.\n"); return; }
    
    int rows, cols, chan;
    char out_fname[256];
    vector<float> colour = {255.0f, 0.0f, 0.0f};
    for (size_t i = 0; i < files.size(); ++i) {
        vector<float> img = bcv_imread<float>(files[i].c_str(), &rows, &cols, &chan);
        //img = imresize(img, rows, cols, rows/2, cols/2);
        //rows /= 2;
        //cols /= 2;
        printf("%s %d x %d x %d \n", files[i].c_str(), rows, cols, chan); 
        
        double t1 = now_ms();
        vector<bbox_scored> boxes = multiscale_detect_in_image(svm_model, svm_threshold, img, rows, cols);
        nms(boxes, FLAGS_nms_overlap_threshold);

        printf("Took: %f s\n", (now_ms()-t1)/1000.0 );
        for (auto det : boxes) {
            det.print(); 
            draw_bbox(img, colour, det, rows, cols);
        }
        sprintf(out_fname, "out_%03d.png", i);
        bcv_imwrite(out_fname, img, rows, cols, chan);
    }
}

vector<bbox_scored> multiscale_detect_in_image(const model* svm_model, double svm_threshold, 
                                const vector<float>& img, int rows, int cols) {
    double scaling = FLAGS_scaling;
    double t1 = log( double(min(IMG_ROWS,IMG_COLS)) / double(min(rows,cols)) );
    int nscales = 1.0+t1/log(scaling);
    vector<int> s_rows(nscales);
    vector<int> s_cols(nscales);
    vector<double> s_scale(nscales);
    vector<int> s_stride(nscales);
    s_rows[0] = rows;
    s_cols[0] = cols;
    s_scale[0] = 1.0;
    s_stride[0] = FLAGS_stride_init;
    for (int i = 1; i < nscales; ++i) { 
        s_scale[i] = scaling*s_scale[i-1];
        s_rows[i] = s_scale[i]*s_rows[0];
        s_cols[i] = s_scale[i]*s_cols[0];
        s_stride[i] = max(1, int(s_scale[i]*s_stride[0]) ); 
    }
    
    vector<bbox_scored> boxes;
    
    for (int s = 0; s < nscales; ++s) {
        printf("at scale: %d/%d\n", s, nscales); 
        vector<float> s_img = imresize(img, rows, cols, s_rows[s], s_cols[s]);
        vector<bbox_scored> dets = detect_in_image(svm_model, svm_threshold, 
                                    s_stride[s], s_img, s_rows[s], s_cols[s]);
        for (auto& det: dets) { 
            det.x1 = min(cols, int(det.x1 / s_scale[s]) );
            det.x2 = min(cols, int(det.x2 / s_scale[s]) );
            det.y1 = min(rows, int(det.y1 / s_scale[s]) );
            det.y2 = min(rows, int(det.y2 / s_scale[s]) );
        }
        boxes.insert(boxes.begin(), dets.begin(), dets.end() );
    }
    return boxes; 
}

     
vector<bbox_scored> detect_in_image(const model* svm_model, double svm_threshold, 
                            int stride,
                            const vector<float>& img, int rows, int cols) {
    int chan = img.size()/(rows*cols);
    vector<float> subimg(IMG_ROWS*IMG_COLS*chan);
    vector<feature_node> svm_data;
    vector<double> prob_estimates(2);
    vector<bbox_scored> boxes;
    int index = 0;

    vector<float> img_gradnorm;
    vector<float> img_gradtheta;
    compute_gradient_norm_angle(img_gradnorm, img_gradtheta, img, rows, cols);    

    for (int r = 0; r <= (rows-IMG_ROWS); r+=stride) { 
        for (int c = 0; c <= (cols-IMG_COLS); c+=stride) {
            // extract subimage
            bbox bb(c, c+IMG_COLS, r, r+IMG_ROWS);
            
            //vector<float> subimg = extract_subimage(img, rows, cols, bb);
            //Hog descr(subimg, IMG_ROWS, IMG_COLS, FLAGS_cell_size, FLAGS_num_orientations);
            
            double t1 = now_ms();
            vector<float> subimg_gradnorm = extract_subimage(img_gradnorm, rows, cols, bb);
            vector<float> subimg_gradtheta = extract_subimage(img_gradtheta, rows, cols, bb);
            Hog descr(subimg_gradnorm, subimg_gradtheta, IMG_ROWS, IMG_COLS, FLAGS_cell_size, FLAGS_num_orientations);

            if (svm_data.size()==0) { svm_data=vector<feature_node>(descr.data.size()+2); }
            hog_to_feature_node(descr.data, &svm_data[0]);
            predict_probability(svm_model, &svm_data[0], &prob_estimates[0]);
            if (prob_estimates[0]>svm_threshold) { 
                printf("object detected: %f.\n", prob_estimates[0]);
                boxes.push_back( {bb, prob_estimates[0]} );
            }
        }
    }
    return boxes;
}

void train_svm_model(const vector<string>& pos_files,
        const vector<string>& neg_files) { 
    // get dimension of the descriptor :3
    string fname_hog(pos_files[0]);
    set_extension(fname_hog, "hog");
    Hog descr(fname_hog);
    int dim = descr.data.size(); 
    int nexamples = pos_files.size() + neg_files.size();

    // create SVM problem    
    problem svm_problem;
    create_svm_problem(svm_problem, nexamples, dim);
   
    // create SVM parameters 
    parameter svm_param;
    svm_param.solver_type = L2R_LR;
    svm_param.eps = FLAGS_svm_eps;
    svm_param.C = 1e-5; //FLAGS_svm_c;
    svm_param.nr_weight = 0;
    svm_param.weight_label = NULL;
    svm_param.weight = NULL;

    vector<vector<string>> all_files = {pos_files, neg_files};
    vector<vector<int>> all_labels = {vector<int>(pos_files.size(), 2),
                                  vector<int>(neg_files.size(), 1)};
    
    int idx = 0;
    for (int v = 0; v < 2; ++v) {
        vector<string> files = all_files[v];
        vector<int> labels = all_labels[v];
        for (size_t i = 0; i < files.size(); ++i, ++idx) { 
            string fname_hog(files[i]);
            set_extension(fname_hog, "hog");
            printf("%d) y=%d, %s\n", i, labels[i], fname_hog.c_str() );

            if (!file_exists(fname_hog.c_str()) ) { 
                printf("FILE DOES NOT EXIST.\n");
                printf("EXITING.\n");
                destroy_svm_problem(svm_problem); 
                return;
            }
            Hog descr(fname_hog);
            svm_problem.y[idx] = labels[i];
            hog_to_feature_node(descr.data, svm_problem.x[idx]);
        }
    }
    // ------------------------------------------------------------------------
    // actually train the svm: 
    // printf("checking parameters\n");
    // const char *out = check_parameter(&svm_problem, &svm_param);
    // if (out!=NULL) { 
    //     printf("ERROR:\n%s\n",out);
    //     destroy_svm_problem(svm_problem);
    //     return;
    // }
    // printf("training model\n");
    // model* svm_model = train(&svm_problem, &svm_param);
    // printf("trained model\n");
    // int ret = save_model(FLAGS_svm_model_fname.c_str(), svm_model);
    // if (ret != 0) { printf("Error saving SVM model.\n"); }

    // ------------------------------------------------------------------------
    // quick and dirty learn best C in SVM
    vector<float> svm_C_vec(10);
    vector<float> svm_C_acc(10);
    svm_C_vec[0] = 1e-5;
    for (int i = 1; i < svm_C_vec.size(); ++i) {
        svm_C_vec[i] = 5*svm_C_vec[i-1];
    }
    for (int k = 0; k < svm_C_vec.size(); ++k) {
        int num_cr_folds = 4;
        vector<int> pred_labels( nexamples, 0);
        svm_param.C = svm_C_vec[k];
        cross_validation(&svm_problem, &svm_param, num_cr_folds, &pred_labels[0]);
        vector<int> nums = {0, 0};
        int nerr = 0;
        for (int i = 0; i < nexamples; ++i) {
            nums[ pred_labels[i]-1 ]++;
            nerr += (pred_labels[i] != svm_problem.y[i]);
        }
        svm_C_acc[k] = 1.0-double(nerr)/nexamples;
        printf("C = %f, acc: %f, classified as 1: %d 2: %d\n", 
            svm_C_vec[k], svm_C_acc[k], nums[0], nums[1]);
    }
    // 
    int best_idx = distance(svm_C_acc.begin(), max_element(svm_C_acc.begin(), svm_C_acc.end() ) );

    // actually train the svm: 
    svm_param.C = svm_C_vec[best_idx];

    printf("best C: %f best accuracy: %f\n", svm_C_vec[best_idx], svm_C_acc[best_idx]);
    printf("checking parameters\n");
    const char *out = check_parameter(&svm_problem, &svm_param);
    if (out!=NULL) { 
        printf("ERROR:\n%s\n",out);
        destroy_svm_problem(svm_problem);
        return;
    }
    printf("training model\n");
    model* svm_model = train(&svm_problem, &svm_param);
    printf("trained model\n");
    int ret = save_model(FLAGS_svm_model_fname.c_str(), svm_model);
    if (ret != 0) { printf("Error saving SVM model.\n"); }

    free_and_destroy_model(&svm_model);
    destroy_svm_problem(svm_problem); 
}

void hog_to_feature_node(const vector<float>& data, feature_node* out) {
    int dim = data.size();
    for (int j = 0; j < dim; ++j) { 
        out[j].index = j+1; // one based
        out[j].value = data[j];
    }
    out[dim].index = dim+1; // bias
    out[dim].value = 1; // bias
    out[dim+1].index = -1; // end
    out[dim+1].value = 0; // end
}

void create_svm_problem(problem& svm_problem, int nexamples, int dim) {
    svm_problem.l = nexamples;
    svm_problem.n = dim+1;
    svm_problem.bias = 0;
    svm_problem.y = (int*)malloc(nexamples*sizeof(int));
    svm_problem.x = (feature_node**)malloc(nexamples*sizeof(feature_node*));
    for (int i = 0; i < nexamples; ++i) { 
        svm_problem.x[i] = (feature_node*)malloc((dim+2)*sizeof(feature_node));
    }
}
void destroy_svm_problem(problem& svm_problem) { 
    for (int i = 0; i < svm_problem.l; ++i) { 
        free( svm_problem.x[i] );
    }
    free(svm_problem.x);
}

void compute_descriptors(const vector<string>& files) {
    int rows, cols, chan;
    double t1, t2;
    for (size_t i = 0; i < files.size(); ++i) { 
        printf("%d) %s\n", i, files[i].c_str() );
        string fname_hog(files[i]);
        set_extension(fname_hog, "hog");

        Hog descr;
        // check if HOG file exists for this file.
        if ( file_exists(fname_hog.c_str()) ) { 
            continue;
        }
        
        // compute hog descriptor
        vector<float> img = bcv_imread<float>(files[i].c_str(), &rows, &cols, &chan);
        // resize to desired size...
        if ((IMG_ROWS!=rows) || (IMG_COLS!=cols)) {
            img = imresize(img, rows, cols, IMG_ROWS, IMG_COLS);
            rows = IMG_ROWS;
            cols = IMG_COLS;
        }
        t1 = now_ms();
        descr = Hog(img, rows, cols, FLAGS_cell_size, FLAGS_num_orientations);
        t2 = now_ms();
        descr.write( fname_hog.c_str() );
        printf("computed hog on %dx%d image, took %f ms\n", rows, cols, t2-t1);
        
        /*
        // visualize descriptor
        vector<float> vis = descr.vis();
        int rows_vis = descr.vis_rows();
        int cols_vis = descr.vis_cols();
        char temp[128]; sprintf(temp, "%04d.png", i);
        bcv_imwrite(temp, vis, rows_vis, cols_vis, 1, true);
        */
    }
}
