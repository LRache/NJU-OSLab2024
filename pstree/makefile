NAME = pstree

include $(OS_HOME)/Makefile

all: $(NAME)-32 $(BUILD)-64

test: $(NAME)-32 $(NAME)-64
	# $(BUILD)-32 -V
	# $(BUILD)-32
	$(BUILD)-64 -V
	$(BUILD)-64
