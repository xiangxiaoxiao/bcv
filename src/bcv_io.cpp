//! @file bcv_io.cpp
#include "bcv_io.h"

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
