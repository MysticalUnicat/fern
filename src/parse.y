%include {

#include <stdint.h>
#include <stdlib.h>
#include <uchar.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <inttypes.h>
#include <stdarg.h>
#include <locale.h>

#include <fern.h>

const char whitespace[] = " \t\n\r";
const char digits[] = "0123456789";
const char lower[] = "abcdefghijklmnopqrstuvwxyz";
const char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char identifier[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const char symbol[] = "abcdefghijklmnopqrstuvwxyz0123456789_.";

char * strdup(const char * s);

typedef char utf8_t;

#define VECTOR(T) struct { uintptr_t length; uintptr_t capacity; T * data; }
#define VECTOR_space_for(V, N) do { \
  if((V)->length + (N) > (V)->capacity) { \
    (V)->capacity = (V)->length + (N) + 1; \
    (V)->capacity += (V)->capacity >> 1; \
    (V)->data = realloc((V)->data, sizeof(*(V)->data) * (V)->capacity); \
  } \
} while(0)
#define VECTOR_push(V) &(V)->data[(V)->length++]

struct Token {
  uint32_t type;
  uint32_t length;
  uint64_t position;
};

uint32_t Token_lexeme_char(struct Token token, char * lexeme, uint32_t max_size);
uint32_t Token_lexeme_char32(struct Token token, char32_t * lexeme, uint32_t max_size);

}

%token WHITESPACE NEWLINE.
%token_type { struct Token }
%default_type { fern_Box }

program ::= statement.

statement ::= expression.

expression ::= dyadic_expression.

dyadic_expression ::= monadic_expression.
dyadic_expression ::= monadic_expression value.
dyadic_expression ::= monadic_expression LEFT_PARENTHESIS expression RIGHT_PARENTHESIS.
dyadic_expression ::= monadic_expression CONTINUATION expression.

monadic_expression ::= value.
monadic_expression ::= monadic_expression evocable.

evocable ::= function.
evocable ::= mod_left_argument modifier1.
evocable ::= mod_left_argument modifier2 mod_right_argument.

mod_left_argument ::= value.
mod_left_argument ::= evocable.
mod_left_argument ::= LEFT_PARENTHESIS train_expression RIGHT_PARENTHESIS.
mod_left_argument ::= LEFT_PARENTHESIS expression RIGHT_PARENTHSIS.

mod_right_argument ::= value.
mod_right_argument ::= function.
mod_right_argument ::= LEFT_PARENTHESIS train_expression RIGHT_PARENTHESIS.
mod_right_argument ::= LEFT_PARENTHESIS expression RIGHT_PARENTHESIS.

train_expression ::= evocable evocable evocable.

function ::= FUNCTION_IDENTIFIER.
function ::= FUNCTION_LITERAL.
function ::= PERIOD VALUE_IDENTIFIER.
// a function with this behaviour:
// 'atom .id'     -> read
// 'atom .id any' -> write

modifier1 ::= MODIFIER1_IDENTIFIER.
modifier1 ::= MODIFIER1_LITERAL.

modifier2 ::= MODIFIER2_IDENTIFIER.
modifier2 ::= MODIFIER2_LITERAL.

value ::= VALUE_IDENTIFIER.
value ::= VALUE_LITERAL.
value(A) ::= CHARACTER(B). {
  char32_t lexeme[4];
  Token_lexeme_char32(B, lexeme, 4);
  A = fern_pack_character(lexeme[1]); // ignore starting '
}
value(A) ::= NUMBER(B). {
  char32_t lexeme[128];
  double number;
  uint32_t lexeme_size = Token_lexeme_char32(B, lexeme, 128);
  fern_init_number(&number, lexeme + 1, lexeme_size);
  A = fern_pack_number(number);
}
value(A) ::= SYMBOL(B). {
  char lexeme[128];
  uint32_t symbol;
  uint32_t lexeme_size = Token_lexeme_char(B, lexeme, 128);
  fern_init_symbol(&symbol, lexeme + 1, lexeme_size - 2); // ignore starting ' and ending '
  A = fern_pack_symbol(symbol);
}
value ::= STRING.

%code {

uint64_t files_position_offset = 1; // 0 is invalid

struct PositionInformation {
  struct File * file;
  uintptr_t line;
  uintptr_t column;
  uintptr_t offset;
};

struct File {
  struct File * previous;

  uint64_t position_start;
  uint64_t position_end;

  const utf8_t * filename;
  const utf8_t * source;
  VECTOR(uint64_t) lines;
} * files = NULL;

struct File * File_from_buffer(const utf8_t * filename, const utf8_t * buffer, uintptr_t buffer_length) {
  struct File * file = malloc(sizeof(*file));
  file->previous = files;
  file->position_start = files_position_offset;
  file->position_end = files_position_offset + buffer_length;
  file->filename = strdup(filename);
  file->source = buffer;

  files_position_offset += buffer_length + 1;
  files = file;

  return file;
}

struct File * File_from_string(const utf8_t * filename, const utf8_t * string) {
  return File_from_buffer(filename, string, strlen((const char *)string));
}

struct File * File_open(const utf8_t * filename) {
  struct File * file = files;

  while(file != NULL) {
    if(strcmp(file->filename, filename) == 0) {
      return file;
    }
    file = file->previous;
  }

  FILE * f = fopen(filename, "r");
  if(f == NULL) {
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long int length = ftell(f);
  fseek(f, 0, SEEK_SET);

  utf8_t * buffer = malloc(length + 1);

  uint8_t * read_ptr = (uint8_t *)buffer;
  uintptr_t remaining = length;
  while(remaining) {
    size_t read_count = fread(read_ptr, remaining, 1, f);
    remaining -= read_count;
    read_ptr += read_count;
  }
  
  buffer[length] = 0;
  return File_from_buffer(filename, buffer, length);
}

int _compar_uint64_t(const void * ap, const void * bp) {
  return *(uint64_t *)ap - *(uint64_t *)bp;
}

void File_insert_line(struct File * file, uint64_t position) {
  void * line = bsearch(&position, file->lines.data, file->lines.length, sizeof(*file->lines.data), _compar_uint64_t);
  if(line == NULL) {
    VECTOR_space_for(&file->lines, 1);
    *VECTOR_push(&file->lines) = position;
    qsort(file->lines.data, file->lines.length, sizeof(*file->lines.data), _compar_uint64_t);
  }
}

struct PositionInformation File_get_position_information(struct File * file, uint64_t position) {
  uintptr_t line = 0;
  for(line = 0; line < file->lines.length - 1; line++) {
    if(file->lines.data[line] > position) {
      line = line ? line - 1 : 0;
      break;
    }
  }
  struct PositionInformation pos;
  pos.file = file;
  pos.line = line + 1;
  pos.column = position - file->lines.data[line] + 1;
  pos.offset = position - file->position_start;
  return pos;
}

struct PositionInformation Files_get_position_information(uint64_t position) {
  struct PositionInformation pos = { 0 };
  struct File * file = files;
  while(file) {
    if(file->position_start >= position && file->position_end <= position) {
      return File_get_position_information(file, position);
    }
    file = file->previous;
  }
  return pos;
}

void File_print_prefix_space(struct File * file) {
  int line_print_width = (int)ceil(log10(file->lines.length)) + 2;
  printf("%*s  ", line_print_width, " ");
}

void File_print_prefix(struct File * file, uintptr_t line, bool print_line) {
  int line_print_width = (int)ceil(log10(file->lines.length)) + 2;
  if(print_line) {
    printf("%*" PRIuPTR " | ", line_print_width, line);
  } else {
    printf("%*s | ", line_print_width, " ");
  }
}

void File_print_line(struct File * file, uintptr_t line, bool print_line) {
  if(line > file->lines.length) {
    return;
  }

  uint64_t start = file->lines.data[line - 1];
  uint64_t end = line == file->lines.length - 1 ? file->position_end : file->lines.data[line + 1 - 1];

  File_print_prefix(file, line, print_line);

  printf(
      "%.*s\n"
    , (int)(end - start)
    , file->source + (start - file->position_start)
    );
}

struct Character {
  uint64_t position;
  char32_t character;
};

enum LexerPrimitiveType {
    LexerPrimitiveType_Value
  , LexerPrimitiveType_Function
  , LexerPrimitiveType_Modifier1
  , LexerPrimitiveType_Modifier2
};

struct LexerPrimitive {
  char32_t name;
  enum LexerPrimitiveType type;
};

struct LexerConfig {
  VECTOR(struct LexerPrimitive) primitives;
};

void LexerConfig_initialize(struct LexerConfig * lexer_config) {
  memset(lexer_config, 0, sizeof(*lexer_config));
}

void LexerConfig_set_primitive(struct LexerConfig * lexer_config, char32_t name, enum LexerPrimitiveType type) {
  for(uint32_t i = 0; i < lexer_config->primitives.length; i++) {
    if(lexer_config->primitives.data[i].name == name) {
      // error
    }
  }

  VECTOR_space_for(&lexer_config->primitives, 1);
  *VECTOR_push(&lexer_config->primitives) = (struct LexerPrimitive) { .name = name, .type = type };
}

void LexerConfig_shutdown(struct LexerConfig * lexer_config) {
  (void)lexer_config;
}

struct Lexer {
  struct LexerConfig * config;
  struct File * file;

  mbstate_t mb;

  struct Character c1;
  struct Character c2;
  uint64_t c3_position;
};

void _Lexer_update_c_0(struct Lexer * lexer) {
  lexer->c1 = lexer->c2;
  lexer->c2.position = lexer->c3_position;

  if(lexer->c3_position >= lexer->file->position_end) {
    lexer->c2.character = 0;
    return;
  }
  
  size_t len = mbrtoc32(
      &lexer->c2.character
    , lexer->file->source + lexer->c2.position - lexer->file->position_start
    , lexer->file->position_end - lexer->c2.position
    , &lexer->mb
    );

  lexer->c3_position += len;
}

void _Lexer_update_c(struct Lexer * lexer) {
  _Lexer_update_c_0(lexer);
  if(lexer->c1.character == '\r' && lexer->c2.character == '\n') {
    uint64_t position = lexer->c1.position;
    _Lexer_update_c_0(lexer);
    lexer->c1.position = position;
  }
  if(lexer->c1.character == '\n') {
    File_insert_line(lexer->file, lexer->c2.position);
  }
}

void Lexer_initialize(struct Lexer * lexer, struct LexerConfig * config, struct File * file) {
  memset(lexer, 0, sizeof(*lexer));

  lexer->config = config;
  lexer->file = file;
  lexer->c3_position = file->position_start;

  File_insert_line(lexer->file, lexer->c3_position);
  _Lexer_update_c(lexer);
  _Lexer_update_c(lexer);
}

void _Lexer_error(struct Lexer * lexer, uint64_t start, uint64_t end, const utf8_t * format, ...) {
  struct PositionInformation start_info = File_get_position_information(lexer->file, start);
  struct PositionInformation end_info = File_get_position_information(lexer->file, end);
  printf("%s %" PRIuPTR " : ", lexer->file->filename, start_info.line);
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
  puts("");
  switch(end_info.line - start_info.line) {
  case 1:
    File_print_line(lexer->file, start_info.line, true);
    File_print_line(lexer->file, end_info.line, false);
    break;
  case 0:
    File_print_line(lexer->file, end_info.line, true);

    File_print_prefix_space(lexer->file);
    printf("%*s", (int)start_info.column, " ");
    printf("^");
    for(uint32_t i = 0; i < end_info.column - start_info.column; i++) {
      printf("~");
    }
    printf("\n");

    break;
  default:
    File_print_line(lexer->file, start_info.line, true);
    File_print_prefix(lexer->file, start_info.line, false);
    printf(" ...\n");
    File_print_line(lexer->file, end_info.line, false);
    break;
  }

  exit(EXIT_FAILURE);
}

bool Lexer_next(struct Lexer * lexer, int * token_id, struct Token * token) {
  bool newline = false;

  token->position = lexer->c1.position;

whitespace:
  while(lexer->c1.character && strchr(whitespace, lexer->c1.character) != NULL) {
    newline = newline || lexer->c1.character == '\n';
    _Lexer_update_c(lexer);
  }
  if(!lexer->c1.character) {
    return false;
  }
  if(lexer->c1.character == '#') {
    newline = true;
    while(lexer->c1.character && lexer->c1.character != '\n') {
      _Lexer_update_c(lexer);
    }
    goto whitespace;
  }
  if(lexer->c1.position > token->position) {
    *token_id = newline ? NEWLINE : WHITESPACE;
    token->length = lexer->c1.position - token->position;
    return true;
  }

  switch(lexer->c1.character) {
  case U'\\':
    _Lexer_update_c(lexer);

    *token_id = CONTINUATION;

    if(lexer->c1.character == U'\\') {
      _Lexer_update_c(lexer);

      while(lexer->c1.character && lexer->c1.character != U'\n') {
        _Lexer_update_c(lexer);
      }
      _Lexer_update_c(lexer);
    }
    goto done;
  case U'¯':
  case U'0'...U'9':
    goto number;
  case U'"':
    _Lexer_update_c(lexer);

    *token_id = STRING;

    while(lexer->c1.character != U'"') {
      if(lexer->c1.character == 0) {
        _Lexer_error(lexer, token->position, lexer->c1.position, "unterminated string");
        return false;
      }
      if(lexer->c1.character == U'\\') {
        _Lexer_update_c(lexer);
      }
      _Lexer_update_c(lexer);
    }

    _Lexer_update_c(lexer);

    token->length = lexer->c1.position - token->position;
    goto done;
  case U'_':
    *token_id = MODIFIER1_IDENTIFIER;
    goto identifier;
  case U'a'...U'z':
    *token_id = VALUE_IDENTIFIER;
    goto identifier;
  case U'A'...U'Z':
    *token_id = FUNCTION_IDENTIFIER;
    goto identifier;
  case U'(':
    _Lexer_update_c(lexer);
    *token_id = LEFT_PARENTHESIS;
    goto done;
  case U')':
    _Lexer_update_c(lexer);
    *token_id = RIGHT_PARENTHESIS;
    goto done;
  case U'\'':
    _Lexer_update_c(lexer);
    *token_id = lexer->c2.character == U'\'' ? CHARACTER : SYMBOL;
    while(lexer->c1.character != U'\'') {
      if(*token_id == SYMBOL && strchr(symbol, lexer->c1.character) != NULL) {
        _Lexer_error(lexer, token->position, lexer->c1.position, "invalid symbol character");
        return false;
      }
      _Lexer_update_c(lexer);
    }
    _Lexer_update_c(lexer);
    goto done;
  case U'.':
    _Lexer_update_c(lexer);
    *token_id = PERIOD;
    goto done;
  default:
    for(uint32_t i = 0; i < lexer->config->primitives.length; i++) {
      if(lexer->config->primitives.data[i].name == lexer->c1.character) {
        *token_id = lexer->config->primitives.data[i].type == LexerPrimitiveType_Value     ? VALUE_LITERAL     :
                    lexer->config->primitives.data[i].type == LexerPrimitiveType_Function  ? FUNCTION_LITERAL  :
                    lexer->config->primitives.data[i].type == LexerPrimitiveType_Modifier1 ? MODIFIER1_LITERAL :
                                                                                             MODIFIER2_LITERAL ;
        _Lexer_update_c(lexer);
        goto done;
      }
    }

    _Lexer_error(lexer, lexer->c1.position, lexer->c1.position, "invalid character '%c'", lexer->c1.character);
    return false;
  }

identifier:
  while(lexer->c1.character && strchr(identifier, lexer->c1.character) != NULL) {
    _Lexer_update_c(lexer);
  }
  
  goto done;

// number           = '¯'? ('∞' | number_mantissa ('E' number_exponent)?) ('i' | 'e' digit+)?
// number_mantissa  = 'π' | 'τ' | digit+ ('.' digit+)?
// number_exponent  = '¯'? digit+

number:
  *token_id = NUMBER;

  if(lexer->c1.character == U'¯') {
    _Lexer_update_c(lexer);
  }
  if(lexer->c1.character == U'∞') {
    _Lexer_update_c(lexer);
  } else {
    if(lexer->c1.character == U'π' || lexer->c1.character == U'τ') {
      _Lexer_update_c(lexer);
    } else if(lexer->c1.character && strchr(digits, lexer->c1.character) != NULL) {
      while(lexer->c1.character && strchr(digits, lexer->c1.character) != NULL) {
        _Lexer_update_c(lexer);
      }
      if(lexer->c1.character == U'.') {
        _Lexer_update_c(lexer);
        if(lexer->c1.character && strchr(digits, lexer->c1.character) != NULL) {
          while(lexer->c1.character && strchr(digits, lexer->c1.character) != NULL) {
            _Lexer_update_c(lexer);
          }
        } else {
          _Lexer_error(lexer, lexer->c1.position - 1, lexer->c1.position, "expecting digits following '.' in a number");
          return false;
        }
      }
    } else {
      _Lexer_error(lexer, lexer->c1.position - 1, lexer->c1.position - 1, "expecting a mantissa value");
      return false;
    }

    if(lexer->c1.character == U'E') {
      if(lexer->c1.character == U'¯') {
        _Lexer_update_c(lexer);
      }

      if(lexer->c1.character && strchr(digits, lexer->c1.character) != NULL) {
        while(lexer->c1.character && strchr(digits, lexer->c1.character) != NULL) {
          _Lexer_update_c(lexer);
        }
      } else {
        _Lexer_error(lexer, lexer->c1.position, lexer->c1.position, "expecting a exponent value");
        return false;
      }     
    }
  }

  if(lexer->c1.character == U'i') {
    _Lexer_update_c(lexer);
  } else if(lexer->c1.character == U'e') {
    _Lexer_update_c(lexer);
    if(lexer->c1.character && strchr(digits, lexer->c1.character) != NULL) {
      while(lexer->c1.character && strchr(digits, lexer->c1.character) != NULL) {
        _Lexer_update_c(lexer);
      }
    } else {
      _Lexer_error(lexer, lexer->c1.position, lexer->c1.position, "expecting component digits");
    }
  }

done:
  token->length = lexer->c1.position - token->position;

  if(*token_id == MODIFIER1_IDENTIFIER && lexer->file->source[token->position - lexer->file->position_start + token->length - 1] == '_') {
    *token_id = MODIFIER2_IDENTIFIER;
  }

  return true;
}

void Lexer_shutdown(struct Lexer * lexer) {
  (void)lexer;
}

void * parse(struct Lexer * lexer) {
  void * parser = ParseAlloc(malloc);

  int token_id;
  struct Token token;

  while(Lexer_next(lexer, &token_id, &token)) {
    Parse(parser, token_id, token);
  }

  ParseFree(parser, free);

  return NULL;
}

const char test[] = "2+2";

int main(int argc, char * argv []) {
  setlocale(LC_ALL, "en_US.utf8");
  
  struct LexerConfig lexer_config;
  struct Lexer lexer;

  LexerConfig_initialize(&lexer_config);
  LexerConfig_set_primitive(&lexer_config, U'+', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'-', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'×', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'⌊', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'=', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'≤', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'≢', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'⥊', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'⊑', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'↕', LexerPrimitiveType_Function);
  LexerConfig_set_primitive(&lexer_config, U'⌜', LexerPrimitiveType_Modifier1);
  LexerConfig_set_primitive(&lexer_config, U'⊘', LexerPrimitiveType_Modifier2);
  
  Lexer_initialize(&lexer, &lexer_config, File_from_string("<test>", argc > 1 ? argv[1] : test));

  void * ast = parse(&lexer);
  //int token_id;
  //struct Token token;
  //while(Lexer_next(&lexer, &token_id, &token)) {
  //  printf("% 3i '%.*s'\n", token_id, (int)token.length, lexer.file->source + token.position - lexer.file->position_start);
  //}

  Lexer_shutdown(&lexer);
  LexerConfig_shutdown(&lexer_config);

  return EXIT_SUCCESS;
}

}
