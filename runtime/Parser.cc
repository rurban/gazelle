#include <gazelle/Parser.hh>
#include <gazelle/Grammar.hh>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

using namespace gazelle;

static void will_start_rule_callback(struct gzl_parse_state *state,
                                     struct gzl_rtn *rtn,
                                     struct gzl_offset *start_offset) {
  ((Parser*)state->user_data)->onWillStartRule(rtn, rtn->name, start_offset);
}

static void did_start_rule_callback(struct gzl_parse_state *state) {
  gzl_parse_stack_frame *frame = DYNARRAY_GET_TOP(state->parse_stack);
  assert(frame->frame_type == gzl_parse_stack_frame::GZL_FRAME_TYPE_RTN);
  gzl_rtn_frame *rtn_frame = &frame->f.rtn_frame;
  ((Parser*)state->user_data)->onDidStartRule(rtn_frame, rtn_frame->rtn->name);
}

static void will_end_rule_callback(struct gzl_parse_state *state) {
  gzl_parse_stack_frame *frame = DYNARRAY_GET_TOP(state->parse_stack);
  assert(frame->frame_type == gzl_parse_stack_frame::GZL_FRAME_TYPE_RTN);
  gzl_rtn_frame *rtn_frame = &frame->f.rtn_frame;
  ((Parser*)state->user_data)->onWillEndRule(rtn_frame, rtn_frame->rtn->name);
}

static void did_end_rule_callback(struct gzl_parse_state *state,
                                  struct gzl_parse_stack_frame *frame) {
  assert(frame->frame_type == gzl_parse_stack_frame::GZL_FRAME_TYPE_RTN);
  gzl_rtn_frame *rtn_frame = &frame->f.rtn_frame;
  ((Parser*)state->user_data)->onDidEndRule(rtn_frame, rtn_frame->rtn->name);
}

static void terminal_callback(struct gzl_parse_state *state,
                              struct gzl_terminal *terminal) {
  ((Parser*)state->user_data)->onTerminal(terminal);
}

static void error_unknown_trans_callback(struct gzl_parse_state *state, int ch) {
  ((Parser*)state->user_data)->onUnknownTransitionError(ch);
}

static void error_terminal_callback(struct gzl_parse_state *state,
                                    struct gzl_terminal *terminal) {
  ((Parser*)state->user_data)->onUnexpectedTerminalError(terminal);
}


Parser::Parser(Grammar *grammar) {
  setGrammar(grammar);
}


Parser::~Parser() {
  if (state_)
    gzl_free_parse_state(state_);
}


void Parser::setGrammar(Grammar *grammar) {
  state_ = gzl_alloc_parse_state();
  assert(state_ != NULL);
  state_->user_data = (void*)this;
  // setup bound grammar
  boundGrammar_.terminal_cb = terminal_callback;
  boundGrammar_.will_start_rule_cb = will_start_rule_callback;
  boundGrammar_.did_start_rule_cb = did_start_rule_callback;
  boundGrammar_.will_end_rule_cb = will_end_rule_callback;
  boundGrammar_.did_end_rule_cb = did_end_rule_callback;
  boundGrammar_.error_char_cb = error_unknown_trans_callback;
  boundGrammar_.error_terminal_cb = error_terminal_callback;
  gzl_init_parse_state(state_, &boundGrammar_);
  boundGrammar_.grammar = grammar ? grammar->grammar() : NULL;
}


// Parse a chunk of text. Note that the text need to begin with a valid token
gzl_status Parser::parse(const char *source, size_t len, bool finalize) {
  if (!boundGrammar_.grammar)
    return GZL_STATUS_BAD_GRAMMAR;
  if (len == 0)
    len = strlen(source);
  gzl_status status = gzl_parse(state_, source, len);
  if (finalize && (status == GZL_STATUS_HARD_EOF || status == GZL_STATUS_OK) ) {
    if (!finalizeParsing())
      status = GZL_STATUS_PREMATURE_EOF_ERROR;
  }
  return status;
}


bool Parser::finalizeParsing() {
  /* Call this function to complete the parse.  This primarily involves
   * calling all the final callbacks.  Will return false if the parse
   * state does not allow EOF here. */
  return gzl_finish_parse(state_);
}


// Convenience method to parse the complete |file|
gzl_status Parser::parseFile(FILE *file) {
  // TODO implementation
  return GZL_STATUS_IO_ERROR;
}
