# vecexpr
Vector-based pocket calculator for Tcl implemented in C++ : a much faster alternative to long loops calling `expr` over vector data.

## Compiling and loading vecexpr

Compile using the Makefile provided (amending the Tcl lib path).
Run tests from the shell: `tclsh test.tcl`
Start Tcl interpreter: `tclsh`
Then load into Tcl interpreter: `load vecexpr.so`

## Syntax

Uses reverse Polish syntax (operands followed by operators).
Uses a LIFO stack, plus an extra register (used via store and recall)
Arguments: scalars, vectors (Tcl lists), matrices (flattened, [row-major](https://en.wikipedia.org/wiki/Row-_and_column-major_order)) and built-in operators.

Examples:

```
% set vec "1 2 3"
% vecexpr $vec "4 5 6" add >othervec
% puts $othervec
5.0 7.0 9.0
```

`vecexpr "1.2 -2.3 3.2" floor >integers; puts $integers` 
(prints `1.0 -3.0 3.0`)

## Operators

### nullary
`pi` (constant), `height` (current stack height, for debugging), `<varName` (push Tcl var - can be done with $varName as well), `recall` (after calling store)

### unary
`abs` `cos` `sin` `tan` `exp` `floor` `log` `mean` `min` `max` `round` `sq` `sqrt` `sum` `>varName` (pop into Tcl var), `&varName` (copy floor values to an int variable), `store` (`recall` is 0-ary) `dup` (duplicate in the stack), `pop`

### binary
`add` `sub` `mult` `matmult` (see below) `dot` `div` `concat` `swap` `atan2`

All binary functions except `dot` and `matmult` accept mixed scalar/vector operands.
Vector lengths must match, except for `concat` and `swap`.

Example usage of `atan2`, converting result to degrees (note order of operands: y, x):
```
% vecexpr 5 0 atan2 pi div 180 mult
90.0
```

### Quaternary: bin

`vecexpr <data> <xmin> <dx> <nbins>`
where xmin, dx and nbins are scalars. Will push on the stack a histogram of the data, with `nbins` bins of width `dx` starting at `xmin`. 
The histogram can be saved as an integer-typed Tcl list using the '&' operator:
```
% vecexpr "1.1 2.1 2.2 3.5 4.1" 0. 1. 5 bin &histogram
1.1 2.1 2.2 3.5 4.1
% puts $histogram
0 1 2 1 1
```

## Matrix operations

Matrices are stored unrolled (in row-major order).
"Unary" matrix operations (`min_ew`, `transp`) are binary because they require the shape of the matrix, provided as the number of lines at the top of the stack.

Transposing a (2,3) matrix:

`vecexpr "1 2 3 4 5 6" 2 transp`  ->  `1.0 4.0 2.0 5.0 3.0 6.0`

Transposing the (3,2) matrix with the same (flattened) elements:

`vecexpr "1 2 3 4 5 6" 3 transp`  ->  `1.0 3.0 5.0 2.0 4.0 6.0`

For **matrix multiplication**, push both matrices on the stack, then the common dimension:

`vecexpr "1 0 0 1" "1 2" 2 matmult`  ->  `"1.0 2.0"`

`vecexpr "1 0 0 1" "1 2" 1 matmult`  ->  `"1.0 2.0 0.0 0.0 0.0 0.0 1.0 2.0"`


## Complete table of operators
This table lists each operator, the number of operands it uses (top n vectors on the stack), and the change in stack height after execution, that is, how many items are added or removed.

| Operator | n. operands | Stack chg. | Action                                                                                                                |
|----------|-------------|------------|-----------------------------------------------------------------------------------------------------------------------|
| <*varName* | 0         | +1         | push Tcl var. *varName* on the stack (in practice, $*varName* is faster)                                              |
| >*varName* | 1         | -1         | pop into Tcl var. *varName*                                                                                           |
| &*varName* | 1         | 0          | copy integer-typed floor values to variable *varName*                                                                 |
| abs      | 1           | 0          | absolute value                                                                                                        |
| add      | 2           | -1         | add 2 same-length vectors, or vector and scalar (element-wise), or column-vector and matrix, or matrix and row-vector |
| atan2    | 2           | -1         | given vectors y and x, push element-wise arctangent of y / x (in radians)                                             |
| bin      | 4           | -3         | histogram of the data, with `nbins` bins of width `dx` starting at `xmin`: `vecexpr $data $xmin $dx $nbins bin`       |
| concat   | 2           | -1         | concatenate two top vectors                                                                                           |
| cos      | 1           | 0          | cosine (angles in radians)                                                                                            |
| div      | 2           | -1         | division (same-length vectors or vector by scalar or scalar by vector)                                                |
| dot      | 2           | -1         | dot product                                                                                                           |
| dup      | 1           | +1         | push copy of top vector onto stack                                                                                    |
| exp      | 1           | 0          | exponential                                                                                                           |
| floor    | 1           | 0          | floor (type: double)                                                                                                  |
| height   | 0           | 0          | push current stack height                                                                                             |
| log      | 1           | 0          | natural log                                                                                                           |
| matmult  | 3           | 0          | multiply matrices, using 3 args: M1 M2 n, where n is the common dimension                                             |
| max      | 1           | +1         | push max element of top vector                                                                                        |
| mean     | 1           | +1         | push mean of top vector                                                                                               |
| min      | 1           | +1         | push min of top vector                                                                                                |
| min_ew   | 2           | -1         | element-wise minimum between lines of the top matrix (M, n, where n is the number of lines)                           |
| mult     | 2           | -1         | element-wise multiply vectors, or multiply vector and scalar                                                          |
| pi       | 0           | +1         | push pi constant onto stack                                                                                           |
| pop      | 1           | -1         | remove top vector from stack                                                                                          |
| recall   | 0           | +1         | push stored data (register)                                                                                           |
| round    | 1           | 0          | round all elements to nearest integer (keep double type)                                                              |
| sin      | 1           | 0          | sine (angles in radians)                                                                                              |
| sq       | 1           | 0          | square                                                                                                                |
| sqrt     | 1           | 0          | square root                                                                                                           |
| store    | 1           | 0          | copy top vector to register                                                                                           |
| sub      | 2           | -1         | subtract (see `add` for details)                                                                                      |
| sum      | 1           | +1         | push sum of values of top vector                                                                                      |
| swap     | 2           | 0          | swap top two vectors of stack                                                                                         |
| tan      | 1           | 0          | tangent (angles in radians)                                                                                           |
| transp   | 2           | -1         | transpose top matrix (M, n, where n is the number of lines)                                                           |
