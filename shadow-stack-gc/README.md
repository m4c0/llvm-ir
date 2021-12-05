# Hello GC world

This is a practical example of how to use LLVM IR with garbage collection.

All info here is available on LLVM's manuals. You have to read the whole
[GC][gc] manual. Some code was copied from the
[Shadow Stack GC strategy][shadow-stack]

Reading some stuff about GC in general might help.

So far, this only shows non-relocating GCs (i.e. we don't move pointers
around).

How it works? In a nutshell:
* Your runtime must provide memory initialisation and allocation
  (`llvm_gc_*` in this example)
* Your compiler `alloca`s space to store managed pointers and mark them via
  the `llvm.gcroot`
* Your compiler must manage the content of that `alloca`ed space - this means
  `store`ing new values and disposing old ones
* Your runtime must do the garbage collect itself. The "visitor" in this
  example is able to visit all reachable roots

There are more stuff to be done in order to make this really usable in a real
world project:
* Mark-sweep. Using `shadow-stack` requires some way of identify discarded
  objects, since it only shows what is still in the stack.
* Introspection of roots. When managing objects composed by more managed
  objects, the collector must be smart enough to tag sub-objects as well

[gc]: https://llvm.org/docs/GarbageCollection.html
[shadow-stack]: https://llvm.org/docs/GarbageCollection.html#the-shadow-stack-gc


