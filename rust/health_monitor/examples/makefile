# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# <https://www.apache.org/licenses/LICENSE-2.0>
#
# SPDX-License-Identifier: Apache-2.0

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
