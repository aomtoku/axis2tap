`default_nettype none
`timescale 1ns / 1ns

module testbench #(
	parameter max_recvpkt = 10,
	parameter nPreamble = 8,
	parameter nIFG = 12
)(
	input wire clk100,
	input wire cold_reset, 
	/* AXIS RX */
	input  wire        s_axis_rx_tvalid,
	input  wire [63:0] s_axis_rx_tdata,
	input  wire [ 7:0] s_axis_rx_tkeep,
	input  wire        s_axis_rx_tlast,
	input  wire        s_axis_rx_tuser,
	
	/* AXIS TX */
	input  wire        m_axis_tx_tready,
	output wire        m_axis_tx_tvalid,
	output wire [63:0] m_axis_tx_tdata,
	output wire [ 7:0] m_axis_tx_tkeep,
	output wire        m_axis_tx_tlast,
	output wire        m_axis_tx_tuser
);

app u_app (
	.clk (clk100),
	.rst (cold_reset),
	.*
);


endmodule

`default_nettype wire

