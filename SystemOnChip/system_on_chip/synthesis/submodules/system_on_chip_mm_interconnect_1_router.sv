// (C) 2001-2018 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Intel Program License Subscription 
// Agreement, Intel FPGA IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Intel and sold by 
// Intel or its authorized distributors.  Please refer to the applicable 
// agreement for further details.



// Your use of Altera Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Altera Program License Subscription 
// Agreement, Altera MegaCore Function License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Altera and sold by 
// Altera or its authorized distributors.  Please refer to the applicable 
// agreement for further details.


// $Id: //acds/rel/18.0std/ip/merlin/altera_merlin_router/altera_merlin_router.sv.terp#1 $
// $Revision: #1 $
// $Date: 2018/01/31 $
// $Author: psgswbuild $

// -------------------------------------------------------
// Merlin Router
//
// Asserts the appropriate one-hot encoded channel based on 
// either (a) the address or (b) the dest id. The DECODER_TYPE
// parameter controls this behaviour. 0 means address decoder,
// 1 means dest id decoder.
//
// In the case of (a), it also sets the destination id.
// -------------------------------------------------------

`timescale 1 ns / 1 ns

module system_on_chip_mm_interconnect_1_router_default_decode
  #(
     parameter DEFAULT_CHANNEL = 1,
               DEFAULT_WR_CHANNEL = -1,
               DEFAULT_RD_CHANNEL = -1,
<<<<<<< HEAD
               DEFAULT_DESTID = 2 
   )
  (output [136 - 135 : 0] default_destination_id,
   output [4-1 : 0] default_wr_channel,
   output [4-1 : 0] default_rd_channel,
   output [4-1 : 0] default_src_channel
=======
<<<<<<< HEAD:SystemOnChip/system_on_chip/synthesis/submodules/system_on_chip_mm_interconnect_1_router.sv
               DEFAULT_DESTID = 0 
   )
  (output [87 - 87 : 0] default_destination_id,
   output [2-1 : 0] default_wr_channel,
   output [2-1 : 0] default_rd_channel,
   output [2-1 : 0] default_src_channel
=======
               DEFAULT_DESTID = 1 
   )
  (output [76 - 74 : 0] default_destination_id,
   output [7-1 : 0] default_wr_channel,
   output [7-1 : 0] default_rd_channel,
   output [7-1 : 0] default_src_channel
>>>>>>> origin/Develop:SystemOnChip/system_on_chip2/testbench/system_on_chip_tb/simulation/submodules/system_on_chip_mm_interconnect_0_router_001.sv
>>>>>>> origin/Develop
  );

  assign default_destination_id = 
    DEFAULT_DESTID[136 - 135 : 0];

  generate
    if (DEFAULT_CHANNEL == -1) begin : no_default_channel_assignment
      assign default_src_channel = '0;
    end
    else begin : default_channel_assignment
<<<<<<< HEAD
      assign default_src_channel = 4'b1 << DEFAULT_CHANNEL;
=======
<<<<<<< HEAD:SystemOnChip/system_on_chip/synthesis/submodules/system_on_chip_mm_interconnect_1_router.sv
      assign default_src_channel = 2'b1 << DEFAULT_CHANNEL;
=======
      assign default_src_channel = 7'b1 << DEFAULT_CHANNEL;
>>>>>>> origin/Develop:SystemOnChip/system_on_chip2/testbench/system_on_chip_tb/simulation/submodules/system_on_chip_mm_interconnect_0_router_001.sv
>>>>>>> origin/Develop
    end
  endgenerate

  generate
    if (DEFAULT_RD_CHANNEL == -1) begin : no_default_rw_channel_assignment
      assign default_wr_channel = '0;
      assign default_rd_channel = '0;
    end
    else begin : default_rw_channel_assignment
<<<<<<< HEAD
      assign default_wr_channel = 4'b1 << DEFAULT_WR_CHANNEL;
      assign default_rd_channel = 4'b1 << DEFAULT_RD_CHANNEL;
=======
<<<<<<< HEAD:SystemOnChip/system_on_chip/synthesis/submodules/system_on_chip_mm_interconnect_1_router.sv
      assign default_wr_channel = 2'b1 << DEFAULT_WR_CHANNEL;
      assign default_rd_channel = 2'b1 << DEFAULT_RD_CHANNEL;
=======
      assign default_wr_channel = 7'b1 << DEFAULT_WR_CHANNEL;
      assign default_rd_channel = 7'b1 << DEFAULT_RD_CHANNEL;
>>>>>>> origin/Develop:SystemOnChip/system_on_chip2/testbench/system_on_chip_tb/simulation/submodules/system_on_chip_mm_interconnect_0_router_001.sv
>>>>>>> origin/Develop
    end
  endgenerate

endmodule


module system_on_chip_mm_interconnect_1_router
(
    // -------------------
    // Clock & Reset
    // -------------------
    input clk,
    input reset,

    // -------------------
    // Command Sink (Input)
    // -------------------
    input                       sink_valid,
    input  [161-1 : 0]    sink_data,
    input                       sink_startofpacket,
    input                       sink_endofpacket,
    output                      sink_ready,

    // -------------------
    // Command Source (Output)
    // -------------------
    output                          src_valid,
<<<<<<< HEAD
    output reg [161-1    : 0] src_data,
    output reg [4-1 : 0] src_channel,
=======
<<<<<<< HEAD:SystemOnChip/system_on_chip/synthesis/submodules/system_on_chip_mm_interconnect_1_router.sv
    output reg [112-1    : 0] src_data,
    output reg [2-1 : 0] src_channel,
=======
    output reg [90-1    : 0] src_data,
    output reg [7-1 : 0] src_channel,
>>>>>>> origin/Develop:SystemOnChip/system_on_chip2/testbench/system_on_chip_tb/simulation/submodules/system_on_chip_mm_interconnect_0_router_001.sv
>>>>>>> origin/Develop
    output                          src_startofpacket,
    output                          src_endofpacket,
    input                           src_ready
);

    // -------------------------------------------------------
    // Local parameters and variables
    // -------------------------------------------------------
<<<<<<< HEAD
    localparam PKT_ADDR_H = 101;
    localparam PKT_ADDR_L = 72;
    localparam PKT_DEST_ID_H = 136;
    localparam PKT_DEST_ID_L = 135;
    localparam PKT_PROTECTION_H = 151;
    localparam PKT_PROTECTION_L = 149;
    localparam ST_DATA_W = 161;
    localparam ST_CHANNEL_W = 4;
=======
    localparam PKT_ADDR_H = 56;
    localparam PKT_ADDR_L = 36;
<<<<<<< HEAD:SystemOnChip/system_on_chip/synthesis/submodules/system_on_chip_mm_interconnect_1_router.sv
    localparam PKT_DEST_ID_H = 87;
    localparam PKT_DEST_ID_L = 87;
    localparam PKT_PROTECTION_H = 102;
    localparam PKT_PROTECTION_L = 100;
    localparam ST_DATA_W = 112;
    localparam ST_CHANNEL_W = 2;
=======
    localparam PKT_DEST_ID_H = 76;
    localparam PKT_DEST_ID_L = 74;
    localparam PKT_PROTECTION_H = 80;
    localparam PKT_PROTECTION_L = 78;
    localparam ST_DATA_W = 90;
    localparam ST_CHANNEL_W = 7;
>>>>>>> origin/Develop:SystemOnChip/system_on_chip2/testbench/system_on_chip_tb/simulation/submodules/system_on_chip_mm_interconnect_0_router_001.sv
>>>>>>> origin/Develop
    localparam DECODER_TYPE = 0;

    localparam PKT_TRANS_WRITE = 104;
    localparam PKT_TRANS_READ  = 105;

    localparam PKT_ADDR_W = PKT_ADDR_H-PKT_ADDR_L + 1;
    localparam PKT_DEST_ID_W = PKT_DEST_ID_H-PKT_DEST_ID_L + 1;



    // -------------------------------------------------------
    // Figure out the number of bits to mask off for each slave span
    // during address decoding
    // -------------------------------------------------------
    localparam PAD0 = log2ceil(64'h8864 - 64'h8860); 
    localparam PAD1 = log2ceil(64'h8920 - 64'h8900); 
    localparam PAD2 = log2ceil(64'h8954 - 64'h8950); 
    localparam PAD3 = log2ceil(64'h8980 - 64'h8960); 
    // -------------------------------------------------------
    // Work out which address bits are significant based on the
    // address range of the slaves. If the required width is too
    // large or too small, we use the address field width instead.
    // -------------------------------------------------------
    localparam ADDR_RANGE = 64'h8980;
    localparam RANGE_ADDR_WIDTH = log2ceil(ADDR_RANGE);
    localparam OPTIMIZED_ADDR_H = (RANGE_ADDR_WIDTH > PKT_ADDR_W) ||
                                  (RANGE_ADDR_WIDTH == 0) ?
                                        PKT_ADDR_H :
                                        PKT_ADDR_L + RANGE_ADDR_WIDTH - 1;

    localparam RG = RANGE_ADDR_WIDTH-1;
    localparam REAL_ADDRESS_RANGE = OPTIMIZED_ADDR_H - PKT_ADDR_L;

      reg [PKT_ADDR_W-1 : 0] address;
      always @* begin
        address = {PKT_ADDR_W{1'b0}};
        address [REAL_ADDRESS_RANGE:0] = sink_data[OPTIMIZED_ADDR_H : PKT_ADDR_L];
      end   

    // -------------------------------------------------------
    // Pass almost everything through, untouched
    // -------------------------------------------------------
    assign sink_ready        = src_ready;
    assign src_valid         = sink_valid;
    assign src_startofpacket = sink_startofpacket;
    assign src_endofpacket   = sink_endofpacket;
    wire [PKT_DEST_ID_W-1:0] default_destid;
<<<<<<< HEAD
    wire [4-1 : 0] default_src_channel;
=======
<<<<<<< HEAD:SystemOnChip/system_on_chip/synthesis/submodules/system_on_chip_mm_interconnect_1_router.sv
    wire [2-1 : 0] default_src_channel;
=======
    wire [7-1 : 0] default_src_channel;
>>>>>>> origin/Develop:SystemOnChip/system_on_chip2/testbench/system_on_chip_tb/simulation/submodules/system_on_chip_mm_interconnect_0_router_001.sv
>>>>>>> origin/Develop




    // -------------------------------------------------------
    // Write and read transaction signals
    // -------------------------------------------------------
    wire write_transaction;
    assign write_transaction = sink_data[PKT_TRANS_WRITE];
    wire read_transaction;
    assign read_transaction  = sink_data[PKT_TRANS_READ];


    system_on_chip_mm_interconnect_1_router_default_decode the_default_decode(
      .default_destination_id (default_destid),
      .default_wr_channel   (),
      .default_rd_channel   (),
      .default_src_channel  (default_src_channel)
    );

    always @* begin
        src_data    = sink_data;
        src_channel = default_src_channel;
        src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = default_destid;

        // --------------------------------------------------
        // Address Decoder
        // Sets the channel and destination ID based on the address
        // --------------------------------------------------

<<<<<<< HEAD
    // ( 0x8860 .. 0x8864 )
    if ( {address[RG:PAD0],{PAD0{1'b0}}} == 16'h8860  && write_transaction  ) begin
            src_channel = 4'b0001;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 3;
    end

    // ( 0x8900 .. 0x8920 )
    if ( {address[RG:PAD1],{PAD1{1'b0}}} == 16'h8900   ) begin
            src_channel = 4'b0010;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 2;
    end

    // ( 0x8950 .. 0x8954 )
    if ( {address[RG:PAD2],{PAD2{1'b0}}} == 16'h8950  && read_transaction  ) begin
            src_channel = 4'b0100;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 1;
    end

    // ( 0x8960 .. 0x8980 )
    if ( {address[RG:PAD3],{PAD3{1'b0}}} == 16'h8960   ) begin
            src_channel = 4'b1000;
=======
<<<<<<< HEAD:SystemOnChip/system_on_chip/synthesis/submodules/system_on_chip_mm_interconnect_1_router.sv
    // ( 0x3060 .. 0x3064 )
    if ( {address[RG:PAD0],{PAD0{1'b0}}} == 14'h3060  && write_transaction  ) begin
            src_channel = 2'b01;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 1;
    end

    // ( 0x3100 .. 0x3120 )
    if ( {address[RG:PAD1],{PAD1{1'b0}}} == 14'h3100   ) begin
            src_channel = 2'b10;
=======
    // ( 0x0 .. 0x2000 )
    if ( {address[RG:PAD0],{PAD0{1'b0}}} == 14'h0   ) begin
            src_channel = 7'b10;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 1;
    end

    // ( 0x2000 .. 0x2800 )
    if ( {address[RG:PAD1],{PAD1{1'b0}}} == 14'h2000   ) begin
            src_channel = 7'b01;
>>>>>>> origin/Develop:SystemOnChip/system_on_chip2/testbench/system_on_chip_tb/simulation/submodules/system_on_chip_mm_interconnect_0_router_001.sv
>>>>>>> origin/Develop
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 0;
    end

end


    // --------------------------------------------------
    // Ceil(log2()) function
    // --------------------------------------------------
    function integer log2ceil;
        input reg[65:0] val;
        reg [65:0] i;

        begin
            i = 1;
            log2ceil = 0;

            while (i < val) begin
                log2ceil = log2ceil + 1;
                i = i << 1;
            end
        end
    endfunction

endmodule


