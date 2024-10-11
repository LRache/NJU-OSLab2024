NAME := $(shell basename $(PWD))
export MODULE := M1
export BUILD := ./build/
all: $(BUILD)/$(NAME)-32 $(BUILD)/$(NAME)-64

test: $(NAME)-32 $(NAME)-64
	# $(BUILD)-32 -V
	# $(BUILD)-32
	$(BUILD)/$(NAME)-64 -V
	$(BUILD)/$(NAME)-64

include ../Makefile
