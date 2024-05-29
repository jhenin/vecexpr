#!/bin/tclsh

load ./vecexpr.so

proc test { cmd } {
  puts "running: vecexpr $cmd"
  puts "-> [vecexpr {*}$cmd]"
}

test "{1 2 3 5} {0 1 2 3} sub store {9 8 7 6} 0.5 mult recall add dup mult"

test "pi { 1 2 } mult { 1 0 0 1 } 2 matmult"

test "pi { 1 1 1 1 1 1 1 1 1 1 } mult {12. 54 21 23 -23.12} concat dup pop pop"

test "{1 2} {0 0 0 0 0 0} add"
test "{0 0 0 0 0 0} {1 2} add"


test "{0 23 -4 5 2 8} 2 min_ew"

#puts [vecexpr 1 2 asd]

#puts [vecexpr {1 2 3} {1 2 *&}]
