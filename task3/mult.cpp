#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <vector>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <papi.h>

using namespace std;

typedef vector <vector <int32_t> > Matrix;

void handle_error(int retval, const string &str) {
    if (retval != PAPI_OK) {
        cerr << str << ": PAPI error " << retval << ": "
            << PAPI_strerror(retval) << endl;
        }


}

void multiply(const Matrix &a, const Matrix &b, Matrix &c, int dim, char mode) {
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


    int event_set = PAPI_NULL;
    long long values[2];

    int retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI library init error!\n");
        exit(1);
    }

    handle_error(PAPI_create_eventset(&event_set), "create eventset");

    handle_error(PAPI_add_event(event_set, PAPI_L1_DCM), "add PAPI_L1_DCM");
    handle_error(PAPI_add_event(event_set, PAPI_L2_DCM), "add PAPI_L2_DCM");

    handle_error(PAPI_start(event_set), "papi start");

    multiply(a, b, c, dim, *argv[4]);

    handle_error(PAPI_stop(event_set, values), "papi stop");

    cout << "PAPI_L1_DCM = " << values[0] << endl
        << "PAPI_L2_DCM = " << values[1] << endl;


    ofstream outC(argv[3], ios::binary | ios::out);
    outC.write(reinterpret_cast<const char *> (&dim), sizeof(dim));

    for (const auto &line : c) {
        outC.write(reinterpret_cast<const char *> (&line[0]),
            sizeof(line[0]) * line.size());
    }

    return 0;
}
