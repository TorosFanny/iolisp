iolisp
======

A C++ implementation of "Write Yourself a Scheme in 48 Hours"
http://en.wikibooks.org/wiki/Write_Yourself_a_Scheme_in_48_Hours

Example:

$ iolisp
iolisp>>> (define file (open-output-file "fact"))
<IO port>
iolisp>>> (write '(define (fact n) (if (<= n 0) 1 (* n (fact (- n 1))))) file)
#t
iolisp>>> (close-output-port file)
#t
iolisp>>> (load "fact")
(lambda ("n") ...)
iolisp>>> (fact 10)
3628800
iolisp>>> quit

$ 
