@fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00"

declare i32 @printf(i8* , ...) nounwind

; (def! sqr (fn* (x) (* x x)))
define private i32 @sqr(i32 %x) nounwind {
  %xx = mul i32 %x, %x
  ret i32 %xx
}
; could be "define @0" followed by @sqr = gep @0, but then call requires deref
; the shortcut makes sense since "def" can be smart enough to create a function when necessary

; (def! dot (fn* (x y) (+ (sqr x) (sqr y))))
define private i32 @dot(i32 %x, i32 %y) nounwind {
  %xx = call i32 @sqr(i32 %x)
  %yy = call i32 @sqr(i32 %y)
  %res = add i32 %xx, %yy
  ret i32 %res 
}

; (dot 3 2)
define i32 @main(i32 %argc, i8** %argv) nounwind {
  %fmt = getelementptr inbounds [4 x i8], [4 x i8]* @fmt, i64 0, i64 0
  %res = call i32 @dot(i32 3, i32 2)
  tail call i32(i8*, ...) @printf(i8* %fmt, i32 %res)
  ret i32 0
} 
