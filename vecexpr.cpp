#include <tcl.h>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>

extern "C" {
int Vecexpr_Init(Tcl_Interp *interp);
}
static int obj_vecexpr(ClientData, Tcl_Interp *interp, int argc, Tcl_Obj * const objv[]);

// A simple-minded, vector-based pocket calculator for Tcl.
// There is a FIFO stack, plus an extra register (used via store and recall)
// Arguments: vectors, scalars, builtin function names

// TODO: norm (normalize), asin acos atan, cross (3-vectors), range (unary, creates sequence to given int)

// Available functions:
// nullary: pi (constant), height (current stack height, for debugging), <varName (push Tcl var - can be done with $var as well)
// recall (after calling store)
// Unary: abs cos sin exp floor log mean min max pow pi sq sqrt sum >varName (pop into Tcl var)
// also unary: store (recall is 0-ary); dup (duplicate in the stack), pop
// Binary: add sub mult dot div concat swap (*)
// (*) all binary functions except dot accept mixed scalar/vector operands
// vector lengths must match except for concat and swap

// Matrix multiplication: matrices are unrolled in row-major order
// the common dimension is pushed on the stack last:
// vecexpr "1 0 0 1" "1 2" 2 matmult   gives  "1.0 2.0"
// vecexpr "1 0 0 1" "1 2" 1 matmult   gives  "1.0 2.0 0.0 0.0 0.0 0.0 1.0 2.0"

static int obj_vecexpr(ClientData, Tcl_Interp *interp, int argc, Tcl_Obj * const objv[])
{
  if (argc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, (char *)"data data/funct ?data/funct? ...");
    return TCL_ERROR;
  }

  // all vectors have same size
  // int vecsize = 0;

  int     funct_length;
  char *  funct;

  Tcl_Obj **data;
  int       num;
  double    scalar;
  bool      is_data;
  size_t    count_back;
  size_t    count_prev;
  bool      mismatched;     // are two different-length vectors on top of the stack?

  std::vector<std::vector <double> > stack;
  int nv; // number of vectors among top two items of stack

  // additional register
  std::vector <double> reg;

  for (int a = 1; a < argc; a++) {

    if (Tcl_ListObjGetElements(interp, objv[a], &num, &data) != TCL_OK) {
      Tcl_SetResult(interp, (char *) "vecexpr: error parsing arguments", TCL_STATIC);
      return TCL_ERROR;
    }

    if ( !num ) {
      Tcl_SetResult(interp, (char *) "vecexpr: empty list passed as argument", TCL_STATIC);
      return TCL_ERROR;
    }

    is_data = (Tcl_GetDoubleFromObj(interp, data[0], &scalar) == TCL_OK);

    if ( is_data ) {
      // parse list and push
      stack.push_back (std::vector <double>());
      stack.back().resize (num);
      stack.back()[0] = scalar;
      for  (int i = 1; i < num; i++) {
        if (Tcl_GetDoubleFromObj(interp, data[i], &scalar) != TCL_OK) {
          Tcl_SetResult(interp, (char *) "vecexpr: error parsing vector element as floating-point", TCL_STATIC);
          return TCL_ERROR;
        }
        stack.back()[i] = scalar;
      }
    } else {
      // functions

      if (num != 1) {
        Tcl_SetResult(interp, (char *) "vecexpr: error parsing list with more than one element as function", TCL_STATIC);
        return TCL_ERROR;
      }
      funct = Tcl_GetStringFromObj(data[0], &funct_length);
      if ( !funct_length ) {
        Tcl_SetResult(interp, (char *) "vecexpr: found empty string when trying to parse function name (should not happen!)", TCL_STATIC);
        return TCL_ERROR;
      }

      // Zero-ary function first

      if (!strcmp(funct, "pi")) { // FUNCTION: PI
        stack.push_back(std::vector<double> (1, M_PI));
        continue;
      }

      if (!strcmp(funct, "height")) { // FUNCTION: HEIGHT
        //return stack height
        stack.push_back(std::vector<double> (1, (double) stack.size()));
        continue;
      }

      if (!strcmp(funct, "recall")) { // FUNCTION: RECALL
        if ( reg.size() == 0 ) {
          Tcl_SetResult(interp, (char *) "vecexpr: trying to recall value from empty register", TCL_STATIC);
          return TCL_ERROR;
        }
        stack.push_back( std::vector<double> (reg) );
        continue;
      }

      if ( funct[0] == '<' ) { // FUNCTION: PUSH VARIABLE
      // This does not seem very useful, as the variable can be passed as
      // a parameter to vecexpr slightly more efficiently than using this code
      // 1000-buck question: why is {expr  1 + 2} slower than { vecexpr 1 2 add }, but
      // { expr {$a + $b} } much faster than either { vecexpr $a $b add } or
      // { vecexpr <a <b add } ???
      // (the answer might lie in the code used internally to parse expr:
      // http://tcl.cvs.sourceforge.net/viewvc/tcl/tcl/generic/tclCompExpr.c?view=markup )

        const char *varName = &funct[1];
        Tcl_Obj *varData = Tcl_GetVar2Ex( interp, varName, NULL, 0);
        if ( varData == NULL ) {
          Tcl_SetResult(interp, (char *) "vecexpr: tried to push unknown Tcl variable", TCL_STATIC);
          return TCL_ERROR;
        }
        int num_elts;
        Tcl_Obj ** listData;
        if (Tcl_ListObjGetElements(interp, varData, &num_elts, &listData) != TCL_OK) {
          Tcl_SetResult(interp, (char *) "vecexpr: error parsing arguments", TCL_STATIC);
          return TCL_ERROR;
        }

        if ( !num_elts ) {
          Tcl_SetResult(interp, (char *) "vecexpr: empty list passed as argument", TCL_STATIC);
          return TCL_ERROR;
        }

        stack.push_back (std::vector <double>());
        stack.back().resize (num_elts);
        for  (int i = 0; i < num_elts; i++) {
          if (Tcl_GetDoubleFromObj(interp, listData[i], &scalar) != TCL_OK) {
            Tcl_SetResult(interp, (char *) "vecexpr: error parsing vector element as floating-point", TCL_STATIC);
            return TCL_ERROR;
          }
          stack.back()[i] = scalar;
        }
        continue;
      }

      if (stack.size() == 0) {
        Tcl_SetResult(interp, (char *) "vecexpr: calling function with empty stack", TCL_STATIC);
        return TCL_ERROR;
      }

      count_back = stack.back().size();

      // Unary functions

      if ( funct[0] == '>' ) { // FUNCTION: POP TO VARIABLE
        const char *varName = &funct[1];
        Tcl_Obj * newList = Tcl_NewListObj (0, NULL);
        for (int i = 0; i < count_back; i++) {
          Tcl_ListObjAppendElement (interp, newList, Tcl_NewDoubleObj(stack.back()[i]));
        }
        Tcl_SetVar2Ex (interp, varName, NULL, newList, 0);
        stack.pop_back();
        continue;
      }

      if ( funct[0] == '&' ) { // FUNCTION: pop to integer variable
        const char *varName = &funct[1];
        Tcl_Obj * newList = Tcl_NewListObj (0, NULL);
        for (int i = 0; i < count_back; i++) {
          double const val = stack.back()[i];
          long int floor = val < 0 ? (long int)val - 1 : (long int)val;
          Tcl_ListObjAppendElement (interp, newList, Tcl_NewIntObj(floor));
        }
        Tcl_SetVar2Ex (interp, varName, NULL, newList, 0);
        stack.pop_back();
        continue;
      }

      if (!strcmp(funct, "abs")) { // FUNCTION: ABS
        for (int i = 0; i < count_back; i++) {
          stack.back()[i] = fabs(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "cos")) { // FUNCTION: COS
        for (int i = 0; i < count_back; i++) {
          stack.back()[i] = cos(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "sin")) { // FUNCTION: SIN
        for (int i = 0; i < count_back; i++) {
          stack.back()[i] = sin(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "tan")) { // FUNCTION: SIN
        for (int i = 0; i < count_back; i++) {
          stack.back()[i] = tan(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "exp")) { // FUNCTION: EXP
        for (int i = 0; i < count_back; i++) {
          stack.back()[i] = exp(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "log")) { // FUNCTION: LOG
        for (int i = 0; i < count_back; i++) {
          if (stack.back()[i] <= 0.0) {
            Tcl_SetResult(interp, (char *) "vecexpr: taking log of non-positive value", TCL_STATIC);
            return TCL_ERROR;
          }
          stack.back()[i] = log(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "mean")) { // FUNCTION: MEAN
        double sum = stack.back()[0];
        for (int i = 1; i < count_back; i++) {
          sum += stack.back()[i];
        }
        stack.push_back(std::vector<double> (1, sum / count_back));
        continue;
      }

      if (!strcmp(funct, "min")) { // FUNCTION: MIN
        double min = stack.back()[0];
        for (int i = 1; i < count_back; i++) {
          if (stack.back()[i] < min)
            min = stack.back()[i];
        }
        stack.push_back(std::vector<double> (1, min));
        continue;
      }

      if (!strcmp(funct, "max")) { // FUNCTION: MAX
        double max = stack.back()[0];
        for (int i = 1; i < count_back; i++) {
          if (stack.back()[i] > max)
            max = stack.back()[i];
        }
        stack.push_back(std::vector<double> (1, max));
        continue;
      }

      if (!strcmp(funct, "sum")) { // FUNCTION: SUM
        double sum = stack.back()[0];
        for (int i = 1; i < count_back; i++) {
          sum += stack.back()[i];
        }
        stack.push_back(std::vector<double> (1, sum));
        continue;
      }

      if (!strcmp(funct, "floor")) { // FUNCTION: FLOOR
        for (int i = 0; i < count_back; i++) {
          stack.back()[i] = floor(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "round")) { // FUNCTION: ROUND
        for (int i = 0; i < count_back; i++) {
          stack.back()[i] = round(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "sq")) { // FUNCTION: SQ
        for (int i = 0; i < count_back; i++) {
          stack.back()[i] *= stack.back()[i];
        }
        continue;
      }

      if (!strcmp(funct, "sqrt")) { // FUNCTION: SQRT
        for (int i = 0; i < count_back; i++) {
#ifdef DEBUG
          if (stack.back()[i] < 0.0) {
            Tcl_SetResult(interp, (char *) "vecexpr: taking sqrt of negative value", TCL_STATIC);
            return TCL_ERROR;
          }
#endif
          stack.back()[i] = sqrt(stack.back()[i]);
        }
        continue;
      }

      if (!strcmp(funct, "dup")) { // FUNCTION: DUP
        stack.push_back( std::vector<double> (stack.back()) );
        continue;
      }

      if (!strcmp(funct, "pop")) { // FUNCTION: POP
        stack.pop_back();
        continue;
      }

      if (!strcmp(funct, "store")) { // FUNCTION: STORE
        reg = stack.back();
        continue;
      }

      // ########## End of unary functions

      if (stack.size() < 2) {
        Tcl_SetResult(interp, (char *)  "vecexpr: not a unary function, and only one item on stack", TCL_STATIC);
        return TCL_ERROR;
      }

      int back = stack.size()-1; // last on stack
      int prev = stack.size()-2; // second-to-last on stack
      count_prev = stack[prev].size();
      mismatched = (count_back != count_prev);

      if (!strcmp(funct, "concat")) { // FUNCTION: CONCAT
        stack[prev].reserve(count_prev + count_back);
        for (int i = 0; i < count_back; i++) {
          stack[prev].push_back (stack.back()[i]);
        }
        stack.pop_back();
        continue;
      }

      if (!strcmp(funct, "swap")) { // FUNCTION: SWAP
        stack.back().swap(stack[prev]);
        continue;
      }

      if (!strcmp(funct, "add")) { // FUNCTION: ADD
        if ( count_back == 1 || count_prev == 1 ) { // Add scalar to vector / matrix
          if (count_back > 1) {
            stack.back().swap(stack[prev]);
            count_prev = count_back;
          }
          for (int i = 0; i < count_prev; i++) {
            stack[prev][i] += stack.back()[0];
          }
        } else {
          if ( mismatched ) {
            // This could be a vector-matrix addition
            const int ivec = (count_back < count_prev ? back : prev);
            const int imat = (count_back < count_prev ? prev : back);
            std::vector<double> & vec = stack[ivec];
            std::vector<double> & mat = stack[imat];
            int count_mat = mat.size();
            int count_vec = vec.size();
            if (count_mat % count_vec) {
              Tcl_SetResult(interp, (char *) "vecexpr: matrix-vector add with non-divisor vector length", TCL_STATIC);
              return TCL_ERROR;
            }
            if (count_back < count_prev) {
              // <matrix> <vector> add: add vector to matrix rows
              int k = 0;
              for (int i = 0; i < count_mat / count_vec; i++) {
                for (int j = 0; j < count_vec; j++) {
                  mat[k++] += vec[j];
                }
              }
              stack.pop_back(); // get vector off the stack
            } else {
              // <vector> <matrix> add: add vector to matrix columns
              int k = 0;
              for (int j = 0; j < count_vec; j++) {
                for (int i = 0; i < count_mat / count_vec; i++) {
                  mat[k++] += vec[j];
                }
              }
              stack.back().swap(stack[prev]); // move vector to the back
              stack.pop_back(); // get vector off the stack
            }

            // Leave matrix at the back of the stack
            continue;

          } else {
            for (int i = 0; i < count_back; i++) {
              stack[prev][i] += stack.back()[i];
            }
          }
        }
        stack.pop_back();
        continue;
      }

      if ( !strcmp(funct, "div") ) { // FUNCTION: DIV
        bool zero = false;
        for (int i = 0; i < count_back; i++) {if (stack.back()[i] == 0.0) zero = true;}
        if (zero) {
          Tcl_SetResult(interp, (char *) "vecexpr: divide by zero in function div", TCL_STATIC);
          return TCL_ERROR;
        }
        if ( count_back == 1 || count_prev == 1 ) {
          if (count_back > 1) {
            stack.back().swap(stack[prev]);
            count_prev = count_back;
            for (int i = 0; i < count_prev; i++) {
              stack[prev][i] = stack.back()[0] / stack[prev][i];
            }
          } else {
            for (int i = 0; i < count_prev; i++) {
              stack[prev][i] /= stack.back()[0];
            }
          }
        } else {
          if ( mismatched ) {
            Tcl_SetResult(interp, (char *)  "vecexpr: attempting binary function on different-length vectors", TCL_STATIC);
            return TCL_ERROR;
          }
          for (int i = 0; i < count_back; i++) {
            stack[prev][i] /= stack.back()[i];
          }
        }
        stack.pop_back();
        continue;
      }

      if (!strcmp(funct, "dot")) { // FUNCTION: DOT
        if (mismatched) {
          Tcl_SetResult(interp, (char *) "vecexpr: function dot requires vectors of same length", TCL_STATIC);
          return TCL_ERROR;
        }
        double dot = 0.0;
        for (int i = 0; i < count_back; i++) {
          dot += stack.back()[i] * stack[prev][i];
        }
        stack.pop_back();
        stack.back().resize (1);
        stack.back()[0] = dot;
        continue;
      }

      if (!strcmp(funct, "mult")) { // FUNCTION: MULT
        if ( count_back == 1 || count_prev == 1 ) {
          if (count_back > 1) {
            stack.back().swap(stack[prev]);
            count_prev = count_back;
          }
          for (int i = 0; i < count_prev; i++) {
            stack[prev][i] *= stack.back()[0];
          }
        } else {
          if ( mismatched ) {
            Tcl_SetResult(interp, (char *)  "vecexpr: cannot element-wise multiply different-length vectors", TCL_STATIC);
            return TCL_ERROR;
          }
          for (int i = 0; i < count_back; i++) {
            stack[prev][i] *= stack.back()[i];
          }
        }
        stack.pop_back();
        continue;
      }

      if ( !strcmp(funct, "sub") ) { // FUNCTION: SUB
        if ( count_back == 1 || count_prev == 1 ) { // subtract scalar from vector or reverse
          if (count_back > 1) {
            stack.back().swap(stack[prev]);
            count_prev = count_back;
            for (int i = 0; i < count_prev; i++) {
              stack[prev][i] = stack.back()[0] - stack[prev][i];
            }
          } else {
            for (int i = 0; i < count_prev; i++) {
              stack[prev][i] -= stack.back()[0];
            }
          }
        } else {
          if ( mismatched ) {
            Tcl_SetResult(interp, (char *)  "vecexpr: cannot element-wise subtract different-length vectors", TCL_STATIC);
            return TCL_ERROR;
          }
          for (int i = 0; i < count_back; i++) {
            stack[prev][i] -= stack.back()[i];
          }
        }
        stack.pop_back();
        continue;
      }

      if ( !strcmp(funct, "atan2") ) { // FUNCTION: ATAN2(Y, X)
        if ( mismatched ) {
          Tcl_SetResult(interp, (char *)  "vecexpr: function atan2 requires two vectors of same length", TCL_STATIC);
          return TCL_ERROR;
        }
        for (int i = 0; i < count_back; i++) {
          stack[prev][i] = atan2(stack[prev][i], stack[back][i]);
        }
        stack.pop_back();
        continue;
      }
      // end of binary functions

      if (stack.size() < 3) {
        Tcl_SetResult(interp, (char *)  "vecexpr: unrecognized vector function, or too few items on stack", TCL_STATIC);
        return TCL_ERROR;
      }

      if ( !strcmp(funct, "matmult") ) { // FUNCTION: MATMULT

        if (stack.back().size() != 1) {
            Tcl_SetResult (interp, (char *) "matmult: common dimension specifier should be a scalar", TCL_STATIC);
            return TCL_ERROR;
        }
        int nj1 = stack.back()[0];
        int ni2 = nj1;

        stack.pop_back();
        std::vector<double> * mat1 = &(stack[stack.size() - 2]);
        std::vector<double> * mat2 = &(stack.back());

        if (mat1->size() % nj1 || mat2->size() % ni2) {
            Tcl_SetResult (interp, (char *) "matmult: matrix size not a multiple of common dimension", TCL_STATIC);
            return TCL_ERROR;
        }
        int ni1 = mat1->size() / nj1;
        int nj2 = mat2->size() / ni2;

        stack.push_back (std::vector<double> (ni1 * nj2, 0.0));

        double sum;
        int index1 = 0;
        int index2 = 0;
        int index = 0;
        for (int i = 0; i < ni1; i++) {
            for (int j = 0; j < nj2; j++) {
                sum = 0.0;
                index2 = j;
                for (int k = 0; k < nj1; k++) {
                    sum += (*mat1)[index1] * (*mat2)[index2];
                    index1++;
                    index2 += nj2;
                }
                index1 -= nj1;
                stack.back()[index++] = sum;
            }
            index1 += nj1;
        }
        // TODO: pop the previous matrices?
        // stack.erase(...) may incur a peformance penalty
        continue;
      }

      // end of ternary functions

      if (stack.size() < 3) {
        Tcl_SetResult(interp, (char *)  "vecexpr: unrecognized vector function, or too few items on stack", TCL_STATIC);
        return TCL_ERROR;
      }

      if ( !strcmp(funct, "bin") ) { // FUNCTION: BIN

        if (stack.back().size() * stack[stack.size()-2].size() * stack[stack.size()-3].size() != 1 ) {
            Tcl_SetResult (interp, (char *) "bin needs 3 scalars on the stack: min, dx, and nbins.", TCL_STATIC);
            return TCL_ERROR;
        }
        const double nbins = stack.back()[0]; stack.pop_back();
        const double dx = stack.back()[0]; stack.pop_back();
        const double min = stack.back()[0]; stack.pop_back();
        stack.push_back(std::vector<double> (static_cast<size_t>(nbins), 0.0)); // Histogram
        back = stack.size()-1; // histogram
        prev = stack.size()-2; // data to bin

        const int count = stack[prev].size();
        for (int i = 0; i < count; i++) {
          int bin = floor((stack[prev][i] - min) / dx);
          if (bin >= 0 && bin < nbins) stack[back][bin] += 1.0;
        }
        stack.back().swap(stack[prev]); // move data to the back
        stack.pop_back(); // get data off the stack
        continue;
      }

      Tcl_SetResult(interp, (char *) "vecexpr: unrecognized function keyword", TCL_STATIC);
      return TCL_ERROR;
    }
  }

  if ( stack.size() == 0 ) {
    // Stack is empty at end of evaluation, just return 0
    stack.push_back(std::vector<double> (1, 0.0));
  }

  count_back = stack.back().size();
  Tcl_Obj *tcl_result = Tcl_NewListObj(0, NULL);
  for (int i = 0; i < count_back; i++) {
    Tcl_ListObjAppendElement(interp, tcl_result, Tcl_NewDoubleObj(stack.back()[i]));
  }
  Tcl_SetObjResult(interp, tcl_result);
  return TCL_OK;
}

extern "C" {
  int Vecexpr_Init(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "vecexpr", obj_vecexpr,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    return TCL_OK;
  }
}
