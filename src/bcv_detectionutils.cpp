#include "bcv_detectionutils.h"

namespace bcv {

void nms(vector<bbox_scored>& boxes, double threshold) {
    if (boxes.empty()) { return; }
    size_t n = boxes.size();

    vector<size_t> idx(n);
    for (size_t i = 0; i < idx.size(); ++i) { idx[i]=i; }
    sort(idx.begin(), idx.end(), 
       [&boxes](size_t i, size_t j) {return boxes[i].score < boxes[j].score;});
    // note: corners in dpm bounding boxes are specified "inclusively",
    // whereas here we *do not* include right-bottom corner, so the area computation
    // is a little different
    
    vector<bool> pick(n, false);
    while (!idx.empty()) {
        size_t last = idx.size()-1;
        size_t i = idx[last];
        pick[i] = true;
        vector<bool> suppress( idx.size(), false);
        suppress[last]=true;
        for (int pos = 0; pos < last; ++pos) {
            int j = idx[pos];
            double o = double(boxes[i].overlap_area(boxes[j])) / double(boxes[j].area());
            if (o > threshold) { suppress[pos]=true; }
        }
        erase_indices_if(idx, suppress);
    }
    keep_indices_if(boxes, pick);
}


}