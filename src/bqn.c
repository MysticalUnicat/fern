#include "local.h"

#define STACK_DEPTH 1024

uint32_t _decode_num(const uint8_t * bytecode, uint32_t ip, int64_t * num) {
  int64_t b = 128;
  int64_t n = bytecode[ip++];
  if(n < b) {
    return n;
  }
  int64_t i = 1;
  int64_t t = 0;
  do {
    t += i * (n - b);
    i *= b;
    n = bytecode[ip++];
  } while(n >= b);
  *num = t + 1 * n;
  return ip;
}

enum {
    bc_push_object  = 0
  , bc_push_array_v1 = 3
  , bc_push_array_v2 = 4
};

fern_Box fern_BQN_execute(fern_BQN * bqn, uint32_t ip, fern_Namespace * e) {
  fern_Box stack_buffer[STACK_DEPTH];
  fern_Box * stack = stack_buffer;

#define PUSH(X) do { *stack++ = (X); } while(0)
#define POP     (*--stack)

  const uint8_t * bytecode = bqn->bytecode;

  int64_t arg;

  for(;;) {
    switch(bytecode[ip++]) {
    case bc_push_object:
      ip = _decode_num(bytecode, ip, &arg);
      PUSH(bqn->objects[arg]);
      break;
    case bc_push_array_v1:
    case bc_push_array_v2:
      ip = _decode_num(bytecode, ip, &arg);
      {
        union fern_Data data;
        fern_Box * result = fern_init_data(&data, fern_Format_box, arg);
        memcpy(result, stack - arg, sizeof(*result) * arg);
        PUSH(fern_mk_array3(&data, fern_nil()));
        stack -= arg;
      }
      break;
    case 5:
    case 16:
      {
        fern_Box f = POP;
        fern_Box x = POP;
        PUSH(fern_evoke(f, fern_Evokation_monad, x, fern_nothing()));
        fern_free(f);
        fern_free(x);
      }
      break;
    case 6:
    case 17:
      {
        fern_Box w = POP;
        fern_Box f = POP;
        fern_Box x = POP;
        PUSH(fern_evoke(f, w.bits == fern_nothing().bits ? fern_Evokation_monad : fern_Evokation_dyad, x, w));
        fern_free(w);
        fern_free(f);
        fern_free(x);
      }
      break;
    case 7:
      {
        fern_Box f = POP;
        fern_Box m = POP;
        fern_Modifier1 modifier1 = fern_unpack_modifier1(m);
        fern_Function result = fern_allocate_function();
        if(modifier1->type == fern_Modifier1Type_c) {
          result->type = fern_FunctionType_applied_c_m1;
          result->applied_c_m1.f = f;
          result->applied_c_m1.m = modifier1->c;
        } else {
          result->type = fern_FunctionType_applied_m1;
          result->applied_m1.f = f;
          result->applied_m1.m = m;
        }
        PUSH(fern_pack_function(result));
      }
      break;
    case 8:
      {
        fern_Box f = POP;
        fern_Box m = POP;
        fern_Box g = POP;
        fern_Modifier2 modifier2 = fern_unpack_modifier2(m);
        fern_Function result = fern_allocate_function();
        if(modifier2->type == fern_Modifier2Type_c) {
          result->type = fern_FunctionType_applied_c_m2;
          result->applied_c_m2.f = f;
          result->applied_c_m2.m = modifier2->c;
          result->applied_c_m2.g = g;
        } else {
          result->type = fern_FunctionType_applied_m2;
          result->applied_m2.f = f;
          result->applied_m2.m = m;
          result->applied_m2.g = g;
        }
        PUSH(fern_pack_function(result));
      }
      break;
    case 9:
      {
        fern_Box g = POP;
        fern_Box h = POP;
        fern_Function result = fern_allocate_function();
        result->type = fern_FunctionType_train2;
        result->train2.g = g;
        result->train2.h = h;
        PUSH(fern_pack_function(result));
      }
      break;
    case 10:
    case 19:
      {
        fern_Box f = POP;
        fern_Box g = POP;
        fern_Box h = POP;
        fern_Function result = fern_allocate_function();
        result->type = fern_FunctionType_train3;
        result->train2.f = f;
        result->train2.g = g;
        result->train2.h = h;
        PUSH(fern_pack_function(result));
      }
      break;
    case 11:
      {
        fern_Box i = POP;
        fern_Box v = POP;
        /*
         * set = (d, id, v) => {
         *   eq = (a, b) => a.length == b.length && (all elements equal)
         *   if(id is array) {
         *     assert v is array && eq(shape(id), shape(v))
         *     for each cell in id and v {
         *       set(d, id.cell, v.cell
         *     }
         *   } else {
         *     (namespace, index) = id;
         *     
         *   }
         * }
         */
        //s.push(set(1,i,v));
      }
      break;
    case 12:
      {
        fern_Box i = POP;
        fern_Box v = POP;
        //s.push(set(0,i,v));
      }
      break;
    case 13:
      {
        fern_Box i = POP;
        fern_Box f = POP;
        fern_Box v = POP;
        //s.push(set(0,i,call(f,x,get(i))));
      }
      break;
    case 14:
      --stack;
      break;
    case 15:
      {
        ip = _decode_num(bytecode, ip, &arg);
        // call section
      }
      break;
    case 21:
      {
        //let v=ge(e,num())[num()];assert(v!==null);s.push(v);break;
        
      }
      break;
    case 22:
      {
        
        // push [ge(e, 
        
      }
      break;
    case 25:
      fern_assert_fatal_error(stack == stack_buffer + 1, "invalid BQN vm state at bytecode 25");
      return stack_buffer[0];
    default:
      fern_fatal_error("invalid bytecode");
    }

    
  }

#undef PUSH
}
