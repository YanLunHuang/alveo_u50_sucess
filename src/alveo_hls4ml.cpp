/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/*******************************************************************************
Description:
    HLS pragmas can be used to optimize the design : improve throughput, reduce latency and 
    device resource utilization of the resulting RTL code
    This is a wrapper to be used with an hls4ml project to enable proper handling by SDAccel
*******************************************************************************/
#include <iostream>

#include "myproject.h"
#include "kernel_params.h"


extern "C" {

void alveo_hls4ml(
	const bigdata_t *in, // Read-Only Vector
	bigdata_t *out       // Output Result
	)
{
	#pragma HLS INTERFACE m_axi port=in  offset=slave bundle=gmem
	#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem
	#pragma HLS INTERFACE s_axilite port=in   bundle=control
	#pragma HLS INTERFACE s_axilite port=out  bundle=control
	#pragma HLS INTERFACE s_axilite port=return bundle=control
	//necessary for hls4ml kernel, not used
	#pragma HLS DATAFLOW

	bigdata_t in_bigbuf[DATA_SIZE_IN*IN_STREAM_LEN];
	bigdata_t out_bigbuf;
	
	hls::stream<input_t> in_buf;
	hls::stream<result_t> out_buf;
	//these will get partitioned properly in the hls4ml code

	//#pragma HLS ARRAY_PARTITION   variable=in_buf  complete dim=0
	//#pragma HLS ARRAY_PARTITION   variable=out_buf complete dim=0
	#pragma HLS STREAM   variable=in_buf  depth=1000
	#pragma HLS STREAM   variable=out_buf depth=1
	
	//getting data from DRAM
	for (int i = 0; i < DATA_SIZE_IN*IN_STREAM_LEN; i++) {
		#pragma HLS LOOP UNROLL
		in_bigbuf[i] = in[i];
		//std::cout <<int(in[i])<<std::endl;
	}
	
	std::cout<<"------------------"<<std::endl;
	//=============================================
	//input
	//=============================================
	input_t tmp;
	for(int i0 = 0; i0 < IN_STREAM_LEN; i0++) { 
		for(int i1 = 0; i1 < DATA_SIZE_IN; i1++) { 
			#pragma HLS UNROLL
			tmp = in_bigbuf[i0*4+i1];
			in_buf.write(tmp);
			//std::cout<<double(tmp)<<std::endl;
		}
	}
	
	std::cout<<"inf start"<<std::endl;
	myproject(in_buf,out_buf);
	std::cout<<"inf end"<<std::endl;

	//=============================================
	//output
	//=============================================
	for(int i1 = 0; i1 < DATA_SIZE_OUT*OUT_STREAM_LEN; i1++) {
		#pragma HLS UNROLL
		//std::cout<<"reading from ["<<i1<<"]"<<std::endl;
		result_t tmp_small = out_buf.read();
		out_bigbuf = tmp_small;
	}
	out[0] = out_bigbuf;
	out[1] = 0;
	//std::cout <<(double)out_bigbuf<<std::endl;
}
}
