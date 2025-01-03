# Compact Elastic Binary Tree (cebtree)

## Abstract

CEBTree is a form of very compact binary search tree that only requires two
pointers (like a list) to index data. The structure is a much more compact
variant of the [EBTree](https://github.com/wtarreau/ebtree) structure, so no
allocation is needed when inserting a node. There are no upward pointers so
some operations are slower as they will require a preliminary lookup. For
example a `next()` operation requires a first descent to identify the closest
fork point, then a second descent from that optimal point.

But the structure provides a number of other benefits. The first one being the
memory usage: **the tree uses the same storage as a list**, thus can be
installed anywhere a list would be used. This can be particularly interesting
for read-mostly data (configuration, append-only indexes etc). It preserves
structure alignment, thus does not require to contain the data itself, the data
may be appended just after the pointer nodes, which saves the need for typed
trees thus typed operations. It may also make the code a bit cleaner, because
with EBTree it's often tempting to touch node->key from the main code, without
always realizing the impacts (namely with signed values).

It should also be easier to implement variants (e.g. case insensitive strings
lookups, or faster memory lookups matching one word at a time, etc) thanks to
the unified data types.

## Limitations and future improvements

Duplicates are algorithmically possible by using list elements, but are a bit
tricky to implement, so they'll be implemented later. They should be agnostic
to the data representation anyway.

Performance is a bit lower than EBTrees even for small keys due to the need to
read both branches at each node to figure whether to stop or continue the
descent. It effectively doubles the number of visited nodes (hence is less TLB
friendly), though it does not necessarily increase the memory bandwidth since
the nodes are much smaller.

It was verified that an almost lockless approach could be implemented: lookups
and insertion could be done without locking but deletion requires locking. As
such, an approach would require [rwlocks](https://github.com/wtarreau/plock)
with shared locking for insertion and lookup, and exclusive locking for
deletion. This might come as an advantage over EBTrees for highly contended
environments.
