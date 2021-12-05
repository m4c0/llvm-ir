#include <stdio.h>

static int a = 0;
static const char *b[] = {"first", "second"};
extern "C" void *llvm_gc_allocate(int) {
  puts("alloc");
  return (void *)b[a++];
}
extern "C" void llvm_gc_initialize(int) { puts("init"); }

extern "C" void okay();

struct FrameMap {
  int32_t NumRoots;    //< Number of roots in stack frame.
  int32_t NumMeta;     //< Number of metadata entries.  May be < NumRoots.
  const void *Meta[0]; //< Metadata for each root.
};
struct StackEntry {
  StackEntry *Next;    //< Link to next stack entry (the caller's).
  const FrameMap *Map; //< Pointer to constant FrameMap.
  void *Roots[0];      //< Stack roots (in-place array).
};
StackEntry *llvm_gc_root_chain;

void visitGCRoots(void (*Visitor)(void **Root, const void *Meta)) {
  for (StackEntry *R = llvm_gc_root_chain; R; R = R->Next) {
    unsigned i = 0;

    // For roots [0, NumMeta), the metadata pointer is in the FrameMap.
    for (unsigned e = R->Map->NumMeta; i != e; ++i)
      Visitor(&R->Roots[i], R->Map->Meta[i]);

    // For roots [NumMeta, NumRoots), the metadata pointer is null.
    for (unsigned e = R->Map->NumRoots; i != e; ++i)
      Visitor(&R->Roots[i], NULL);
  }
}

void visitor(void **root, const void *meta) {
  printf("   %p (%s) %p\n", root, *root, meta);
}
extern "C" void visit() {
  printf("visiting\n");
  visitGCRoots(visitor);
}

int main() {
  okay();
  visit();
}
