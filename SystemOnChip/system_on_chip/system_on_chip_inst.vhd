	component system_on_chip is
		port (
			clk_clk       : in  std_logic := 'X'; -- clk
			reset_reset_n : in  std_logic := 'X'; -- reset_n
			timer_irq_irq : out std_logic         -- irq
		);
	end component system_on_chip;

	u0 : component system_on_chip
		port map (
			clk_clk       => CONNECTED_TO_clk_clk,       --       clk.clk
			reset_reset_n => CONNECTED_TO_reset_reset_n, --     reset.reset_n
			timer_irq_irq => CONNECTED_TO_timer_irq_irq  -- timer_irq.irq
		);

