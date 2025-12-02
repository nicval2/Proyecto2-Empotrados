
module system_on_chip (
	audio_BCLK,
	audio_DACDAT,
	audio_DACLRCK,
	audio_clk_clk,
	audio_config_SDAT,
	audio_config_SCLK,
	button_export,
	clk_clk,
	id7_segment_export,
	reset_reset_n,
	switches_export,
	vga_controller_CLK,
	vga_controller_HS,
	vga_controller_VS,
	vga_controller_BLANK,
	vga_controller_SYNC,
	vga_controller_R,
	vga_controller_G,
	vga_controller_B);	

	input		audio_BCLK;
	output		audio_DACDAT;
	input		audio_DACLRCK;
	output		audio_clk_clk;
	inout		audio_config_SDAT;
	output		audio_config_SCLK;
	input	[3:0]	button_export;
	input		clk_clk;
	output	[31:0]	id7_segment_export;
	input		reset_reset_n;
	input	[7:0]	switches_export;
	output		vga_controller_CLK;
	output		vga_controller_HS;
	output		vga_controller_VS;
	output		vga_controller_BLANK;
	output		vga_controller_SYNC;
	output	[7:0]	vga_controller_R;
	output	[7:0]	vga_controller_G;
	output	[7:0]	vga_controller_B;
endmodule
