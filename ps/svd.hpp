#include <iostream>
#include <complex>
#include <fstream>
#include <sstream>
#include <cstdlib>
using namespace std;

template<int COL, int ROW>
void read(const std::string& filename, float* complex_numbers) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open the file." << std::endl;
            return;
        }

        complex<float> tmp[ROW][COL];

        std::string line;
        int row = 0;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            float real, imag;

            int col = 0;
            while (iss >> real >> imag && col < COL) {
                tmp[row][col].real(real);
                tmp[row][col].imag(imag);
                col ++;
                iss.ignore();
            }
            row ++;
        }

        int index = 0;
        for(int j = 0;j < COL;j ++) {
            for(int i = 0;i < ROW;i ++) {
                complex_numbers[index ++] = tmp[i][j].real();
                complex_numbers[index ++] = tmp[i][j].imag();
            }
        }

        file.close();
}

template<int COL, int ROW>
void writeU(const std::string& filename, const complex<float> complex_numbers[COL][ROW]) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file for writing." << std::endl;
        return;
    }

    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            if(complex_numbers[j][i].real() > 0) file << "+";
            file << complex_numbers[j][i].real();
            if(complex_numbers[j][i].imag() > 0) file << "+";
            file << complex_numbers[j][i].imag() << "j";
            if (j < COL - 1) {
                file << " ";
            }
        }
        file << "\n";
    }
    file.close();
}

template<int COL, int ROW>
void writeSigma(const std::string& filename, const complex<float> complex_numbers[COL][ROW]) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file for writing." << std::endl;
        return;
    }

    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            if(i == j)
                file << complex_numbers[j][i].real();
        }
        file << "\n";
    }
    file.close();
}