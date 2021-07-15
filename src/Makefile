BUILD_DIR=build

#Compiler Parameters
CFLAGS = -Ofast -g -std=gnu11 -march=native -masm=att
LIB=-pthread -lm
LIB+= -lrt
UNAME:=$(shell uname)
ifneq ($(UNAME), Darwin)
LIB+= -latomic
endif

DEFINES=
DEPENDS=

SRCS=commCharaterize.c laminarFifoClient.c laminarFifoServer.c laminarFifoRunner.c timeHelpers.c vitisNumaAllocHelpers.c
OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

#Production
all: commCharaterize

commCharaterize: $(OBJS) 
	$(CC) $(CFLAGS) $(INC) $(DEFINES) -o commCharaterize $(OBJS) $(LIB)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/
	$(CC) $(CFLAGS) -c $(INC) $(DEFINES) -o $@ $<

$(BUILD_DIR)/:
	mkdir -p $@

clean:
	rm -f commCharaterize
	rm -rf build/*

.PHONY: clean
