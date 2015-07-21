//! @file bcv_io.cpp
#include "bcv_io.h"
namespace bcv {
//! Returns a vector of strings, where each string is a line from a given file.
vector<string> read_file_lines(const char* fname) {
    string l;
    ifstream fp(fname);
    vector<string> lines;
    if (!fp.is_open()) {
        printf("Could not open %s\n", fname);
        return lines;
    }
    while (getline(fp,l)) {
        // trim trailing spaces
        size_t endpos = l.find_last_not_of(" \t");
        if( string::npos != endpos ) {
            l = l.substr( 0, endpos+1 );
        }
        lines.push_back(l);
    }
    fp.close();
    return lines;
}

bool is_image_file(const char* fname) {
    char* img_exts[] = {"png", "jpg"};
    int num_exts = 2;

    char ext[4];
    int n = strlen(fname);
    for (int k = 0; k < 3; ++k) { ext[k] = tolower(fname[n-3+k]); } ext[3] = 0;

    for (int k = 0; k < num_exts; ++k) {
        if (strcmp( ext, img_exts[k])==0) { return true; }
    }
    return false;
}

bool is_video_file(const char* fname) {
    // there are probably tons more that are actually supported..video exte
    char* img_exts[] = {"mp4", "mp2", "mov", "avi", "mkv", "wmv", "m4v"};
    int num_exts = 7;

    char ext[4];
    int n = strlen(fname);
    for (int k = 0; k < 3; ++k) { ext[k] = tolower(fname[n-3+k]); } ext[3] = 0;
    for (int k = 0; k < num_exts; ++k) {
        if (strcmp( ext, img_exts[k])==0) { return true; }
    }
    return false;
}


} // namespace bcv