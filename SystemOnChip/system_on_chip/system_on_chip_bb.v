
module system_on_chip (
	button_export,
	clk_clk,
	id7_segment_export,
	lcd_DATA,
	lcd_ON,
	lcd_BLON,
	lcd_EN,
	lcd_RS,
	lcd_RW,
	memory_mem_a,
	memory_mem_ba,
	memory_mem_ck,
	memory_mem_ck_n,
	memory_mem_cke,
	memory_mem_cs_n,
	memory_mem_ras_n,
	memory_mem_cas_n,
	memory_mem_we_n,
	memory_mem_reset_n,
	memory_mem_dq,
	memory_mem_dqs,
	memory_mem_dqs_n,
	memory_mem_odt,
	memory_mem_dm,
	memory_oct_rzqin,
	reset_reset_n,
	switches_export,
	audio_clk_clk,
	audio_BCLK,
	audio_DACDAT,
	audio_DACLRCK,
	audio_config_SDAT,
	audio_config_SCLK);	

	input	[3:0]	button_export;
	input		clk_clk;
	output	[31:0]	id7_segment_export;
	inout	[7:0]	lcd_DATA;
	output		lcd_ON;
	output		lcd_BLON;
	output		lcd_EN;
	output		lcd_RS;
	output		lcd_RW;
	output	[12:0]	memory_mem_a;
	output	[2:0]	memory_mem_ba;
	output		memory_mem_ck;
	output		memory_mem_ck_n;
	output		memory_mem_cke;
	output		memory_mem_cs_n;
	output		memory_mem_ras_n;
	output		memory_mem_cas_n;
	output		memory_mem_we_n;
	output		memory_mem_reset_n;
	inout	[7:0]	memory_mem_dq;
	inout		memory_mem_dqs;
	inout		memory_mem_dqs_n;
	output		memory_mem_odt;
	output		memory_mem_dm;
	input		memory_oct_rzqin;
	input		reset_reset_n;
	input	[7:0]	switches_export;
	output		audio_clk_clk;
	input		audio_BCLK;
	output		audio_DACDAT;
	input		audio_DACLRCK;
	inout		audio_config_SDAT;
	output		audio_config_SCLK;
endmodule
