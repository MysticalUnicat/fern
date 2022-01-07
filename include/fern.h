#include <uchar.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// from <math.h>
double round(double);

// ============================================================================================================================================================
// minimal user messenging
__attribute__((noreturn))
void fern_fatal_error(const char * format, ...);

void fern_assert_fatal_error(bool condition, const char * format, ...);

// ============================================================================================================================================================
// basic double NaN packing
// 0111 1111 1111 0--- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- | signaling NaN
// 0111 1111 1111 1--- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- | quiet NaN
// 0111 1111 1111 1ttt ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- | 3 tag bits
// 0111 1111 1111 1--- pppp pppp pppp pppp pppp pppp pppp pppp pppp pppp pppp pppp | 48-bit payload

typedef union {
  uint64_t bits;
  double   number;
} fern_Box;

#define FERN_BOX_NAN_MASK     0xfff8000000000000ull
#define FERN_BOX_NAN_QUIET    0x7ff8000000000000ull
#define FERN_BOX_PAYLOAD_MASK 0x000fffffffffffffull
#define FERN_BOX_INVALID      (1 << 3)

#define FERN_CONSTRUCT_BOX(TAG, PAYLOAD) ((fern_Box) { .bits = FERN_BOX_NAN_QUIET | ((uint64_t)(TAG) << 48) | (PAYLOAD) })

static inline fern_Box fern_box(uint64_t tag, uint64_t payload) {
  return FERN_CONSTRUCT_BOX(tag, payload);
}

static inline uint64_t fern_tag(fern_Box b) {
  return ((b.bits & FERN_BOX_NAN_MASK) == FERN_BOX_NAN_QUIET) ? (b.bits >> 48) & 0x7 : FERN_BOX_INVALID;
}

static inline uint64_t fern_payload(fern_Box b) {
  return b.bits & FERN_BOX_PAYLOAD_MASK;
}

// ============================================================================================================================================================
// core types
// store in 3 bits, so it can be encoded into fern_PackedData
typedef enum {
    fern_Format_natural_1_bit
  , fern_Format_natural_8_bit
  , fern_Format_natural_16_bit
  , fern_Format_natural_32_bit
  , fern_Format_character
  , fern_Format_symbol
  , fern_Format_box
  , fern_Format_unused1
  , fern_Format_LAST
} fern_Format;
static_assert(fern_Format_LAST == (1 << 3));

// this is a 'fat pointer' to some immutable data
// this includes storing the length, pointer to reference counting integer, and the real pointer to the data
// this also optionally small arrays into the 'fat pointer' itself. hopefully removing *many* allocations
// 
// for 64-bit archetectures, this means the 'fat pointer' is 192 bits. the in-place header is 16 bits leaving 176 bits for data
typedef union fern_Data {
  struct {
    uintptr_t is_pointer : 1;
    uintptr_t _pad1      : (sizeof(uintptr_t) * 8) - 1;
    uintptr_t _pad2;
    uintptr_t _pad3;
  };
  struct {
    uintptr_t is_pointer : 1;
    uintptr_t format     : 3;
    uintptr_t size       : (sizeof(uintptr_t) * 8) - 4;
    uintptr_t rc;
    uintptr_t pointer;
  } pointer;
  struct {
    uint16_t is_pointer : 1;
    uint16_t format     : 3;
    uint16_t size       : 12;
    uint8_t  data[sizeof(uintptr_t) * 3 - 2];
  } inplace;
} *fern_Data;

// an array is just a shape, data, and fill element
typedef struct fern_Array {
  union fern_Data shape;
  union fern_Data cells;
  fern_Box        fill;
} *fern_Array;

typedef enum {
    fern_Evokation_monad
  , fern_Evokation_dyad
  , fern_Evokation_write_to_backend
  , fern_Evokation_inverse
} fern_Evokation;

// functions come from primitive roots, each with monad and dyad and each with evoking (run-time) and emit (compile-time)
typedef fern_Box (*fern_FunctionEvokation)(fern_Evokation evokation, fern_Box x, fern_Box w);
typedef fern_Box (*fern_Modifier1Evokation)(fern_Evokation evokation, fern_Box f, fern_Box x, fern_Box w);
typedef fern_Box (*fern_Modifier2Evokation)(fern_Evokation evokation, fern_Box f, fern_Box g, fern_Box x, fern_Box w);

// these will be expanded later
enum fern_FunctionType {
    fern_FunctionType_c
  , fern_FunctionType_block
  , fern_FunctionType_applied_m1
  , fern_FunctionType_applied_c_m1
  , fern_FunctionType_applied_m2
  , fern_FunctionType_applied_c_m2
  , fern_FunctionType_train2
  , fern_FunctionType_train3
};

typedef struct fern_Function {
  enum fern_FunctionType type;
  union {
    fern_FunctionEvokation c;
    struct {
      fern_Box f;
      fern_Box m;
    } applied_m1;
    struct {
      fern_Box f;
      fern_Modifier1Evokation m;
    } applied_c_m1;
    struct {
      fern_Box f;
      fern_Box m;
      fern_Box g;
    } applied_m2;
    struct {
      fern_Box f;
      fern_Modifier2Evokation m;
      fern_Box g;
    } applied_c_m2;
    struct {
      fern_Box g;
      fern_Box h;
    } train2;
    struct {
      fern_Box f;
      fern_Box g;
      fern_Box h;
    } train3;
  };
} *fern_Function;

enum fern_Modifier1Type {
    fern_Modifier1Type_c
  , fern_Modifier1Type_block
  , fern_Modifier1Type_partial_m2
  , fern_Modifier1Type_partial_c_m2
};

typedef struct fern_Modifier1 {
  enum fern_Modifier1Type type;
  union {
    fern_Modifier1Evokation c;
    struct {
      fern_Box m;
      fern_Box g;
    } partial_m2;
    struct {
      fern_Modifier2Evokation m;
      fern_Box g;
    } partial_c_m2;
  };
} *fern_Modifier1;

enum fern_Modifier2Type {
    fern_Modifier2Type_c
  , fern_Modifier2Type_block
};

typedef struct fern_Modifier2 {
  enum fern_Modifier2Type type;
  union {
    fern_Modifier2Evokation c;
  };
} *fern_Modifier2;

// namespaces are not immutable, simple storage here
typedef struct fern_Namespace {
  struct fern_Namespace * parent;
  uint32_t length;
  struct fern_Namespace_pair {
    uint32_t symbol;
    fern_Box value;
  } * data;
} *fern_Namespace;

// a type to get async io and things as a core feature
typedef struct fern_Stream {
  uintptr_t flag_is_file : 1;
  uintptr_t flag_unused  : (sizeof(uintptr_t) * 8) - 1;
  union {
    void * f;
  };
} *fern_Stream;

// ============================================================================================================================================================
// BOXING where the optimal case (numbers) are transparently used
enum {
    fern_Tag_character
  , fern_Tag_symbol
  , fern_Tag_array
  , fern_Tag_function
  , fern_Tag_modifier1
  , fern_Tag_modifier2
  , fern_Tag_namespace
  , fern_Tag_stream
  , fern_Tag_number
};
static_assert(fern_Tag_number == FERN_BOX_INVALID);

static inline bool fern_is_character(fern_Box b) { return fern_tag(b) == fern_Tag_character; }
static inline bool fern_is_symbol(fern_Box b) { return fern_tag(b) == fern_Tag_symbol; }
static inline bool fern_is_array(fern_Box b) { return fern_tag(b) == fern_Tag_array; }
static inline bool fern_is_function(fern_Box b) { return fern_tag(b) == fern_Tag_function; }
static inline bool fern_is_modifier1(fern_Box b) { return fern_tag(b) == fern_Tag_modifier1; }
static inline bool fern_is_modifier2(fern_Box b) { return fern_tag(b) == fern_Tag_modifier2; }
static inline bool fern_is_namespace(fern_Box b) { return fern_tag(b) == fern_Tag_namespace; }
static inline bool fern_is_stream(fern_Box b) { return fern_tag(b) == fern_Tag_stream; }
static inline bool fern_is_number(fern_Box b) { return fern_tag(b) == fern_Tag_number; }

static inline char32_t fern_unpack_character(fern_Box b) {
  fern_assert_fatal_error(fern_is_character(b), "expected character");
  return fern_payload(b);
}

static inline uint32_t fern_unpack_symbol(fern_Box b) {
  fern_assert_fatal_error(fern_is_symbol(b), "expected symbol");
  return fern_payload(b);
}

static inline fern_Array fern_unpack_array(fern_Box b) {
  fern_assert_fatal_error(fern_is_array(b), "expected array");
  return (fern_Array)fern_payload(b);
}

static inline fern_Function fern_unpack_function(fern_Box b) {
  fern_assert_fatal_error(fern_is_function(b), "expected function");
  return (fern_Function)fern_payload(b);
}

static inline fern_Modifier1 fern_unpack_modifier1(fern_Box b) {
  fern_assert_fatal_error(fern_is_modifier1(b), "expected modifier1");
  return (fern_Modifier1)fern_payload(b);
}

static inline fern_Modifier2 fern_unpack_modifier2(fern_Box b) {
  fern_assert_fatal_error(fern_is_modifier2(b), "expected modifier2");
  return (fern_Modifier2)fern_payload(b);
}

static inline fern_Namespace fern_unpack_namespace(fern_Box b) {
  fern_assert_fatal_error(fern_is_namespace(b), "expected namespace");
  return (fern_Namespace)fern_payload(b);
}

static inline fern_Stream fern_unpack_stream(fern_Box b) {
  fern_assert_fatal_error(fern_is_stream(b), "expected namespace");
  return (fern_Stream)fern_payload(b);
}

static inline double fern_unpack_number(fern_Box b) {
  fern_assert_fatal_error(fern_is_number(b), "expected namespace");
  return b.number;
}

static inline fern_Box fern_pack_character(char32_t character) {
  return fern_box(fern_Tag_character, character);
}

static inline fern_Box fern_pack_symbol(uint32_t symbol) {
  return fern_box(fern_Tag_symbol, symbol);
}

static inline fern_Box fern_pack_array(fern_Array array) {
  return fern_box(fern_Tag_array, (uint64_t)array);
}

static inline fern_Box fern_pack_function(fern_Function function) {
  return fern_box(fern_Tag_function, (uint64_t)function);
}

static inline fern_Box fern_pack_modifier1(fern_Modifier1 modifier1) {
  return fern_box(fern_Tag_modifier1, (uint64_t)modifier1);
}

static inline fern_Box fern_pack_modifier2(fern_Modifier2 modifier2) {
  return fern_box(fern_Tag_modifier2, (uint64_t)modifier2);
}

static inline fern_Box fern_pack_namespace(fern_Namespace namespace) {
  return fern_box(fern_Tag_namespace, (uint64_t)namespace);
}

static inline fern_Box fern_pack_stream(fern_Stream stream) {
  return fern_box(fern_Tag_stream, (uint64_t)stream);
}

static inline fern_Box fern_pack_number(double number) {
  return (fern_Box) { .number = number };
}

// ============================================================================================================================================================
// object lifetime
void * fern_init_data(fern_Data data, fern_Format format, uint32_t size);
void fern_clone_data(fern_Data data, fern_Data other);
void fern_free_data(fern_Data data);

fern_Array fern_allocate_array(void);
fern_Function fern_allocate_function(void);
fern_Modifier1 fern_allocate_modifier1(void);
fern_Modifier2 fern_allocate_modifier2(void);
fern_Namespace fern_allocate_namespace(void);
fern_Stream fern_allocate_stream(void); // TODO

void fern_init_shape(fern_Data data, uint32_t rank, uint32_t * shape);

void fern_init_array(fern_Array array, fern_Data shape, fern_Data data, fern_Box fill);
void fern_init_array2(fern_Array array, uint32_t rank, uint32_t * shape, fern_Data data, fern_Box fill);
void fern_init_array3(fern_Array array, fern_Data data, fern_Box fill);
void fern_init_array_singleton(fern_Array array, fern_Box cell, fern_Box fill);

void fern_init_symbol(uint32_t * symbol, const char * string, uint32_t string_size);

void fern_init_function_c(
    fern_Function function
  , fern_FunctionEvokation evokation
  );
void fern_init_modifier1_c(
    fern_Modifier1 modifier1
  , fern_Modifier1Evokation evokation
  );
void fern_init_modifier2_c(
    fern_Modifier2 modifier2
  , fern_Modifier2Evokation evokation
  );
void fern_init_namespace(fern_Namespace namespace, fern_Namespace parent);
void fern_init_stream(fern_Stream stream); // TODO
void fern_init_number(double * number, const char32_t * string, uint32_t string_size);

fern_Box fern_clone(fern_Box);
void fern_free(fern_Box);

const char * fern_symbol_string(uint32_t symbol);

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// most data allocated is immutable, thus read-only. make them easier to read with this
typedef struct {
  fern_Format format;
  uint32_t    size;
  union {
    const uintptr_t pointer;
    const uint8_t   * natural_1_bit;
    const uint8_t   * natural_8_bit;
    const uint16_t  * natural_16_bit;
    const uint32_t  * natural_32_bit;
    const char32_t  * character;
    const uint32_t  * symbol;
    const fern_Box  * box;
  };
} fern_DataReader;

static inline fern_DataReader fern_read_data(fern_Data data) {
  if(data->is_pointer) {
    return (fern_DataReader) {
        .format  = data->pointer.format
      , .size    = data->pointer.size
      , .pointer = data->pointer.pointer
    };
  } else {
    return (fern_DataReader) {
        .format  = data->inplace.format
      , .size    = data->inplace.size
      , .pointer = (uintptr_t)data->inplace.data
    };
  }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// arrays are immutable, so they are kinda packed. make them easier to read with this
typedef struct {
  fern_DataReader shape;
  fern_DataReader cells;
  fern_Box        fill;
} fern_ArrayReader;

static inline fern_ArrayReader fern_read_array(fern_Array array) {
  static uint32_t zero = 0;

  if(array == 0) {
    return (fern_ArrayReader) {
        .shape = (fern_DataReader) { .format = fern_Format_natural_32_bit, .size = 1, .natural_32_bit = &zero }
      , .cells = (fern_DataReader) { .format = fern_Format_box,            .size = 0, .pointer = 0 }
      , .fill  = fern_pack_number(0)
      };
  }

  return (fern_ArrayReader) {
      .shape = fern_read_data(&array->shape)
    , .cells = fern_read_data(&array->cells)
    , .fill  = array->fill
    };
}

static inline fern_Box fern_data_get_cell(fern_DataReader reader, uintptr_t index) {
  fern_assert_fatal_error(index < reader.size, "out of bounds error");
  switch(reader.format) {
  case fern_Format_natural_1_bit:  return fern_pack_number((reader.natural_1_bit[index >> 3] & (1 << (index & 7))) != 0);
  case fern_Format_natural_8_bit:  return fern_pack_number(reader.natural_8_bit[index]);
  case fern_Format_natural_16_bit: return fern_pack_number(reader.natural_16_bit[index]);
  case fern_Format_natural_32_bit: return fern_pack_number(reader.natural_32_bit[index]);
  case fern_Format_character:      return fern_pack_character(reader.character[index]);
  case fern_Format_symbol:         return fern_pack_symbol(reader.symbol[index]);
  case fern_Format_box:            return reader.box[index];
  default:                         fern_fatal_error("invalid format");
  }
}

static inline int64_t fern_force_natural(fern_Box b) {
  if(fern_is_number(b)) {
    if(round(b.number) == b.number && b.number < (1ull << 32)) {
      return (uint32_t)b.number;
    } else {
      return -1;
    }
  } else {
    fern_fatal_error("%a is cannot be coreced into an natural number");
  }
}

static inline int64_t fern_data_get_natural(fern_DataReader reader, uintptr_t index) {
  fern_assert_fatal_error(index < reader.size, "out of bounds error");
  switch(reader.format) {
  case fern_Format_natural_1_bit:  return (reader.natural_1_bit[index >> 3] & (1 << (index & 7))) != 0;
  case fern_Format_natural_8_bit:  return reader.natural_8_bit[index];
  case fern_Format_natural_16_bit: return reader.natural_16_bit[index];
  case fern_Format_natural_32_bit: return reader.natural_32_bit[index];
  case fern_Format_box:            return fern_force_natural(reader.box[index]);
  default:                         fern_fatal_error("invalid format");
  }
}

static inline uint32_t fern_array_rank(fern_ArrayReader reader) {
  return reader.shape.size;
}

static inline uint32_t fern_array_axis_length(fern_ArrayReader reader, uint32_t axis) {
  fern_assert_fatal_error(axis < reader.shape.size, "out of bounds error");
  switch(reader.shape.format) {
  case fern_Format_natural_8_bit:  return reader.shape.natural_8_bit[axis];
  case fern_Format_natural_16_bit: return reader.shape.natural_16_bit[axis];
  case fern_Format_natural_32_bit: return reader.shape.natural_32_bit[axis];
  default:                         fern_fatal_error("invalid format");
  }
}

static inline uint32_t fern_array_num_cells(fern_ArrayReader reader) {
  uint32_t result = 1;
  for(uint32_t i = 0; i < fern_array_rank(reader); i++) {
    result *= fern_array_axis_length(reader, i);
  }
  return result;
}

static inline fern_Box fern_array_get_cell(fern_ArrayReader reader, uint32_t index) {
  if(index >= reader.cells.size) {
    return reader.fill;
  }
  return fern_data_get_cell(reader.cells, index);
}

static inline int64_t fern_array_get_natural(fern_ArrayReader reader, uint32_t index) {
  if(index >= reader.cells.size) {
    return fern_force_natural(reader.fill);
  }
  return fern_data_get_natural(reader.cells, index);
}

static inline fern_Box fern_array_fill(fern_ArrayReader reader) {
  return reader.fill;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// shortcuts
static inline fern_Box fern_mk_array(fern_Data shape, fern_Data cells, fern_Box fill) {
  fern_Array array = fern_allocate_array();
  fern_init_array(array, shape, cells, fill);
  return fern_pack_array(array);
}

static inline fern_Box fern_mk_array2(uint32_t rank, uint32_t * shape, fern_Data cells, fern_Box fill) {
  fern_Array array = fern_allocate_array();
  fern_init_array2(array, rank, shape, cells, fill);
  return fern_pack_array(array);
}

static inline fern_Box fern_mk_array3(fern_Data cells, fern_Box fill) {
  fern_Array array = fern_allocate_array();
  fern_init_array3(array, cells, fill);
  return fern_pack_array(array);
}

static inline fern_Box fern_mk_symbol(const char * string) {
  size_t strlen(const char *);
  uint32_t result;
  fern_init_symbol(&result, string, strlen(string));
  return fern_pack_symbol(result);
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// namespaces are mutable, only by setting and resetting
fern_Box fern_namespace_get(fern_Namespace namespace, uint32_t symbol);
void fern_namespace_define(fern_Namespace namespace, uint32_t symbol, fern_Box value);
void fern_namespace_redefine(fern_Namespace namespace, uint32_t symbol, fern_Box value);

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// \ makes the remaining of the expression the rhs
//
//   2 + \ 2 × 2 == 2 + (2 × 2) = 6
//
//   2 + 2 \ × 2 == 2 + 2 × 2 = 8
// 
// \\ is the same but allows the programmer to comment in the middle of an expression by skipping the remainder of the line and joining them
//   2 +     \\ join comment
//     2 × 2 == 2 + (2 × 2) = 6
//
//   2 + 2   \\ join comment
//     × 2   == 2 + 2 × 2 = 8
//
// the stream IO symbol is ⇋
// 
// stream ⇋                                 - read a single value from the stream. same as `stream⇋1⊑`
// stream ⇋ natural number                  - read values from stream where the result array is 𝕨 long
// stream ⇋ 1-rank array of natural numbers - read values from stream where the result array is shape 𝕨
// stream ⇋ ∞                               - wait and read until all values have been read
// array  ⇋ stream                          - write cells from 𝕩 to stream, error it type is invalid (like writing a number larger than 256 to a byte stream)
// symbol ⇋ stream                          - per stream command sent to the stream
// 
// IO, files, network ..?
//
// = async primitives
// 
// == •Queue
// first-in-first-out ==  limit •Queue [ initial_contents ]
// first-in-last-out  == ¯limit •Queue [ initial_contents ]
// 
// 'close' ⇋ queue - deallocates the queue data and any reads will give fill elements and writes do nothing
// 
// Mutex ⇐ {1 •Queue 𝕩}
// _with_mutex_ ⇐ { 𝕘 ⇋ 𝔽 ⇋ 𝕘 }
// _with_rmutex_ ⇐ { x ← 𝕘 ⇋ 
// _with_wmutex_ ⇐ { 𝕘 
// 
// •Show _with_mutex_(0 Mutex)
//
// RWMutex ⇐ {1 •Queue 𝕩}
//
// == •Timer
// 
// duration_ns •Timer [ repeat_ns ]
// accepts no writes
// 
// 'close'  ⇋ timer - any reads will give fill elements
// 'pause'  ⇋ timer - pauses the timer (initial or repeat)
// 'resume' ⇋ timer - resumes the timer from where it left off
//
// ticker ⇋ 2    \\ causes a to create an array of shape 𝕨. in this case 1-rank array of shape 2
//               \\ waits for two ticks
//   •Time •Show // assuming Time ignores 𝕩

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
static inline fern_Box fern_evoke(fern_Box evokable, fern_Evokation evokation, fern_Box x, fern_Box w) {
  switch(fern_tag(evokable)) {
  case fern_Tag_function:
    {
      fern_Function function = fern_unpack_function(evokable);
      switch(function->type) {
      case fern_FunctionType_c:
        return function->c(evokation, x, w);
      case fern_FunctionType_block:
        fern_fatal_error("not implemented");
      case fern_FunctionType_applied_m1:
        fern_fatal_error("not implemented");
      case fern_FunctionType_applied_c_m1:
        return function->applied_c_m1.m(evokation, function->applied_c_m1.f, x, w);
      case fern_FunctionType_applied_m2:
        fern_fatal_error("not implemented");
      case fern_FunctionType_applied_c_m2:
        return function->applied_c_m2.m(evokation, function->applied_c_m2.f, function->applied_c_m2.g, x, w);
      case fern_FunctionType_train2:
        return fern_evoke(
            function->train2.g
          , fern_Evokation_monad
          , fern_evoke(function->train2.h, evokation, x, w)
          , w
          );
      case fern_FunctionType_train3:
        return fern_evoke(
            function->train3.g
          , fern_Evokation_dyad
          , fern_evoke(function->train3.h, evokation, x, w)
          , fern_evoke(function->train3.f, evokation, x, w)
          );
      }
    }
  default:
    return evokable;
  }
}

