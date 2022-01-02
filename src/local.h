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

// ============================================================================================================================================================
// BQN vm
typedef struct {
  uint32_t t;
  uint32_t i;
  uint32_t st;
  uint32_t l;
} fern_BQN_bytecode_block;

typedef struct {
  uint32_t bytecode_size;
  uint8_t * bytecode;

  uint32_t num_objects;
  fern_Box * objects;

  uint32_t num_blocks;
  fern_BQN_bytecode_block * blocks;
} fern_BQN;

fern_Box fern_BQN_execute(fern_BQN * bqn, uint32_t p, fern_Namespace * e);
