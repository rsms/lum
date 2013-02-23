#ifndef _LUM_EVAL_H_
#define _LUM_EVAL_H_

#include <lum/common.h>
#include <lum/env.h>
#include <lum/sym.h>
#include <lum/fn.h>
#include <lum/print.h>

namespace lum {

struct Cell;

Cell* eval(Env* env, Cell* c);


inline Cell* eval_symbol(Env* env, Cell* c) {
  assert(c->type == Type::SYM);
  Sym* sym = (Sym*)c->value.p;
  Var* v = env->resolve_symbol(sym);
  if (v == 0) {
    std::cerr << "Unable to resolve symbol '" << sym << "' in this context\n";
    return 0;
  }
  return v->get();
}


inline Cell* eval_quote(Env* env, Cell* c) {
  assert(c->type == Type::QUOTE);
  return (Cell*)c->value.p;
}


inline Cell* eval_var(Env* env, Cell* c) {
  assert(c->type == Type::VAR);
  return ((Var*)c->value.p)->get();
}


inline Cell* eval_local(Env* env, Cell* c) {
  assert(c->type == Type::LOCAL);
  size_t stack_offset = (size_t)c->value.i;
  assert(stack_offset < env->locals.index);
  return env->locals.at(stack_offset);
}


inline Cell* eval_cons(Env* env, Cell* c) {
  //std::cout << "eval("; print_1(std::cout, c) << ")\n";
  assert(c->type == Type::CONS);

  size_t result_entry_index = env->results.index();
  Cell* target = (Cell*)c->value.p;
  Cell* args = target->rest;
  std::cout << "apply("; print1(std::cout, target) << ", " << args << ")\n";
  target = eval(env, target);
  Cell* result;

  if (target->type == Type::BIF) {
    // Apply built-in function
    BIFImpl impl = (BIFImpl)target->value.p;
    result = impl(env, args);

    // Free results from previous evals
    env->results.unwind(result_entry_index);

    // Add our result to the result stack
    env->results.push(result);

  } else if (target->type == Type::FN) {
    // User function
    Fn* fn = (Fn*)target->value.p;

    // push args to the locals stack
    Cell* arg = args;
    size_t locals_entry_index = env->locals.index;
    while (arg != 0) {
      // TODO: variable "..." args
      env->locals.push(arg);
      arg = arg->rest;
    }

    // Did we satisfy the param count?
    size_t arg_count = env->locals.index - locals_entry_index;
    if (arg_count != fn->param_count()) {
      if (arg_count < fn->param_count()) {
        std::cerr << "too few arguments to function " << fn << "\n";
      } else {
        std::cerr << "too many arguments to function " << fn << "\n";
      }
      env->unwind_locals(locals_entry_index);
      return 0;
    }

    std::cout << "locals" << env->locals << "\n";

    // Apply
    result = fn->apply(env, args);

    // pop args from the local stack
    env->unwind_locals(locals_entry_index);

  } else {
    std::cerr << "first item in list is not a function\n";
    return 0;
  }

  // // Free results from previous evals
  // env->results.unwind(result_entry_index);

  // // Add our result to the result stack
  // env->results.push(result);
  return result;
}


inline Cell* eval(Env* env, Cell* c) {
  switch (c->type) {
    case Type::SYM:     { return eval_symbol(env, c); }
    case Type::VAR:     { return eval_var(env, c); }
    case Type::LOCAL:   { return eval_local(env, c); }
    case Type::QUOTE:   { return eval_quote(env, c); }
    case Type::CONS:    { return eval_cons(env, c); }
    default: { return c; }
  }
}

} // namespace lum
#endif // _LUM_EVAL_H_
