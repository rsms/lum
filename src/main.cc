#include <lum/lum.h>
#include <lum/str.h>
#include <lum/hash.h>
#include <lum/sym.h>
#include <lum/cell.h>
#include <lum/bif.h>
#include <lum/var.h>
#include <lum/env.h>
#include <lum/namespace.h>
#include <lum/read.h>
#include <lum/eval.h>
#include <lum/print.h>

namespace lum {

} // namespace lum

using namespace lum;

static Cell* print_eval(Env& env, Cell* expr) {
  std::cout << expr << "\n";
  Cell* result = eval(&env, expr);
  std::cout << ";=> " << result << '\n';
  return result;
}

// -----------------------------------------------------------------------
// (+ 1 5000 (+ 25))
// 
// CONS MEMORY              SYMBOL MEMORY
// C0. CONS   &C1    0      S0. "+"
// C1. SYM    &S0  &C2
// C2. INT      1  &C3
// C3. INT   5000  &C4
// C4. CONS   &C5    0
// C5. SYM    &S0  &C6
// C6. INT    25     0
//
// p = &C1
// Note: There's a 1:1 relation between a cons cell and a value
//
int main(int argc, char** argv) {
  using std::cout;
  Env env;

  #if 0
  Reader R("('lol cat) () (Z elda) ");
  while (R.waiting()) {
    Cell* r = R.read();
    if (!r) {
      if (R.error) {
        std::cerr << "read error: " << R.error->message << "\n";
      } else if (R.waiting()) {
        std::cerr << "read: waiting for more input\n";
      }
      break;
    } else {
      std::cout << ">> " << r << "\n";
      //std::cout << ";=> " << eval(&env, r) << '\n';
      //env.results.unwind(0);
      Cell::free(r);
    }
  }
  return 0;
  #endif // reader


  #if 0
  Sym* sym1 = Sym::create(intern_str("foo-bar"), intern_str("lol"));
  std::cout << "sym1 => " << sym1 << "\n";

  static constexpr auto _cs = ConstStrCreate("nil");
  const Str const* cs = (const Str const*)&_cs;
  const Str* ds = intern_str("nil");
  assert(cs->length == 3);
  assert(cs->length == ds->length);
  assert(cs->hash == 228849900);
  assert(cs->hash == ds->hash);
  assert(cs->equals(ds));
  assert(ds->equals(cs));

  constexpr auto csym = ConstSymCreate(&_cs);
  auto dsym = Sym::create(0, ds);
  const Sym* dcsym = (const Sym*)&csym;
  assert(csym.hash == dsym->hash);
  assert(csym.hash == dcsym->hash);
  assert(dcsym->hash == dsym->hash);

  std::cout << "csym =>\n"
            << "  hash:   " << csym.hash << "\n"
            << "  name:   " << csym.name << "\n"
            << "dsym =>\n"
            << "  hash:   " << dsym->hash << "\n"
            << "  name:   " << dsym->name << "\n"
            << "dcsym =>\n"
            << "  hash:   " << dcsym->hash << "\n"
            << "  name:   " << dcsym->name << "\n"
            << "cs =>\n"
            << "  length: " << cs->length << "\n"
            << "  hash:   " << cs->hash << "\n"
            << "  c_str:  " << cs->c_str() << "\n"
            << "ds =>\n"
            << "  length: " << ds->length << "\n"
            << "  hash:   " << ds->hash << "\n"
            << "  c_str:  " << ds->c_str() << "\n"
            << "cs->equals(ds) => " << cs->equals(ds) << "\n"
            << "ds->equals(cs) => " << ds->equals(cs) << "\n"
            ;
  return 0;
  #endif // const sym and str

  Cell* a, * b, * c, * r;

  // (+ 1 (+ 6 7) (+ 25))
  // {CONS
  //   {SYM +} ->
  //   {INT 1} ->
  //   {CONS
  //     {SYM +} ->
  //     {INT 6} ->
  //     {INT 7} -x
  //   } ->
  //   {CONS
  //     {SYM +} ->
  //     {INT 25} -x
  //   } -x
  // }
  a = Cell::createInt(25);
  b = Cell::createSym(kSym_sum, a);
  Cell* plus_25 = Cell::createCons(b); // (+ 25)
  a = Cell::createInt(7);
  b = Cell::createInt(6, a);
  a = Cell::createSym(kSym_sum, b);
  Cell* plus_6_7 = Cell::createCons(a, plus_25); // (+ 6 7) -> (+ 25)
  a = Cell::createInt(1, plus_6_7);
  b = Cell::createSym(kSym_sum, a);
  Cell* expr1 = Cell::createCons(b); // (+ 1 (+ 6 7) (+ 25))
  std::cout << expr1 << '\n';
  Cell* result = eval(&env, expr1);
  std::cout << ";=> " << result << '\n';
  assert(result != 0);
  assert(result->type == Type::INT);
  assert(result->value.i == 39);
  env.results.unwind(0);

  // `(+ 6 7) => (+ 6 7)
  std::cout << "------\n";
  a = Cell::createInt(7);
  b = Cell::createInt(6, a);
  a = Cell::createSym(kSym_sum, b);
  b = Cell::createCons(a);
  expr1 = Cell::createQuote(b);
  std::cout << expr1 << '\n';
  result = eval(&env, expr1);
  std::cout << ";=> " << result << '\n';
  // (+ 6 7) => 13
  std::cout << "------\n";
  result = eval(&env, result);
  std::cout << ";=> " << result << '\n';
  env.results.unwind(0);

  // (+ 1 `(+ 6 7) (+ 25)) => ERROR
  std::cout << "------\n";
  a = Cell::createInt(25);
  b = Cell::createSym(kSym_sum, a);
  plus_25 = Cell::createCons(b); // (+ 25)
  a = Cell::createInt(7);
  b = Cell::createInt(6, a);
  a = Cell::createSym(kSym_sum, b);
  b = Cell::createCons(a);
  plus_6_7 = Cell::createQuote(b, plus_25); // `(+ 6 7) -> (+ 25)
  a = Cell::createInt(1, plus_6_7);
  b = Cell::createSym(kSym_sum, a);
  expr1 = Cell::createCons(b); // (+ 1 `(+ 6 7) (+ 25))
  std::cout << expr1 << '\n';
  result = eval(&env, expr1);
  assert(result == 0);
  env.results.unwind(0);

  // (cons 3 nil) => (3)
  std::cout << "------\n";
  a = Cell::createInt(3);             // 3 -x
  b = Cell::createSym(kSym_cons, a);  // cons -> 3 -x
  a = Cell::createCons(b);            // ( cons -> 3 -x ) -x
  std::cout << a << '\n';
  result = eval(&env, a);
  std::cout << ";=> " << result << '\n';
  env.results.unwind(0);

  // (cons 1 (cons 2 (cons 3 nil))) => (1 2 3)
  std::cout << "------\n";
  a = Cell::createInt(3);             //                                         3 -x
  b = Cell::createSym(kSym_cons, a);  //                                 cons -> 3 -x
  a = Cell::createCons(b);            //                               ( cons -> 3 -x ) -x
  b = Cell::createInt(2, a);          //                          2 -> ( cons -> 3 -x ) -x
  a = Cell::createSym(kSym_cons, b);  //                  cons -> 2 -> ( cons -> 3 -x ) -x
  b = Cell::createCons(a);            //                ( cons -> 2 -> ( cons -> 3 -x ) -x ) -x
  a = Cell::createInt(1, b);          //           1 -> ( cons -> 2 -> ( cons -> 3 -x ) -x ) -x
  b = Cell::createSym(kSym_cons, a);  //   cons -> 1 -> ( cons -> 2 -> ( cons -> 3 -x ) -x ) -x
  a = Cell::createCons(b);            // ( cons -> 1 -> ( cons -> 2 -> ( cons -> 3 -x ) -x ) -x ) -x
  std::cout << a << '\n';
  result = eval(&env, a);
  std::cout << ";=> " << result << '\n';
  env.results.unwind(0);

  // (cons 1 `(2 3)) => (1 2 3)
  std::cout << "------\n";
  a = Cell::createInt(3);       // 3 -x
  b = Cell::createInt(2, a);    // 2 -> 3
  a = Cell::createCons(b);      // ( 2 -> 3 -x ) -x
  b = Cell::createQuote(a);     // ` ( 2 -> 3 -x) -x ` -x
  a = Cell::createInt(1, b);    // 1 -> ` ( 2 -> 3 -x ) -x ` -x
  b = Cell::createSym(kSym_cons, a);// cons -> 1 -> ` ( 2 -> 3 -x ) -x ` -x
  a = Cell::createCons(b);      // ( cons -> 1 -> ` ( 2 -> 3 -x ) -x ` -x ) -x
  std::cout << a << '\n';
  result = eval(&env, a);
  std::cout << ";=> " << result << '\n';
  env.results.unwind(0);

  // (cons (+ 1) `(2 3)) => (1 2 3)
  std::cout << "------\n";
  a = Cell::createInt(3);       // 3 -x
  b = Cell::createInt(2, a);    // 2 -> 3
  a = Cell::createCons(b);      // ( 2 -> 3 -x ) -x
  c = Cell::createQuote(a);     // ` ( 2 -> 3 -x) -x ` -x
  a = Cell::createInt(1);
  b = Cell::createSym(kSym_sum, a);
  a = Cell::createCons(b, c);
  b = Cell::createSym(kSym_cons, a);// cons -> 1 -> ` ( 2 -> 3 -x ) -x ` -x
  a = Cell::createCons(b);      // ( cons -> 1 -> ` ( 2 -> 3 -x ) -x ` -x ) -x
  std::cout << a << '\n';
  result = eval(&env, a);
  std::cout << ";=> " << result << '\n';
  env.results.unwind(0);

  // (= 1) => true
  std::cout << "------\n";
  b = Cell::createInt(1);
  a = Cell::createSym(kSym_eq, b);
  b = Cell::createCons(a);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (= 1 1 1 1) => true
  std::cout << "------\n";
  a = Cell::createInt(1);
  b = Cell::createInt(1, a);
  a = Cell::createInt(1, b);
  b = Cell::createInt(1, a);
  a = Cell::createSym(kSym_eq, b);
  b = Cell::createCons(a);
  env.results.push(b);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (= 1 2) => false
  std::cout << "------\n";
  a = Cell::createInt(2);
  b = Cell::createInt(1, a);
  a = Cell::createSym(kSym_eq, b);
  b = Cell::createCons(a);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (= (+ 1 1) 2) => true
  std::cout << "------\n";
  c = Cell::createInt(2);
  a = Cell::createInt(1);
  b = Cell::createInt(1, a);
  a = Cell::createSym(kSym_sum, b);
  b = Cell::createCons(a, c);      // (+ 1 1) -> 2
  a = Cell::createSym(kSym_eq, b); // = -> (+ 1 1) -> 2
  b = Cell::createCons(a);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // true => true
  std::cout << "------\n";
  b = Cell::createSym(kSym_true);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (true) => ERROR
  std::cout << "------\n";
  b = Cell::createCons(Cell::createSym(kSym_true));
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (- 18.5 4.2) => 14.3
  std::cout << "------\n";
  a = Cell::createFloat(4.2);
  b = Cell::createFloat(18.5, a);
  a = Cell::createSym(kSym_sub, b);
  b = Cell::createCons(a);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (rem 18.5 4.2) => 1.6993
  std::cout << "------\n";
  a = Cell::createFloat(4.2);
  b = Cell::createFloat(18.5, a);
  a = Cell::createSym(kSym_rem, b);
  b = Cell::createCons(a);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (rem 18 4) => 2
  std::cout << "------\n";
  a = Cell::createInt(4);
  b = Cell::createInt(18, a);
  a = Cell::createSym(kSym_rem, b);
  b = Cell::createCons(a);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (in-ns lol) => #<ns lol>
  std::cout << "------\n";
  a = Cell::createSym(intern_sym("lol"));
  b = Cell::createSym(kSym_in_ns, a);
  c = Cell::createCons(b);
  std::cout << c << "\n;=> " << eval(&env, c) << '\n';
  env.results.unwind(0);

  // (def cat 123) => #<var lol/cat>
  std::cout << "------\n";
  b = Cell::createInt(123);
  a = Cell::createSym(intern_sym("cat"), b);
  b = Cell::createSym(kSym_def, a);
  c = Cell::createCons(b);
  std::cout << c << "\n;=> " << eval(&env, c) << '\n';
  env.results.unwind(0);

  // (+ cat 100) => 223
  std::cout << "------\n";
  a = Cell::createInt(100);
  b = Cell::createSym(intern_sym("cat"), a);
  a = Cell::createSym(kSym_sum, b);
  b = Cell::createCons(a);
  std::cout << b << "\n;=> " << eval(&env, b) << '\n';
  env.results.unwind(0);

  // (def cat 321) => #<var lol/cat>
  std::cout << "------\n";
  b = Cell::createInt(321);
  a = Cell::createSym(intern_sym("cat"), b);
  b = Cell::createSym(kSym_def, a);
  c = Cell::createCons(b);
  std::cout << c << "\n;=> " << eval(&env, c) << '\n';
  env.results.unwind(0);

  // (+ cat 100) => 421
  std::cout << "------\n";
  a = Cell::createInt(100);
  b = Cell::createSym(intern_sym("cat"), a);
  a = Cell::createSym(kSym_sum, b);
  b = Cell::createCons(a);
  r = print_eval(env, b);
  env.results.unwind(0);

  // (fn (x y) (+ y x)) => #<fn(x y)>
  std::cout << "------\n";
  a = Cell::createSym(intern_sym("x"));
  a = Cell::createSym(intern_sym("y"), a);
  a = Cell::createSym(intern_sym("+"), a);
  c = Cell::createCons(a); // (+ y x)
  a = Cell::createSym(intern_sym("y"));
  a = Cell::createSym(intern_sym("x"), a);
  c = Cell::createCons(a, c); // (x y) (+ y x)
  a = Cell::createSym(kSym_fn, c); // fn (x y) (+ y x)
  c = Cell::createCons(a); // (fn (x y) (+ y x))
  r = print_eval(env, c);
  std::cout << "------\n";
  // (#<fn> 4 5) => 9
  a = Cell::createInt(5);
  a = Cell::createInt(4, a);
  a = Cell::copy(r, a);
  c = Cell::createCons(a); // (#<fn(x y)> 4 5)
  r = print_eval(env, c);
  env.results.unwind(0);

  // (fn (x y) (fn (z) (+ y z x))) => #<fn(x y)>
  std::cout << "------\n";
  a = Cell::createSym(intern_sym("x"));
  a = Cell::createSym(intern_sym("y"), a);
  a = Cell::createSym(intern_sym("z"), a);
  a = Cell::createSym(intern_sym("+"), a);
  c = Cell::createCons(a); // (+ y z x)
  a = Cell::createSym(intern_sym("z"));
  c = Cell::createCons(a, c); // (z) (+ y z x)
  a = Cell::createSym(kSym_fn, c);
  c = Cell::createCons(a); // (fn (z) (+ y z x))
  a = Cell::createSym(intern_sym("y"));
  a = Cell::createSym(intern_sym("x"), a);
  c = Cell::createCons(a, c); // (x y) (fn (z) (+ y z x))
  a = Cell::createSym(kSym_fn, c); // fn (x y) (fn (z) (+ y z x))
  c = Cell::createCons(a); // (fn (x y) (fn (z) (+ y z x)))
  r = print_eval(env, c);
  std::cout << "------\n";
  // (#<fn> 4 5) => #<fn(z)>
  a = Cell::createInt(5);
  a = Cell::createInt(4, a);
  a = Cell::copy(r, a);
  c = Cell::createCons(a); // (#<fn(x y)> 4 5)
  r = print_eval(env, c);
  std::cout << "------\n";
  // (#<fn(z)> 6) => 15
  a = Cell::createInt(6);
  a = Cell::copy(r, a);
  c = Cell::createCons(a); // (#<fn(z)> 6)
  r = print_eval(env, c);
  env.results.unwind(0);


  return 0;
}

