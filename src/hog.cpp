#include "hog.h"
#include "bcv_io.h"

namespace bcv {

//! \brief visualizes the hog descriptor if the descriptor has been computed.
//! \param rows_out, cols_out - size of the output image 
//! for now the size of the glyph in the output image is hardcoded
//! also, same cells in different *blocks* are averaged, so visualization loses some
//! information present in the descriptor.
vector<float> Hog::vis() {
    if (!is_initialized()) { 
        printf("Cannot visualize uninitialized descriptor.\n");
        return vector<float>();
    }
    // make into parameter later
    int rows = (this->rows*out_block_sz)/2;
    int cols = (this->cols*out_block_sz)/2;
    vector<vector<float>> tiles(this->num_orientations);
    double v0[2] = {0.0, -out_block_sz/2.0};
    double v1[2] = {0.0, +out_block_sz/2.0};
    double cx = cos(M_PI/double(this->num_orientations));
    double sx = sin(M_PI/double(this->num_orientations));
    // create tiles
    double tx,ty;
    for (int o = 0; o < this->num_orientations; ++o) {
        tiles[o] = vector<float>(out_block_sz*out_block_sz, 0);
        bresenham(tiles[o], out_block_sz, out_block_sz, 1.0f,  
                    v0[0]+out_block_sz/2.0, v0[1]+out_block_sz/2.0, 
                    v1[0]+out_block_sz/2.0, v1[1]+out_block_sz/2.0);
        tx = cx*v0[0]+sx*v0[1];
        ty =-sx*v0[0]+cx*v0[1];
        v0[0]=tx; v0[1]=ty;
        tx = cx*v1[0]+sx*v1[1];
        ty =-sx*v1[0]+cx*v1[1];
        v1[0]=tx; v1[1]=ty;
    }
    vector<float> img( rows*cols, 0);
    vector<int> counts( rows*cols, 0);
    for (int r = 0; r < this->rows; ++r) {
        int out_r = r/2;
        for (int c = 0; c < this->cols; ++c) {
            int out_c = c/2;
            int id = linear_index(r,c,0, this->cols, this->num_orientations);
            for (int o = 0; o < this->num_orientations; ++o) {
                float weight = this->data[id+o];
                int i = 0;
                for (int rr = 0; rr < out_block_sz; ++rr) {
                    for (int cc = 0; cc < out_block_sz; ++cc, ++i) {
                        int id2 = linear_index(out_block_sz*out_r+rr, 
                                               out_block_sz*out_c+cc, cols);
                        img[id2] += weight*tiles[o][i];
                        counts[id2]++;
                    }
                }
            }
        }
    }
    for (int i = 0; i < img.size(); ++i) { img[i]/=counts[i]; }
    return img;
}

//! \brief computes the descriptor on the passed image (grayscale or RGB)
Hog::Hog(const vector<float>& img, int rows, int cols, int cell_size, int num_orientations) {
    vector<float> gradnorm_vec;
    vector<float> theta_vec;
    compute_gradient_norm_angle(gradnorm_vec, theta_vec, img, rows, cols); 
    init(gradnorm_vec, theta_vec, rows, cols, cell_size, num_orientations);
}

void Hog::init(const vector<float>& gradnorm_vec, const vector<float>& theta_vec_,
                        int rows, int cols, int cell_size, int num_orientations) {
    int chan = gradnorm_vec.size() / (rows*cols);
    int cell_rows = ceil(rows/cell_size);
    int cell_cols = ceil(cols/cell_size);
    int block_rows = cell_rows-1;
    int block_cols = cell_cols-1;

    float* descr_cells = new float[cell_rows*cell_cols*num_orientations];
    memset(descr_cells, 0, sizeof(float)*cell_rows*cell_cols*num_orientations);
    float* descr_blocks = new float[block_rows*block_cols];
    memset(descr_blocks, 0, sizeof(float)*block_rows*block_cols);

    int id0, id1, id2;
    double theta, theta_id, gradnorm, w;
    vector<size_t> block_ids; block_ids.reserve(4);
    vector<intpair> block_rcs; block_rcs.reserve(4);

    // multiply gradient orientations to be in range [0, num_orientations]
    float theta_scale = num_orientations / M_PI;
    vector<float> theta_vec = theta_vec_;
    transform(theta_vec.begin(), theta_vec.end(), theta_vec.begin(),
              bind1st( multiplies<float>(), theta_scale ) );
    // compute "cumulative gradnorm" over multiple channels.
    float* gradnorm_sq_cum = new float[rows*cols];
    memset(gradnorm_sq_cum, 0, sizeof(float)*rows*cols);

    for (int i = 0; i < rows*cols; ++i) {
        for (int ch = 0; ch < chan; ++ch) {
            gradnorm_sq_cum[i] += gradnorm_vec[chan*i+ch]*gradnorm_vec[chan*i+ch];
        }
    }
    // init lookup table for exp
    double inv_cell_size_sq = 1.0/(cell_size*cell_size);    
    vector<float> exp_lut_( (cell_size+1)*(cell_size+1) );
    for (int i = 0; i < cell_size+1; ++i) {
        for (int j = 0; j < cell_size+1; ++j) {
            exp_lut_[i*(cell_size+1)+j] = exp(-(i*i + j*j)*inv_cell_size_sq);
        }
    }

    vector<vector<size_t>> block_rrr(cell_rows);
    vector<vector<size_t>> block_ccc(cell_cols);
    for (int cr = 0; cr < cell_rows; ++cr) {
        if (cr > 0) { block_rrr[cr].push_back(cr-1); }
        if (cr < (cell_rows-1)) { block_rrr[cr].push_back(cr); }
    }
    for (int cc = 0; cc < cell_cols; ++cc) {
        if (cc > 0) { block_ccc[cc].push_back(cc-1); }
        if (cc < (cell_cols-1)) { block_ccc[cc].push_back(cc); }
    }

    //double t1 = now_ms();
    for (int cr = 0; cr < cell_rows; ++cr) { 
        for (int cc = 0; cc < cell_cols; ++cc) {     
            // find normalizing factor in the blocks
            for (auto& byy : block_rrr[cr]) { 
                for (auto& bxx : block_ccc[cc]) {
                    double val = 0;
                    int blk_center_r = (byy+1)*cell_size;
                    int blk_center_c = (bxx+1)*cell_size;

                    for (int r = cr*cell_size; r < min(rows, (cr+1)*cell_size); ++r) { 
                        int dy = abs(r-blk_center_r);
                        for (int c = cc*cell_size; c < min(cols, (cc+1)*cell_size); ++c) { 
                            int dx = abs(c-blk_center_c);
                            val += exp_lut_[dy*(cell_size+1)+dx]*gradnorm_sq_cum[r*cols+c];
                        }
                    }
                    descr_blocks[ linear_index(byy,bxx,block_cols) ] += val;
                }
            }
        }
    }            
    //printf("took(1): %f\n", now_ms()-t1);
    //t1 = now_ms();
    
    for (int cr = 0; cr < cell_rows; ++cr) { 
        for (int cc = 0; cc < cell_cols; ++cc) {     
            id0 = linear_index(cr, cc, 0, cell_cols, num_orientations);
            for (int r = cr*cell_size; r < min(rows, (cr+1)*cell_size); ++r) { 
                int nc = min(cols, (cc+1)*cell_size)-cc*cell_size;
                int img_id0 = linear_index(r, cc*cell_size, 0, cols, chan);
                for (int img_id = img_id0; img_id < img_id0+nc*chan; ++img_id) {
                    theta_id = theta_vec[img_id];
                    gradnorm = gradnorm_vec[img_id];
                    id1 = int(theta_id); 
                    id2 = int(theta_id+1); //int(ceil(theta_id));
                    if (id1==num_orientations) { id1 = 0; }
                    if (id2==num_orientations) { id2 = 0; }
                    w = theta_id-id1;                               
                    descr_cells[ id0 + id1 ]+= (1-w)*gradnorm;
                    descr_cells[ id0 + id2 ]+= w*gradnorm;
                }
            }
        }
    }
    //printf("took(2): %f\n", now_ms()-t1);    

    this->data = vector<float>( 4*cell_rows*cell_cols*num_orientations);
    this->rows = 2*cell_rows;
    this->cols = 2*cell_cols;
    
    this->num_orientations = num_orientations;
    this->cell_size = cell_size;
    
    vector<intpair> rcs(4);
    rcs[0] = intpair(0,0);
    rcs[1] = intpair(0,1);
    rcs[2] = intpair(1,0);
    rcs[3] = intpair(1,1);

    // compute normalizing factor in-place
    for (int i = 0; i < block_rows*block_cols; ++i) {
        descr_blocks[i] = 1.0f/sqrt(descr_blocks[i]+1e-8f);
    }
    for (int br = 0; br < block_rows; ++br) {
        for (int bc = 0; bc < block_cols; ++bc) { 
            id2 = linear_index(br, bc, block_cols);
            for (int o = 0; o < num_orientations; ++o) {
                // id0 : index into output, id1: index into cells
                id0 = linear_index(2*br+0, 2*bc+0, 0, 2*cell_cols, num_orientations);
                id1 = linear_index(br+0, bc+0, 0, cell_cols, num_orientations);
                this->data[id0+o] = descr_cells[id1+o]*descr_blocks[id2];
                id0 = linear_index(2*br+0, 2*bc+1, 0, 2*cell_cols, num_orientations);
                id1 = linear_index(br+0, bc+1, 0, cell_cols, num_orientations);
                this->data[id0+o] = descr_cells[id1+o]*descr_blocks[id2];
                id0 = linear_index(2*br+1, 2*bc+0, 0, 2*cell_cols, num_orientations);
                id1 = linear_index(br+1, bc+0, 0, cell_cols, num_orientations);
                this->data[id0+o] = descr_cells[id1+o]*descr_blocks[id2];
                id0 = linear_index(2*br+1, 2*bc+1, 0, 2*cell_cols, num_orientations);
                id1 = linear_index(br+1, bc+1, 0, cell_cols, num_orientations);
                this->data[id0+o] = descr_cells[id1+o]*descr_blocks[id2];
            }
        }
    }
    delete[] descr_cells;
    delete[] descr_blocks;
    delete[] gradnorm_sq_cum;
}

//! returns true if the descriptor appears to be set up.
bool Hog::is_initialized() { 
    return ((rows>0) && (cols>0) && (data.size()>0));
}

void Hog::write(const char* fname) { 
    ofstream file(fname, ios::out | ios::binary );
    if (!file.is_open()) {
        printf("could not open file for writing: %s\n", fname);
        return;
    }
    file.write((const char*)&rows, sizeof(int));
    file.write((const char*)&cols, sizeof(int));
    file.write((const char*)&num_orientations, sizeof(int));
    file.write((const char*)&cell_size, sizeof(int));
    file.write((const char*)&data[0], sizeof(float)*data.size());
    file.close(); 
}

void Hog::read(const char* fname) { 
    ifstream file(fname, ios::in | ios::binary );
    if (!file.is_open()) { 
        printf("could not open file for reading: %s\n", fname);
        return;
    }
    file.read((char*)&rows, sizeof(int));
    file.read((char*)&cols, sizeof(int));
    file.read((char*)&num_orientations, sizeof(int));
    file.read((char*)&cell_size, sizeof(int));
    data = vector<float>(rows*cols*num_orientations);
    file.read((char*)&data[0], sizeof(float)*(rows*cols*num_orientations) );
    file.close(); 
}

} // namespace bcv
