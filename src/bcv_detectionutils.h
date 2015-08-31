#ifndef BCV_DETECTIONUTILS_H_
#define BCV_DETECTIONUTILS_H_
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cassert>
#include "bcv_utils.h"
#include "bcv_bbox.h"

namespace bcv {
using namespace std;
    
//! \brief Nonmaximum suppression. This is just a C++ port of the code used in DPM
//! (http://people.cs.uchicago.edu/~rbg/latent/)
//! \param boxes - input/output
//! \param threshold - overlap fraction in [0,1] above which a bbox is suppressed
void nms(vector<bbox_scored>& boxes, double threshold);


} // namespace

#endif // BCV_DETECTIONUTILS_H_
