

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#include <hls_streamofblocks.h>
#include <hls_print.h>
#include "config.h"

void send(ap_uint<DDR_WIDTH>* dataIn, axis_pkt_stream& toAIEstrm0, axis_pkt_stream& toAIEstrm1) {
  axis_pkt tmp0, tmp1;
  ap_uint<W> tmp[2];
  for(int i = 0;i < NUM;i += 2) {
    tmp[0] = dataIn[i];
    tmp[1] = dataIn[i+1];

    for(int j = 0;j < 4;j ++) {
      tmp0.data(32*(j+1)-1, 32*j) = tmp[0](32*(j+1)-1, 32*j);
      tmp1.data(32*(j+1)-1, 32*j) = tmp[1](32*(j+1)-1, 32*j);
    }
    
    toAIEstrm0.write(tmp0);   
    toAIEstrm1.write(tmp1);

  }

}

void receive(ap_uint<DDR_WIDTH>* dataOut, axis_pkt_stream& fromAIEstrm0, axis_pkt_stream& fromAIEstrm1) {
  axis_pkt tmp0, tmp1;
  ap_uint<W> tmp[2];
  for(int i = 0;i < NUM;i += 2) {
    tmp0 = fromAIEstrm0.read();
    tmp1 = fromAIEstrm1.read();

    for(int j = 0;j < 4;j ++) {
      tmp[0](32*(j+1)-1, 32*j) = tmp0.data(32*(j+1)-1, 32*j);
      tmp[1](32*(j+1)-1, 32*j) = tmp1.data(32*(j+1)-1, 32*j);
    }
    
    dataOut[i] = tmp[0];
    dataOut[i+1] = tmp[1];

  }
}

void TopFunc(ap_uint<DDR_WIDTH>* dataIn, ap_uint<DDR_WIDTH>* dataOut,
             axis_pkt_stream& toAIEstrm0, axis_pkt_stream& toAIEstrm1,
             axis_pkt_stream& fromAIEstrm0, axis_pkt_stream& fromAIEstrm1) {
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem0 port = dataIn depth = 8192
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem0 port = dataOut depth = 8192
#pragma HLS INTERFACE s_axilite port = dataIn bundle = control
#pragma HLS INTERFACE s_axilite port = dataOut bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS INTERFACE axis port = toAIEstrm0
#pragma HLS INTERFACE axis port = toAIEstrm1
#pragma HLS INTERFACE axis port = fromAIEstrm0
#pragma HLS INTERFACE axis port = fromAIEstrm1

    #pragma HLS dataflow
    send(dataIn, toAIEstrm0, toAIEstrm1);
    receive(dataOut, fromAIEstrm0, fromAIEstrm1);
}



