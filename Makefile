CFLAGS = -O3 -W -Wall -Wdeclaration-after-statement -Wno-unused-parameter -ggdb3

COMMON_DIR = common

CB_DIR = cb
CB_SRC = $(wildcard $(CB_DIR)/cb*.c)
CB_OBJ = $(CB_SRC:%.c=%.o)

OBJS = $(CB_OBJ)

TEST_DIR = tests
TEST_BIN = $(addprefix $(TEST_DIR)/,stresscbu32 testcbu32 stresscbu64 testcbu64 testcbul stresscbul speedcbul testcbub speedcbub testcbus speedcbus)

all: test

libcbtree.a: $(OBJS)
	$(AR) rv $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -o $@ -c $^

test: $(TEST_BIN)

tests/%: tests/%.c libcbtree.a
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(CB_DIR) -o $@ $< -L. -lcbtree

tests/stresscbul: tests/stresscbul.c libcbtree.a
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(CB_DIR) -o $@ $< -L. -lcbtree -pthread

clean:
	-rm -fv libcbtree.a $(OBJS) *~ *.rej core $(TEST_BIN) ${EXAMPLES}
	-rm -fv $(addprefix $(CB_DIR)/,*~ *.rej core)

ifeq ($(wildcard .git),.git)
VERSION := $(shell [ -d .git/. ] && ref=`(git describe --tags --match 'v*') 2>/dev/null` && ref=$${ref%-g*} && echo "$${ref\#v}")
SUBVERS := $(shell comms=`git log --no-merges v$(VERSION).. 2>/dev/null |grep -c ^commit `; [ $$comms -gt 0 ] && echo "-$$comms" )
endif

git-tar: .git
	git archive --format=tar --prefix="cbtree-$(VERSION)/" HEAD | gzip -9 > cbtree-$(VERSION)$(SUBVERS).tar.gz

.PHONY: examples tests
