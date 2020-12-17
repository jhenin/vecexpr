# vecexpr
Vector-based pocket calculator for Tcl : a much faster alternative to long loops calling `expr` over vector data.

## Compiling and loading vecexpr

Compile using the Makefile provided (amending the Tcl lib path).
Run tests from the shell: `$ tclsh test.tcl`
Start Tcl interpreter: `tclsh`
Then load into Tcl interpreter: `% load vecexpr.so`

## Syntax

Uses reverse Polish syntax (operands followed by operators).
Uses a FIFO stack, plus an extra register (used via store and recall)
Arguments: scalars, vectors (Tcl lists), matrices (flattened, [row-major](https://en.wikipedia.org/wiki/Row-_and_column-major_order)) and builtin operators.

Examples:

```
set vec "1 2 3"
vecexpr $vec "4 5 6" add
```
(returns `5 7 9`)

`vecexpr "1.2 -2.3 3.2" floor >integers; puts $integers` 
(prints `1.0 -3.0 3.0`)

## Operators

### nullary
`pi` (constant), `height` (current stack height, for debugging), `<varName` (push Tcl var - can be done with $varName as well), `recall` (after calling store)

### unary
`abs` `cos` `sin` `tan` `exp` `floor` `log` `mean` `min` `max` `round` `sq` `sqrt` `sum` `>varName` (pop into Tcl var), `&varName` (push floor values to an int variable), `store` (`recall` is 0-ary) `dup` (duplicate in the stack), `pop`

### binary
`add` `sub` `mult` `matmult` (see below) `dot` `div` `concat` `swap`

All binary functions except `dot` and `matmult` accept mixed scalar/vector operands.
Vector lengths must match, except for `concat` and `swap`.

## Matrix multiplication
Matrices are stored unrolled (by lines).
Push on the stack both matrices, then the common dimension:

`vecexpr "1 0 0 1" "1 2" 2 matmult`   gives  `"1.0 2.0"`

`vecexpr "1 0 0 1" "1 2" 1 matmult`   gives  `"1.0 2.0 0.0 0.0 0.0 0.0 1.0 2.0"`

## Complete table of operators
This table lists each operator, the number of operands it uses (top n vectors on the stack), and the change in stack height after execution, that is, how many items are added or removed.

| Operator | n. operands | Stack chg. | Action                                                                                                                |
|----------|-------------|------------|-----------------------------------------------------------------------------------------------------------------------|
| <*varName* | 0         | +1         | push Tcl var. *varName* on the stack                                                                                  |
| >*varName* | 1         | -1         | pop into Tcl var. *varName*                                                                                           |
| &*varName* | 1         | 0          | push integer-typed floor values into variable *varName*                                                               |
| abs      | 1           | 0          | absolute value                                                                                                        |
| add      | 2           | -1         | add 2 same-length vectors, or vector and scalar (element-wise), or column-vector and matrix, or matrix and row-vector |
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
| sum      | 1           | +1         | push sum of values on top vector                                                                                      |
| swap     | 2           | 0          | swap top two vectors of stack                                                                                         |
| tan      | 1           | 0          | tangent (angles in radians)                                                                                           |
