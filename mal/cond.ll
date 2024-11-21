%list = type { i32*, i32 }

; (defmacro! cond (fn* (& xs) (if (> (count xs) 0) (list 'if (first xs) (if (> (count xs) 1) (nth xs 1) (throw \"odd number of forms to cond\")) (cons 'cond (rest (rest xs)))))))

; (defmacro!
; cond
; (fn* (& xs) ...))

define i32 @first(%list %l) {
  %data = extractvalue %list %l, 0
  %res = load i32, i32* %data
  ret i32 %res
}

define void @cond(%list %xs) {
; (if 
;   (> (count xs) 0)
  %count.0 = extractvalue %list %xs, 1
  %icmp.0 = icmp sgt i32 %count.0, 0
  br i1 %icmp.0, label %if.0, label %else.0

if.0:
; (list
;   'if 
;   (first xs)
  %list.1 = call i32 @first(%list %xs)
;   (if (> (count xs) 1) (nth xs 1) (throw \"odd number of forms to cond\"))
;   (cons 'cond (rest (rest xs)))))
  ret void


else.0:
  ret void
}
