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

## Properties

Just like EBTrees, duplicate keys are supported and are visited in insertion
order. This allows for duplicate detection and graceful handling for example
in configuration files, as well as timer management (though ebtrees are much
faster for timer management, albeit bigger).

#### Comparison of costs by model
_(TODO: add hash table: logN+N/logN, and rbtree: logN)_

|             model |      list       |     cebtree     |      ebtree       |
|-------------------|:---------------:|:---------------:|:-----------------:|
|__operation__      | min / avg / max | min / avg / max |  min / avg / max  |
|lookup ops         |  - / O(N) / -   | - / O(logN) / - |  - / O(logN) / -  |
|insert ordered     |  - / O(N) / -   | - / O(logN) / - |  - / O(logN) / -  |
|append (unordered) |  - / O(1) / -   | - / O(logN) / - |  - / O(logN) / -  |
|delete             |  - / O(1) / -   | - / O(logN) / - |  - /   O(1)  / -  |
|first/last         |  - / O(1) / -   | - / O(logN) / - |  - / O(logN) / -  |
|next/prev          |  - / O(1) / -   | - / O(logN) / - | O(1) / O(1) / O(logN) |
||
|__Costs per operation__|
|string lookup cost|    N*strcmp()    |  ~1*strcmp()    |    ~1*strcmp()    |
|mem accesses / node|       N         |    2*logN       |      1*logN       |

#### Synthetic performance comparison
- enumeration in insertion order: lists > ebtree > cebtree
- enumeration in key order: ebtree > cebtree > lists
- random lookups: ebtree > cebtree > lists
- random deletion: lists > ebtree > cebtree
- total purge: lists > ebtree > cebtree

The tagged pointers permit the string lookup cost to remain low. Without tagged
pointers (e.g. version 0.2), the string lookup cost becomes logN*strcmp() since
a complete string needs to be compared at each layer (like in other non-radix
trees).

## Limitations and future improvements

Relative addressing is not yet implemented but is in progress. This is handy to
manipulate data in memory areas shared between multiple processes, where no
pointer is stored.

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
