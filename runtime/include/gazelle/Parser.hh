#ifndef GAZELLE_CXX_PARSER_H_
#define GAZELLE_CXX_PARSER_H_

#include <gazelle/parse.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

namespace gazelle {
class Grammar;

/**
 * A stateful Gazelle parser
 *
 * Example:
 *
 *    gazelle::Parser parser;
 *    gazelle::Grammar grammar;
 *    if (!grammar.loadFile("./json.gzc"))
 *      exit(1);
 *    parser.setGrammar(&grammar);
 *    gzl_status status = parser.parse("the text to parse");
 *
 */
class Parser {
 public:
  // Creates a new parser optionally bound to a grammar
  Parser(Grammar *grammar=NULL);
  virtual ~Parser();

  // Set the grammar which should be used for the next call to parse
  void setGrammar(Grammar *grammar);

  // A structure which contains the current state (see parse.h for details)
  inline gzl_parse_state *state() { return state_; }
  inline void setState(gzl_parse_state *state) {
    if (state_) gzl_free_parse_state(state_);
    state_ = state;
  }
  inline gzl_parse_state *swapState(gzl_parse_state *state) {
    gzl_parse_state *prevState = state_;
    state_ = state;
    return prevState;
  }

  // Parse a chunk of text. Note that the text need to begin with a valid token.
  // If |finalize| is true, finalizeParsing is called after successfully parsing
  // |source| (a convenience feature).
  gzl_status parse(const char *source, size_t len=0, bool finalize=false);

  // Complete the parsing. This primarily involves calling all the final
  // callbacks. Returns false if the parse state does not allow EOF here.
  bool finalizeParsing();

  // Convenience method to parse the complete |file|
  gzl_status parseFile(FILE *file);

  // Retrieve a stack frame |offset| levels down
  inline gzl_parse_stack_frame *stackFrameAt(int offset) {
    if (offset < 0 || offset >= state_->parse_stack_len)
      return NULL;
    return &(state_->parse_stack[(state_->parse_stack_len - 1) - offset]);
  }

  // The top ("latest") frame in the stack
  inline gzl_parse_stack_frame *currentStackFrame() {
    return stackFrameAt(0);
  }

  // Current stack depth
  inline int stackDepth() { return state_->parse_stack_len; }

  // Current source line number (starts at 1)
  inline size_t line() { return state_->offset.line; }
  // Current source column number (starts at 1)
  inline size_t column() { return state_->offset.column; }
  // Current source byte offset
  inline size_t offset() { return state_->offset.byte; }

  // ---- parser events ----
  // These methods are called during parsing by the parser machine

  // Invoked when a rule starts
  virtual void onWillStartRule(gzl_rtn *frame,
                               const char *name,
                               gzl_offset *offset) {}
  virtual void onDidStartRule(gzl_rtn_frame *frame, const char *name) {}

  virtual void onEndRule(gzl_rtn_frame *frame, const char *name) {}

  virtual void onTerminal(gzl_terminal *terminal) {}

  virtual void onUnknownTransitionError(int ch) {}

  virtual void onUnexpectedTerminalError(gzl_terminal *terminal) {}

 protected:
  // Grammar and callbacks struct needed for the Gazelle API
  gzl_bound_grammar boundGrammar_;

  // The Gazelle parse state
  gzl_parse_state *state_;
};


}  // namespace gazelle
#endif  // GAZELLE_CXX_PARSER_H_
