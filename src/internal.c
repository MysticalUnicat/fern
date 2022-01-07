#include "local.h"

static fern_ExStack * current_exstack;

bool fern_ExStack_begin(fern_ExStack * exstack) {
  fern_ExStack * previous_exstack = current_exstack;
  exstack->previous = current_exstack;
  current_exstack = exstack;
  if(setjmp(exstack->buf) == 0) {
    return true;
  } else {
    current_exstack = previous_exstack;
    return false;
  }
}

void fern_ExStack_end(fern_ExStack * exstack) {
  fern_assert_fatal_error(current_exstack == exstack, "bad catch/throw state");
  current_exstack = exstack->previous;
}

void fern_internal_throw(fern_Box message) {
  fern_assert_fatal_error(current_exstack == NULL, "%a", message);
  current_exstack->message = message;
  longjmp(current_exstack->buf, 1);
}

bool fern_internal_match_shape(fern_Array a1, fern_Array a2) {
  fern_ArrayReader a1r = fern_read_array(a1);
  fern_ArrayReader a2r = fern_read_array(a2);
  if(fern_array_rank(a1r) != fern_array_rank(a2r)) {
    return false;
  }
  for(uint32_t i = 0; i < fern_array_rank(a1r); i++) {
    if(fern_array_axis_length(a1r, i) == fern_array_axis_length(a2r, i)) {
      return false;
    }
  }
  return true;
}

bool fern_internal_match_full(fern_Box x, fern_Box w) {
  switch(fern_tag(x)) {
  case fern_Tag_array:
    {
      fern_Array a1 = fern_unpack_array(x);
      fern_Array a2 = fern_unpack_array(w);
      fern_ArrayReader a1r = fern_read_array(a1);
      fern_ArrayReader a2r = fern_read_array(a2);
      if(fern_array_rank(a1r) != fern_array_rank(a2r)) {
        return false;
      }
      for(uint32_t i = 0; i < fern_array_rank(a1r); i++) {
        if(fern_array_axis_length(a1r, i) == fern_array_axis_length(a2r, i)) {
          return false;
        }
      }
      for(uint32_t i = 0; i < fern_array_num_cells(a1r); i++) {
        if(!fern_internal_match(fern_array_get_cell(a1r, i), fern_array_get_cell(a2r, i))) {
          return false;
        }
      }
    }
    break;
  case fern_Tag_function:
    {
      fern_Function f1 = fern_unpack_function(x);
      fern_Function f2 = fern_unpack_function(w);
      if(f1->type != f2->type) {
        return false;
      }
      switch(f1->type) {
      case fern_FunctionType_c:
        return f1->c == f2->c;
      case fern_FunctionType_block:
        return false;
      case fern_FunctionType_applied_m1:
        return fern_internal_match(f1->applied_m1.f, f2->applied_m1.f) &&
               fern_internal_match(f1->applied_m1.m, f2->applied_m1.m);
      case fern_FunctionType_applied_c_m1:
        return fern_internal_match(f1->applied_c_m1.f, f2->applied_c_m1.f) &&
                                (f1->applied_c_m1.m == f2->applied_c_m1.m) ;
      case fern_FunctionType_applied_m2:
        return fern_internal_match(f1->applied_m2.f, f2->applied_m2.f) &&
               fern_internal_match(f1->applied_m2.m, f2->applied_m2.m) &&
               fern_internal_match(f1->applied_m2.g, f2->applied_m2.g) ;
      case fern_FunctionType_applied_c_m2:
        return fern_internal_match(f1->applied_c_m2.f, f2->applied_c_m2.f) &&
                                (f1->applied_c_m2.m == f2->applied_c_m2.m) &&
               fern_internal_match(f1->applied_c_m2.g, f2->applied_c_m2.g) ;
      case fern_FunctionType_train2:
        return fern_internal_match(f1->train2.g, f2->train2.g) &&
               fern_internal_match(f1->train2.h, f2->train2.h);
      case fern_FunctionType_train3:
        return fern_internal_match(f1->train3.f, f2->train3.f) &&
               fern_internal_match(f1->train3.g, f2->train3.g) &&
               fern_internal_match(f1->train3.h, f2->train3.h) ;
      }
    }
    break;
  case fern_Tag_modifier1:
    {
      fern_Modifier1 m1 = fern_unpack_modifier1(x);
      fern_Modifier1 m2 = fern_unpack_modifier1(w);
      if(m1->type != m2->type) {
        return false;
      }
      switch(m1->type) {
      case fern_Modifier1Type_c:
        return m1->c == m2->c;
      case fern_Modifier1Type_block:
        return false;
      case fern_Modifier1Type_partial_m2:
        return fern_internal_match(m1->partial_m2.m, m2->partial_m2.m) &&
               fern_internal_match(m1->partial_m2.g, m2->partial_m2.g) ;
      case fern_Modifier1Type_partial_c_m2:
        return                  (m1->partial_c_m2.m == m2->partial_c_m2.m) &&
               fern_internal_match(m1->partial_c_m2.g, m2->partial_c_m2.g) ;
      }
    }
    break;
  case fern_Tag_modifier2:
    {
      fern_Modifier2 m1 = fern_unpack_modifier2(x);
      fern_Modifier2 m2 = fern_unpack_modifier2(w);
      if(m1->type != m2->type) {
        return false;
      }
      switch(m1->type) {
      case fern_Modifier2Type_c:
        return m1->c == m2->c;
      case fern_Modifier2Type_block:
        return false;
      }
    }
    break;
  }
  return true;
}

fern_Box fern_internal_tofill(fern_Box x) {
  switch(fern_tag(x)) {
  case fern_Tag_character: return fern_SPACE();
  case fern_Tag_number:    return fern_DIGIT_ZERO();
  case fern_Tag_array:
    {
      fern_Array xa = fern_unpack_array(x);
      fern_ArrayReader xar = fern_read_array(xa);

      union fern_Data data;
      fern_Box * cells = fern_init_data(&data, fern_Format_box, fern_array_num_cells(xar));
      for(uint32_t i = 0; i < fern_array_num_cells(xar); i++) {
        *cells++ = fern_internal_tofill(fern_array_get_cell(xar, i));
      }

      return fern_mk_array(&xa->shape, &data, fern_array_fill(xar));
    } 
  default:
    return fern_nil();
  }
}

