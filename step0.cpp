#include <iostream>
#include <cstdlib>

/// arg 1: real>0: fundamental frequency
/// arg 2: int>0: number of harmonics (1 meaning only fundamental)
/// 
int main(int argc, char* argv[]) {
    if (argc != 3) {
        return 1;
    }
    double fundamental = std::atof(argv[1]);
    int n_harmonics = std::atoi(argv[2]);

    std::cout << "doing some magic with " << n_harmonics << " harmonics of " << fundamental << "Hz" << std::endl;

    return 0;
}