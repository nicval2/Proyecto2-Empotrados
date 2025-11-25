
module system_on_chip (
	clk_clk,
	reg_7_segments_external_connection_export,
	reg_button_external_connection_export,
	reg_switches_external_connection_export,
	reset_reset_n);	

	input		clk_clk;
	output	[31:0]	reg_7_segments_external_connection_export;
	input	[3:0]	reg_button_external_connection_export;
	input	[9:0]	reg_switches_external_connection_export;
	input		reset_reset_n;
endmodule
