#include <cstdlib>
#include <vector>
#include <gtest/gtest.h>
#include "bcv_utils.h"
#include "bcv_bbox.h"
#include "bcv_detectionutils.h"

using namespace bcv;
using namespace std;

// Test for equality operators of "bbox_scored" class
TEST(bbox_scored, equality_operators) { 
    bbox_scored bb1, bb2; 
    
    bb1 = bbox_scored(10,30,20,40, 1.0);
    bb2 = bbox_scored(10,30,20,40, 2.0);
    ASSERT_NE( bb1, bb2);

    bb1 = bbox_scored(10,30,20,40, 1.0);
    bb2 = bbox_scored(10,30,20,40, 1.0);
    ASSERT_EQ( bb1, bb2);

    bb1 = bbox_scored(10,30,20,40, 1.0);
    bb2 = bbox_scored(10,30,20,42, 1.0);
    ASSERT_NE( bb1, bb2);
}

// Test for equality operators in the "bbox" class
TEST(bbox, equality_operators) { 
    bbox bb1, bb2;     
    bb1 = bbox(10,30,20,40);
    bb2 = bbox(10,35,20,40);
    ASSERT_NE( bb1, bb2);

    bb1 = bbox(10,30,20,40);
    bb2 = bbox(10,30,20,40);
    ASSERT_EQ( bb1, bb2);
}

// Test nonmaximum suppression (two random samples.)
TEST(non_max_suppression, comparison_with_dpm) {
    vector<bbox_scored> boxes;
    boxes.push_back( bbox_scored(35, 51, 91, 171, 57.0) );
    boxes.push_back( bbox_scored(83, 99, 29, 108, 47.0) );
    boxes.push_back( bbox_scored(59, 75, 76, 155, 1.00) );
    boxes.push_back( bbox_scored(55, 71, 75, 155, 34.0) );

    nms(boxes, 0.01);
    sort(boxes.begin(), boxes.end());

    vector<bbox_scored> boxes_gt;
    boxes_gt.push_back( bbox_scored(35, 51, 91, 171, 57.0) );
    boxes_gt.push_back( bbox_scored(83, 99, 29, 108, 47.0) );
    boxes_gt.push_back( bbox_scored(55, 71, 75, 155, 34.0) );
    sort(boxes_gt.begin(), boxes_gt.end());

    ASSERT_EQ( boxes_gt.size(), boxes.size() );
    for (int i = 0; i < boxes.size(); ++i) {
        ASSERT_EQ( boxes_gt[i], boxes[i] );
    }
    //------------------------------------------------------------------------// 
    boxes.clear();
    boxes.push_back( bbox_scored(31,  66,  45,  96, 15));
    boxes.push_back( bbox_scored(53,  88,   8,  60, 14));
    boxes.push_back( bbox_scored(17,  52,  23,  74, 87));
    boxes.push_back( bbox_scored(60,  95,  91, 143, 58));
    boxes.push_back( bbox_scored(26,  61,  15,  67, 55));
    boxes.push_back( bbox_scored(65, 101,  83, 134, 14));
    boxes.push_back( bbox_scored(69, 104,  54, 105, 85));
    boxes.push_back( bbox_scored(75, 110, 100, 151, 62));

    boxes_gt.clear();
    boxes_gt.push_back( bbox_scored(17,  52,  23,  74, 87) );
    boxes_gt.push_back( bbox_scored(69, 104,  54, 105, 85) );
    boxes_gt.push_back( bbox_scored(75, 110, 100, 151, 62) );
    boxes_gt.push_back( bbox_scored(60,  95,  91, 143, 58) );
    boxes_gt.push_back( bbox_scored(31,  66,  45,  96, 15) );
    boxes_gt.push_back( bbox_scored(53,  88,   8,  60, 14) );
    sort(boxes_gt.begin(), boxes_gt.end());

    nms(boxes, 0.50);
    sort(boxes.begin(), boxes.end());

    ASSERT_EQ( boxes_gt.size(), boxes.size() );
    for (int i = 0; i < boxes.size(); ++i) {
        ASSERT_EQ( boxes_gt[i], boxes[i] );
    }    
    //------------------------------------------------------------------------// 
}
TEST(non_max_suppression, sanity_checks) {
    vector<bbox_scored> boxes;
    boxes.push_back( bbox_scored(10, 20, 30, 40, 1.0) );
    nms( boxes, 0.50);
    ASSERT_EQ( boxes.size(), 1 );
    boxes.pop_back();
    nms( boxes, 0.50);
    ASSERT_EQ( boxes.size(), 0 );    
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
