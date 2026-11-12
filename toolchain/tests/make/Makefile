# test Makefile for p68-make: variables, functions, pattern rules, phony, automatic vars
CC      := echo cc
SRCS    := a.c b.c c.c
OBJS    := $(patsubst %.c,%.o,$(SRCS))
UPPER    = $(shell echo hi)
LIST     = one two three four
FILTERED = $(filter %o %r,$(LIST))
NUM     := $(words $(LIST))

.PHONY: all clean show
all: prog
	@echo built all

prog: $(OBJS)
	@echo link $@ from $^

%.o: %.c
	@echo compile $< to $@ "(stem=$*)"

a.c b.c c.c:
	@echo (source $@ assumed present)

show:
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"
	@echo "FILTERED=$(FILTERED)"
	@echo "NUM=$(NUM)"
	@echo "foreach: $(foreach x,$(LIST),[$(x)])"
	@echo "if: $(if $(FILTERED),has-match,no-match)"
	@echo "call: $(call reverse,A,B)"

reverse = $(2) $(1)

clean:
	@echo cleaning
