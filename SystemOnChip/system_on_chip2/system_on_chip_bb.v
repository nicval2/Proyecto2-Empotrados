
module system_on_chip (
	clk_clk,
	reg_button_external_connection_export,
	reg_leds_external_connection_export,
	reg_switches_external_connection_export,
	reset_reset_n,
	timer_irq_irq,
	uart_irq_irq);	

	input		clk_clk;
	input	[7:0]	reg_button_external_connection_export;
	output	[31:0]	reg_leds_external_connection_export;
	output	[9:0]	reg_switches_external_connection_export;
	input		reset_reset_n;
	output		timer_irq_irq;
	output		uart_irq_irq;
endmodule
