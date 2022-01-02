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

