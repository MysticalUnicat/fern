#include "local.h"

struct Program {
  uint32_t num_consts;
  fern_Box * consts;

  uint32_t num_blocks;
  uint32_t * blocks;

  uint32_t num_names;
  uint32_t * names;
};

enum ObjectType {
    ObjectType_var_unset
  , ObjectType_var_set
  , ObjectType_var_cleared
  , ObjectType_vnot
  , ObjectType_env
  , ObjectType_ns
  , ObjectType_matcher
  , ObjectType_array
  , ObjectType_alias
};

uint32_t has_get_f = (1 << ObjectType_var_unset)
                   | (1 << ObjectType_var_set)
                   | (1 << ObjectType_var_cleared)
                   | (1 << ObjectType_alias) ;

uint32_t is_allocated = (1 << ObjectType_env)
                      | (1 << ObjectType_ns)
                      | (1 << ObjectType_matcher)
                      | (1 << ObjectType_array)
                      | (1 << ObjectType_alias) ;

static void * m_allocate(uint32_t size) {
  uint32_t * rc = malloc(size + sizeof(uint32_t));
  *rc = 1;
  return (rc + 1);
}

static void m_free(void * ptr) {
  uint32_t * rc = (uint32_t *)ptr - 1;
  fern_assert_fatal_error(*rc == 0, "invalid state");
  free(rc);
}

static void m_inc(void * ptr) {
  uint32_t * rc = (uint32_t *)ptr - 1;
  (*rc)++;
}

static bool m_dec(void * ptr) {
  uint32_t * rc = (uint32_t *)ptr - 1;
  if(*rc <= 1) {
    *rc = 0;
    return true;
  } else {
    (*rc)--;
    return false;
  }
}

struct NS;
union Object;

// Var ----------------------------------------------------------------------------------------------------------------
struct Var {
  enum ObjectType type;
  struct Program * program;
  uint32_t name;
  fern_Box value;
};

static void Var_init(struct Var * var, struct Program * program, uint32_t name);
static void Var_tini(struct Var * var);
static fern_Box Var_get(struct Var * var, fern_Box x);
static fern_Box Var_set_n(struct Var * var, fern_Box x);
static fern_Box Var_set_u(struct Var * var, fern_Box x);
static fern_Box Var_set_q(struct Var * var, fern_Box x);
static fern_Box Var_get_c(struct Var * var, fern_Box x);
static struct Var * Var_get_f(struct Var * var, struct NS * x);

// VNot ---------------------------------------------------------------------------------------------------------------
struct VNot {
  enum ObjectType type;
};

static struct VNot vnot = { .type = ObjectType_vnot };

// Env ----------------------------------------------------------------------------------------------------------------
struct Env {
  enum ObjectType type;
  struct Env * parent;
  struct Program * program;
  uint32_t num_vars;
  uint32_t first_named_var;
  struct Var vars[1];
};

struct Env * Env_allocate(uint32_t num_vars);
void Env_init(struct Env * env, struct Env * p, uint32_t v, uint32_t * n, uint32_t num_n);
void Env_tini(struct Env * env);

// NS -----------------------------------------------------------------------------------------------------------------
struct NS {
  enum ObjectType type;
  struct Env * env;
};

static struct NS * NS_allocate(void);
static void NS_init(struct NS * ns, struct Env * env);
static void NS_tini(struct NS * ns);
static struct Var * NS_field(struct NS * ns, uint32_t x, struct Program * w);
static fern_Box NS_read(struct NS * ns, struct Env * e, uint32_t i);

// Matcher ------------------------------------------------------------------------------------------------------------
struct Matcher {
  enum ObjectType type;
  fern_Box value;
};

static struct Matcher * Matcher_allocate(void);
static void Matcher_init(struct Matcher * matcher, fern_Box x);
static void Matcher_tini(struct Matcher * matcher);
static fern_Box Matcher_set_q(struct Matcher * matcher, fern_Box x);

// Array --------------------------------------------------------------------------------------------------------------
struct Array {
  enum ObjectType type;
  uint32_t length;
  union Object * objects[1];
};

static struct Array * Array_allocate(uint32_t length);
static void Array_init(struct Array * array, uint32_t length, fern_Box * stack);
static void Array_tini(struct Array * array);
static fern_Box Array_get(struct Array * array, fern_Box x);
static fern_Box Array_set_n(struct Array * array, fern_Box x);
static fern_Box Array_set_u(struct Array * array, fern_Box x);
static fern_Box Array_set_q(struct Array * array, fern_Box x);

// Alias --------------------------------------------------------------------------------------------------------------
struct Alias {
  enum ObjectType type;
  struct Env * env;
  uint32_t name;
  union Object * r;
};

static struct Alias * Alias_allocate(void);
static void Alias_init(struct Alias * alias, struct Env * env, uint32_t name, union Object * r);
static void Alias_tini(struct Alias * alias);
static fern_Box Alias_set_n(struct Alias * alias, fern_Box x);
static fern_Box Alias_set_u(struct Alias * alias, fern_Box x);
static fern_Box Alias_set_q(struct Alias * alias, fern_Box x);
static struct Var * Alias_get_f(struct Alias * alias, struct NS * x);

// Object -------------------------------------------------------------------------------------------------------------
typedef union Object {
  enum ObjectType type;
  struct Var var;
  struct Env env;
  struct NS ns;
  struct Matcher matcher;
  struct Array array;
  struct Alias alias;
} Object;

static inline void Object_tini(union Object * object) {
  switch(object->type) {
  case ObjectType_var_unset:
  case ObjectType_var_set:
  case ObjectType_var_cleared:
    Var_tini(&object->var);
    break;
  case ObjectType_vnot:
    break;
  case ObjectType_env:
    Env_tini(&object->env);
    break;
  case ObjectType_ns:
    NS_tini(&object->ns);
    break;
  case ObjectType_matcher:
    Matcher_tini(&object->matcher);
    break;
  case ObjectType_array:
    Array_tini(&object->array);
  case ObjectType_alias:
    Alias_tini(&object->alias);
    break;
  }
}

static inline union Object * Object_clone(union Object * object) {
  m_inc(object);
  return object;
}

static inline void Object_free(union Object * object) {
  if(!(is_allocated & (1 << object->type))) {
    return;
  }
  
  if(m_dec(object)) {
    Object_tini(object);
    m_free(object);
  }
}

static inline fern_Box Object_get(union Object * object, fern_Box x) {
  switch(object->type) {
  case ObjectType_var_unset:
  case ObjectType_var_set:
  case ObjectType_var_cleared:
    return Var_get(&object->var, x);
  case ObjectType_array:
    return Array_get(&object->array, x);
  default:
    return fern_COMMERCIAL_AT();
  }
}

static inline fern_Box Object_set_n(union Object * object, fern_Box x) {
  switch(object->type) {
  case ObjectType_var_unset:
  case ObjectType_var_set:
  case ObjectType_var_cleared:
    return Var_set_n(&object->var, x);
  case ObjectType_vnot:
    return x;
  case ObjectType_array:
    return Array_set_n(&object->array, x);
  case ObjectType_alias:
    return Alias_set_n(&object->alias, x);
  default:
    return fern_COMMERCIAL_AT();
  }
}

static inline fern_Box Object_set_u(union Object * object, fern_Box x) {
  switch(object->type) {
  case ObjectType_var_unset:
  case ObjectType_var_set:
  case ObjectType_var_cleared:
    return Var_set_u(&object->var, x);
  case ObjectType_vnot:
    return x;
  case ObjectType_array:
    return Array_set_u(&object->array, x);
  case ObjectType_alias:
    return Alias_set_u(&object->alias, x);
  default:
    return fern_COMMERCIAL_AT();
  }
}

static inline fern_Box Object_set_q(union Object * object, fern_Box x) {
  switch(object->type) {
  case ObjectType_var_unset:
  case ObjectType_var_set:
  case ObjectType_var_cleared:
    return Var_set_q(&object->var, x);
  case ObjectType_vnot:
    return x;
  case ObjectType_matcher:
    return Matcher_set_q(&object->matcher, x);
  case ObjectType_array:
    return Array_set_q(&object->array, x);
  case ObjectType_alias:
    return Alias_set_q(&object->alias, x);
  default:
    return fern_COMMERCIAL_AT();
  }
}

static inline struct Var * Object_get_f(union Object * object, struct NS * x) {
  switch(object->type) {
  case ObjectType_var_unset:
  case ObjectType_var_set:
  case ObjectType_var_cleared:
    return Var_get_f(&object->var, x);
  case ObjectType_alias:
    return Alias_get_f(&object->alias, x);
  default:
    return NULL;
  }
}

// Var ----------------------------------------------------------------------------------------------------------------
void Var_init(struct Var * var, struct Program * program, uint32_t name) {
  var->type = ObjectType_var_unset;
  var->program = program;
  var->name = name;
  var->value = fern_COMMERCIAL_AT();
}

void Var_tini(struct Var * var) {
  fern_free(var->value);
}

fern_Box Var_get(struct Var * var, fern_Box x) {
  (void)x;
  fern_assert_fatal_error(var->type != ObjectType_var_unset, u8"Runtime: Variable referenced before definition");
  fern_assert_fatal_error(var->type != ObjectType_var_cleared, u8"Internal error: Variable used after clear");
  return var->value;
}

fern_Box Var_set_n(struct Var * var, fern_Box x) {
  fern_assert_fatal_error(var->type != ObjectType_var_cleared, u8"Internal error: Variable used after clear");
  var->type = ObjectType_var_set;
  var->value = x;
  return x;
}

fern_Box Var_set_u(struct Var * var, fern_Box x) {
  fern_assert_fatal_error(var->type != ObjectType_var_unset, u8"↩: Variable modified before definition");
  fern_assert_fatal_error(var->type != ObjectType_var_cleared, u8"Internal error: Variable used after clear");
  var->value = x;
  return x;
}

fern_Box Var_set_q(struct Var * var, fern_Box x) {
  Var_set_n(var, x);
  return fern_DIGIT_ZERO();
}

fern_Box Var_get_c(struct Var * var, fern_Box x) {
  fern_Box r = Var_get(var, x);
  var->type = ObjectType_var_cleared;
  return r;
}

struct Var * Var_get_f(struct Var * var, struct NS * x) {
  return NS_field(x, var->name, var->program);
}

// Env ----------------------------------------------------------------------------------------------------------------
struct Env * Env_allocate(uint32_t num_vars) {
  return (struct Env *)m_allocate(sizeof(struct Env) + sizeof(struct Var) * (num_vars - 1));
}

void Env_init(struct Env * env, struct Env * p, uint32_t v, uint32_t * n, uint32_t num_n) {
  env->type = ObjectType_env;
  env->parent = p;
  env->program = p->program;
  env->num_vars = v;
  env->first_named_var = v - num_n;
  for(uint32_t i = 0; i < v - num_n; i++) {
    Var_init(env->vars + i, env->program, 0);
  }
  for(uint32_t i = env->first_named_var; i < v; i++) {
    Var_init(env->vars + i, env->program, n[i - num_n]);
  }
}

void Env_tini(struct Env * env) {
  for(uint32_t i = 0; i < env->num_vars; i++) {
    Var_tini(env->vars + i);
  }
}

// NS -----------------------------------------------------------------------------------------------------------------
static struct NS * NS_allocate(void) {
  return (struct NS *)m_allocate(sizeof(struct NS));
}

static void NS_init(struct NS * ns, struct Env * env) {
  ns->type = ObjectType_ns;
  ns->env = (struct Env *)Object_clone((union Object *)env);
}

static void NS_tini(struct NS * ns) {
  Object_free((union Object *)ns->env);
}

static struct Var * NS_field(struct NS * ns, uint32_t x, struct Program * w) {
  if(w != NULL) {
    uint32_t symbol = w->names[x];
    for(uint32_t j = ns->env->first_named_var; j < ns->env->num_vars; j++) {
      if(ns->env->vars[j].name == symbol) {
        return &ns->env->vars[j];
      }
    }
    return NULL;
  } else {
    return &ns->env->vars[x];
  }
}

static fern_Box NS_read(struct NS * ns, struct Env * e, uint32_t i) {
  return Var_get(NS_field(ns, i, e->program), fern_COMMERCIAL_AT());
}

// Matcher ------------------------------------------------------------------------------------------------------------
static struct Matcher * Matcher_allocate(void) {
  return (struct Matcher *)m_allocate(sizeof(struct Matcher));
}

static void Matcher_init(struct Matcher * matcher, fern_Box x) {
  matcher->type = ObjectType_matcher;
  matcher->value = fern_clone(x);
}

static void Matcher_tini(struct Matcher * matcher) {
  fern_free(matcher->value);
}

static fern_Box Matcher_set_q(struct Matcher * matcher, fern_Box x) {
  return fern_internal_match(matcher->value, x) ? fern_DIGIT_ONE() : fern_DIGIT_ZERO();
}

// Array --------------------------------------------------------------------------------------------------------------
static struct Array * Array_allocate(uint32_t length) {
  return (struct Array *)m_allocate(sizeof(struct Array) + sizeof(union Object *) * (length - 1));
}

static void Array_init(struct Array * array, uint32_t length, fern_Box * stack) {
  array->type = ObjectType_array;
  array->length = length;
  for(uint32_t i = 0; i < length; i++) {
    array->objects[i] = (union Object *)fern_unpack_namespace(stack[i]);
  }
}

static void Array_tini(struct Array * array) {
  for(uint32_t i = 0; i < array->length; i++) {
    Object_free(array->objects[i]);
  }
}

#define Array_map(F) \
  fern_Array result = fern_allocate_array(); \
  fern_Box * cells = fern_init_data(&result->cells, fern_Format_box, array->length); \
  for(uint32_t i = 0; i < array->length; i++) { \
    F \
  } \
  fern_init_shape(&result->shape, 1, &array->length); \
  return fern_pack_array(result);

static fern_Box Array_get(struct Array * array, fern_Box x) {
  Array_map(
    cells[i] = Object_get(array->objects[i], fern_COMMERCIAL_AT());
  )
}

#define Array__set_(S, e) \
  if(fern_is_array(x)) { \
    fern_Array xa = fern_unpack_array(x); \
    fern_ArrayReader xar = fern_read_array(xa); \
    fern_assert_fatal_error( \
        fern_array_rank(xar) == 1 && fern_array_axis_length(xar, 0) == array->length \
      , e u8": Target and value shapes don't match" \
      ); \
    for(uint32_t i = 0; i < array->length; i++) { \
      S ((Object *)(&array->objects[i]), fern_array_get_cell(xar, i)); \
    } \
    Array_map( \
      cells[i] = S (array->objects[i], fern_array_get_cell(xar, i)); \
    ) \
  } else if(fern_is_namespace(x)) { \
    Object * ns = (Object *)fern_unpack_namespace(x); \
    /* TODO: "Cannot extract non-name from namespace" if not our Object (not the best test) */ \
    fern_assert_fatal_error(ns->type == ObjectType_ns, e u8": Cannot extract non-name from namespace"); \
    Array_map( \
      fern_Box c = Var_get(Object_get_f(array->objects[i], &ns->ns), fern_COMMERCIAL_AT()); \
      cells[i] = S (array->objects[i], c); \
    ) \
  } else { \
    fern_fatal_error(e u8": Multiple targets but atomic value"); \
  }

static fern_Box Array_set_n(struct Array * array, fern_Box x) {
  Array__set_(Object_set_n, u8"←")
}

static fern_Box Array_set_u(struct Array * array, fern_Box x) {
  Array__set_(Object_set_u, u8"↩")
}

static fern_Box Array_set_q(struct Array * array, fern_Box x) {
  Array__set_(Object_set_q, u8" ")
}

// Alias --------------------------------------------------------------------------------------------------------------
static struct Alias * Alias_allocate(void) {
  return (struct Alias *)m_allocate(sizeof(struct Alias));
}

static void Alias_init(struct Alias * alias, struct Env * env, uint32_t name, union Object * r) {
  alias->type = ObjectType_alias;
  alias->env = env;
  alias->name = name;
  alias->r = Object_clone(r);
}

static void Alias_tini(struct Alias * alias) {
  Object_free(alias->r);
}

static fern_Box Alias_set_n(struct Alias * alias, fern_Box x) {
  return Object_set_n(alias->r, x);
}

static fern_Box Alias_set_u(struct Alias * alias, fern_Box x) {
  return Object_set_u(alias->r, x);
}

static fern_Box Alias_set_q(struct Alias * alias, fern_Box x) {
  return Object_set_q(alias->r, x);
}

static struct Var * Alias_get_f(struct Alias * alias, struct NS * x) {
  return NS_field(x, alias->name, alias->env->program);
}

// Stack --------------------------------------------------------------------------------------------------------------
struct Stack {
  uint32_t s_length;
  uint32_t s_capacity;
  fern_Box * s;
  bool cont;
  fern_Box rslt;
};

static inline void Stack_init(struct Stack * stack, uint32_t initial_length, fern_Box * initial) {
  stack->s_length = 0;
  stack->s_capacity = 0;
  stack->s = NULL;
  stack->cont = true;
  stack->rslt = fern_COMMERCIAL_AT();
}

static inline void Stack_push(struct Stack * stack, fern_Box value) {
  if(stack->s_length + 1 >= stack->s_capacity) {
    stack->s_capacity = (stack->s_length + 1);
    stack->s_capacity += stack->s_capacity >> 1;
    stack->s = realloc(stack->s, sizeof(*stack->s) * stack->s_capacity);
  }
  stack->s[stack->s_length++] = value;
}

static inline fern_Box * Stack_pop(struct Stack * stack, uint32_t count) {
  fern_assert_fatal_error(count < stack->s_length, "internal error");
  stack->s_length -= count;
  return stack->s + stack->s_length;
}

static inline fern_Box Stack_peek(struct Stack * stack) {
  fern_assert_fatal_error(stack->s_length > 0, "internal error");
  return stack->s[stack->s_length - 1];
}

static inline void Stack_ret(struct Stack * stack, fern_Box x, uint32_t w) {
  stack->rslt = x;
  stack->cont = false;
  fern_assert_fatal_error(w >= stack->s_length, "Internal compiler error: Wrong stack size");
}

static inline void Stack_skip(struct Stack * stack) {
  stack->cont = false;
}

// ops ----------------------------------------------------------------------------------------------------------------

fern_Box run_bc(uint32_t * bc, uint32_t pos, struct Env * e) {
  struct Stack s;
  Stack_init(&s, 0, NULL);

  #define NEXT (bc[pos++])

  while(s.cont) {
    uint32_t op = NEXT, op_a, op_b;
    struct Var * v;
    
    switch(op) {
    // CONSTANTS AND DROP
    case 0:
      op_a = NEXT;
      Stack_push(&s, e->program->consts[op_a]);
      break;
    case 1:
      // TODO
      break;
    case 6:
      Stack_pop(&s, 1);
      break;

    // RETURNS
    case 7:
      Stack_ret(&s, *Stack_pop(&s, 1), 0);
      break;
    case 8:
      {
        struct NS * ns = NS_allocate();
        NS_init(ns, e);
        Stack_ret(&s, fern_pack_namespace((fern_Namespace)ns), 1);
      }
      break;

    // ARRAYS
    case 11:
      op_a = NEXT;
      fern_Box * src = Stack_pop(&s, op_a);
      {
        fern_Array result = fern_allocate_array();
        fern_init_shape(&result->shape, 1, &op_a);
        fern_Box * dst = fern_init_data(&result->cells, fern_Format_box, op_a);
        memcpy(dst, src, sizeof(*dst) * op_a);
        Stack_push(&s, fern_pack_array(result));
      }
      break;
    case 12:
      op_a = NEXT;
      {
        struct Array * result = Array_allocate(op_a);
        Array_init(result, op_a, src);
        Stack_push(&s, fern_pack_namespace((fern_Namespace)result));
      }
      break;

    // APPLICATION
    case 16:
      {
        fern_Box * f_x = Stack_pop(&s, 2);
        Stack_push(&s, fern_evoke(f_x[0], fern_Evokation_monad, f_x[1], fern_nothing()));
        fern_free(f_x[0]);
        fern_free(f_x[1]);
      }
      break;
    case 17:
      {
        fern_Box * w_f_x = Stack_pop(&s, 3);
        Stack_push(&s, fern_evoke(w_f_x[1], fern_Evokation_dyad, w_f_x[2], w_f_x[0]));
        fern_free(w_f_x[0]);
        fern_free(w_f_x[1]);
        fern_free(w_f_x[2]);
      }
      break;
    case 20:
      {
        fern_Box * g_h = Stack_pop(&s, 2);
        fern_Function result = fern_allocate_function();
        result->type = fern_FunctionType_train2;
        result->train2.g = g_h[0];
        result->train2.h = g_h[1];
        Stack_push(&s, fern_pack_function(result));
      }
      break;
    case 21:
      {
        fern_Box * f_g_h = Stack_pop(&s, 3);
        fern_Function result = fern_allocate_function();
        result->type = fern_FunctionType_train3;
        result->train3.f = f_g_h[0];
        result->train3.g = f_g_h[1];
        result->train3.h = f_g_h[2];
        Stack_push(&s, fern_pack_function(result));
      }
      break;
    case 26:
      {
        fern_Box * f_m = Stack_pop(&s, 2);
        fern_Function result = fern_allocate_function();
        fern_Modifier1 m = fern_unpack_modifier1(f_m[1]);
        if(m->type == fern_Modifier1Type_c) {
          result->type = fern_FunctionType_applied_c_m1;
          result->applied_c_m1.f = f_m[0];
          result->applied_c_m1.m = m->c;
        } else {
          result->type = fern_FunctionType_applied_m1;
          result->applied_m1.f = f_m[0];
          result->applied_m1.m = f_m[1];
        }
        Stack_push(&s, fern_pack_function(result));
      }
      break;
    case 27:
      {
        fern_Box * f_m_g = Stack_pop(&s, 3);
        fern_Function result = fern_allocate_function();
        fern_Modifier2 m = fern_unpack_modifier2(f_m_g[1]);
        if(m->type == fern_Modifier2Type_c) {
          result->type = fern_FunctionType_applied_c_m2;
          result->applied_c_m2.f = f_m_g[0];
          result->applied_c_m2.m = m->c;
          result->applied_c_m2.g = f_m_g[2];
        } else {
          result->type = fern_FunctionType_applied_m2;
          result->applied_m2.f = f_m_g[0];
          result->applied_m2.m = f_m_g[1];
          result->applied_m2.g = f_m_g[2];
        }
        Stack_push(&s, fern_pack_function(result));
      }
      break;

    // APPLICATION WITH NOTHING
    case 18:
      {
        fern_Box * f_x = Stack_pop(&s, 2);
        fern_Box result = f_x[1];
        if(!fern_internal_match(f_x[1], fern_nothing())) {
          result = fern_evoke(f_x[0], fern_Evokation_monad, f_x[1], fern_nothing());
        }
        Stack_push(&s, result);
      }
      break;
    case 19:
      {
        fern_Box * w_f_x = Stack_pop(&s, 3);
        fern_Box result = w_f_x[2];
        if(!fern_internal_match(w_f_x[2], fern_nothing())) {
          if(!fern_internal_match(w_f_x[0], fern_nothing())) {
            result = fern_evoke(w_f_x[1], fern_Evokation_dyad, w_f_x[2], w_f_x[0]);
          } else {
            result = fern_evoke(w_f_x[1], fern_Evokation_monad, w_f_x[2], fern_nothing());
          }
        }        
        Stack_push(&s, result);
      }
      break;
    case 23:
      {
        fern_Box * f_g_h = Stack_pop(&s, 3);
        fern_Function result = fern_allocate_function();
        if(fern_internal_match(f_g_h[0], fern_nothing())) {
          result->type = fern_FunctionType_train2;
          result->train2.g = f_g_h[1];
          result->train2.h = f_g_h[2];
        } else {
          result->type = fern_FunctionType_train3;
          result->train3.f = f_g_h[0];
          result->train3.g = f_g_h[1];
          result->train3.h = f_g_h[2];
        }
        Stack_push(&s, fern_pack_function(result));
      }
      break;
    case 22:
      fern_assert_fatal_error(!fern_internal_match(Stack_peek(&s), fern_nothing()), "Left argument required");
      break;

    // VARIABLES
    case 32:
      op_a = NEXT;
      op_b = NEXT;
      { struct Env * voe = e;
        while(op_a--) voe = voe->parent;
        v = voe->vars + op_b;
      }
      {
        Stack_push(&s, Var_get(v, fern_COMMERCIAL_AT()));
      }
      break;
    case 34:
      op_a = NEXT;
      op_b = NEXT;
      { struct Env * voe = e;
        while(op_a--) voe = voe->parent;
        v = voe->vars + op_b;
      }
      {
        Stack_push(&s, Var_get_c(v, fern_COMMERCIAL_AT()));
      }
      break;
    case 33:
      op_a = NEXT;
      op_b = NEXT;
      { struct Env * voe = e;
        while(op_a--) voe = voe->parent;
        v = voe->vars + op_b;
      }
      {
        Stack_push(&s, fern_pack_namespace((fern_Namespace)v));
      }
      break;

    // HEADERS
    case 42:
      {
        fern_Box predicate = *Stack_pop(&s, 1);
        if(fern_internal_match(predicate, fern_DIGIT_ZERO())) {
          Stack_skip(&s);
        } else {
          fern_assert_fatal_error(fern_internal_match(predicate, fern_DIGIT_ONE()), "Predicate value must be 0 or 1");
        }
      }
      break;
    case 43:
      {
        struct Matcher * matcher = Matcher_allocate();
        Matcher_init(matcher, *Stack_pop(&s, 1));
        Stack_push(&s, fern_pack_namespace((fern_Namespace)matcher));
      }
      break;
    case 44:
      {
        Stack_push(&s, fern_pack_namespace((fern_Namespace)&vnot));
      }
      break;

    // ASSIGNMENT
    case 47:
      {
        fern_Box * r_v = Stack_pop(&s, 2);
        union Object * object = (union Object *)fern_unpack_namespace(r_v[0]);
        fern_Box result = Object_set_q(object, r_v[1]);
        if(fern_force_natural(result) > 0) {
          Stack_skip(&s);
        }
      }
      break;
    case 48:
      {
        fern_Box * r_v = Stack_pop(&s, 2);
        union Object * object = (union Object *)fern_unpack_namespace(r_v[0]);
        fern_Box result = Object_set_n(object, r_v[1]);
        Stack_push(&s, result);
      }
      break;
    case 49:
      {
        fern_Box * r_v = Stack_pop(&s, 2);
        union Object * object = (union Object *)fern_unpack_namespace(r_v[0]);
        fern_Box result = Object_set_u(object, r_v[1]);
        Stack_push(&s, result);
      }
      break;
    case 50:
      {
        fern_Box * r_f_x = Stack_pop(&s, 3);
        union Object * object = (union Object *)fern_unpack_namespace(r_f_x[0]);
        fern_Box result = Object_get(object, fern_COMMERCIAL_AT());
        result = fern_evoke(r_f_x[1], fern_Evokation_dyad, result, r_f_x[2]);
        result = Object_set_u(object, result);
        Stack_push(&s, result);
      }
      break;
    case 51:
      {
        fern_Box * r_f = Stack_pop(&s, 2);
        union Object * object = (union Object *)fern_unpack_namespace(r_f[0]);
        fern_Box result = Object_get(object, fern_COMMERCIAL_AT());
        result = fern_evoke(r_f[1], fern_Evokation_monad, result, fern_nothing());
        result = Object_set_u(object, result);
        Stack_push(&s, result);
      }
      break;

    // NAMESPACES
    case 64:
      op_a = NEXT;
      {
        struct NS * ns = (struct NS *)fern_unpack_namespace(*Stack_pop(&s, 1));
        Stack_push(&s, NS_read(ns, e, op_a));
      }
      break;
    case 66:
      op_a = NEXT;
      {
        struct Alias * alias = Alias_allocate();
        Alias_init(alias, e, op_a, (union Object *)fern_unpack_namespace(*Stack_pop(&s, 1)));
        Stack_push(&s, fern_pack_namespace((fern_Namespace)alias));
      }
      break;

    default:
      fern_fatal_error("unknown opcode");
    }
  }

  #undef NEXT

  return s.rslt;
}

// PROGRAM ------------------------------------------------------------------------------------------------------------
