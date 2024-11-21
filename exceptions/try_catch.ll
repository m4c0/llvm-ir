%exc_t = type { i64, void(i32, i8*)*, i64, i64 }

@g_exc = private constant %exc_t { i64 69, void(i32, i8*)* null, i64 0, i64 0 }

declare void @abort()
declare i32 @_Unwind_RaiseException(%exc_t*)

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
define dso_local void @err(i32 %0) local_unnamed_addr {
  %2 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %0)
  ret void
}
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr

define void @throw() {
  %ops = tail call i32 @_Unwind_RaiseException(%exc_t* @g_exc)
  tail call void @err(i32 %ops)
  tail call void @abort()
  ret void
}

define i32 @main(i32 %argc, i8** %argv) {
  invoke void @throw() to label %ok unwind label %nok

ok:
  ret i32 0

nok:
  %lp = landingpad i32 catch i32 69
  call void @err(i32 %lp)
  ret i32 1
}

