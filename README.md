# vecexpr
Vector-based pocket calculator for Tcl : a much faster alternative to long loops calling `expr` over vector data.

## Compiling and loading vecexpr

Compile using the Makefile provided (amending the Tcl lib path).
Load into a Tcl interpreter: `load vecexpr.so`

## Syntax

Uses reverse Polish syntax (operands followed by operators).
Uses a FIFO stack, plus an extra register (used via store and recall)
Arguments: vectors, scalars, and builtin operators.

Examples:

`vecexpr "1 2 3" "4 5 6" add`

`vecexpr "1.2 -2.3 3.2" &int; puts $int` (prints `1 -3 3`)

## Operators

### nullary
`pi` (constant), `height` (current stack height, for debugging), `<varName` (push Tcl var - can be done with $var as well), `recall` (after calling store)

### unary
`abs` `cos` `sin` `exp` `floor` `log` `mean` `min` `max` `pow` `pi` `sq` `sqrt` `sum` `>varName` (pop into Tcl var), `#varName` (push floor values to an int variable), `store` (`recall` is 0-ary) `dup` (duplicate in the stack), `pop`

### binary
`add` `sub` `mult` `dot` `div` `concat` `swap`

All binary functions except `dot` accept mixed scalar/vector operands.
Vector lengths must match, except for `concat` and `swap`.

## Matrix multiplication
Matrices are stored unrolled (by lines).
Push on the stack both matrices, then the common dimension:
vecexpr "1 0 0 1" "1 2" 2 matmult   gives  "1.0 2.0"
vecexpr "1 0 0 1" "1 2" 1 matmult   gives  "1.0 2.0 0.0 0.0 0.0 0.0 1.0 2.0"


## Projected additions
(if there is demand for them)
norm (normalize), asin acos atan, cross (3-vectors)
