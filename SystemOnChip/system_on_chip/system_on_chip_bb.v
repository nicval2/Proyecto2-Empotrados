
module system_on_chip (
	clk_clk,
	id7_segment_export,
	button_export,
	reset_reset_n,
	switches_export);	

	input		clk_clk;
	output	[31:0]	id7_segment_export;
	input	[3:0]	button_export;
	input		reset_reset_n;
	input	[7:0]	switches_export;
endmodule
