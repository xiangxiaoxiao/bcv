#ifndef BCV_BBOX_H_
#define BCV_BBOX_H_

namespace bcv {

//! bounding box class
class bbox {
public:
    int x1; // left
    int x2; // right (non-inclusive)
    int y1; // top
    int y2; // bottom (non-inclusive)
    bbox() {};
    //! x-left, x-right, y-top, y-bottom
    bbox(int _x1, int _x2, int _y1, int _y2): 
        x1(_x1),  x2(_x2), y1(_y1), y2(_y2) {
        assert( ((x1<=x2) && (y1<=y2)) && "sane bounding box" );    
    };
    int height() const { return (y2-y1); }
    int width() const { return (x2-x1); }
    int area() const { return (y2-y1)*(x2-x1); }
    void clip(int xmax, int ymax) {
        x1=min(max(0,x1),xmax-1);
        x2=min(max(0,x2),xmax-1);
        y1=min(max(0,y1),ymax-1);
        y2=min(max(0,y2),ymax-1);
    }
    //! Computes area of overlap. If there is no overlap, returns 0.
    int overlap_area(const bbox& other) const {
        int xx1 = max(x1, other.x1);
        int xx2 = min(x2, other.x2);
        int yy1 = max(y1, other.y1);
        int yy2 = min(y2, other.y2);
        if ((yy2>=yy1) && (xx2>=xx1)) { return (yy2-yy1)*(xx2-xx1); }
        else { return 0; }
    }
    void print() { printf("bounding box: x:%d - %d, y:%d - %d\n", x1,x2,y1,y2); }
    bool operator ==(const bbox& b) const { return is_equal(b); }
    bool operator !=(const bbox& b) const { return !is_equal(b); }

protected:
    bool is_equal(const bbox& b) const {
        return ((x1==b.x1) && (x2==b.x2) && (y1==b.y1) && (y2==b.y2));        
    }
};

//! bounding box class that keeps score.
class bbox_scored : public bbox {
    public:
    double score;
    bbox_scored() {};
    bbox_scored(int _x1, int _x2, int _y1, int _y2, double s) : 
        bbox(_x1, _x2, _y1, _y2), score(s) {}
    bbox_scored(const bbox& bb, double _score) :
        bbox(bb), score(_score) {}
    void print() { printf("bounding box: x:%d - %d, y:%d - %d - %f\n", x1,x2,y1,y2,score); }
    bool operator ==(const bbox_scored& b) const {
        return ((score == b.score) && this->is_equal(b));
    }
    bool operator !=(const bbox_scored& b) const {
        return ((score != b.score) || !(this->is_equal(b)));
    }    
    bool operator <(const bbox_scored& b) const {
        return (score < b.score);
    }
};

} // namespace bcv

#endif // BCV_BBOX_H_
