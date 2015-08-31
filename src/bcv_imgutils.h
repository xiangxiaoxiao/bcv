#ifndef BCV_IMGUTILS_H_
#define BCV_IMGUTILS_H_
#include <cstdlib>
#include <vector>
#include <cassert>
#include "bcv_utils.h"
#include "bcv_bbox.h"

namespace bcv {
using namespace std;

//! \brief reorders the image, such that data for each channel appears continuous
template <typename T>
vector<T> stack_channels(const vector<T>& in, int chan) {
    if (chan == 1) { return in; }
    int n = in.size()/chan;
    vector<T> out = vector<T>(in.size());    
    for (int ch = 0; ch < chan; ++ch) {
        for (int i = 0; i < n; ++i) { out[i+n*ch] = in[chan*i + ch]; }
    }
    return out;
}

//! \brief interleaves the image such that the channel dimension is the fastest varying.
template <typename T>
vector<T> interleave_channels(const vector<T>& in, int chan) {
    if (chan == 1) { return in; }
    int n = in.size()/chan;
    vector<T> out = vector<T>(in.size());
    for (int ch = 0; ch < chan; ++ch) {
        for (int i = 0; i < n; ++i) { out[chan*i + ch] = in[i+n*ch]; }
    }
    return out;
}

//! Returns a subset of the image within bounding box 
template <typename T>
vector<T> extract_subimage(const vector<T>& in, int rows, int cols, 
                           const bbox& bb) {
    //! TODO: asserts about bounding box size
    int chan = in.size()/(rows*cols);
    int out_rows = bb.height();
    int out_cols = bb.width();
    vector<T> out(out_rows*out_cols*chan);
    for (int rr = 0; rr < out_rows; ++rr) {                             
        int id_out = linear_index(rr, 0, 0, out_cols, chan);               
        int id_in = linear_index(bb.y1+rr, bb.x1, 0, cols,chan);                   
        memcpy(&out[id_out], &in[id_in], sizeof(T)*chan*out_cols);
    }
    return out;
}

//!\brief draws a bounding box on the image.
//! \param I - input/output image
//! \param colour - color to draw bounding box
//! \param bb - bounding box
template <typename T>
void draw_bbox(vector<T>& I, const vector<T>& colour,
                             const bbox& bb, int rows, int cols) {
    int chan = I.size()/(rows*cols);
    assert( (colour.size() >= chan) && "sane color vector." );
    
    bbox bb_ = bb;
    bb_.clip(cols, rows);

    for (int xx=bb_.x1; xx<=bb_.x2; ++xx) {
        for (int ch = 0; ch < chan; ++ch) { 
            I[ linear_index(bb_.y1, xx, ch, cols, chan) ] = colour[ch];
            I[ linear_index(bb_.y2, xx, ch, cols, chan) ] = colour[ch];
        }
    }
    for (int yy=bb_.y1; yy<=bb_.y2; ++yy) {
        for (int ch = 0; ch < chan; ++ch) { 
            I[ linear_index(yy, bb_.x1, ch, cols, chan) ] = colour[ch];
            I[ linear_index(yy, bb_.x2, ch, cols, chan) ] = colour[ch];
        }
    }
}


//! \brief Bresenham line drawing algorithm (lifted from Rosetta code)
//! http://rosettacode.org/wiki/Bitmap/Bresenham's_line_algorithm
template <typename T>
void bresenham(vector<T>& in, int rows, int cols, T val, 
        int x0, int y0, int x1, int y1) {
    int chan = in.size()/rows/cols;

    int dx = abs(x1-x0); int sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0); int sy = y0<y1 ? 1 : -1; 
    int err = (dx>dy ? dx : -dy)/2;

    while(1) {
        for (int ch=0; ch<chan; ++ch) {
            in[linear_index(x0,y0,ch,cols,chan)]=val;
        }
        if (x0==x1 && y0==y1) break;
        int e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

} // namespace

#endif // BCV_IMGUTILS_H_
