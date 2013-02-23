#include <lum/fn.h>
#include <lum/sym.h>
#include <lum/cell.h>
#include <lum/eval.h>
#include <lum/print.h>
#include <lum/bif.h>

namespace lum {

Fn* Fn::create(Env* env, Cell* params) {
  assert(params != 0);
  assert(params->type == Type::CONS);

  Cell* body = params->rest;
  assert(body != 0);
  //params->rest = 0; // separate params from body

  // Count parameters and check their types
  uint32_t param_count = 0;
  Cell* param = (Cell*)params->value.p;
  while (param != 0) {
    // Only symbols are accepted as parameters
    if (param->type != Type::SYM) {
      std::cerr << "function parameter #" << param_count
                << " is not a symbol.\n";
      return 0;
    } else if (((Sym*)param->value.p)->ns != 0) {
      std::cerr << "function parameter #" << param_count
                << " is not an unqualified symbol.\n";
      return 0;
    }
    ++param_count;
    param = param->rest;
  }

  // Allocate struct
  Fn* fn = (Fn*)malloc(sizeof(Fn) + (sizeof(Param) * param_count));
  fn->_body = body;
  fn->_param_count = 0;

  // Initialize Params
  param = (Cell*)params->value.p;
  while (param != 0) {
    Param& p = fn->_params[fn->_param_count++];
    assert(param->value.p != 0);
    p.name = ((Sym*)param->value.p)->name;
    p.type = Type::UNKNOWN;
    p.is_variable = p.name->ends_with("...");

    //Cell* prev_param = param;
    param = param->rest;
    // we only needed to inspect the param and copy the pointer to the interned
    // string, so now we need to free the cell since we own it.

    //Cell::free(prev_param); // FIXME Can't do this since inner compilation
    // only borrows these, and does not give them away.
  }

  return fn;

  // Q: How do we efficiently bind parameters into the body we are about
  // to compile, but keep things immutable? First idea was to replace the
  // in-body symbols with Vars which is bound to input values at apply-time,
  // but that would cause race conditions in MP environments.
  //
  // Perhaps by position? apply() would eat param_count args into a
  // Cell*[param_count] array and pass that array as "locals" when evaluating
  // the function body. When compiling the body, we would replace symbols
  // with some new cell type e.g. "Local" which value is the parameter
  // position.
  //
  // Or we could use a "locals stack" where each frame is a map Str=>Cell
  // where Str is the local symbol and Cell is the input value of the current
  // function call.
  //
  // BTW, we should probably have a separate Env in each scheduler (OS thread).
  // That would greatly simplify things, and as we already pass around env when
  // evaluating, env could be used to carry a mutable stack of locals. This is
  // also how we can implement thread-local vars in an efficient way.

  // f(x y) => (+ x y)
  // (f 1 2)
  // f(args[2]) => (+ args[0] args[1])
  // (f [1,2])
  //
  // call eval_fn(env, fn, args):
  //   arg = args
  //   for i in fn.param_count:
  //     env.locals.push(arg)
  //     arg = arg->rest
  //     if (i == fn.param_count && arg != 0 && !fn->params_varargs):
  //       error("Too many arguments to function")
  //     else if (i != fn.param_count && arg == 0):
  //       error("Too few arguments to function")
  //   call fn->apply(env):
  //     eval(env, "+", env.locals.at(1) env.locals.at(0))
  //     ; order is reversed since locals is a stack
  //
}


void Fn::free(Fn* fn) {
  std::free(fn);
}


static Cell* _compile(Fn* fn, Env* env, Cell* c);


static Cell* compile_chain(Fn* fn, Env* env, Cell* first) {
  Cell* c = first;
  Cell* prev = 0;
  Cell* head = 0;

  while (c) {
    Cell* nc = _compile(fn, env, c);

    #if 1
    // Special case: (core/fn ...) stays untouched for now
    if (c == first && nc->type == Type::VAR) {
      Cell* vc = ((Var*)nc->value.p)->get();
      if (vc->type == Type::BIF && ((BIFImpl)vc->value.p) == &BIF_fn) {
        // TODO: Actually compile the contents of (fn ...) but mark env with
        // something like env->is_compiling_future_fn=true so that unresolved
        // symbols can be ignored (e.g. (fn (z) (+ z)) would not cause a
        // "symbol z can't be resolved" error).
        std::cout << "[fn compile_cons] inner (core/fn ...)\n";

        Fn* inner_fn = Fn::create(env, c->rest);
        if (inner_fn == 0) {
          return 0;
        }

        env->compile_stack.push(inner_fn);

        Cell* head = compile_chain(inner_fn, env, 
          const_cast<Cell*>(inner_fn->body()));
        
        assert(env->compile_stack.top() == inner_fn);
        env->compile_stack.pop();

        Fn::free(inner_fn); // throw away
        if (head == 0) {
          return 0;
        }
        return first;
      }
    }
    #endif

    if (head == 0) {
      head = nc;
    } else {
      prev->rest = nc;
    }
    prev = nc;

    c = c->rest;
  }

  return head;
}


static Cell* compile_cons(Fn* fn, Env* env, Cell* cons) {
  std::cout << "compile_cons(" << cons << ")...\n";
  Cell* head = compile_chain(fn, env, (Cell*)cons->value.p);
  if (head == 0) {
    return 0;
  }
  cons->value.p = (void*)head;
  std::cout << "compile_cons(...) => " << cons << "\n";
  return cons;
}


static Cell* compile_symbol(Fn* fn, Env* env, Cell* symcell) {
  Sym* sym = (Sym*)symcell->value.p;
  std::cout << "compile_symbol(" << sym << ")\n";

  // Does this symbol exist in the function's parameters?
  if (sym->ns == 0) {
    uint32_t i = 0;
    while (i != fn->_param_count) {
      if (fn->_params[i++].name == sym->name) {
        uint32_t stack_offset = fn->_param_count - i;
        std::cout << "symbol(" << sym << ") => local #"
                  << stack_offset << "\n";
        // The symbol references a local/lexical value. Since we own symcell,
        // it's safe to convert it to a LOCAL cell
        symcell->type = Type::LOCAL;
        symcell->value.i = (int64_t)stack_offset;
        return symcell;
      }
    }
  }

  // Look outside our compile scope, as we might be inside another compile
  // scope which have locals that match our symbol
  if (env->compile_stack.index > 1) {
    const Str* name = sym->name;
    size_t compile_stack_offs = env->compile_stack.index - 2;
    do {
      Fn* parent_fn = env->compile_stack.at(compile_stack_offs);
      assert(parent_fn != fn);

      uint32_t i = 0;
      for (; i != parent_fn->param_count(); ++i) {
        if (parent_fn->param(i).name == name) {
          // Found a match
          uint64_t stack_offset = parent_fn->_param_count - (i+1);
          // Offset by the local param_count
          stack_offset += fn->_param_count;

          std::cout << "symbol(" << sym << ") => in parent "
                    << parent_fn << " at offset " << stack_offset <<  "\n";
          // Convert sym to local
          symcell->type = Type::LOCAL;
          symcell->value.i = (int64_t)stack_offset;
          return symcell;
        }
      }

    } while (compile_stack_offs--);

  }

  // Perhaps the symbol references something in env then
  Var* var = env->resolve_symbol(sym);
  if (var == 0) {
    std::cerr << "Unable to resolve symbol '" << sym << "' in this context\n";
    return 0;
  }
  std::cout << "resolve_symbol(" << sym << ") => " << var << "\n";

  // Since we own symcell, it's safe to convert it to a VAR cell
  symcell->type = Type::VAR;
  symcell->value.p = (void*)var;
  return symcell;
}


static Cell* compile_local(Fn* fn, Env* env, Cell* cell) {
  std::cout << "compile_local("; print1(std::cout, cell) << ")\n";

  uint32_t stack_offset = (uint32_t)cell->value.i;

  if (stack_offset < env->locals.index) {
    std::cout << "  => ";
    print1(std::cout, env->locals.at(stack_offset)) << "\n";
    return Cell::copy(env->locals.at(stack_offset));
  }

  // offset by the current locals stack size
  assert(fn->_param_count-1 <= stack_offset);
  cell = Cell::copy(cell);
  cell->value.i = (int64_t)(stack_offset - fn->_param_count-1);
  std::cout << "  => outer_scope " << cell << "\n";
  return cell;
}


static Cell* compile_fn(Fn* fn, Env* env, Cell* cell) {
  std::cout << "compile_fn(" << cell << ") => " << cell << "\n";
  return cell;
}


static Cell* _compile(Fn* fn, Env* env, Cell* c) {
  switch (c->type) {
    case Type::SYM:     { return compile_symbol(fn, env, c); }
    case Type::LOCAL:   { return compile_local(fn, env, c); }
    case Type::CONS:    { return compile_cons(fn, env, c); }
    case Type::FN:      { return compile_fn(fn, env, c); }
    default: { return c; }
  }
}


bool Fn::compile(Env* env) {
  // Add to the compile stack
  env->compile_stack.push(this);

  // TODO: Walk body chain and resolve any symbols from `env`, unless the
  // symbol exists in `params`. The vars of the resolved symbols are retained,
  // so that reconfiguring the value of the var outside the function will
  // reflect on the function body.
  Cell* body = _body;
  Cell* prev = 0;
  bool success = true;

  while (body != 0) {
    Cell* c = _compile(this, env, body);
    if (c == 0) {
      // Error
      success = false;
      break;
    }
    if (prev == 0) {
      _body = c;
    } else {
      prev->rest = c;
    }
    prev = body;
    body = body->rest;
  }

  // Remove from the compile stack
  assert(env->compile_stack.top() == this);
  env->compile_stack.pop();

  return success;
}


Cell* Fn::apply(Env* env, Cell* args) {
  std::cout << "Fn::apply("; printchain(std::cout, args) << ")\n";

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
  if (arg_count != param_count()) {
    if (arg_count < param_count()) {
      std::cerr << "too few arguments to function " << this << "\n";
    } else {
      std::cerr << "too many arguments to function " << this << "\n";
    }
    env->unwind_locals(locals_entry_index);
    return 0;
  }

  std::cout << "locals" << env->locals << "\n";
  Cell* result = eval(env, _body);

  // pop args from the local stack
  env->unwind_locals(locals_entry_index);

  return result;
}


std::ostream& operator<< (std::ostream& os, const Fn const* fn) {
  os << "#<fn(";
  uint32_t i = 0;
  for (; i != fn->param_count(); ++i) {
    if (i) { os << ' '; }
    os << fn->param(i).name;
    if (fn->param(i).type != Type::UNKNOWN) {
      os << ':' << Cell::type_name(fn->param(i).type);
    }
  }
  return os << ") " << fn->_body << ">";
  return os << ")" << (void*)fn << ">";
}

} // namespace lum
