#include <iostream>
#include <thread>
#include <sstream>
#include <vector>
#include <mutex>
#include <iomanip>
#include <chrono>

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

mutex m;

void calc_area(double inter_mid, double delt, int ni, double &dest) {
    double area = 0.0;

    for (int i = 0; i < ni; ++i) {
        area += 4 * delt / (1 + inter_mid * inter_mid);
        inter_mid += delt;
    }

    m.lock();
    dest += area;
    m.unlock();
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return -1;
    }

    stringstream ss(argv[1]);
    int ni;
    ss >> ni;

    ss = stringstream(argv[2]);
    int nt;
    ss >> nt;

    vector <thread> thrd;
    double pi = 0.0;

    double delt = 1.0 / ni;
    double left_inter_mid = delt / 2;
    int thrd_ni = ni / nt, r = ni % nt;
    

    auto t1 = high_resolution_clock::now();

    for (int i = 0; i < nt; ++i) {
        int now_ni = thrd_ni + (r-- > 0);
        thrd.emplace_back(thread(calc_area, left_inter_mid, delt, now_ni, ref(pi)));
        left_inter_mid += delt * now_ni;
    }
        
    for (int i = 0; i < nt; ++i) {
        thrd[i].join();
    }

    auto t2 = high_resolution_clock::now();
    duration <double, milli> dur(t2 - t1);


    cout << "Pi = " << setprecision(9) << setw(9) << pi << endl;
    cout << "Elapsed time = " << dur.count() << " milliseconds" << endl;


    return 0;
}