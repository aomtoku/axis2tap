module app (
	input  clk,
	input  rst,
	
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


/*
 * Bridge
 */

assign m_axis_tx_tvalid = s_axis_rx_tvalid;
assign m_axis_tx_tdata = s_axis_rx_tdata;
assign m_axis_tx_tkeep = s_axis_rx_tkeep;
assign m_axis_tx_tlast = s_axis_rx_tlast;
assign m_axis_tx_tuser = 1'b0;



endmodule 
