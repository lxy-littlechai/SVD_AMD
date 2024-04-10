#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <time.h>
#include <vector>
#include <math.h>
#include <string>
#include <complex>
#include <xrt/xrt_device.h>
#include <experimental/xrt_xclbin.h>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>
#include <experimental/xrt_aie.h>
#include <experimental/xrt_graph.h>
#include "../aie/aie_graph_params.h"
#include "svd.hpp"

using namespace std;
int main(int argc, char** argv) {
    
    
    const int COL = col_num;
    const int ROW = row_num;
    const int NUM = COL*(ROW + COL)*2;

    const int DATA_SIZE_BYTE = NUM*sizeof(float);
    const int RES_SIZE_BYTE = NUM*sizeof(float);
 
    unsigned int dev_index = 0;
    auto device = xrt::device(dev_index);
    auto xclbin_uuid = device.load_xclbin(argv[1]);
    int ITER = 1;
    if(argc == 3) sscanf (argv[2], "%i", &ITER);
    

    auto kernel = xrt::kernel(device, xclbin_uuid, "TopFunc");

    auto input_buffer = xrt::bo(device, DATA_SIZE_BYTE, kernel.group_id(0));
    auto output_buffer = xrt::bo(device, RES_SIZE_BYTE, kernel.group_id(1));

    auto data = input_buffer.map<float*>();
    auto result = output_buffer.map<float*>();
    complex<float>U[COL][ROW], S[COL][COL];

    float matrix[COL*ROW*2];

    read<COL, ROW>("./data/matrix.txt", matrix);

    std::cout << "data\n";
    int index = 0, matrix_id = 0;
    for(int j = 0;j < COL;j ++) {
        for(int i = 0;i < ROW;i ++) {
            data[index ++] = matrix[matrix_id ++];
            data[index ++] = matrix[matrix_id ++];
        }
        for(int i = 0;i < COL;i ++) {
            data[index ++] = 0;
            data[index ++] = 0;
        }
        
    }

    auto topGraph = xrt::graph(device, xclbin_uuid, "mygraph");
    topGraph.run(ITER);

    for(int i = 0;i < ITER;i ++) {
        input_buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        auto run = kernel(input_buffer, output_buffer);
        run.wait();
        output_buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        for(int j = 0;j < NUM;j ++) {
            data[j] = result[j];
        }
        for(int j = 0;j < COL;j ++) {
            for(int i = 0;i < ROW;i ++) {
                U[j][i].real(result[index ++]);
                U[j][i].imag(result[index ++]);
            }
            for(int i = 0;i < COL;i ++) {
                S[j][i].real(result[index ++]);
                S[j][i].imag(result[index ++]);
            }

        }
        for(int i = 0;i < COL;i ++) {
            for(int j = 0;j < COL;j ++) {
                cout << S[j][i] << " ";
            }
            cout <<endl;
        }
    }
    
    
    topGraph.end(0);

    
   

    std::cout << "result\n";
    for(int j = 0;j < NUM;j ++) {
        cout << result[j] << " ";
    }
    cout << endl;

    
    index = 0;
    for(int j = 0;j < COL;j ++) {
        for(int i = 0;i < ROW;i ++) {
            U[j][i].real(result[index ++]);
            U[j][i].imag(result[index ++]);
        }
        for(int i = 0;i < COL;i ++) {
            S[j][i].real(result[index ++]);
            S[j][i].imag(result[index ++]);
        }
        
    }

    writeU<COL, ROW>("./data/test/U.txt", U);
    cout << "U\n";
    for(int i = 0;i < ROW;i ++) {
        for(int j = 0;j < COL;j ++) {
            cout << U[j][i] << " ";
        }
        cout <<endl;
    }

    writeSigma<COL, ROW>("./data/test/Sigma.txt", S);
    cout << "S\n";
    for(int i = 0;i < COL;i ++) {
        for(int j = 0;j < COL;j ++) {
            cout << S[j][i] << " ";
        }
        cout <<endl;
    }
    

    
    return 0;
}

