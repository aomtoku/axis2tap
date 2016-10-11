sim_top = testbench

VERILATOR_SRC := sim_main.cpp

SIM_SRC := testbench.sv
RTL_SRC := app.v

WFLAGS = -Wall -Wno-PINCONNECTEMPTY -Wno-UNUSED -Wno-UNDRIVEN -Wno-SYNCASYNCNET

all: test

lint: $(SIM_SRC) $(RTL_SRC)
	verilator $(WFLAGS) -Wall --lint-only --cc --top-module $(sim_top) -sv $(SIM_SRC) $(RTL_SRC)

sim:
	sudo ./obj_dir/Vtestbench &

test:
	verilator $(WFLAGS) -DSIMULATION --cc --trace --top-module testbench -sv $(SIM_SRC) $(RTL_SRC) --exe $(VERILATOR_SRC)
	make -j -C obj_dir/ -f V$(sim_top).mk V$(sim_top)

.PHONY: clean
clean:
	rm -f wave.vcd
	rm -rf obj_dir

