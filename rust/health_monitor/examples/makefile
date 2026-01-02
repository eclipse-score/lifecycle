SOURCES := $(wildcard *.cpp)
EXAMPLES := $(addprefix build/,$(basename $(SOURCES)))

.PHONY: run
run: cargo build $(EXAMPLES)
	@ for example in $(EXAMPLES); do echo "Running $$example"; ./$$example; done

.PHONY: clean
clean:
	rm $(EXAMPLES)

.PHONY: cargo
cargo:
	cargo build

# Creates the build folder
build:
	mkdir -p build/

build/%: %.cpp
	g++ -std=c++23 -Wall -g -O0 $< -l:libhealth_monitor.a -L../../../target/debug/ -I../src/ -o $@
