CFLAGS = -O3 -W -Wall -Wdeclaration-after-statement -Wno-unused-parameter -ggdb3

COMMON_DIR = common

CEB_DIR = ceb
CEB_SRC = $(wildcard $(CEB_DIR)/ceb*.c)
CEB_OBJ = $(CEB_SRC:%.c=%.o)

OBJS = $(CEB_OBJ)

TEST_DIR = tests
TEST_BIN = $(addprefix $(TEST_DIR)/,stresscebu32 stresscebu64 stresscebul speedcebul speedcebub speedcebus)
TEST_BIN += $(addprefix $(TEST_DIR)/,testceb32 testceb64 testcebl testcebb testcebs)
TEST_BIN += $(addprefix $(TEST_DIR)/,testcebu32 testcebu64 testcebul testcebub testcebus)

all: test

libcebtree.a: $(OBJS)
	$(AR) rv $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -o $@ -c $^

test: $(TEST_BIN)

tests/%: tests/%.c libcebtree.a
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(CEB_DIR) -o $@ $< -L. -lcebtree

tests/stresscebul: tests/stresscebul.c libcebtree.a
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread

clean:
	-rm -fv libcebtree.a $(OBJS) *~ *.rej core $(TEST_BIN) ${EXAMPLES}
	-rm -fv $(addprefix $(CEB_DIR)/,*~ *.rej core)

ifeq ($(wildcard .git),.git)
VERSION := $(shell [ -d .git/. ] && ref=`(git describe --tags --match 'v*') 2>/dev/null` && ref=$${ref%-g*} && echo "$${ref\#v}")
SUBVERS := $(shell comms=`git log --no-merges v$(VERSION).. 2>/dev/null |grep -c ^commit `; [ $$comms -gt 0 ] && echo "-$$comms" )
endif

git-tar: .git
	git archive --format=tar --prefix="cebtree-$(VERSION)/" HEAD | gzip -9 > cebtree-$(VERSION)$(SUBVERS).tar.gz

.PHONY: examples tests
