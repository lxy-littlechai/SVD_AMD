#pragma once
#include "../aie/aie_graph_params.h"

const int W = 128;
const int DDR_WIDTH = 128;

typedef ap_axiu<W, 0, 0, 0> axis_pkt;
typedef hls::stream<ap_uint<W>> axis_stream;
typedef hls::stream<axis_pkt> axis_pkt_stream;

const int COL = col_num;
const int ROW = row_num;
const int NUM = COL*(ROW + COL)/4;

