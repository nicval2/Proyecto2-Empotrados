	component system_on_chip is
		port (
			clk_clk                                 : in  std_logic                     := 'X';             -- clk
			reg_button_external_connection_export   : in  std_logic_vector(7 downto 0)  := (others => 'X'); -- export
			reg_leds_external_connection_export     : out std_logic_vector(31 downto 0);                    -- export
			reg_switches_external_connection_export : out std_logic_vector(9 downto 0);                     -- export
			reset_reset_n                           : in  std_logic                     := 'X';             -- reset_n
			timer_irq_irq                           : out std_logic;                                        -- irq
			uart_irq_irq                            : out std_logic                                         -- irq
		);
	end component system_on_chip;

	u0 : component system_on_chip
		port map (
			clk_clk                                 => CONNECTED_TO_clk_clk,                                 --                              clk.clk
			reg_button_external_connection_export   => CONNECTED_TO_reg_button_external_connection_export,   --   reg_button_external_connection.export
			reg_leds_external_connection_export     => CONNECTED_TO_reg_leds_external_connection_export,     --     reg_leds_external_connection.export
			reg_switches_external_connection_export => CONNECTED_TO_reg_switches_external_connection_export, -- reg_switches_external_connection.export
			reset_reset_n                           => CONNECTED_TO_reset_reset_n,                           --                            reset.reset_n
			timer_irq_irq                           => CONNECTED_TO_timer_irq_irq,                           --                        timer_irq.irq
			uart_irq_irq                            => CONNECTED_TO_uart_irq_irq                             --                         uart_irq.irq
		);

