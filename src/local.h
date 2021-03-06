#include <fern.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define CALL_1(F, X) fern_evoke(F, fern_Evokation_monad, X, fern_nil())
#define CALL_2(F, X, W) fern_evoke(F, fern_Evokation_dyad, X, W)

// ============================================================================================================================================================

// constant primitives
static inline fern_Box fern_nil(void) { return            fern_pack_symbol(0); }      // 'nil'
static inline fern_Box fern_nothing(void) { return        fern_pack_symbol(1); }      // 'nothing' | ·
static inline fern_Box fern_DIGIT_ZERO(void) { return     fern_pack_number(0); }      // 0
static inline fern_Box fern_DIGIT_ONE(void) { return      fern_pack_number(1); }      // 1
static inline fern_Box fern_COMMERCIAL_AT(void) { return  fern_pack_character(0); }   // @
static inline fern_Box fern_SPACE(void) { return          fern_pack_character(' '); } // ' '
static inline fern_Box fern_EMPTY_ARRAY(void) { return    fern_pack_array(0); }       // ⟨⟩

// other functions and a modifier-2 provided to allow building the BQN runtime from scratch
fern_Box fern_provide_Fill(void);                                                     // Fill
fern_Box fern_provide_Log(void);                                                      // Log
fern_Box fern_provide_GroupLen(void);                                                 // GroupLen
fern_Box fern_provide_GroupOrd(void);                                                 // GroupOrd
fern_Box fern_provide__fill_by_(void);                                                // _fill_by_

// function primitives
fern_Box fern_PLUS_SIGN(void);                                                        // +
fern_Box fern_HYPHEN_MINUS(void);                                                     // -
fern_Box fern_MULTIPLICATION_SIGN(void);                                              // ×
fern_Box fern_DIVISION_SIGN(void);                                                    // ÷
fern_Box fern_LEFT_FLOOR(void);                                                       // ⌊
fern_Box fern_VERTICAL_LINE(void);                                                    // |
fern_Box fern_LESS_THAN_OR_EQUAL_TO(void);                                            // ≤
fern_Box fern_LESS_THAN_SIGN(void);                                                   // <
fern_Box fern_GREATER_THAN_SIGN(void);                                                // >
fern_Box fern_GREATER_THAN_OR_EQUAL_TO(void);                                         // ≥
fern_Box fern_EQUAL_SIGN(void);                                                       // =
fern_Box fern_NOT_EQUAL_SIGN(void);                                                   // ≠
fern_Box fern_NOT_IDENTICAL_TO(void);                                                 // ≢
fern_Box fern_LEFT_TACK(void);                                                        // ⊣
fern_Box fern_RIGHT_TACK(void);                                                       // ⊢
fern_Box fern_LEFT_BARB_UP_RIGHT_BARB_DOWN_HARPOON(void);                             // ⥊
fern_Box fern_UP_DOWN_ARROW(void);                                                    // ↕
fern_Box fern_SQUARE_IMAGE_OF_OR_EQUAL_TO(void);                                      // ⊑
fern_Box fern_EXCLAMATION_MARK(void);                                                 // !

// modifier-1 primitives
fern_Box fern_DOT_ABOVE(void);                                                        // ˙ constant
fern_Box fern_SMALL_TILDE(void);                                                      // ˜ swap
fern_Box fern_DIAERESIS(void);                                                        // ¨ each
fern_Box fern_TOP_LEFT_CORNER(void);                                                  // ⌜ table
fern_Box fern_GRAVE_ACCENT(void);                                                     // ` scan

// modifier-2 primitives
fern_Box fern_RING_OPERATOR(void);                                                    // ∘ atop
fern_Box fern_WHITE_CIRCLE(void);                                                     // ○ over
fern_Box fern_MULTIMAP(void);                                                         // ⊸ before
fern_Box fern_LEFT_MULTIMAP(void);                                                    // ⟜ after
fern_Box fern_CIRCLED_DIVISION_SLASH(void);                                           // ⊘ valences
fern_Box fern_WHITE_CIRCLE_WITH_LOWER_RIGHT_QUADRANT(void);                           // ◶ choose
fern_Box fern_CIRCLED_TRIANGLE_DOWN(void);                                            // ⎊ catch

// ============================================================================================================================================================
// internal functionallity for the primitives

// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// catch / throw - with 'terrible setjmp/longjmp' (try ucontext later)
#include <setjmp.h>

typedef struct fern_ExStack {
  struct fern_ExStack * previous;
  fern_Box            message;
  jmp_buf             buf;
} fern_ExStack;

bool fern_ExStack_begin(fern_ExStack * exstack);
void fern_ExStack_end(fern_ExStack * exstack);

void fern_internal_throw(fern_Box message);

fern_Box fern_internal_tofill(fern_Box x);

bool fern_internal_match_shape(fern_Array x, fern_Array w);
bool fern_internal_match_full(fern_Box x, fern_Box w);
static inline bool fern_internal_match(fern_Box x, fern_Box w) {
  if(x.bits == w.bits) {
    return true;
  }
  if(fern_tag(x) != fern_tag(w)) {
    return false;
  }
  return fern_internal_match_full(x, w);
}

// ============================================================================================================================================================
// BQN vm
typedef enum {
    fern_BQN_ExecutionResult_success
  , fern_BQN_ExecutionResult_return
  , fern_BQN_ExecutionResult_error
} fern_BQN_ExecutionResult;

typedef struct {
  uint32_t type;
  uint32_t imm;
  uint32_t body;
} fern_BQN_bytecode_block;
typedef struct {
  uint32_t bytecode_size;
  uint8_t * bytecode;

  uint32_t num_consts;
  fern_Box * consts;

  uint32_t num_blocks;
  fern_BQN_bytecode_block * blocks;
} fern_BQN;

fern_BQN_ExecutionResult fern_BQN_execute(fern_BQN * bqn, uint32_t p, fern_Namespace e, fern_Box * result);

typedef uint32_t fern_BQN_File_Bytecode;
typedef struct {
  enum {
      fern_BQN_File_Constant_runtime
    , fern_BQN_File_Constant_runtime_0
    , fern_BQN_File_Constant_provide
    , fern_BQN_File_Constant_character
    , fern_BQN_File_Constant_string
    , fern_BQN_File_Constant_number
  } type;
  uint32_t index;
  union {
    uint32_t runtime;
    uint32_t runtime_0;
    uint32_t provide;
    uint32_t character;
    const char * string;
    double number;
  };
} fern_BQN_File_Constant;

typedef struct {
  uint32_t type;
  uint32_t imm;
  uint32_t * bodies;
  uint32_t num_bodies;
} fern_BQN_File_Block;

typedef struct {
  uint32_t start;
  uint32_t vars;
} fern_BQN_File_Body;

typedef struct {
  const fern_BQN_File_Bytecode * bytecode;
  uint32_t bytecode_size;
  
  const fern_BQN_File_Constant * constants;
  uint32_t constants_size;

  const fern_BQN_File_Block * blocks;
  uint32_t blocks_size;

  const fern_BQN_File_Body * bodies;
  uint32_t bodies_size;
} fern_BQN_File;

