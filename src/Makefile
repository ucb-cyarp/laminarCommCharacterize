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

ifneq ($(TRANSACTIONS_BLKS),)
DEFINES+= -DTRANSACTIONS_BLKS=$(TRANSACTIONS_BLKS)
endif

ifneq ($(FIFO_BLK_SIZE_CPLX_FLOAT),)
FIFO_BLK_SIZE_CPLX_FLOAT=256
DEFINES+= -DFIFO_BLK_SIZE_CPLX_FLOAT=$(FIFO_BLK_SIZE_CPLX_FLOAT)
endif

ifneq ($(FIFO_LEN_BLKS),)
DEFINES+= -DFIFO_LEN_BLKS=$(FIFO_LEN_BLKS)
endif

TEMPLATE_FILES=

SRCS=commCharaterize.c laminarFifoClient.c laminarFifoServer.c laminarFifoRunner.c timeHelpers.c vitisNumaAllocHelpers.c
OBJS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

#Production
all: commCharaterize

commCharaterize: $(OBJS) 
	$(CC) $(CFLAGS) $(INC) $(DEFINES) -o commCharaterize $(OBJS) $(LIB)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/
	$(CC) $(CFLAGS) -c $(INC) $(DEFINES) -o $@ $<

$(OBJS): $(TEMPLATE_FILES)

#https://www.gnu.org/software/make/manual/html_node/Static-Usage.html
$(filter %.h,$(TEMPLATE_FILES)): %.h: %.h.template
	cp $< $@;
	sed -i 's/FIFO_BLK_SIZE_CPLX_FLOAT/$(FIFO_BLK_SIZE_CPLX_FLOAT)/g' $@;

$(filter %.c,$(TEMPLATE_FILES)): %.c: %.c.template
	cp $< $@;
	sed -i 's/FIFO_BLK_SIZE_CPLX_FLOAT/$(FIFO_BLK_SIZE_CPLX_FLOAT)/g' $@;

$(BUILD_DIR)/:
	mkdir -p $@

clean:
	rm -f commCharaterize
	rm -rf build/*
	rm -f $(TEMPLATE_FILES)

.PHONY: clean
