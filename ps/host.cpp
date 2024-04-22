#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <time.h>
#include <vector>
#include <math.h>
#include <string>
#include <complex>
#include <iostream>
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
    const int NUM = COL*(ROW + COL);

    const int DATA_SIZE_BYTE = NUM*sizeof(float);
    const int RES_SIZE_BYTE = NUM*sizeof(float);
 
    unsigned int dev_index = 0;
    auto device = xrt::device(dev_index);
    auto xclbin_uuid = device.load_xclbin(argv[1]);
    int ITER = 1;
    if(argc == 3) ITER = atoi(argv[2]);
    

    auto kernel = xrt::kernel(device, xclbin_uuid, "TopFunc");

    auto input_buffer0 = xrt::bo(device, DATA_SIZE_BYTE, kernel.group_id(0));
    auto input_buffer1 = xrt::bo(device, DATA_SIZE_BYTE, kernel.group_id(1));
    auto output_buffer0 = xrt::bo(device, RES_SIZE_BYTE, kernel.group_id(2));
    auto output_buffer1 = xrt::bo(device, RES_SIZE_BYTE, kernel.group_id(3));

    auto in0 = input_buffer0.map<float*>();
    auto in1 = input_buffer1.map<float*>();
    auto out0 = output_buffer0.map<float*>();
    auto out1 = output_buffer1.map<float*>();
    complex<float>U[COL][ROW], S[COL][COL];
    for(int j = 0;j < COL;j ++) {
        for(int i = 0;i < ROW;i ++) {
            S[j][i].real(0);
            S[j][i].imag(0);
        }
    }

    read<4>("./data/A0.txt", in0);
    read<4>("./data/A1.txt", in1);

    std::cout << "data\n";
    for(int j = 0;j < NUM;j ++) {
        cout << in0[j] << " ";
    }
    cout << endl;
    cout << endl;
    for(int j = 0;j < NUM;j ++) {
        cout << in1[j] << " ";
    }
    cout << endl;

    int index0 = 0, index1 = 0;
    for(int j = 0;j < COL;j ++) {
            for(int i = 0;i < ROW;i ++) {
                if(i%4 < 2) {
                    U[j][i].real(in0[index0 ++]);
                    U[j][i].imag(in0[index0 ++]);
                }
                else {
                    U[j][i].real(in1[index1 ++]);
                    U[j][i].imag(in1[index1 ++]);
                }
                
            }
            for(int i = 0;i < COL;i ++) {
                if(i%4 < 2) {
                    S[j][i].real(in0[index0 ++]);
                    S[j][i].imag(in0[index0 ++]);
                }
                else {
                    S[j][i].real(in1[index1 ++]);
                    S[j][i].imag(in1[index1 ++]);
                }
            }
        }

    auto topGraph = xrt::graph(device, xclbin_uuid, "mygraph");
    topGraph.run(ITER);

    for(int k = 0;k < ITER;k ++) {

        cout << "Start ---------------------------\n";
        for(int i = 0;i < COL;i ++) {
            for(int j = 0;j < COL;j ++) {
                cout << U[j][i] << " ";
            }
            cout <<endl;
        }

        for(int i = 0;i < COL;i ++) {
            for(int j = 0;j < COL;j ++) {
                cout << S[j][i] << " ";
            }
            cout <<endl;
        }
        cout << "---------------------------\n\n";


        input_buffer0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        input_buffer1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        auto run = kernel(input_buffer0, input_buffer1, output_buffer0, output_buffer1);
        run.wait();
        output_buffer0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        output_buffer1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        
        int index0 = 0, index1 = 0;
        for(int j = 0;j < COL;j ++) {
            for(int i = 0;i < ROW;i ++) {
                if(i%4 < 2) {
                    U[j][i].real(out0[index0 ++]);
                    U[j][i].imag(out0[index0 ++]);
                }
                else {
                    U[j][i].real(out1[index1 ++]);
                    U[j][i].imag(out1[index1 ++]);
                }
                
            }
            for(int i = 0;i < COL;i ++) {
                if(i%4 < 2) {
                    S[j][i].real(out0[index0 ++]);
                    S[j][i].imag(out0[index0 ++]);
                }
                else {
                    S[j][i].real(out1[index1 ++]);
                    S[j][i].imag(out1[index1 ++]);
                }
            }
        }

        cout << "Result ---------------------------\n";
        for(int i = 0;i < COL;i ++) {
            for(int j = 0;j < COL;j ++) {
                cout << U[j][i] << " ";
            }
            cout <<endl;
        }

        for(int i = 0;i < COL;i ++) {
            for(int j = 0;j < COL;j ++) {
                cout << S[j][i] << " ";
            }
            cout <<endl;
        }
        cout << "---------------------------\n\n";

        if(k != ITER-1) {
            for(int j = 0;j < COL;j ++) {
                float sigma = S[j][j].real();
                for(int i = 0;i < ROW;i ++) {
                    S[j][i].real(0);
                    S[j][i].imag(0);
                    float re = U[j][i].real();
                    float im = U[j][i].imag();
                    U[j][i].real(re*sigma);
                    U[j][i].imag(im*sigma);
                }
            }
        }

        index0 = 0, index1 = 0;
        for(int j = 0;j < COL;j ++) {
            for(int i = 0;i < ROW;i ++) {
                if(i%4 < 2) {
                    in0[index0 ++] = U[j][i].real();
                    in0[index0 ++] = U[j][i].imag();
                }
                else {
                    in1[index1 ++] = U[j][i].real();
                    in1[index1 ++] = U[j][i].imag();
                }
                
            }
            for(int i = 0;i < COL;i ++) {
                if(i%4 < 2) {
                    in0[index0 ++] = S[j][i].real();
                    in0[index0 ++] = S[j][i].imag();
                }
                else {
                    in1[index1 ++] = S[j][i].real();
                    in1[index1 ++] = S[j][i].imag();
                }
            }
        }

        
    }
    
    
    topGraph.wait(0);

    
   

    std::cout << "result\n";
    for(int j = 0;j < NUM;j ++) {
        cout << out0[j] << " ";
    }
    cout << endl;
    cout << endl;
    for(int j = 0;j < NUM;j ++) {
        cout << out1[j] << " ";
    }
    cout << endl;

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

