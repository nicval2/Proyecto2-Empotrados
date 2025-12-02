	system_on_chip u0 (
		.clk_clk                                 (<connected-to-clk_clk>),                                 //                              clk.clk
		.reg_button_external_connection_export   (<connected-to-reg_button_external_connection_export>),   //   reg_button_external_connection.export
		.reg_leds_external_connection_export     (<connected-to-reg_leds_external_connection_export>),     //     reg_leds_external_connection.export
		.reg_switches_external_connection_export (<connected-to-reg_switches_external_connection_export>), // reg_switches_external_connection.export
		.reset_reset_n                           (<connected-to-reset_reset_n>),                           //                            reset.reset_n
		.timer_irq_irq                           (<connected-to-timer_irq_irq>),                           //                        timer_irq.irq
		.uart_irq_irq                            (<connected-to-uart_irq_irq>)                             //                         uart_irq.irq
	);

