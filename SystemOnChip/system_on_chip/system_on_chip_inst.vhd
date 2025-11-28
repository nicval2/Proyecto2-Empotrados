	component system_on_chip is
		port (
			button_export      : in  std_logic_vector(3 downto 0)  := (others => 'X'); -- export
			clk_clk            : in  std_logic                     := 'X';             -- clk
			id7_segment_export : out std_logic_vector(31 downto 0);                    -- export
			reset_reset_n      : in  std_logic                     := 'X';             -- reset_n
			switches_export    : in  std_logic_vector(7 downto 0)  := (others => 'X')  -- export
		);
	end component system_on_chip;

	u0 : component system_on_chip
		port map (
			button_export      => CONNECTED_TO_button_export,      --      button.export
			clk_clk            => CONNECTED_TO_clk_clk,            --         clk.clk
			id7_segment_export => CONNECTED_TO_id7_segment_export, -- id7_segment.export
			reset_reset_n      => CONNECTED_TO_reset_reset_n,      --       reset.reset_n
			switches_export    => CONNECTED_TO_switches_export     --    switches.export
		);

