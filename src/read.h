#ifndef _LUM_READ_H_
#define _LUM_READ_H_

#include <lum/common.h>
#include <lum/cell.h>
#include <deque>

namespace lum {

struct Cell;
struct Reader;

struct SourceOrigin {
  uint32_t offset = 0;
  uint32_t line   = 0;
  uint32_t column = 0;
};

struct Reader {
  enum class State {
    ROOT,
    SYM,
    MAYBE_NUM,   // start with - or +
    NUM,
  } state = State::ROOT;

  struct Error {
    enum class Code {
      Unknown = 0,
      UnbalancedGroup,
    };
    Code         code;
    std::string  message;
    SourceOrigin origin;
    Error(Code code, std::string message, SourceOrigin origin)
        : code(code), message(message), origin(origin) {}
  };

  struct Input {
    const char* begin = 0;
    const char* curr  = 0;
    const char* end   = 0;
    SourceOrigin origin;

    char consume1() {
      ++origin.offset;
      ++origin.column;
      ++curr;
      return *curr;
    }
  } input;

  std::string buffer;
  Error* error = 0;

  struct Frame {
    Cell* container = 0;
    Cell* tail = 0;
    Frame(Cell* container) : container(container) {}
  };
  std::deque<Frame> stack;
  Cell* top_cell = 0;

  Reader(const char* cstr) {
    input.curr = input.begin = cstr;
    input.end = input.curr + strlen(cstr);
  }

  Cell* read();
  bool ended() const { return input.curr == input.end; }
  bool waiting() const { return error == 0 && (!ended() || !stack.empty()); }

  void read_root();
  void read_newline();
  void read_symbol();

  void append_cell(Cell* cell);

  void clear_error() { if (error) { delete error; } }
  template <typename... Args>
  void set_error(Args&&... args) {
    clear_error();
    error = new Error(args..., input.origin);
  }
};

inline Cell* Reader::read() {
  clear_error();
  while (input.curr != input.end) {
    switch (state) {
      case State::ROOT: read_root(); break;
      case State::SYM: read_symbol(); break;
      case State::MAYBE_NUM: printf("TODO read_maybe_num\n"); break;
      case State::NUM: printf("TODO read_num\n"); break;
    }

    // Error?
    if (error != 0) {
      break;
    }

    // Produce a cell?
    if (top_cell && stack.size() == 0) {
      Cell* cell = top_cell;
      top_cell = 0;
      return cell;
    }
  }
  return 0;
}

inline void Reader::read_newline() {
  input.consume1();
  ++input.origin.line;
  input.origin.column = 0;
}

void Reader::append_cell(Cell* cell) {
  if (stack.empty()) {
    assert(top_cell == 0); // ?
    top_cell = cell;
  } else {
    Frame& frame = stack.back();
    if (frame.tail == 0) {
      // this is the first cell in this stack frame
      assert(frame.container != 0);
      frame.container->value.p = (void*)cell;

      if (frame.container->type == Type::QUOTE) {
        // ends the quote since a quote only has a single item
        cell = frame.container;
        stack.pop_back();
        // append the quote to whatever is below in the stack
        append_cell(cell);
        return;
      } else {
        // if frame of stack is not a quote, it must be a cons
        assert(frame.container->type == Type::LIST);
      }
    } else {
      // Next in an existing cons
      assert(frame.tail != 0);
      assert(frame.tail->rest == 0);
      assert(frame.container->type == Type::LIST);
      frame.tail->rest = cell;
    }

    frame.tail = cell;
  }
}

inline void Reader::read_root() {
  std::cout << "read_root()\n";
  switch (*input.curr) {
    case '\n': {
      std::cout << "got newline\n";
      read_newline();
      break;
    }
    case ' ': case '\t': case '\r': case ',': {
      std::cout << "ignoring whitespace\n";
      input.consume1();
      break;
    }
    case '\'': {
      std::cout << "got quote\n";
      stack.emplace_back(Cell::createQuote(0));
      input.consume1();
      break;
    }
    case '(': {
      // beginning of a list
      std::cout << "got '('\n";
      stack.emplace_back(Cell::createList(0));
      input.consume1();
      break;
    }
    case ')': {
      // end of a list
      std::cout << "got ')'\n";
      if (stack.empty() || stack.back().container->type != Type::LIST) {
        set_error(Error::Code::UnbalancedGroup, "Unexpected ')'");
        return;
      }
      Frame& frame = stack.back();
      Cell* cell = frame.container;
      stack.pop_back();
      input.consume1();
      append_cell(cell);
      break;
    }
    // case '"': { ... future State::TEXT }
    default: {
      std::cout << "got char '" << *input.curr << "' -> State::SYM\n";
      buffer.push_back(*input.curr);
      input.consume1();
      state = State::SYM;
      break;
    }
  }
}

inline void Reader::read_symbol() {
  std::cout << "read_symbol()\n";
  switch (*input.curr) {
    case '\n': case '\r': case '\t': case ' ':
    case '(': case ')':
    case '\'': {
      // End of symbol
      const Str* str = intern_str(buffer.c_str());
      const Sym* sym = intern_sym(str);
      std::cout << "got symbol '" << sym << "'\n";
      append_cell(Cell::createSym(sym));
      buffer.clear();
      state = State::ROOT;
      break;
    }
    default: {
      buffer.push_back(*input.curr);
      input.consume1();
      break;
    }
  }
}

} // namespace lum
#endif // _LUM_READ_H_
