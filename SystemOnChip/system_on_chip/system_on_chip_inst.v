	system_on_chip u0 (
		.audio_BCLK           (<connected-to-audio_BCLK>),           //          audio.BCLK
		.audio_DACDAT         (<connected-to-audio_DACDAT>),         //               .DACDAT
		.audio_DACLRCK        (<connected-to-audio_DACLRCK>),        //               .DACLRCK
		.audio_clk_clk        (<connected-to-audio_clk_clk>),        //      audio_clk.clk
		.audio_config_SDAT    (<connected-to-audio_config_SDAT>),    //   audio_config.SDAT
		.audio_config_SCLK    (<connected-to-audio_config_SCLK>),    //               .SCLK
		.button_export        (<connected-to-button_export>),        //         button.export
		.clk_clk              (<connected-to-clk_clk>),              //            clk.clk
		.id7_segment_export   (<connected-to-id7_segment_export>),   //    id7_segment.export
		.memory_mem_a         (<connected-to-memory_mem_a>),         //         memory.mem_a
		.memory_mem_ba        (<connected-to-memory_mem_ba>),        //               .mem_ba
		.memory_mem_ck        (<connected-to-memory_mem_ck>),        //               .mem_ck
		.memory_mem_ck_n      (<connected-to-memory_mem_ck_n>),      //               .mem_ck_n
		.memory_mem_cke       (<connected-to-memory_mem_cke>),       //               .mem_cke
		.memory_mem_cs_n      (<connected-to-memory_mem_cs_n>),      //               .mem_cs_n
		.memory_mem_ras_n     (<connected-to-memory_mem_ras_n>),     //               .mem_ras_n
		.memory_mem_cas_n     (<connected-to-memory_mem_cas_n>),     //               .mem_cas_n
		.memory_mem_we_n      (<connected-to-memory_mem_we_n>),      //               .mem_we_n
		.memory_mem_reset_n   (<connected-to-memory_mem_reset_n>),   //               .mem_reset_n
		.memory_mem_dq        (<connected-to-memory_mem_dq>),        //               .mem_dq
		.memory_mem_dqs       (<connected-to-memory_mem_dqs>),       //               .mem_dqs
		.memory_mem_dqs_n     (<connected-to-memory_mem_dqs_n>),     //               .mem_dqs_n
		.memory_mem_odt       (<connected-to-memory_mem_odt>),       //               .mem_odt
		.memory_mem_dm        (<connected-to-memory_mem_dm>),        //               .mem_dm
		.memory_oct_rzqin     (<connected-to-memory_oct_rzqin>),     //               .oct_rzqin
		.reset_reset_n        (<connected-to-reset_reset_n>),        //          reset.reset_n
		.switches_export      (<connected-to-switches_export>),      //       switches.export
		.vga_controller_CLK   (<connected-to-vga_controller_CLK>),   // vga_controller.CLK
		.vga_controller_HS    (<connected-to-vga_controller_HS>),    //               .HS
		.vga_controller_VS    (<connected-to-vga_controller_VS>),    //               .VS
		.vga_controller_BLANK (<connected-to-vga_controller_BLANK>), //               .BLANK
		.vga_controller_SYNC  (<connected-to-vga_controller_SYNC>),  //               .SYNC
		.vga_controller_R     (<connected-to-vga_controller_R>),     //               .R
		.vga_controller_G     (<connected-to-vga_controller_G>),     //               .G
		.vga_controller_B     (<connected-to-vga_controller_B>)      //               .B
	);

