#ifndef BCV_TVSEGMENT_H_
#define BCV_TVSEGMENT_H_

#include <cstdlib>
#include <cmath>
#include <limits>
#include "bcv_utils.h"
#include "bcv_diff_ops.h"

using namespace std;

//! tv segmentation parameters.
struct tvsegment_params {
    float lambda; //! weight of TV penalty
    float beta; //! sigma of weights. small value creates more extreme weights
    vector<float> unary; // unary term
    vector<float> clusters; // values of clusters (only for visualization)
    int rows;
    int cols;
    int chan; // this is also the dimension of each cluster
    int num_clusters;
    int max_iters; //! maximum number of iterations
    int isotropic; //! perform isotropic TV regularization or not
};

class tvsegment {
    public:
        //! main function that performs optimization
        tvsegment(const vector<float>& img, tvsegment_params* p);
        ~tvsegment();
        tvsegment();
        vector<int> get_assignments();
        vector<float> get_result();
        vector<float> get_weights();
    private:
        vector<float> u;
        vector<float> weights;
        int rows;
        int cols;
        int chan;
        int K;

        //! computes weights for tv regularization
        vector<float> compute_weights(const vector<float>& img, float beta);
 
        void project_onto_prob_simplex(vector<float>& x);
};

#endif // BCV_TVSEGMENT_H_
