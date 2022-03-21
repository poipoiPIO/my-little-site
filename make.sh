#!/bin/bash
ros run -- --load make-static.lisp \
  --eval '(progn (make-markup) (print "Okeee~") (quit))'
