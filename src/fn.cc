#include <lum/fn.h>
#include <lum/sym.h>
#include <lum/cell.h>
#include <lum/eval.h>
#include <lum/print.h>
#include <lum/bif.h>

namespace lum {

static std::ostream& _dpr(Env* env) {
  std::ostream& s = std::cout;
  size_t depth = env->compile_stack.depth;
  std::fill_n(std::ostream_iterator<char>(s, ""), depth * 2, ' ');
  return s << "";
}

Fn* Fn::create(Env* env, Cell* params) {
  assert(params != 0);
  assert(params->type == Type::LIST);

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
  fn->_has_outside_locals = false;
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

    // Special case: (core/fn ...)
    if (c == first && nc->type == Type::VAR) {
      Cell* vc = ((Var*)nc->value.p)->get();
      if (vc->type == Type::BIF && Cell::getBif(vc) == kBif_fn) {
        // Inner function
        _dpr(env) << "compile_chain() inner function\n";

        // Create a new Fn
        Fn* inner_fn = Fn::create(env, c->rest);
        if (inner_fn == 0) {
          return 0;
        }

        // Compile the body (chain) of the inner function
        if (inner_fn->compile(env)) {
          // Steal body cell chain
          head = inner_fn->body();
          inner_fn->_body = Cell::createNil();
          if (c->rest->rest != head) {
            _dpr(env) << "TODO CASE " << __FILE__ << ":" << __LINE__ << "\n";
            c->rest->rest = head;
          }
        } else {
          head = 0;
        }

        // Throw away the temporary inner Fn, and check for errors
        Fn::free(inner_fn);
        return (head == 0) ? 0 : first;
      }
    }

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


static Cell* compile_list(Fn* fn, Env* env, Cell* cons) {
  _dpr(env) << "compile_list(" << cons << ")...\n";
  Cell* head = compile_chain(fn, env, (Cell*)cons->value.p);
  if (head == 0) {
    return 0;
  }
  cons->value.p = (void*)head;
  _dpr(env) << "compile_list(...) => " << cons << "\n";
  return cons;
}


static inline bool find_param_index_by_name(
    uint32_t& index, Fn* fn, const Str* name)
{
  uint32_t i = fn->param_count();
  while (i--) {
    if (fn->param(i).name == name) {
      index = fn->param_count() - i - 1;
      return true;
    }
  }
  return false;
}


// Traverse the function compile_stack starting at `cst_start_index`,
// summarizing param_count for each function, and ending either at the bottom
// of the stack (if `end_fn` is NULL) or when reaching `end_fn` (not included
// in the param_count sum).
static inline uint32_t outer_param_count_sum(
    Env* env, size_t cst_start_index, Fn* end_fn)
{
  uint32_t sum = 0;
  do {
    Fn* f = env->compile_stack.at(cst_start_index);
    if (f == end_fn) {
      break;
    }
    sum += f->param_count();
    // _dpr(env) << " pcount " << f->param_count() << " of " << f << "\n";
  } while (cst_start_index--);
  return sum;
}


static size_t lookup_local_symbol(
    Env* env, Cell* symcell, const Str* name)
{
  // For each function in compile_stack, starting at top:
  size_t cst_index_top = env->compile_stack.depth - 1;
  size_t cst_index = cst_index_top;
  do {
    Fn* fn = env->compile_stack.at(cst_index);
    _dpr(env) << " br 2.1 -- visit " << fn << "\n";

    // Lookup name in fn's parameters
    uint32_t param_index;
    if (find_param_index_by_name(param_index, fn, name)) {
      (_dpr(env) << " br 2.1.1  -- found "
                 << (cst_index < cst_index_top ? "outer" : "inner")
                 << " local param #" << param_index << "\n");

      // Offset param_index by the sum of outer functions' parameter count.
      //
      // E.g. Say we are compiling #<fn(c0)> of the following expression:
      //   (fn (a0 a1) (fn (b0) (fn (c0) (+ a0 a1 b0 c0))))
      // And we are looking up the symbol "b0" which we just found in the
      // parameters of the outer function #<fn(b0)> at index 0.
      // The compile_stack would look like this:
      //   #2 => #<fn(c0)>
      //   #1 => #<fn(b0)>     <-- fn, cst_index
      //   #0 => #<fn(a0 a1)>
      // And so we start at cst_index-1 which would be #<fn(a0 a1)> in the
      // compile_stack.
      // The result in this case would be 2, offsetting b0 from #0 to #2.
      if (env->compile_stack.depth > cst_index-1) {
        param_index += outer_param_count_sum(env, cst_index-1, fn);
      }

      _dpr(env) << "symbol(" << name << ") => in "
                << fn << " at index " << param_index <<  "\n";
      // Convert sym to local
      symcell->type = Type::LOCAL;
      symcell->value.i = (int64_t)param_index;
      return cst_index;
    }

  } while (cst_index--);

  return SIZE_MAX;
}


static Cell* compile_symbol(Fn* fn, Env* env, Cell* symcell) {
  Sym* sym = (Sym*)symcell->value.p;
  _dpr(env) << "compile_symbol(" << sym << ")\n";
  //
  // Looks up a symbol in the following way:
  //
  //   1. If the symbol is namespace-qualified, goto 4.
  //
  //   2. Search the function's parameters for a match, if found, substitute
  //      the cell with a new LOCAL cell, referencing the parameter by its
  //      offset plus the sum of outer functions' parameter count.
  //
  //   3. Search each outer function's parameters for a match, starting with
  //      the one closest to the current function, moving outwards. If a
  //      match is found in function X, substitute the cell with a new LOCAL
  //      cell, referencing the parameter by its offset in function X plus the
  //      sum of outer functions' parameter count.
  //
  //   4. Look up in env and substitute the cell with whatever cell the
  //      symbol references in env.
  //
  //   5. Error: "Unable to resolve symbol"
  //
  // Example:
  //    (fn (a0 a1)
  //      (fn (b0)
  //        (fn (c0)
  //          (+ a0 a1 b0 c0))))
  //
  // Evaluating the above should produce:
  //    #<fn (a0 a1)
  //      (&core/fn (b0)
  //        (&core/fn (c0)
  //          (&core/+ #<local 1> #<local 0> #<local 2> #<local 3>)))>
  //
  // Evaluating the above as (#<fn> 10 20) should produce:
  //    #<fn (b0)
  //      (&core/fn (c0)
  //        (&core/+ 10 20 #<local 0> #<local 1>)))>
  //
  // Evaluating the above as (#<fn> 30) should produce:
  //    #<fn (c0)
  //        (&core/+ 10 20 30 #<local 0>)))>
  //
  // Evaluating the above as (#<fn> 40) should produce:
  //    100
  //

  // 1-3. Does this symbol exist in any functions' parameters?
  if (sym->ns == 0) {
    const Str* name = sym->name;
    size_t cst_index = lookup_local_symbol(env, symcell, name);
    if (cst_index != SIZE_MAX) {
      if (!fn->_has_outside_locals && cst_index < env->compile_stack.depth-1) {
        // Mark the function as needing a closure, since it references
        // locals in an outer function.
        fn->_has_outside_locals = true;
      }
      return symcell;
    } // else: not found, so continue...
  }

  // 4. Lookup symbol in env
  _dpr(env) << " br 3 -- in env?\n";
  Var* var = env->resolve_symbol(sym);
  if (var == 0) {
    std::cerr << "Unable to resolve symbol '" << sym << "' in this context\n";
    return 0;
  }
  _dpr(env) << "resolve_symbol(" << sym << ") => " << var << "\n";

  // Since we own symcell, it's safe to convert it to a VAR cell
  symcell->type = Type::VAR;
  symcell->value.p = (void*)var;
  return symcell;
}


static Cell* compile_local(Fn* fn, Env* env, Cell* cell) {
  // _dpr(env) << "compile_local(" << cell << ") ...\n";
  size_t offset_from_top = (size_t)cell->value.i;

  if (offset_from_top < env->locals.depth) {
    // This local refers to an active local, so we expand to the value
    Cell* value_cell = env->get_local(offset_from_top);
    _dpr(env) << "compile_local(" << cell << ") => " << value_cell << "\n";
    return Cell::copy(value_cell);
  }

  // If we get here, the local references an outside local which means that
  // this function needs a closure.
  fn->_has_outside_locals = true;

  // offset by the currently executing function's param_count
  //
  // L = 5            ; local offset (from locals' stack top)
  // PC = 2           ; executing function's param_count
  // O = L - PC       ; new local offset

  // In this situation the apply_stack top should be the core/fn BIF since we
  // are compiling a function.
  assert(env->apply_stack.top()->type == Type::BIF);
  assert(Cell::getBif(env->apply_stack.top()) == kBif_fn);

  // Now we pick the function that is applying core/fn. So we make sure that
  // the apply_stack is at least 2 cells deep and then we grab the second from
  // the top.
  assert(env->apply_stack.depth > 1);
  Cell* curr_func = env->apply_stack.at_top(1);
  assert(curr_func->type == Type::FN); // TODO: Support BIFs
  size_t param_count = ((Fn*)curr_func->value.p)->param_count();
  assert(param_count <= offset_from_top);
  size_t outer_offset_from_top = offset_from_top - param_count;

  cell = Cell::copy(cell);
  cell->value.i = (int64_t)outer_offset_from_top;
  _dpr(env) << "compile_local(" << cell << ") => "
            << "outer_scope " << cell << "\n";
  return cell;
}


static Cell* compile_fn(Fn* fn, Env* env, Cell* cell) {
  _dpr(env) << "compile_fn(" << cell << ") => " << cell << "\n";
  return cell;
}


static Cell* _compile(Fn* fn, Env* env, Cell* c) {
  switch (c->type) {
    case Type::SYM:     { return compile_symbol(fn, env, c); }
    case Type::LOCAL:   { return compile_local(fn, env, c); }
    case Type::LIST:    { return compile_list(fn, env, c); }
    case Type::FN:      { return compile_fn(fn, env, c); }
    default: { return c; }
  }
}


bool Fn::compile(Env* env) {
  assert(_body != 0);

  // Add to the compile stack
  env->compile_stack.push(this);
  
  // DEBUG print compile stack
  _dpr(env) << "compile_stack:\n";
  size_t i = env->compile_stack.depth;
  while (i--) {
    _dpr(env) << " #" << i << " => " << env->compile_stack.at(i) << "\n";
  }

  // Compile all bodies
  Cell* body = compile_chain(this, env, _body);

  // Remove from the compile stack
  assert(env->compile_stack.top() == this);
  env->compile_stack.pop();

  if (body != _body) {
    if (body == 0) {
      return false;
    }
    Cell::free(_body);
    _body = body;
  }
  return true;
}


Cell* Fn::apply(Env* env, Cell* args) {
  _dpr(env) << "Fn::apply("; printchain(std::cout, args) << ")\n";

  // push args to the locals stack
  Cell* arg = args;
  size_t locals_entry_index = env->locals.depth;
  while (arg != 0) {
    // TODO: variable "..." args
    env->locals.push(arg);
    arg = arg->rest;
  }

  // Did we satisfy the param count?
  size_t arg_count = env->locals.depth - locals_entry_index;
  if (arg_count != param_count()) {
    if (arg_count < param_count()) {
      std::cerr << "too few arguments to function " << this << "\n";
    } else {
      std::cerr << "too many arguments to function " << this << "\n";
    }
    env->unwind_locals(locals_entry_index);
    return 0;
  }

  _dpr(env) << "locals" << env->locals << "\n";
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
