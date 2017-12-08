load ./vecexpr.so

puts [vecexpr {1 2 3 5} {0 1 2 3} sub store {9 8 7 6} 0.5 mult recall add dup mult]

puts [vecexpr pi { 1 1 1 1 1 1 1 1 1 1 } mult {12. 54 21 23 -23.12} concat dup pop pop]


#puts [vecexpr 1 2 asd]

#puts [vecexpr {1 2 3} {1 2 *&}]
