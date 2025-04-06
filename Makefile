CFLAGS = -O3 -W -Wall -Wdeclaration-after-statement -Wno-unused-parameter -ggdb3

CEB_DIR = ceb
CEB_SRC = $(wildcard $(CEB_DIR)/ceb*.c)
CEB_OBJ = $(CEB_SRC:%.c=%.o)

OBJS = $(CEB_OBJ)

WITH_DUMP= -DCEB_ENABLE_DUMP
TEST_DIR = tests
TEST_BIN = $(addprefix $(TEST_DIR)/,stresscebu32 stresscebu64 stresscebul speedcebul speedcebub speedcebus)
TEST_BIN += $(addprefix $(TEST_DIR)/,testceb32 testceb64 testcebl testcebb testcebs)
TEST_BIN += $(addprefix $(TEST_DIR)/,testcebu32 testcebu64 testcebul testcebub testcebus)
TEST_BIN += $(addprefix $(TEST_DIR)/,benchceb32 benchcebu32 benchceb64 benchcebu64 benchcebl benchcebul benchcebb benchcebub benchcebs benchcebus)

all: test

libcebtree.a: $(OBJS)
	$(AR) rv $@ $^

%.o: %.c ceb/cebtree-prv.h ceb/_ceb_int.c ceb/_ceb_blk.c ceb/_ceb_str.c ceb/_ceb_addr.c
	$(CC) $(CFLAGS) $(WITH_DUMP) -o $@ -c $<

test: $(TEST_BIN)
	@for t in $(filter $(TEST_DIR)/testceb%,$^); do    \
		for args in "3 2 1" "3 3 2 2 1 1"; do      \
			echo "=== $$t $$args ===";         \
			$$t $$args || exit 1;              \
		done;                                      \
	done

tests/%: tests/%.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree

tests/stresscebul: tests/stresscebul.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread

tests/benchcebul: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"cebul_tree.h"' -DDATA_TYPE='unsigned long' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -DNODE_INS='cebul_insert' -DNODE_DEL='cebul_delete' -DNODE_INTREE='ceb_intree'

tests/benchcebl: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"cebl_tree.h"' -DDATA_TYPE='unsigned long' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -DNODE_INS='cebl_insert' -DNODE_DEL='cebl_delete' -DNODE_INTREE='ceb_intree'

tests/benchcebu32: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"cebu32_tree.h"' -DDATA_TYPE='unsigned int' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -DNODE_INS='cebu32_insert' -DNODE_DEL='cebu32_delete' -DNODE_INTREE='ceb_intree'

tests/benchceb32: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"ceb32_tree.h"' -DDATA_TYPE='unsigned int' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -DNODE_INS='ceb32_insert' -DNODE_DEL='ceb32_delete' -DNODE_INTREE='ceb_intree'

tests/benchcebu64: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"cebu64_tree.h"' -DDATA_TYPE='unsigned long long' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -DNODE_INS='cebu64_insert' -DNODE_DEL='cebu64_delete' -DNODE_INTREE='ceb_intree'

tests/benchceb64: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"ceb64_tree.h"' -DDATA_TYPE='unsigned long long' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -DNODE_INS='ceb64_insert' -DNODE_DEL='ceb64_delete' -DNODE_INTREE='ceb_intree'

tests/benchcebub: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"cebub_tree.h"' -DDATA_TYPE='unsigned long long' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -D'NODE_INS(r,k)=cebub_insert(r,k,sizeof(long long))' -D'NODE_DEL(r,k)=cebub_delete(r,k,sizeof(long long))' -DNODE_INTREE='ceb_intree'

tests/benchcebb: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"cebb_tree.h"' -DDATA_TYPE='unsigned long long' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -D'NODE_INS(r,k)=cebb_insert(r,k,sizeof(long long))' -D'NODE_DEL(r,k)=cebb_delete(r,k,sizeof(long long))' -DNODE_INTREE='ceb_intree'

tests/benchcebus: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"cebus_tree.h"' -DDATA_TYPE='unsigned long long' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -DNODE_INS='cebus_insert' -DNODE_DEL='cebus_delete' -DNODE_INTREE='ceb_intree' -DSTORAGE_STRING=21

tests/benchcebs: tests/bench.c libcebtree.a
	$(CC) $(CFLAGS) -I$(CEB_DIR) -o $@ $< -L. -lcebtree -pthread -DINCLUDE_FILE='"cebs_tree.h"' -DDATA_TYPE='unsigned long long' -DNODE_TYPE='struct ceb_node' -DROOT_TYPE='struct ceb_root' -DNODE_INS='cebs_insert' -DNODE_DEL='cebs_delete' -DNODE_INTREE='ceb_intree' -DSTORAGE_STRING=21

clean:
	-rm -fv libcebtree.a $(OBJS) *~ *.rej core $(TEST_BIN) ${EXAMPLES}
	-rm -fv $(addprefix $(CEB_DIR)/,*~ *.rej core)

ifeq ($(wildcard .git),.git)
VERSION := $(shell [ -d .git/. ] && (git describe --tags --match 'v*' --abbrev=0 | cut -c 2-) 2>/dev/null)
SUBVERS := $(shell comms=`git log --no-merges v$(VERSION).. 2>/dev/null |grep -c ^commit `; [ $$comms -gt 0 ] && echo "-$$comms" )
endif

git-tar: .git
	git archive --format=tar --prefix="cebtree-$(VERSION)/" HEAD | gzip -9 > cebtree-$(VERSION)$(SUBVERS).tar.gz

.PHONY: examples tests
