/* A simple-minded, vector-based pocket calculator for Tcl.
 * Uses a kind of reverse Polish notation.
 * The "stack" holds only two items, but there is a custom register accessed via "store" and "recall"
 * Arguments: vectors, scalars, functions
 * Note: first data item must be a vector and sets the vector size for this instance of vecexpr
 *
 * Available functions:
 * Unary: abs cos sin floor log mean pow pi sq sqrt sum; store, recall; dup (duplicate in the stack)
 * Binary: add sub mult dot div(*)
 * (*) all binary functions except dot accept mixed scalar/vector operands
 *
 * Example usage:
 * load vecexpr.so
 * set a {1 2 3 4}
 * set scalar [vecexpr $a sqrt 2.0 mult store { 1 2 1 2 } dot recall sub mean] 
 * 
 * Jerome Henin <jhenin@cmm.upenn.edu>, 2008-2009 */

#include <tcl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern "C" {
int Vecexpr_Init(Tcl_Interp *interp);
}
static int obj_vecexpr(ClientData, Tcl_Interp *interp, int argc, Tcl_Obj * const objv[]);

static int obj_vecexpr(ClientData, Tcl_Interp *interp, int argc, Tcl_Obj * const objv[]) {

  if (argc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, (char *)"data data/funct ?data/funct? ...");
    return TCL_ERROR;
  }

  // all vectors have same size
  int vecsize = 0;

  int     funct_length;
  char *  funct;

  Tcl_Obj **data;
  int       num;
  double    scalar;
  bool      is_data;

  // simple two-item stack
  double *  stack[2] = {NULL, NULL};
  bool      stack_is_vect[2]; // says which items are vectors
  int       sp = 0;           // stack pointer (0 or 1)
  int       sh = 0;           // stack height (0, 1 or 2)
  int nv; // number of vectors on stack

  // additional register
  double *  reg = NULL;
  bool      reg_is_set = false;
  bool      reg_is_vect;

  for (int a = 1; a < argc; a++) {

    if (Tcl_ListObjGetElements(interp, objv[a], &num, &data) != TCL_OK) {
      Tcl_SetResult(interp, (char *) "vecexpr: error parsing arguments", TCL_STATIC);
      if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
      return TCL_ERROR;
    }

    if ( !num ) {
      Tcl_SetResult(interp, (char *) "vecexpr: empty list is not a valid argument", TCL_STATIC);
      if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
      return TCL_ERROR;
    }

    is_data = (Tcl_GetDoubleFromObj(interp, data[0], &scalar) == TCL_OK);
    
    if ( is_data ) {
      if ( !vecsize ) {
        vecsize = num;
        stack[0] = new double[vecsize];
        stack[1] = new double[vecsize];
        reg      = new double[vecsize];
      }

      if ( sh < 2 ) sh++;
      // if (sh == 2), we could display a warning that we're overwriting data... 
      sp = 1 - sp;

      if ( num == 1 ) {
        // scalar; just push
        stack_is_vect[sp] = false;
        stack[sp][0] = scalar;

      } else {
        if ( num != vecsize ) {
          Tcl_SetResult(interp, (char *) "vecexpr: vector argument has non-matching size", TCL_STATIC);
          if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
          return TCL_ERROR;
        }
        // parse list and push
        stack_is_vect[sp] = true;
        stack[sp][0] = scalar;
        for  (int i = 1; i < vecsize; i++) {
          if (Tcl_GetDoubleFromObj(interp, data[i], stack[sp]+i) != TCL_OK) {
            Tcl_SetResult(interp, (char *) "vecexpr: error parsing vector element as double", TCL_STATIC);
            if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
            return TCL_ERROR;
          }
        }
      }
    } else {
      // functions
      
      if (num != 1) {
        Tcl_SetResult(interp, (char *) "vecexpr: error parsing list with more than one element as function", TCL_STATIC);
        if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
        return TCL_ERROR;
      }
      if (sh == 0) {
        Tcl_SetResult(interp, (char *) "vecexpr: calling function with empty stack", TCL_STATIC);
        if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
        return TCL_ERROR;
      }
      
      funct = Tcl_GetStringFromObj(data[0], &funct_length);
      int not_sp = 1 - sp; //second operand

      // Unary functions first
      if (!strcmp(funct, "abs")) { // FUNCTION: ABS
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          stack[sp][i] = fabs(stack[sp][i]);
        }
        continue;
      }

      if (!strcmp(funct, "cos")) { // FUNCTION: COS
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          stack[sp][i] = cos(stack[sp][i]);
        }
        continue;
      }

      if (!strcmp(funct, "sin")) { // FUNCTION: SIN
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          stack[sp][i] = sin(stack[sp][i]);
        }
        continue;
      }

      if (!strcmp(funct, "log")) { // FUNCTION: LOG
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          if (stack[sp][i] < 0) {
            Tcl_SetResult(interp, (char *) "vecexpr: taking log of negative value", TCL_STATIC);
            if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
            return TCL_ERROR;
          }
          stack[sp][i] = log(stack[sp][i]);
        }
        continue;
      }

      if (!strcmp(funct, "mean")) { // FUNCTION: MEAN
        if ( !stack_is_vect[sp] ) continue;
        for (int i = 1; i < vecsize; i++) { 
          stack[sp][0] += stack[sp][i];
        }
        stack[sp][0] /= vecsize;
        stack_is_vect[sp] = false;
        continue;
      }

      if (!strcmp(funct, "sum")) { // FUNCTION: SUM
        if ( !stack_is_vect[sp] ) continue;
        for (int i = 1; i < vecsize; i++) { 
          stack[sp][0] += stack[sp][i];
        }
        stack_is_vect[sp] = false;
        continue;
      }

      if (!strcmp(funct, "floor")) { // FUNCTION: FLOOR
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          stack[sp][i] = floor(stack[sp][i]);
        }
        continue;
      }

      if (!strcmp(funct, "pi")) { // FUNCTION: PI
        if (sh < 2) sh++;
        sp = 1 - sp;
        stack[sp][0] = M_PI;
        stack_is_vect[sp] = false; 
        continue;
      }

      if (!strcmp(funct, "sq")) { // FUNCTION: SQ
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          stack[sp][i] *= stack[sp][i];
        }
        continue;
      }

      if (!strcmp(funct, "sqrt")) { // FUNCTION: SQRT
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          if (stack[sp][i] < 0) {
            Tcl_SetResult(interp, (char *) "vecexpr: taking sqrt of negative value", TCL_STATIC);
            if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
            return TCL_ERROR;
          }
          stack[sp][i] = sqrt(stack[sp][i]);
        }
        continue;
      }

      if (!strcmp(funct, "dup")) { // FUNCTION: DUP
        sh = 2;
        stack_is_vect[not_sp] = stack_is_vect[sp];
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          stack[not_sp][i] = stack[sp][i];
        }
        continue;
      }

      if (!strcmp(funct, "store")) { // FUNCTION: STORE
        reg_is_set = true;
        reg_is_vect = stack_is_vect[sp];
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          reg[i] = stack[sp][i];
        }
        continue;
      }

      if (!strcmp(funct, "recall")) { // FUNCTION: RECALL
        if ( !reg_is_set ) {
          Tcl_SetResult(interp, (char *) "vecexpr: trying to recall value from empty register", TCL_STATIC);
          if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
          return TCL_ERROR;
        }
        if (sh < 2) sh++;
        sp = 1 - sp;
        stack_is_vect[sp] = reg_is_vect;
        int count = reg_is_vect ? vecsize : 1;
        for (int i = 0; i < count; i++) { 
          stack[sp][i] = reg[i];
        }
        continue;
      }
      // ########## End of unary functions

      if (sh != 2) {
        Tcl_SetResult(interp, (char *) "vecexpr: not a unary function, and only one item on stack", TCL_STATIC);
        if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
        return TCL_ERROR;
      }

      if (!strcmp(funct, "add")) { // FUNCTION: ADD
        sh = 1;
        nv = stack_is_vect[0] + stack_is_vect[1];
        if (nv == 1) {
          // increment vector "in place"; stack has only 1 item left anyway
          sp = stack_is_vect[0] ? 0 : 1;
          scalar = stack[1-sp][0];
          for (int i = 0; i < vecsize; i++) { 
            stack[sp][i] += scalar;
          }
        } else {
          int count = nv ? vecsize : 1;
          for (int i = 0; i < count; i++) {
            stack[sp][i] += stack[not_sp][i];
          }
        }
        continue;
      } 

      if ( !strcmp(funct, "div") ) { // FUNCTION: DIV
        sh = 1;
        nv = stack_is_vect[0] + stack_is_vect[1];
        bool zero = false;
        int count = stack_is_vect[sp] ? vecsize : 1;
        for (int i = 0; i < count; i++) {if (stack[sp][i] == 0.0) zero = true;}
        if (zero) {
          Tcl_SetResult(interp, (char *) "vecexpr: divide by zero in function div", TCL_STATIC);
          if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
          return TCL_ERROR;
        }
        if (nv ==1) {
          // divide vector "in place"
          bool vec_first = stack_is_vect[not_sp];
          sp = stack_is_vect[0] ? 0 : 1;
          scalar = stack[1-sp][0];
          for (int i = 0; i < vecsize; i++) { 
            stack[sp][i] = vec_first ? stack[sp][i] / scalar : scalar / stack[sp][i];
          }
        } else {
          int count = nv ? vecsize : 1;
          for (int i = 0; i < count; i++) {
            stack[not_sp][i] /= stack[sp][i];
          }
          sp = 1 - sp;
        }
        continue;
      }

      if (!strcmp(funct, "dot")) { // FUNCTION: DOT
        if (stack_is_vect[0] != stack_is_vect[1]) {
          Tcl_SetResult(interp, (char *) "vecexpr: function dot requires two vectors or two scalars", TCL_STATIC);
          if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
          return TCL_ERROR;
        }
        sh = 1;
        stack_is_vect[sp] = false;
        double dot = 0.0;
        int count = stack_is_vect[0] ? vecsize : 1;
        for (int i = 0; i < count; i++) {
          dot += stack[sp][i] * stack[not_sp][i];
        }
        stack[sp][0] = dot;
        continue;
      } 

      if (!strcmp(funct, "mult")) { // FUNCTION: MULT
        sh = 1;
        nv = stack_is_vect[0] + stack_is_vect[1];
        if (nv ==1) {
          // scale vector "in place"; stack has only 1 item left anyway
          sp = stack_is_vect[0] ? 0 : 1;
          scalar = stack[1-sp][0];
          for (int i = 0; i < vecsize; i++) { 
            stack[sp][i] *= scalar;
          }
        } else {
          int count = nv ? vecsize : 1;
          for (int i = 0; i < count; i++) {
            stack[sp][i] *= stack[not_sp][i];
          }
        }
        continue;
      } 

      if ( !strcmp(funct, "sub") ) { // FUNCTION: SUB
        sh = 1;
        nv = stack_is_vect[0] + stack_is_vect[1];
        if (nv ==1) {
          // increment vector "in place"; stack has only 1 item left anyway
          bool vec_first = stack_is_vect[not_sp];
          sp = stack_is_vect[0] ? 0 : 1;
          scalar = stack[1-sp][0];
          for (int i = 0; i < vecsize; i++) { 
            stack[sp][i] = vec_first ? stack[sp][i] - scalar : scalar - stack[sp][i];
          }
        } else {
          int count = nv ? vecsize : 1;
          for (int i = 0; i < count; i++) {
            stack[not_sp][i] -= stack[sp][i];
          }
          sp = 1 - sp;
        }
        continue;
      }

      Tcl_SetResult(interp, (char *) "vecexpr: unrecognized function keyword", TCL_STATIC);
      if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
      return TCL_ERROR;
    }
  }

  Tcl_Obj *tcl_result = Tcl_NewListObj(0, NULL);
  for (int i = 0; i < (stack_is_vect[sp] ? vecsize : 1); i++) {
    Tcl_ListObjAppendElement(interp, tcl_result, Tcl_NewDoubleObj(stack[sp][i]));
  }
  Tcl_SetObjResult(interp, tcl_result);
  if (stack[0]) { delete [] stack[0]; delete [] stack[1]; delete [] reg;}
  return TCL_OK;
}

extern "C" {
int Vecexpr_Init(Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "vecexpr", obj_vecexpr,
                  (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  return TCL_OK;
}
}
