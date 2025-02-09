#!/bin/bash
mkdir static

ros run -- --load make-static.lisp \
  --eval '(progn (make-markup) (print "Static files have been emited~") (main 8000))'
