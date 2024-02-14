#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <vector>
#include <chrono>
#include <sstream>
#include <thread>
#include <unistd.h>

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

typedef vector <vector <int32_t> > Matrix;

void multiply(const Matrix &a, const Matrix &b, Matrix &c, int dim, int mode) {
    if (mode == '0') { //ijk

        for (int i = 0; i < dim; ++i) {
            for (int j = 0; j < dim; ++j) {
                int32_t sum = 0;

                for (int k = 0; k < dim; ++k) {
                    sum += a[i][k] * b[k][j];
                }

                c[i][j] = sum;
            }
        }

    } else if (mode == '1') { //ikj

        for (int i = 0; i < dim; ++i) {
            for (int k = 0; k < dim; ++k) {
                int32_t r = a[i][k];

                for (int j = 0; j < dim; ++j) {
                    c[i][j] += r * b[k][j];
                }
            }
        }

    } else if (mode == '2') { //kij

        for (int k = 0; k < dim; ++k) {
            for (int i = 0; i < dim; ++i) {
                int32_t r = a[i][k];

                for (int j = 0; j < dim; ++j) {
                    c[i][j] += r * b[k][j];
                }
            }
        }

    } else if (mode == '3') { //jik

        for (int j = 0; j < dim; ++j) {
            for (int i = 0; i < dim; ++i) {
                int32_t sum = 0;

                for (int k = 0; k < dim; ++k) {
                    sum += a[i][k] * b[k][j];
                }

                c[i][j] = sum;
            }
        }

    } else if (mode == '4') { //jki

        for (int j = 0; j < dim; ++j) {
            for (int k = 0; k < dim; ++k) {
                int32_t r = b[k][j];

                for (int i = 0; i < dim; ++i) {
                    c[i][j] += a[i][k] * r;
                }
            }
        }

    } else if (mode == '5') { //kji

        for (int k = 0; k < dim; ++k) {
            for (int j = 0; j < dim; ++j) {
                int32_t r = b[k][j];

                for (int i = 0; i < dim; ++i) {
                    c[i][j] += a[i][k] * r;
                }
            }
        }

    }
}

int main(int argc, char* argv[]) {
    ifstream inA(argv[1], ios::binary | ios::in);
    uint32_t dim;
    inA.read(reinterpret_cast<char *> (&dim), sizeof(dim));

    Matrix a(dim, vector <int32_t> (dim));
    for (auto &line : a) {
        inA.read(reinterpret_cast<char *> (&line[0]), 
            sizeof(line[0]) * line.size());
    }

    ifstream inB(argv[2], ios::binary | ios::in);
    inB.read(reinterpret_cast<char *> (&dim), sizeof(dim));

    if (dim != a.size()) {
        cerr << "Wrong input: different sizes of matrixes\n";
        return -1;
    }

    Matrix b(dim, vector <int32_t> (dim));
    for (auto &line : b) {
        inB.read(reinterpret_cast<char *> (&line[0]), 
            sizeof(line[0]) * line.size());
    }

    Matrix c(dim, vector <int32_t> (dim));

    auto t1 = high_resolution_clock::now();

    multiply(a, b, c, dim, *argv[4]);

    auto t2 = high_resolution_clock::now();

    duration <double, milli> dur(t2 - t1);
    cout << "Elapsed time = " << dur.count() << " milliseconds" << endl;

    stringstream ss(argv[4]);
    int mode;
    ss >> mode;

    ofstream outC(argv[3], ios::binary | ios::out);
    outC.write(reinterpret_cast<const char *> (&dim), sizeof(dim));

    for (const auto &line : c) {
        outC.write(reinterpret_cast<const char *> (&line[0]), 
            sizeof(line[0]) * line.size());
    }

    return 0;
}