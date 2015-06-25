#include <cstdlib>
#include <cstdio>
#include "tvsegment.h"
#include "bcv_kmeans.h"
#include "bcv_io.h"

#include <gflags/gflags.h>

using namespace std;
using bcv::linear_index;

void compute_unary_term(const vector<float>& img, bcv::tvsegment_params* p); 
void print_tvsegment_params(bcv::tvsegment_params* p);
vector<float> segutils_vis_segmentation(const vector<float>& u, 
                    int rows, int cols, int chan, const vector<float>& clusters, 
                    int show_rounded, int show_boundary);


DEFINE_bool(isotropic, true, "Apply isotropic TV or not");
DEFINE_int32(num_clusters, 3, "Number of regions");
DEFINE_int32(max_iters, 100, "Maximum number of iterations");
DEFINE_double(lambda, 1.0, "TV weight");
DEFINE_double(beta, 1.0, "TV weight scaling");
DEFINE_string(input, "../images/arches.jpg", "Input image filename");
DEFINE_string(output, "out.png", "Output image filename");
DEFINE_bool(show_boundary, true, "Visualization - show region boundary?");
DEFINE_bool(show_rounded, true, "Visualization - round to produce indicators?");

int main(int argc, char** argv) { 
    gflags::SetUsageMessage("TV-regularized segmentation example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    double t1, t2;
    int rows, cols, chan;
    vector<float> img = bcv::bcv_imread<float>(FLAGS_input.c_str(), &rows, &cols, &chan);

    bcv::tvsegment_params params;
    params.beta = FLAGS_beta;
    params.lambda = FLAGS_lambda;
    params.max_iters = FLAGS_max_iters;
    params.isotropic = FLAGS_isotropic;
    params.num_clusters = FLAGS_num_clusters;
    params.rows = rows;
    params.cols = cols;
    params.chan = chan;
    // ------------------------------------------------------------------------
    // load image
    int n = rows*cols;
    for (size_t i = 0; i < img.size(); ++i) { img[i] /= 256.0; }

    compute_unary_term(img, &params);

    print_tvsegment_params(&params);

    t1 = bcv::now_ms();
    bcv::tvsegment tvs(img, &params);
    t2 = bcv::now_ms();
    printf("TV segmentation took: %f ms\n", (t2-t1) );
   
    int show_rounded = 1;
    int show_boundary = 1; 
    vector<float> res = tvs.get_result();
    vector<float> out = segutils_vis_segmentation( tvs.get_result(), 
            rows, cols, chan, params.clusters, FLAGS_show_rounded, FLAGS_show_boundary );

    for (size_t i = 0; i < out.size(); ++i) { out[i] *= 256; }
    bcv::bcv_imwrite<float>(FLAGS_output.c_str(), out, rows, cols, chan);
    printf("Wrote the result to '%s'\n", FLAGS_output.c_str() );
    return 0;
}

void print_tvsegment_params(bcv::tvsegment_params* p) {
    printf("lambda: %f\n", p->lambda);
    printf("beta: %f\n", p->beta);
    printf("num-clusters: %d\n", p->num_clusters);
    printf("isotropic: %d\n", p->isotropic);
    printf("max-iters: %d\n", p->max_iters);
    printf("image size: %dx%dx%d\n", p->rows, p->cols, p->chan);
}

void compute_unary_term(const vector<float>& img, bcv::tvsegment_params* p) {
    // cluster image values based on intentity
    int num_pts = img.size()/p->chan;
    int dim = p->chan;
    int K = p->num_clusters;
    int num_iterations = 100;

    printf("kmeans: n pts: %d, dim: %d, K: %d\n", num_pts, dim, K);
    bcv::bcv_kmeans km(img, num_pts, dim, K, num_iterations);
    km.get_centers(p->clusters);
    
    printf("learned cluster centers:\n");
    for (int k = 0; k < K; ++k) { 
        printf("K = %d: ", k);
        for (int i = 0; i < dim; ++i) { printf("%f ", p->clusters[dim*k+i]); }
        printf("\n");
    }
    
    // U(x,k) = || I(x) - c_k ||^2
    p->unary = vector<float>( img.size()/dim * K );
    for (size_t i = 0; i < img.size()/dim; ++i) { 
        for (int k = 0; k < K; ++k) {
            float d = 0;
            for (int c = 0; c < dim; ++c) { 
                float temp = img[dim*i + c] - p->clusters[k*dim + c];
                d += temp*temp;
            }
            p->unary[i*K + k] = d;
        }
    }
}

vector<float> segutils_vis_segmentation(const vector<float>& u, int rows, int cols, int chan,
        const vector<float>& clusters, int show_rounded, int show_boundary) {
    int K = clusters.size()/chan;
    assert(u.size() == (size_t)rows*cols*K);

    vector<float> segimg = vector<float>(rows*cols*chan);
     
    if (show_rounded) { 
        // --------------------------------------------------------------------
        // (hard assignments)
        for (int i = 0; i < rows*cols; ++i) { 
            // get assignment:
            int id = -1;
            float maxval = -1.0f;
            for (int k = 0; k < K; ++k) { 
                if (u[K*i + k] > maxval) {
                    maxval = u[K*i+k];
                    id = k;
                }
            }
            // write in cluster center
            for (int k = 0; k < chan; ++k) { 
                segimg[chan*i + k] = clusters[chan*id + k];
            }
        }
    } else {
        // --------------------------------------------------------------------
        // (soft assignments)
        for (int i = 0; i < rows*cols; ++i) { 
            // get assignment:
            for (int k = 0; k < K; ++k) {
                float w = u[K*i + k];
                // write in cluster center
                for (int j = 0; j < chan; ++j) { 
                    segimg[chan*i + j] += w*clusters[chan*k + j];
                }
            }
        }
    }

    if (show_boundary) { 
        vector<int> argmax = vector<int>(rows*cols, -1);
        for (size_t i = 0; i < argmax.size(); ++i) { 
            float maxval = -1.0f;
            for (int k = 0; k < K; ++k) { 
                if (u[K*i + k] > maxval) { maxval=u[K*i+k]; argmax[i] = k; }
            }
        }
        int I, R, L, U, D, UR, UL, LR, LL;

        for (int r = 0; r < rows; ++r) { 
            for (int c = 0; c < cols; ++c) {
                float w = 1.0f;
                I = argmax[ linear_index(r,c,cols) ];
                // 4 neighbors:
                if (c+1 < cols) { 
                    R = argmax[ linear_index(r,c+1,cols) ];
                } else { R = I; }
                if (c-1 >= 0) {
                    L = argmax[ linear_index(r,c-1,cols) ];
                } else { L = I; }
                if (r-1 >= 0) {
                    U = argmax[ linear_index(r-1,c,cols) ];
                } else { U = I; }
                if (r+1 < rows) { 
                    D = argmax[ linear_index(r+1,c,cols) ];
                } else { D = I; }
                // 4 more neighbors
                if ((c+1 < cols) && (r+1 < rows)) { 
                    LR = argmax[ linear_index(r+1,c+1,cols) ];
                } else { LR = I; }
                if ((c-1 >= 0) && (r+1 < rows)) {
                    LL = argmax[ linear_index(r+1,c-1,cols) ];
                } else { LL = I; }
                if ((r-1 >= 0) && (c+1 < cols)) {
                    UR = argmax[ linear_index(r-1,c+1,cols) ];
                } else { UR = I; }
                if ((r-1 < rows) && (c-1 >= 0)) { 
                    UL = argmax[ linear_index(r-1,c-1,cols) ];
                } else { UL = I; }

                //
                if ((I!=R) || (I!=L)) { w -= 0.125f; }
                if ((I!=U) || (I!=D)) { w -= 0.125f; }
                if ((I!=UR) || (I!=UL)) { w -= 0.125f; }
                if ((I!=LR) || (I!=LL)) { w -= 0.125f; }
                w = max(0.0f, w);
                for (int k = 0; k < chan; ++k) { 
                    segimg[ linear_index(r,c,k,cols,chan) ] *= w;
                }
            }
        }
    }
    return segimg;
}