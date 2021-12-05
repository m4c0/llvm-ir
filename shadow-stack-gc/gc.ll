@meh = private constant [5 x i8] c"meh\00\00"
@hmm = private constant [5 x i8] c"hmm\00\00"

declare void @llvm.gcroot(i8** %ptrloc, i8* %metadata)
declare i8* @llvm_gc_allocate(i32)
declare void @llvm_gc_initialize(i32)
declare i32 @puts(i8*)
declare void @visit()

define void @dunno(i8* %s) nounwind gc "shadow-stack" {
entry:
  call i32 @puts(i8* %s)
  ret void
}

define i8* @im_okay() nounwind gc "shadow-stack" {
entry:
  call void @llvm_gc_initialize(i32 1000)
  
  %a = alloca i8*
  call void @llvm.gcroot(i8** %a, i8* getelementptr inbounds ([5 x i8], [5 x i8]* @hmm, i32 0, i32 0))
  %a_ptr = call i8* @llvm_gc_allocate(i32 12)
  store i8* %a_ptr, i8** %a

  %b = alloca i8*
  call void @llvm.gcroot(i8** %b, i8* null)
  %b_ptr = call i8* @llvm_gc_allocate(i32 12)
  store i8* %b_ptr, i8** %b

  call void(i8*) @dunno(i8* %a_ptr)
  call void(i8*) @dunno(i8* %b_ptr)

  ; store i8* null, i8** %a
  call void() @visit()
  ret i8* %a_ptr
}

define void @okay() nounwind gc "shadow-stack" {
entry:
  %a = alloca i8*
  call void @llvm.gcroot(i8** %a, i8* getelementptr inbounds ([5 x i8], [5 x i8]* @hmm, i32 0, i32 0))
  call void() @visit()

  %b = call i8* @im_okay();
  store i8* %b, i8** %a
  call void() @visit()
  ret void
}
