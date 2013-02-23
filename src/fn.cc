#include <lum/fn.h>
#include <lum/sym.h>
#include <lum/cell.h>
#include <lum/eval.h>
#include <lum/print.h>

namespace lum {

Fn* Fn::create(Env* env, Cell* params) {
  assert(params != 0);
  assert(params->type == Type::CONS);

  Cell* body = params->rest;
  assert(body != 0);
  params->rest = 0; // separate params from body

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

    Cell* prev_param = param;
    param = param->rest;
    // we only needed to inspect the param and copy the pointer to the interned
    // string, so now we need to free the cell since we own it.
    Cell::free(prev_param);
  }

  // Compile the function body
  if (!fn->compile(env)) {
    std::free(fn);
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

static Cell* _compile(Fn* fn, Env* env, Cell* c);


static Cell* compile_cons(Fn* fn, Env* env, Cell* cons) {
  Cell* first = (Cell*)cons->value.p;
  if (first != 0) {
    Cell* c = first;
    Cell* prev = 0;
    do {
      Cell* nc = _compile(fn, env, c);
      if (nc != c) {
        // substitute cell
        // TODO memory management
        if (prev == 0) {
          cons->value.p = (void*)c;
        } else {
          prev->rest = c;
        }
      }
    } while ((c = c->rest) != 0);
  }
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

  // The symbol references something in env
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


static Cell* _compile(Fn* fn, Env* env, Cell* c) {
  switch (c->type) {
    case Type::SYM:     { return compile_symbol(fn, env, c); }
    // case Type::VAR:     { return compile_var(fn, env, c); }
    // case Type::QUOTE:   { return compile_quote(fn, env, c); }
    case Type::CONS:    { return compile_cons(fn, env, c); }
    default: { return c; }
  }
}


bool Fn::compile(Env* env) {
  // TODO: Walk body chain and resolve any symbols from `env`, unless the
  // symbol exists in `params`. The vars of the resolved symbols are retained,
  // so that reconfiguring the value of the var outside the function will
  // reflect on the function body.
  Cell* body = _body;
  Cell* prev = 0;
  while (body != 0) {
    Cell* c = _compile(this, env, body);
    if (c == 0) {
      // Error
      return false;
    }
    if (prev == 0) {
      _body = c;
    } else {
      prev->rest = c;
    }
    prev = body;
    body = body->rest;
  }
  return true;
}


Cell* Fn::apply(Env* env, Cell* args) {
  std::cout << "Fn::apply(" << args << ")\n";
  // FIXME arguments
  return eval(env, _body);
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
  // return os << ") " << fn->_body << ">";
  return os << ")" << (void*)fn << ">";
}

} // namespace lum
