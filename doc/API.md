## CEB tree usage and API

### Code organization

All the functions are built on the same set of inline functions present in
cebtree-prv.h which is private and not meant to be exported to the application.
This ensures that anything done there will not pollute the application's
namespace.

The basic functions are type-agnostic. In pratice they support a few types and
use the same arguments differently depending on the type they're called with.
There is notably a single descent function that serves for all types and all
operations. This ensures that bug fixes and optimizations benefit to all types
at once, in order to avoid reproducing some mistakes from EB trees which
suffered a lot from the code duplication.

Two variants of all functions are provided to address different needs in terms
of key indexing:

  - the default one, where the key is expected to be placed immediately after
    the node.

  - an alternate one, where an offset is passed in argument between the node
    and the key. This one permits to use the same key for multiple purposes
    without having to duplicate it (but it must obviously be used with caution).

The code that is built is present in [ceb/\_ceb\_\<class>.c](ceb/). The "\<class>"
here mentions a basic type class among:

  - `addr` : the key is the address of the node itself. There's no storage,
    this may be used for memory allocators for example, or to look up elements
    by their pointer in a safe way. By definition this class does not support
    duplicates!

  - `int` : the key is of type integer (uint32\_t, uint64\_t, or unsigned long).
    The unsigned long actually redirects towards either the u32 or u64 code
    depending on the integer size. Signed ints have not been implemented for
    now (the use case is less frequent).

  - `blk` : the key is a memory block of a specific size. The size is passed to
    all operations. The caller must be extremely careful to ensure that the key
    is the same for all objects and all operations, as this is assumed all
    along the code. Any bytes are permitted, only the length counts, so this
    may be used to index references to objects using digests such as SHA256 for
    example. Memory blocks exist in two forms: direct and indirect. In direct
    mode, the block is stored immediately after the node (or at the designated
    offset). In indirect mode, it's a pointer to the block that's stored at the
    designated location. Indirect mode is a bit slower since it requires an extra
    memory indirection.

  - `str` : the key is a zero-terminated C string. The caller must make sure
    that each stored (or looked up) key is properly terminated by a zero. No
    limit is imposed on the length of the strings, though access times will
    obviously increase with very long strings. Just like with memory blocks,
    strings support both direct and indirect indexing. In direct indexing, the
    strings is stored immediately after the node, while in indirect indexing,
    it's the pointer to the string that is stored after the node. The first one
    is more suited to nodes allocated using malloc() based on a preliminary
    known string length. The second one is better suited to identical nodes
    to which strings are attached using strdup(). Indirect mode is a bit slower
    since it requires an extra memory indirection.

The user-visible code is built from [ceb/ceb\<type>\_tree.c](ceb/), where
"\<type>" is the exact key type among:

  - `a` :  address-based node
  - `32` : 32-bit unsigned integer
  - `64` : 64-bit unsigned integer
  - `l`  : unsigned long integer
  - `b`  : direct memory block
  - `ib` : indirect memory block (pointer to a memory block)
  - `s`  : direct zero-terminated C string
  - `ib` : indirect zero-terminated C string (pointer to such a string)

There are equivalent '.h' files at the same place with a 'u' variant for the
unique keys tree (trees that do not store duplicates).

These files are built together and arranged into `libcebtree.a`.

### Integration into existing applications

Application code only needs to:
  - include `ceb<type>_tree.h` : needed by the application code to access
    functions and types declarations. Note that these files automatically
    include `cebtree.h` which contains types definitions and inline functions.
  - link with libcebtree.a or `ceb<type>_tree.o`

Applications that want to embed a local copy of the library need this in the
same directory, and to build `ceb<type>_tree.o`:
  - `cebtree.h`
  - `cebtree-prv.h`
  - `_ceb_*.c`
  - `ceb<type>_tree.c`
  - `ceb<type>_tree.h`

These files were designed to be easily relocatable without having to edit them,
thus allowing to easily update to newer versions.

### API

Two types are needed to be known by the application (these are defined in
`cebtree.h` which is automatically included by `ceb<type>_tree.h`):

  - the tree root, which is only a pointer, which by convention is of type
    `struct ceb_root *`. The `ceb_root` struct is purposely not defined so as
    to make sure it's never used other than an opaque pointer. An empty tree
    only has a NULL pointer at the top. This is the only situation where a NULL
    pointer may be met in a tree. For better readability, a function is
    provided to determine if a tree is empty:

  - `int ceb_isempty(const struct ceb_root **root)` : returns true if the tree
     is empty, otherwise false.

  - the indexing node : `struct ceb_node`. This structure contains two
    non-NULL `ceb_root` pointers.

No packing, padding nor alignment tricks are played with here, so it
is perfectly possible to include this struct in a union to use a node
alternatively as a list or as a tree node.

By construction, a node that belongs to a tree cannot have any of its pointers
not set. This means that a newly allocated structure containing a node equal to
zero does not belong to a tree. Thus by convention, the belonging of a node to
a tree is verified by checking its left pointer. An inline function is provided
to do that:

  - `int ceb_intree(const struct ceb_node *node)` : returns true if the node is
    in a tree, otherwise false.

All functions are named `ceb<unicity><type><ofs>_<operation>()`, with the
following components:

  - `<unicity>` : `u` when acting on a tree storing only unique keys, empty
    when supporting duplicate keys.

  - `<type>` : one of the types mentioned above (`a`,`32`,`64`,`l`,`b`,`ib`,`s`,`is`).

  - `<ofs>` : either `_ofs` when the keys are placed at an offset relative to
              the address of the node, otherwise empty for the default case.
              When an offset is passed to a function, it is always the first
              argument after the root. Operations made without an offset
              exactly correspond to specifying an offset of
              `sizeof(struct ceb_node)`.

  - `<operation>` : the operation to be performed among:

     * `insert` : insert a node into the tree based on its key and return
       it. Trees using unique keys will return the pointer to the node having
       the same key if one is found, and the new one will not be inserted in
       this case.

     * `first` : return the first lexicographically ordered node from the tree,
       or NULL if the tree is empty. Identical keys are returned in insertion
       order.

     * `last` : return the last lexicographically ordered node from the tree,
       or NULL if the tree is empty. Identical keys are returned in reverse
       insertion order.

     * `delete` : delete a node from the tree and return it. NULL is returned
       if the node was not in the tree.

     * `pick` : look up a node in the tree and delete it and return it if found
       or NULL if not present.

     * `lookup` : return the first inserted node matching the specified key, or
       NULL if the key was not found.

     * `lookup_ge` : return the first inserted node whose key is greater than
       or equal to the specified one, or NULL if no such node can be found.

     * `lookup_gt` : return the first inserted node whose key is strictly
       greater than to the specified one, or NULL if no such node can be found.

     * `lookup_le` : return the last inserted node whose key is less than or
       equal to the specified one, or NULL if no such node can be found.

     * `lookup_lt` : return the last inserted node whose key is strictly less
       than to the specified one, or NULL if no such node can be found.

     * `next` : return the first inserted node after the specified one,
       starting from the ones having the same key and inserted after the
       current one, and continuing with the first node having a strictly
       greater key than the current one, or NULL if the end of the tree was
       reached.

     * `next_dup` : return the first inserted node after the specified one and
       having the exact same key, or NULL if no more duplicate of the specified
       node exists.

     * `next_unique` : return the first node having a strictly greater key
       than the specified one, or NULL if no such node exists.

     * `prev` : return the last node inserted before the specified one,
       starting from the ones having the same key and inserted before the
       current one, and continuing with the last node having a strictly lower
       key than the current one, or NULL if no such node exists.

     * `prev_dup` : return the last inserted node before the specified one and
       having the exact same key, or NULL if no more duplicate of the specified
       node exists.

     * `prev_unique` : return the last node having a strictly lower key than
       the specified one, or NULL if no such node exists.

     * `key` : return a pointer to the beginning of the key in its own type
       (uint32_t, uint64_t, void*, unsigned long, char*, void*) if the node is
       valid, otherwise NULL if the node is NULL. This corresponds to the node
       added to the key offset for immediate keys, or a dereference of the key
       pointer for indirect keys. Note that the key pointer cannot be NULL for
       valid nodes, so it is always safe to assume that `key()==NULL` means
       that the node does not exist.

#### Examples:

- Standard indexing, with support for duplicates:
```
  struct ceb_node *ceb32_lookup(struct ceb_root **root, uint32_t key);
  struct ceb_node *ceb32_insert(struct ceb_root **root, struct ceb_node *node);
  struct ceb_node *ceb32_first(struct ceb_root **root);
  struct ceb_node *ceb32_delete(struct ceb_root **root, struct ceb_node *node);
  struct ceb_node *ceb32_next(struct ceb_root **root, struct ceb_node *node);
```

- Indexing of unique keys:
```
  struct ceb_node *cebu64_lookup(struct ceb_root **root, uint64_t key);
  struct ceb_node *cebu64_insert(struct ceb_root **root, struct ceb_node *node);
  struct ceb_node *cebu64_first(struct ceb_root **root);
  struct ceb_node *cebu64_delete(struct ceb_root **root, struct ceb_node *node);
  struct ceb_node *cebu64_next(struct ceb_root **root, struct ceb_node *node);
```

- Indexing of direct memory blocks located at a specific offset:
```
  struct ceb_node *cebb_ofs_lookup(struct ceb_root **root, ptrdiff_t kofs, const void *key, size_t len);
  struct ceb_node *cebb_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
  struct ceb_node *cebb_ofs_first(struct ceb_root **root, ptrdiff_t kofs, size_t len);
  struct ceb_node *cebb_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
  struct ceb_node *cebb_ofs_next(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
```

- Indexing of indirect strings whose pointers immediately follow the node:
```
  struct ceb_node *cebis_lookup(struct ceb_root **root, const void *key);
  struct ceb_node *cebis_insert(struct ceb_root **root, struct ceb_node *node);
  struct ceb_node *cebis_first(struct ceb_root **root);
  struct ceb_node *cebis_delete(struct ceb_root **root, struct ceb_node *node);
  struct ceb_node *cebis_next(struct ceb_root **root, struct ceb_node *node);
```

- Using `_key` to look up values within a range:
```
   void list_range(const struct ceb_root **root, uint32_t min, uint32_max)
   {
       uint32_t *key;

       while ((key = ceb32_key(cebu32_lookup_ge(root, min))) != NULL) {
            if (*key > max)
                break;
            printf("found: %u\n", *key);
            min = key + 1;
       }
   }
```
