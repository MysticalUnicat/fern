#include "local.h"

// Fill -------------------------------------------------------------------------------------------------------------------------------------------------------
// 'array Fill'     -> any - returns the fill element of 𝕩
// 'array Fill any` -> array - returns 𝕩 with fill valid for 𝕨
static fern_Box fern_Fill_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_array(x)) {
      return fern_unpack_array(x)->fill;
    }
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    {
      fern_Array xa = fern_unpack_array(x);
      return fern_mk_array(&xa->shape, &xa->cells, fern_internal_tofill(w));
    }
    fern_fatal_error("not implemented");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_nil();
}
static struct fern_Function fern_Fill_fn = { .type = fern_FunctionType_c, .c = fern_Fill_evokation0 };
fern_Box fern_provide_Fill(void) {
  return fern_pack_function(&fern_Fill_fn);
}

// Log --------------------------------------------------------------------------------------------------------------------------------------------------------
// '𝕩 Log'   returns the natural logarithm of 𝕩
// '𝕩 Log 𝕨' is equivalent to `𝕩 Log ÷ (𝕨 Log)`
static fern_Box fern_Log_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_number(x)) {
      return fern_pack_number(log(x.number));
    }
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    if(fern_is_number(x) && fern_is_number(w)) {
      return fern_pack_number(log(x.number) / log(w.number));
    }
    fern_fatal_error("not implemented");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_nil();
}
static struct fern_Function fern_Log_fn = { .type = fern_FunctionType_c, .c = fern_Log_evokation0 };
fern_Box fern_provide_Log(void) {
  return fern_pack_function(&fern_Log_fn);
}

// GroupLen ---------------------------------------------------------------------------------------------------------------------------------------------------
// '𝕩 GroupLen 𝕨?' returns the length of each group for ⊔ with an optional minimum result length of the array
static fern_Box fern_GroupLen_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    w = fern_DIGIT_ZERO();
  case fern_Evokation_dyad: {
    if(fern_is_array(x)) {
      fern_Array xa = fern_unpack_array(x);
      fern_ArrayReader xar = fern_read_array(xa);

      int64_t shape = fern_force_natural(w) - 1;
      for(uint32_t i = 0; i < fern_array_num_cells(xar); i++) {
        int64_t nat = fern_array_get_natural(xar, i);
        shape = shape > nat ? shape : nat;
      }
      shape += 1;

      union fern_Data data;
      uint32_t * nums = fern_init_data(&data, fern_Format_natural_32_bit, shape);
      memset(nums, 0, sizeof(*nums) * shape);

      for(uint32_t i = 0; i < fern_array_num_cells(xar); i++) {
        int64_t n = fern_array_get_natural(xar, i);
        if(n >= 0) {
          nums[n]++;
        }
      }

      uint32_t shape_u32 = shape;
      return fern_mk_array2(1, &shape_u32, &data, fern_DIGIT_ZERO());
    }
    fern_fatal_error("not implemented");
  } case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_nil();
}
static struct fern_Function fern_GroupLen_fn = { .type = fern_FunctionType_c, .c = fern_GroupLen_evokation0 };
fern_Box fern_provide_GroupLen(void) {
  return fern_pack_function(&fern_GroupLen_fn);
}

// GroupOrd ---------------------------------------------------------------------------------------------------------------------------------------------------
// '𝕩 GroupOrd 𝕨' assuming 𝕨 is from GroupLen, returns the joined version of ⊔ (deshaped?)
static fern_Box fern_GroupOrd_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    if(fern_is_array(x) && fern_is_array(w)) {
      fern_Array xa = fern_unpack_array(x);
      fern_ArrayReader xar = fern_read_array(xa);

      fern_Array wa = fern_unpack_array(w);
      fern_ArrayReader war = fern_read_array(wa);

      uint32_t * counts = malloc(sizeof(uint32_t) * fern_array_num_cells(war));

      uint32_t shape = 0;
      for(uint32_t i = 0; i < fern_array_num_cells(war); i++) {
        counts[i] = shape;
        shape += fern_array_get_natural(war, i);
      }

      union fern_Data data;

      uint32_t * order = fern_init_data(&data, fern_Format_natural_32_bit, shape);
      for(uint32_t i = 0; i < fern_array_num_cells(xar); i++) {
        int64_t nat = fern_array_get_natural(xar, i);
        if(nat >= 0) {
          uint32_t count = counts[nat];
          counts[nat]++;
          order[count] = i;
        }
      }
      
      free(counts);
      return fern_mk_array2(1, &shape, &data, fern_array_fill(xar));
    }
    fern_fatal_error("not implemented");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_nil();
}
static struct fern_Function fern_GroupOrd_fn = { .type = fern_FunctionType_c, .c = fern_GroupOrd_evokation0 };
fern_Box fern_provide_GroupOrd(void) {
  return fern_pack_function(&fern_GroupOrd_fn);
}

// _fill_by_ --------------------------------------------------------------------------------------------------------------------------------------------------
// '𝔽 _fill_by_ 𝔾'
static inline fern_Box a2fill(fern_Box x) {
  return fern_is_function(x) ? x : fern_is_number(x) ? fern_DIGIT_ZERO() : fern_SPACE();
}
static fern_Box fern__fill_by__evokation0(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w) {
  if(evokation == fern_Evokation_write_to_backend || evokation == fern_Evokation_inverse) {
    fern_fatal_error("not implemented");
  }

  fern_Box r = fern_evoke(f, evokation, x, w);

  fern_Box xf = fern_is_array(xf) ? fern_unpack_array(x)->fill : a2fill(x);

  if(fern_is_array(r)) {
    
  }

  return fern_nil();
}
static struct fern_Modifier2 fern__fill_by__mod2 = { .type = fern_Modifier2Type_c, .c = fern__fill_by__evokation0 };
fern_Box fern_provide__fill_by_(void) {
  return fern_pack_modifier2(&fern__fill_by__mod2);
}

// + ----------------------------------------------------------------------------------------------------------------------------------------------------------
// '𝕩 +' returns itself (complex numbers do different things)
// 'number + number' add two numbers
// 'character + number' returns a character
static fern_Box fern_PLUS_SIGN_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  fern_Box r = { .number = 0 };
  switch(evokation) {
  case fern_Evokation_monad:
    return x;
  case fern_Evokation_dyad:
    r.number = x.number + w.number;
    if(isnan(r.number)) {
      if(fern_is_character(x) && fern_is_number(w)) {
        return fern_pack_character(fern_unpack_character(x) + fern_unpack_number(w));
      }
      fern_fatal_error("+: Arguments must be number + number, or character + number");
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return r;
}
static struct fern_Function fern_PLUS_SIGN_fn = { .type = fern_FunctionType_c, .c = fern_PLUS_SIGN_evokation0 };
fern_Box fern_PLUS_SIGN(void) {
  return fern_pack_function(&fern_PLUS_SIGN_fn);
}

// - ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'number -'              -> number    - negates 𝕩
// 'number - number'       -> number    - subtract two numbers
// 'character - number'    -> character - subtract 𝕨 from 𝕩
// 'character - character' -> number    - get the offset between two codepoints
static fern_Box fern_HYPHEN_MINUS_evokation(fern_Evokation evokation, fern_Box x, fern_Box w) {
  fern_Box r = { .number = 0 };
  switch(evokation) {
  case fern_Evokation_monad:
    r.number = -x.number;
    if(isnan(r.number)) {
      fern_fatal_error("-: Arguments must be a number");
    }
    return r;
  case fern_Evokation_dyad:
    r.number = x.number + w.number;
    if(isnan(r.number)) {
      if(fern_is_character(x) && fern_is_character(w)) {
        return fern_pack_number(fern_unpack_character(x) - fern_unpack_character(w));
      }
      if(fern_is_character(x) && fern_is_number(w)) {
        return fern_pack_character(fern_unpack_character(x) - fern_unpack_number(w));
      }
      fern_fatal_error("-: Arguments must be number - number, character - character, or character - number");
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return r;
}
static struct fern_Function fern_HYPHEN_MINUS_fn = { .type = fern_FunctionType_c, .c = fern_HYPHEN_MINUS_evokation };
fern_Box fern_HYPHEN_MINUS(void) {
  return fern_pack_function(&fern_HYPHEN_MINUS_fn);
}

// × ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'number ×'        -> number - get the sign of 𝕩
// 'number × number' -> number - multiply two numbers
static fern_Box fern_MULTIPLICATION_SIGN_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  fern_Box r = { .number = 0 };
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_number(x)) {
      return fern_pack_number(copysign(fpclassify(x.number) == FP_ZERO ? 0 : 1, x.number));
    }
    fern_fatal_error("×: Arguments must be a number");
  case fern_Evokation_dyad:
    r.number = x.number * w.number;
    if(isnan(r.number)) {
      fern_fatal_error("×: Arguments must be number × number");
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return r;
}
static struct fern_Function fern_MULTIPLICATION_SIGN_fn = { .type = fern_FunctionType_c, .c = fern_MULTIPLICATION_SIGN_evokation0 };
fern_Box fern_MULTIPLICATION_SIGN(void) {
  return fern_pack_function(&fern_MULTIPLICATION_SIGN_fn);
}

// ÷ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'number ÷'        -> number - get the sign of 𝕩
// 'number ÷ number' -> number - multiply two numbers
static fern_Box fern_DIVISION_SIGN_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  fern_Box r = { .number = 0 };
  switch(evokation) {
  case fern_Evokation_monad:
    w = fern_DIGIT_ONE();
  case fern_Evokation_dyad:
    if(fern_is_number(x) && fern_is_number(w)) {
      r.number = x.number / w.number;
    } else {
      fern_fatal_error("÷: Arguments must be number ÷ number");
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return r;
}
static struct fern_Function fern_DIVISION_SIGN_fn = { .type = fern_FunctionType_c, .c = fern_DIVISION_SIGN_evokation0 };
fern_Box fern_DIVISION_SIGN(void) {
  return fern_pack_function(&fern_DIVISION_SIGN_fn);
}

// ⋆ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// √ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⌊ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'number ⌊'        -> number - get the floor of 𝕩
// 'number ⌊ number' -> number - the minimum of 𝕩 and 𝕨
static fern_Box fern_LEFT_FLOOR_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  fern_Box r = { .number = 0 };
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_number(x)) {
      return fern_pack_number(floor(x.number));
    }
    fern_fatal_error("⌊: Arguments must be a number");
  case fern_Evokation_dyad:
    r.number = fmin(x.number, w.number);
    if(isnan(r.number)) {
      fern_fatal_error("⌊: Arguments must be number ⌊ number");
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return r;
}
static struct fern_Function fern_LEFT_FLOOR_fn = { .type = fern_FunctionType_c, .c = fern_LEFT_FLOOR_evokation0 };
fern_Box fern_LEFT_FLOOR(void) {
  return fern_pack_function(&fern_LEFT_FLOOR_fn);
}

// ⌈ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'number ⌈'        -> number - get the floor of 𝕩
// 'number ⌈ number' -> number - the minimum of 𝕩 and 𝕨
fern_Box fern_LEFT_CEILING_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  fern_Box r = { .number = 0 };
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_number(x)) {
      return fern_pack_number(ceil(x.number));
    }
    fern_fatal_error("⌈: Arguments must be a number");
  case fern_Evokation_dyad:
    r.number = fmax(x.number, w.number);
    if(isnan(r.number)) {
      fern_fatal_error("⌈: Arguments must be number ⌊ number");
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return r;
}
struct fern_Function fern_LEFT_CEILING_fn = { .type = fern_FunctionType_c, .c = fern_LEFT_CEILING_evokation0 };
#define fern_LEFT_CEILING fern_pack_function(&fern_LEFT_CEILING_fn)

// ∧ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ∨ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ¬ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// | ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'number |' -> number - get the absolute value
static fern_Box fern_VERTICAL_LINE_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  fern_Box r = { .number = 0 };
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_number(x)) {
      return fern_pack_number(abs(x.number));
    }
    fern_fatal_error("⌈: Arguments must be a number");
  case fern_Evokation_dyad:
    fern_fatal_error("not implemented");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return r;
}
static struct fern_Function fern_VERTICAL_LINE_fn = { .type = fern_FunctionType_c, .c = fern_VERTICAL_LINE_evokation0 };
fern_Box fern_VERTICAL_LINE(void) {
  return fern_pack_function(&fern_VERTICAL_LINE_fn);
}

// ≤ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'number ≤ number'                                                                 -> number - 1 if 𝕩 is less than or equal to 𝕨, 0 otherwise
// 'character ≤ character'                                                           -> number - 1 if 𝕩 is less than or equal to 𝕨, 0 otherwise
// 'array | function | modifier1 | modifier2 | stream ≤ number | character | symbol' -> number - 1
// 'array | function | modifier1 | modifier2 | stream | number ≤ character | symbol' -> number - 1
// 'array | function | modifier1 | modifier2 | stream | number | character ≤ symbol' -> number - 1
//                                                                                   -> number - 0
static inline double lesseq(fern_Box x, fern_Box w) {
  uint32_t _type_rank[] = {
      [fern_Tag_character] = 2
    , [fern_Tag_symbol]    = 3
    , [fern_Tag_array]     = 0
    , [fern_Tag_function]  = 0
    , [fern_Tag_modifier1] = 0
    , [fern_Tag_modifier2] = 0
    , [fern_Tag_namespace] = 0
    , [fern_Tag_stream]    = 0
    , [fern_Tag_number]    = 1
  };
  if(fern_is_number(x) && fern_is_number(w)) {
    return x.number <= w.number;
  }
  if(fern_is_character(x) && fern_is_character(w)) {
    return x.number <= w.number;
  }
  uint32_t x_rank = _type_rank[fern_tag(x)]; 
  uint32_t w_rank = _type_rank[fern_tag(w)]; 
  if(x_rank == 0 || w_rank == 0) {
    fern_fatal_error("≤: Arguments must be number, character, or symbol");
  }
  return x_rank <= w_rank;
}
static fern_Box fern_LESS_THAN_OR_EQUAL_TO_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    return fern_pack_number(lesseq(x, w));
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_LESS_THAN_OR_EQUAL_TO_fn = { .type = fern_FunctionType_c, .c = fern_LESS_THAN_OR_EQUAL_TO_evokation0 };
fern_Box fern_LESS_THAN_OR_EQUAL_TO(void) {
  return fern_pack_function(&fern_LESS_THAN_OR_EQUAL_TO_fn);
}

// < ----------------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_LESS_THAN_SIGN_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    {
      fern_Array array = fern_allocate_array();
      fern_init_array_singleton(array, x, fern_internal_tofill(x));
      return fern_pack_array(array);
    }
  case fern_Evokation_dyad:
    return fern_pack_number(1 - lesseq(x, w));
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_LESS_THAN_SIGN_fn = { .type = fern_FunctionType_c, .c = fern_LESS_THAN_SIGN_evokation0 };
fern_Box fern_LESS_THAN_SIGN(void) {
  return fern_pack_function(&fern_LESS_THAN_SIGN_fn);
}

// > ----------------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_GREATER_THAN_SIGN_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    return fern_pack_number(1 - lesseq(w, x));
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_GREATER_THAN_SIGN_fn = { .type = fern_FunctionType_c, .c = fern_GREATER_THAN_SIGN_evokation0 };
fern_Box fern_GREATER_THAN_SIGN(void) {
  return fern_pack_function(&fern_GREATER_THAN_SIGN_fn);
}

// ≥ ----------------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_GREATER_THAN_OR_EQUAL_TO_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    return fern_pack_number(lesseq(w, x));
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_GREATER_THAN_OR_EQUAL_TO_fn = { .type = fern_FunctionType_c, .c = fern_GREATER_THAN_OR_EQUAL_TO_evokation0 };
fern_Box fern_GREATER_THAN_OR_EQUAL_TO(void) {
  return fern_pack_function(&fern_GREATER_THAN_OR_EQUAL_TO_fn);
}

// = ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'array ='               -> number - the rank of the array
// 'number = number'       -> number - 1 if 𝕩 and 𝕨 are equal, 0 otherwise
// 'character = character' -> number - 1 if 𝕩 and 𝕨 are equal, 0 otherwise
// 'symbol = symbol'       -> number - 1 if 𝕩 and 𝕨 are equal, 0 otherwise
static fern_Box fern_EQUAL_SIGN_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_array(x)) {
      return fern_pack_number(
        fern_array_rank(fern_read_array(fern_unpack_array(x)))
      );
    }
    fern_fatal_error("=: Argument must be a number");
  case fern_Evokation_dyad:
    if(fern_is_number(x) && fern_is_number(w)) {
      return fern_pack_number(x.number == w.number);
    }
    if(fern_is_character(x) && fern_is_character(w)) {
      return fern_pack_number(x.number == w.number);
    }
    fern_fatal_error("=: Arguments must be number = number, or character = character");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_EQUAL_SIGN_fn = { .type = fern_FunctionType_c, .c = fern_EQUAL_SIGN_evokation0 };
fern_Box fern_EQUAL_SIGN(void) {
  return fern_pack_function(&fern_EQUAL_SIGN_fn);
}

// ≠ ----------------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_NOT_EQUAL_SIGN_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
  case fern_Evokation_dyad:
    if(fern_is_array(x)) {
      fern_Array xa = fern_unpack_array(x);
      return fern_pack_number(fern_array_shape(fern_read_array(xa), 0));
    } else {
      return fern_DIGIT_ONE();
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_NOT_EQUAL_SIGN_fn = { .type = fern_FunctionType_c, .c = fern_NOT_EQUAL_SIGN_evokation0 };
fern_Box fern_NOT_EQUAL_SIGN(void) {
  return fern_pack_function(&fern_NOT_EQUAL_SIGN_fn);
}

// ≡ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ≢ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'array ≢' -> 1d array of natural numbers - the shape of the array
// 'any ≢'   -> ⟨⟩                          - non-array has no shape
static fern_Box fern_NOT_IDENTICAL_TO_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_array(x)) {
      fern_Array src_array = fern_unpack_array(x);

      fern_ArrayReader src_array_read = fern_read_array(src_array);
      uint32_t shape = fern_array_rank(src_array_read);

      union fern_Data cells;
      uint32_t * nums = fern_init_data(&cells, fern_Format_natural_32_bit, shape);
      for(uint32_t i = 0; i < shape; i++) {
        *nums = fern_array_shape(src_array_read, i);
      }

      return fern_mk_array2(1, &shape, &cells, fern_DIGIT_ZERO());
    }
    return fern_EMPTY_ARRAY();
  case fern_Evokation_dyad:
    fern_fatal_error("not implemented");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_NOT_IDENTICAL_TO_fn = { .type = fern_FunctionType_c, .c = fern_NOT_IDENTICAL_TO_evokation0 };
fern_Box fern_NOT_IDENTICAL_TO(void) {
  return fern_pack_function(&fern_NOT_IDENTICAL_TO_fn);
}

// ⊣ ----------------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_LEFT_TACK_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    return x;
  case fern_Evokation_dyad:
    return w;
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_LEFT_TACK_fn = { .type = fern_FunctionType_c, .c = fern_LEFT_TACK_evokation0 };
fern_Box fern_LEFT_TACK(void) {
  return fern_pack_function(&fern_LEFT_TACK_fn);
}

// ⊢ ----------------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_RIGHT_TACK_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
  case fern_Evokation_dyad:
    return x;
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_RIGHT_TACK_fn = { .type = fern_FunctionType_c, .c = fern_RIGHT_TACK_evokation0 };
fern_Box fern_RIGHT_TACK(void) {
  return fern_pack_function(&fern_RIGHT_TACK_fn);
}

// ⥊ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'array ⥊'       -> array - 𝕩 array without shape
// 'array ⥊ array' -> array - 𝕩 with shape defined by 𝕨, 𝕨 being a 1d array of natural numbers
static fern_Box fern_LEFT_BARB_UP_RIGHT_BARB_DOWN_HARPOON_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  if(evokation == fern_Evokation_write_to_backend || evokation == fern_Evokation_inverse) {
    fern_fatal_error("not implemented");
  }

  fern_Array xa = fern_unpack_array(x);
  fern_ArrayReader xar = fern_read_array(xa);

  uint32_t rank = 1;
  uint32_t linear_shape;
  uint32_t * shape = &linear_shape;

  if(evokation == fern_Evokation_dyad) {
    fern_Array wa = fern_unpack_array(w);
    fern_ArrayReader war = fern_read_array(wa);
    
    rank = fern_array_num_cells(war);
    shape = malloc(sizeof(uint32_t) * rank);

    for(uint32_t i = 0; i < rank; i++) {
      shape[i] = fern_array_get_natural(war, i);
    }
  } else {
    *shape = fern_array_num_cells(xar);
  }

  fern_Box result = fern_mk_array2(1, shape, &xa->cells, fern_array_fill(xar));

  if(shape != &linear_shape) {
    free(shape);
  }

  return result;
}
static struct fern_Function fern_LEFT_BARB_UP_RIGHT_BARB_DOWN_HARPOON_fn = { .type = fern_FunctionType_c, .c = fern_LEFT_BARB_UP_RIGHT_BARB_DOWN_HARPOON_evokation0 };
fern_Box fern_LEFT_BARB_UP_RIGHT_BARB_DOWN_HARPOON(void) {
  return fern_pack_function(&fern_LEFT_BARB_UP_RIGHT_BARB_DOWN_HARPOON_fn);
}

// ∾ ----------------------------------------------------------------------------------------------------------------------------------------------------------

// ≍ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⋈ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ↑ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ↓ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ↕ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'natural ↕' -> 1d array - make an array of natural numbers from 0 to 𝕩
static fern_Box fern_UP_DOWN_ARROW_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    {
      uint32_t shape = fern_force_natural(x);

      union fern_Data data;
      uint32_t * nums = fern_init_data(&data, fern_Format_natural_32_bit, shape);

      for(uint32_t i = 0; i < shape; i++) {
        nums[i] = i;
      }

      return fern_mk_array2(1, &shape, &data, fern_DIGIT_ZERO());
    }
  case fern_Evokation_dyad:
    fern_fatal_error("not implemented");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_UP_DOWN_ARROW_fn = { .type = fern_FunctionType_c, .c = fern_UP_DOWN_ARROW_evokation0 };
fern_Box fern_UP_DOWN_ARROW(void) {
  return fern_pack_function(&fern_UP_DOWN_ARROW_fn);
}

// « ----------------------------------------------------------------------------------------------------------------------------------------------------------
// » ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⌽ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⍉ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// / ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⍋ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⍒ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⊏ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⊑ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// 'array ⊑ natural' -> any - get the first item from 𝕩, index is 𝕨
static fern_Box fern_SQUARE_IMAGE_OF_OR_EQUAL_TO_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    {
      fern_Array xa = fern_unpack_array(x);
      fern_ArrayReader xar = fern_read_array(xa);
      return fern_array_get_cell(xar, fern_force_natural(w));
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_SQUARE_IMAGE_OF_OR_EQUAL_TO_fn = { .type = fern_FunctionType_c, .c = fern_SQUARE_IMAGE_OF_OR_EQUAL_TO_evokation0 };
fern_Box fern_SQUARE_IMAGE_OF_OR_EQUAL_TO(void) {
  return fern_pack_function(&fern_SQUARE_IMAGE_OF_OR_EQUAL_TO_fn);
}

// ⊐ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⊒ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ∊ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⍷ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ⊔ ----------------------------------------------------------------------------------------------------------------------------------------------------------
// ! ----------------------------------------------------------------------------------------------------------------------------------------------------------
// '𝕩 !'   -> nothing - panic! if 𝕩 != 1 printing 𝕩
// '𝕩 ! 𝕨' -> nothing - panic! if 𝕩 != 1 printing 𝕨
static fern_Box fern_EXCLAMATION_MARK_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    w = x;
  case fern_Evokation_dyad:
    if(!fern_is_number(x) || x.number != 1) {
      fern_internal_throw(w);
    }
    return x;
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Function fern_EXCLAMATION_MARK_fn = { .type = fern_FunctionType_c, .c = fern_EXCLAMATION_MARK_evokation0 };
fern_Box fern_EXCLAMATION_MARK(void) {
  return fern_pack_function(&fern_EXCLAMATION_MARK_fn);
}

// ============================================================================================================================================================

// ˙ constant -------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_DOT_ABOVE_evokation0(fern_Evokation evokation, fern_Box f, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
  case fern_Evokation_dyad:
    return f;
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier1 fern_DOT_ABOVE_mod1 = { .type = fern_Modifier1Type_c, .c = fern_DOT_ABOVE_evokation0 };
fern_Box fern_DOT_ABOVE(void) {
  return fern_pack_modifier1(&fern_DOT_ABOVE_mod1);
}

// ˜ swap -----------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_SMALL_TILDE_evokation0(fern_Evokation evokation, fern_Box f, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    w = x;
  case fern_Evokation_dyad:
    CALL_2(f, w, x);
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier1 fern_SMALL_TILDE_mod1 = { .type = fern_Modifier1Type_c, .c = fern_SMALL_TILDE_evokation0 };
fern_Box fern_SMALL_TILDE(void) {
  return fern_pack_modifier1(&fern_SMALL_TILDE_mod1);
}

// ˘ cells ----------------------------------------------------------------------------------------------------------------------------------------------------

// ¨ each -----------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_DIAERESIS_evokation0(fern_Evokation evokation, fern_Box f, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_array(x)) {
      fern_Array xa = fern_unpack_array(x);
      fern_ArrayReader xar = fern_read_array(xa);

      union fern_Data data;
      fern_Box * cells = fern_init_data(&data, fern_Format_box, fern_array_rank(xar));
      for(uint32_t i = 0; i < fern_array_rank(xar); i++) {
        cells[i] = CALL_1(f, x);
      }
      
      return fern_mk_array(&xa->shape, &data, fern_DIGIT_ZERO());
    }
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    fern_fatal_error("not implemented");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier1 fern_DIAERESIS_mod1 = { .type = fern_Modifier1Type_c, .c = fern_DIAERESIS_evokation0 };
fern_Box fern_DIAERESIS(void) {
  return fern_pack_modifier1(&fern_DIAERESIS_mod1);
}

// ⌜ table ----------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_TOP_LEFT_CORNER_evokation0(fern_Evokation evokation, fern_Box f, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    if(fern_is_array(x)) {
      fern_Array xa = fern_unpack_array(x);
      fern_ArrayReader xar = fern_read_array(xa);

      union fern_Data data;
      fern_Box * cells = fern_init_data(&data, fern_Format_box, fern_array_rank(xar));
      for(uint32_t i = 0; i < fern_array_rank(xar); i++) {
        cells[i] = CALL_1(f, x);
      }
      
      return fern_mk_array(&xa->shape, &data, fern_DIGIT_ZERO());
    }
    fern_fatal_error("not implemented");
  case fern_Evokation_dyad:
    if(fern_is_array(x) && fern_is_array(w)) {
      fern_Array xa = fern_unpack_array(x);
      fern_ArrayReader xar = fern_read_array(xa);

      fern_Array wa = fern_unpack_array(w);
      fern_ArrayReader war = fern_read_array(wa);

      union fern_Data data;
      fern_Box * cells = fern_init_data(&data, fern_Format_box, fern_array_num_cells(xar) * fern_array_num_cells(war));

      for(uint32_t i = 0; i < fern_array_num_cells(war); i++) {
        fern_Box w_cell = fern_array_get_cell(war, i);
        for(uint32_t j = 0; j < fern_array_num_cells(xar); j++) {
          fern_Box x_cell = fern_array_get_cell(xar, i);
          *cells++ = CALL_2(f, x_cell, w_cell);
        }
      }

      size_t new_rank = fern_array_rank(war) + fern_array_rank(xar);
      uint32_t * shape = malloc(sizeof(*shape) * new_rank);
      for(uint32_t i = 0; i < fern_array_rank(war); i++) {
        shape[i] = fern_array_shape(war, i);
      }
      for(uint32_t i = fern_array_rank(war), j = 0; j < fern_array_rank(xar); i++, j++) {
        shape[i] = fern_array_shape(xar, j);
      }

      fern_Box result = fern_mk_array2(new_rank, shape, &data, fern_DIGIT_ZERO());
      
      free(shape);

      return result;
    }
    fern_fatal_error("not implemented");
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier1 fern_TOP_LEFT_CORNER_mod1 = { .type = fern_Modifier1Type_c, .c = fern_TOP_LEFT_CORNER_evokation0 };
fern_Box fern_TOP_LEFT_CORNER(void) {
  return fern_pack_modifier1(&fern_TOP_LEFT_CORNER_mod1);
}

// ⁼ inverse --------------------------------------------------------------------------------------------------------------------------------------------------
// ´ fold -----------------------------------------------------------------------------------------------------------------------------------------------------
// ˝ insert ---------------------------------------------------------------------------------------------------------------------------------------------------
// ` scan -----------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_GRAVE_ACCENT_evokation0(fern_Evokation evokation, fern_Box f, fern_Box x, fern_Box w) {
  uint32_t one = 1;
  
  if(evokation == fern_Evokation_write_to_backend || evokation == fern_Evokation_inverse) {
    fern_fatal_error("not implemented");
  }

  if(!fern_is_array(x)) {
    fern_fatal_error("not implemented");
  }

  fern_Array xa = fern_unpack_array(x);
  fern_ArrayReader xar = fern_read_array(xa);

  fern_Array wa;
  struct fern_Array w_singleton;

  if(evokation == fern_Evokation_dyad) {
    uint32_t w_rank = 0;

    if(fern_is_array(w)) {
      wa = fern_unpack_array(w);
      w_rank = fern_array_rank(fern_read_array(wa));
    }

    if(w_rank + 1 != fern_array_rank(xar)) {
      fern_fatal_error("`: rank of 𝕨 must be cell rank of 𝕩");
    }

    if(fern_is_array(w)) {
      fern_ArrayReader war = fern_read_array(wa);

      for(uint32_t i = 0; i < fern_array_rank(war); i++) {
        if(fern_array_shape(war, i) != fern_array_shape(xar, i + 1)) {
          fern_fatal_error("`: shape of 𝕨 must be cell shape of 𝕩");
        }
      }
    } else {
      fern_init_shape(&w_singleton.shape, 1, &one);
      *(fern_Box *)fern_init_data(&w_singleton.cells, fern_Format_box, 1) = w;
      wa = &w_singleton;
    }
  }

  uint32_t l = fern_array_num_cells(xar);
  if(l == 0) {
    return fern_EMPTY_ARRAY();
  }

  union fern_Data cells;
  fern_Box * result = fern_init_data(&cells, fern_Format_box, l);

  uint32_t c = 1;
  for(uint32_t i = 1; i < fern_array_rank(xar); i++) {
    c *= fern_array_shape(xar, i);
  }

  uint32_t i;
  if(evokation == fern_Evokation_dyad) {
    fern_ArrayReader war = fern_read_array(wa);
    
    for(i = 0; c; i++) {
      result[i] = CALL_2(f, fern_array_get_cell(xar, i), fern_array_get_cell(war, i));
    }
  } else {
    for(i = 0; c; i++) {
      result[i] = fern_array_get_cell(xar, i);
    }
  }

  for(; i < l; i++) {
    result[i] = CALL_2(f, fern_array_get_cell(xar, i), result[i - c]);
  }

  return fern_mk_array(&xa->shape, &cells, fern_array_fill(xar));
}
static struct fern_Modifier1 fern_GRAVE_ACCENT_mod1 = { .type = fern_Modifier1Type_c, .c = fern_GRAVE_ACCENT_evokation0 };
fern_Box fern_GRAVE_ACCENT(void) {
  return fern_pack_modifier1(&fern_GRAVE_ACCENT_mod1);
}

// ∘ atop -----------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_RING_OPERATOR_evokation0(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
  case fern_Evokation_dyad:
    return CALL_1(f, fern_evoke(g, evokation, x, w));
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier2 fern_RING_OPERATOR_mod2 = { .type = fern_Modifier2Type_c, .c = fern_RING_OPERATOR_evokation0 };
fern_Box fern_RING_OPERATOR(void) {
  return fern_pack_modifier2(&fern_RING_OPERATOR_mod2);
}

// ○ over -----------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_WHITE_CIRCLE_evokation0(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    return CALL_1(f, CALL_1(g, x));
  case fern_Evokation_dyad:
    return CALL_2(f, CALL_1(g, x), CALL_1(g, w));
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier2 fern_WHITE_CIRCLE_mod2 = { .type = fern_Modifier2Type_c, .c = fern_WHITE_CIRCLE_evokation0 };
fern_Box fern_WHITE_CIRCLE(void) {
  return fern_pack_modifier2(&fern_WHITE_CIRCLE_mod2);
}

// ⊸ before ---------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_MULTIMAP_evokation0(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    w = x;
  case fern_Evokation_dyad:
    return CALL_2(g, x, CALL_1(f, w));
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier2 fern_MULTIMAP_mod2 = { .type = fern_Modifier2Type_c, .c = fern_MULTIMAP_evokation0 };
fern_Box fern_MULTIMAP(void) {
  return fern_pack_modifier2(&fern_MULTIMAP_mod2);
}

// ⟜ after ----------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_LEFT_MULTIMAP_evokation0(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    w = x;
  case fern_Evokation_dyad:
    return CALL_2(f, CALL_1(g, x), w);
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier2 fern_LEFT_MULTIMAP_mod2 = { .type = fern_Modifier2Type_c, .c = fern_LEFT_MULTIMAP_evokation0 };
fern_Box fern_LEFT_MULTIMAP(void) {
  return fern_pack_modifier2(&fern_LEFT_MULTIMAP_mod2);
}

// ⌾ under ----------------------------------------------------------------------------------------------------------------------------------------------------
// ⊘ valences -------------------------------------------------------------------------------------------------------------------------------------------------
// 'any 𝔽⊘𝔾'     -> any - call '𝕩 𝔽'
// 'any 𝔽⊘𝔾 any' -> any - call '𝕩 𝔾 𝕨'
static fern_Box fern_CIRCLED_DIVISION_SLASH_evokation0(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
    return fern_evoke(f, evokation, x, w);
  case fern_Evokation_dyad:
    return fern_evoke(g, evokation, x, w);
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier2 fern_CIRCLED_DIVISION_SLASH_mod2 = { .type = fern_Modifier2Type_c, .c = fern_CIRCLED_DIVISION_SLASH_evokation0 };
fern_Box fern_CIRCLED_DIVISION_SLASH(void) {
  return fern_pack_modifier2(&fern_CIRCLED_DIVISION_SLASH_mod2);
}

// ◶ choose ---------------------------------------------------------------------------------------------------------------------------------------------------
static fern_Box fern_WHITE_CIRCLE_WITH_LOWER_RIGHT_QUADRANT_evokation0(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w) {
  switch(evokation) {
  case fern_Evokation_monad:
  case fern_Evokation_dyad:
    {
      fern_Box index = fern_evoke(f, evokation, x, w);
      fern_Array ga = fern_unpack_array(g);
      fern_ArrayReader gar = fern_read_array(ga);
      g = fern_array_get_cell(gar, fern_force_natural(index));
      return fern_evoke(g, evokation, x, w);
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier2 fern_WHITE_CIRCLE_WITH_LOWER_RIGHT_QUADRANT_mod2 = { .type = fern_Modifier2Type_c, .c = fern_WHITE_CIRCLE_WITH_LOWER_RIGHT_QUADRANT_evokation0 };
fern_Box fern_WHITE_CIRCLE_WITH_LOWER_RIGHT_QUADRANT(void) {
  return fern_pack_modifier2(&fern_WHITE_CIRCLE_WITH_LOWER_RIGHT_QUADRANT_mod2);
}

// ⎊ catch ----------------------------------------------------------------------------------------------------------------------------------------------------
// 'any 𝔽⎊𝔾'     -> any - return '𝕩 𝔽'   if there is an error, then return '𝕩 𝔾'
// 'any 𝔽⎊𝔾 any' -> any - return '𝕩 𝔽 𝕨' if there is an error, then return '𝕩 𝔾 𝕨' 
static fern_Box fern_CIRCLED_TRIANGLE_DOWN_evokation0(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w) {
  fern_ExStack exstack;
  switch(evokation) {
  case fern_Evokation_monad:
  case fern_Evokation_dyad:
    if(fern_ExStack_begin(&exstack)) {
      fern_Box result = fern_evoke(f, evokation, x, w);
      fern_ExStack_end(&exstack);
      return result;
    } else {
      return fern_evoke(g, evokation, x, w);
    }
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
static struct fern_Modifier2 fern_CIRCLED_TRIANGLE_DOWN_mod2 = { .type = fern_Modifier2Type_c, .c = fern_CIRCLED_TRIANGLE_DOWN_evokation0 };
fern_Box fern_CIRCLED_TRIANGLE_DOWN(void) {
  return fern_pack_modifier2(&fern_CIRCLED_TRIANGLE_DOWN_mod2);
}

// ⎉ rank -----------------------------------------------------------------------------------------------------------------------------------------------------
// ⚇ depth ----------------------------------------------------------------------------------------------------------------------------------------------------
// ⍟ repeat ---------------------------------------------------------------------------------------------------------------------------------------------------

