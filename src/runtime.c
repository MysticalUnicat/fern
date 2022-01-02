#include "local.h"

static uint32_t _format_bit_size[] = {
    [fern_Format_natural_1_bit]  =                    1
  , [fern_Format_natural_8_bit]  = 8 *  sizeof(uint8_t)
  , [fern_Format_natural_16_bit] = 8 * sizeof(uint16_t)
  , [fern_Format_natural_32_bit] = 8 * sizeof(uint32_t)
  , [fern_Format_character]      = 8 * sizeof(char32_t)
  , [fern_Format_symbol]         = 8 * sizeof(uint32_t)
  , [fern_Format_box]            = 8 * sizeof(fern_Box)
};

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
__attribute__((noreturn)) void fern_fatal_error(const char * format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  exit(EXIT_FAILURE);
}

void fern_assert_fatal_error(bool condition, const char * format, ...) {
  if(!condition) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
  }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// padding ... | pointer | unaligned | aligned 0 | aligned 1 ... aligned n
// ^             ^         ^           ^
// |             |         |           \ beginning of the aligned section
// |             |         |
// |             |         \ returned by the function, to be used by the callee
// |             |
// |             \ the value of the result from malloc is saved here
// |
// \ allocated from malloc
void * memory_allocate(size_t unaligned_size, size_t alignment, size_t aligned_size) {
  if(aligned_size == 0) {
    size_t total_unaligned_size = sizeof(void *) + unaligned_size;
    void * pointer = malloc(total_unaligned_size);
    void * unaligned = (void *)(((uintptr_t)pointer) + sizeof(void *));
    *(void **)pointer = pointer;
    return unaligned;
  } else {
    size_t total_unaligned_size = alignment - 1 + sizeof(void *) + unaligned_size;
    void * padding = malloc(total_unaligned_size + aligned_size);
    void * aligned_0 = (void *)((((uintptr_t)padding) + total_unaligned_size) & ~(alignment - 1));
    void * unaligned = (void *)(((uintptr_t)aligned_0) - unaligned_size);
    void * pointer = (void *)(((uintptr_t)unaligned) - sizeof(size_t));
    *(void **)pointer = padding;
    return unaligned;
  }
}

void memory_free(void * unaligned) {
  void * pointer = (void *)(((uintptr_t)unaligned) - sizeof(size_t));
  free(pointer);
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
void * fern_init_data(fern_Data data, fern_Format format, uint32_t size) {
  void * result = data->inplace.data;
  
  uint32_t bit_size  = _format_bit_size[format] * size;
  uint32_t byte_size = (bit_size + 7) >> 3;
  
  if(byte_size < sizeof(uintptr_t) - 2) {
    data->is_pointer = 1;
    data->pointer.format = format;
    data->pointer.size = size;
    data->pointer.rc = (uintptr_t)malloc(sizeof(uint32_t));
    data->pointer.pointer = (uintptr_t)malloc(byte_size);
    result = (void *)data->pointer.pointer;
  } else {
    data->is_pointer = 0;
    data->inplace.format = format;
    data->inplace.size = size;
  }

  return result;
}

void fern_clone_data(fern_Data data, fern_Data other) {
  memcpy(data, other, sizeof(*other));
  if(data->is_pointer && data->pointer.rc) {
    uint32_t * rc = (uint32_t *)data->pointer.rc;
    (*rc)++;
  }
}

void fern_free_data(fern_Data data) {
  if(data->is_pointer && data->pointer.rc) {
    uint32_t * rc = (uint32_t *)data->pointer.rc;
    fern_assert_fatal_error(*rc != 0, "reference counted data has invalid state");
    if(0 == --(*rc)) {
      free(rc);
      free((void *)data->pointer.pointer);
    }
  }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
fern_Array fern_allocate_array(void) {
  fern_Array array = malloc(sizeof(*array));
  return array;
}

fern_Function fern_allocate_function(void) {
  fern_Function function = malloc(sizeof(*function));
  return function;
}

fern_Modifier1 fern_allocate_modifier1(void) {
  fern_Modifier1 modifier1 = malloc(sizeof(*modifier1));
  return modifier1;
}

fern_Modifier2 fern_allocate_modifier2(void) {
  fern_Modifier2 modifier2 = malloc(sizeof(*modifier2));
  return modifier2;
}

fern_Namespace fern_allocate_namespace(void) {
  fern_Namespace namespace = malloc(sizeof(*namespace));
  return namespace;
}

// TODO
fern_Stream fern_allocate_stream(void) {
  fern_Stream stream = malloc(sizeof(*stream));
  return stream;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
void fern_init_shape(fern_Data data, uint32_t rank, uint32_t * shape) {
  fern_Format shape_format = fern_Format_natural_8_bit;
  for(uint32_t i = 0; i < rank; i++) {
    while(shape[i] > (shape_format << 3)) {
      shape_format++; 
    }
  }
  void * shape_w = fern_init_data(data, shape_format, rank);
  if(shape_format == fern_Format_natural_8_bit) {
    for(uint32_t i = 0; i < rank; i++) {
      ((uint8_t *)shape_w)[i] = shape[i];
    }
  } else if(shape_format == fern_Format_natural_16_bit) {
    for(uint32_t i = 0; i < rank; i++) {
      ((uint16_t *)shape_w)[i] = shape[i];
    }
  } else {
    for(uint32_t i = 0; i < rank; i++) {
      ((uint32_t *)shape_w)[i] = shape[i];
    }
  }
}

void fern_init_array(fern_Array array, fern_Data shape, fern_Data cells, fern_Box fill) {
  fern_clone_data(&array->shape, shape);
  fern_clone_data(&array->cells, cells);
  array->fill = fill;
}

void fern_init_array2(fern_Array array, uint32_t rank, uint32_t * shape, fern_Data cells, fern_Box fill) {
  fern_init_shape(&array->shape, 1, shape);
  fern_clone_data(&array->cells, cells);
  array->fill = fill;
}

void fern_init_array3(fern_Array array, fern_Data cells, fern_Box fill) {
  fern_DataReader cells_reader = fern_read_data(cells);
  fern_init_array2(array, 1, &cells_reader.size, cells, fill);
}

void fern_init_array_singleton(fern_Array array, fern_Box cell, fern_Box fill) {
  uint32_t one = 1;
  fern_init_shape(&array->shape, 1, &one);
  *(fern_Box *)fern_init_data(&array->cells, fern_Format_box, 1) = cell;
  array->fill = fill;
}

// TODO needs to be atomic
typedef struct {
  bool init;
  
  const char * search;
  uint32_t search_size;

  uint32_t   length;
  const char * * string;
  uint32_t   * index; // stored +1 so _compare function can read the key while searching using bsearch
} SymbolStore;
static SymbolStore symbol_store = { false };

static int _compar_symbol_store(const void * a, const void * b) {
  uint32_t ai = *(uint32_t *)a;
  uint32_t bi = *(uint32_t *)b;
  return strncmp(
      ai ? symbol_store.string[(ai - 1)] : symbol_store.search
    , bi ? symbol_store.string[(bi - 1)] : symbol_store.search
    , (ai == 0 || bi == 0) ? symbol_store.search_size : SIZE_MAX
    );
}

static uint32_t _add_symbol(const char * string, uint32_t string_size) {
  symbol_store.string = realloc(symbol_store.string, sizeof(*symbol_store.string) * (symbol_store.length + 1));
  symbol_store.index  = realloc(symbol_store.index,  sizeof(*symbol_store.index)  * (symbol_store.length + 1));

  char * string_copy = malloc(string_size + 1);
  memcpy(string_copy, string, string_size);
  string_copy[string_size] = 0;
  
  symbol_store.string[symbol_store.length] = string_copy;
  symbol_store.index[symbol_store.length] = symbol_store.length + 1;
  qsort(symbol_store.index, symbol_store.length, sizeof(uint32_t), _compar_symbol_store);

  return symbol_store.length++;
}

void fern_init_symbol(uint32_t * symbol, const char * string, uint32_t string_size) {
  if(!symbol_store.init) {
    memset(&symbol_store, 0, sizeof(symbol_store));

    fern_assert_fatal_error(_add_symbol("nil", 3) == 0, "");
    fern_assert_fatal_error(_add_symbol("nothing", 7) == 1, "");

    symbol_store.init = true;
  }
  
  symbol_store.search = string;
  symbol_store.search_size = string_size;
  
  *symbol = 0;
  uint32_t * index_find = bsearch(symbol, symbol_store.index, symbol_store.length, sizeof(uint32_t), _compar_symbol_store);
  if(index_find != NULL) {
    *symbol = *index_find - 1;
    return;
  }

  *symbol = _add_symbol(string, string_size);
}

void fern_init_function_c(
    fern_Function function
  , fern_FunctionEvokation evokation
) {
  function->type = fern_FunctionType_c;
  function->c = evokation;
}

void fern_init_modifier1_c(
    fern_Modifier1 modifier1
  , fern_Modifier1Evokation evokation
) {
  modifier1->type = fern_Modifier1Type_c;
  modifier1->c = evokation;
}

void fern_init_modifier2_c(
    fern_Modifier2 modifier2
  , fern_Modifier2Evokation evokation
) {
  modifier2->type = fern_Modifier2Type_c;
  modifier2->c = evokation;
}

void fern_init_namespace(fern_Namespace namespace, fern_Namespace parent) {
  memset(namespace, 0, sizeof(*namespace));
  namespace->parent = parent;
}

// TODO
void fern_init_stream(fern_Stream stream) {
  memset(stream, 0, sizeof(*stream));
}

void fern_init_number(double * number, const char32_t * string, uint32_t string_size) {
  *number = NAN;

  double negate = *string == u'¯' ? -1 : 1;
  
  if(*string == u'¯') {
    ++string;
  }

  bool inf = *string == u'∞';
  if(inf) {
    *number = INFINITY * negate;
    string++;
  } else {
    double base = 0;
    if(*string == u'π') {
      base = ((fern_Box){.bits = 0x400921fb54442d18ull}).number;
      string++;
    } else if(*string == u'τ') {
      base = ((fern_Box){.bits = 0x401921fb54442d18ull}).number;
      string++;
    } else {
      // read mantissa
      while(*string >= u'0' && *string <= u'9') {
        base *= 10;
        base += *string - u'0';
        string++;
      }
      if(*string == u'.') {
        string++;
        uint32_t digits = 1;
        while(*string >= u'0' && *string <= u'9') {
          base += *string - u'0' * (0.1 / digits);
          digits *= 10;
          string++;
        }
      }
    }
    if(*string == u'E') {
      string++;
      double negate_exponent = *string == u'¯' ? -1 : 1;
      if(negate_exponent < 0) {
        string++;
      }
      double exponent = 0;
      while(*string >= u'0' && *string <= u'9') {
        exponent *= 10;
        exponent += *string - u'0';
        string++;
      }
      *number = pow(base, exponent * negate_exponent) * negate;
    } else {
      *number = base * negate;
    }
  }
  if(*string == u'i' || *string == u'e') {
    fern_fatal_error("complex numbers not implemented");
  }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// TODO
fern_Box fern_clone(fern_Box b) {
  return b;
}

// TODO
void fern_free(fern_Box b) {
}

const char * fern_symbol_string(uint32_t symbol) {
  return symbol_store.string[symbol];
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
static int _compar_fern_Namespace_pair(const void * a, const void * b) {
  return ((struct fern_Namespace_pair *)a)->symbol - ((struct fern_Namespace_pair *)b)->symbol;
}

fern_Box fern_namespace_get(fern_Namespace ns, uint32_t symbol) {
  struct fern_Namespace_pair key;
  key.symbol = symbol;
  key.value.number = 0;
  while(ns != NULL) {
    struct fern_Namespace_pair * find = bsearch(&key, ns->data, ns->length, sizeof(*ns->data), _compar_fern_Namespace_pair);
    if(find != NULL) {
      return fern_clone(find->value);
    }
    ns = ns->parent;
  }
  fern_fatal_error("%s not set", fern_symbol_string(symbol));
}

void fern_namespace_define(fern_Namespace ns, uint32_t symbol, fern_Box value) {
  struct fern_Namespace_pair key;
  key.symbol = symbol;
  key.value = value;
  struct fern_Namespace_pair * find = bsearch(&key, ns->data, ns->length, sizeof(*ns->data), _compar_fern_Namespace_pair);
  fern_assert_fatal_error(find == NULL, "cannot define already defined value");
  ns->data = realloc(ns->data, sizeof(*ns->data) * (ns->length + 1));
  ns->data[ns->length] = key;
  ++ns->length;
  qsort(ns->data, ns->length, sizeof(*ns->data), _compar_fern_Namespace_pair);
}

void fern_namespace_redefine(fern_Namespace ns, uint32_t symbol, fern_Box value) {
  struct fern_Namespace_pair key;
  key.symbol = symbol;
  key.value = value;
  struct fern_Namespace_pair * find = bsearch(&key, ns->data, ns->length, sizeof(*ns->data), _compar_fern_Namespace_pair);
  fern_assert_fatal_error(find != NULL, "cannot redefine what does not exist");
  fern_free(find->value);
  find->value = value;
}

// •Type ------------------------------------------------------------------------------------------------------------------------------------------------------
fern_Box fern_SYSTEM_Type_evokation0(fern_Evokation evokation, fern_Box x, fern_Box w) {
  double _tag_to_type[] = {
      [fern_Tag_character] = 2
    , [fern_Tag_symbol]    = 7
    , [fern_Tag_array]     = 0
    , [fern_Tag_function]  = 3
    , [fern_Tag_modifier1] = 4
    , [fern_Tag_modifier2] = 5
    , [fern_Tag_namespace] = 6
    , [fern_Tag_stream]    = 8
    , [fern_Tag_number]    = 1
  };
  switch(evokation) {
  case fern_Evokation_monad:
  case fern_Evokation_dyad:
    return fern_pack_number(_tag_to_type[fern_tag(x)]);
  case fern_Evokation_write_to_backend:
    fern_fatal_error("not implemented");
  case fern_Evokation_inverse:
    fern_fatal_error("not implemented");
  }
  return fern_DIGIT_ZERO();
}
struct fern_Function fern_SYSTEM_Type_fn = { .type = fern_FunctionType_c, .c = fern_SYSTEM_Type_evokation0 };
#define fern_SYSTEM_Type fern_pack_function(&SYSTEM_Type_fn)

