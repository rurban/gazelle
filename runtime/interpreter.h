/*********************************************************************

  Gazelle: a system for building fast, reusable parsers

  interpreter.h

  This file presents the public API for loading compiled grammars and
  parsing text using Gazelle.  There are a lot of structures, but they
  should all be considered read-only.

  Copyright (c) 2007-2008 Joshua Haberman.  See LICENSE for details.

*********************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include "bc_read_stream.h"
#include "dynarray.h"

#define GAZELLE_VERSION "0.3"
#define GAZELLE_WEBPAGE "http://www.reverberate.org/gazelle/"

/*
 * This group of structures are for storing a complete grammar in the form as
 * it is emitted from the compiler.  There are structures for each RTN, GLA,
 * and IntFA, states and transitions for each.
 */

/* Functions for loading a grammar from a bytecode file. */
struct grammar *load_grammar(struct bc_read_stream *s);
void free_grammar(struct grammar *g);

/*
 * RTN
 */

struct rtn_state;
struct rtn_transition;

struct rtn
{
    char *name;
    int num_slots;

    int num_states;
    struct rtn_state *states;  /* start state is first */

    int num_transitions;
    struct rtn_transition *transitions;
};

struct rtn_transition
{
    enum {
      TERMINAL_TRANSITION,
      NONTERM_TRANSITION,
    } transition_type;

    union {
      char            *terminal_name;
      struct rtn      *nonterminal;
    } edge;

    struct rtn_state *dest_state;
    char *slotname;
    int slotnum;
};

struct rtn_state
{
    bool is_final;

    enum {
      STATE_HAS_INTFA,
      STATE_HAS_GLA,
      STATE_HAS_NEITHER
    } lookahead_type;

    union {
      struct intfa *state_intfa;
      struct gla *state_gla;
    } d;

    int num_transitions;
    struct rtn_transition *transitions;
};

/*
 * GLA
 */

struct gla_state;
struct gla_transition;

struct gla
{
    int num_states;
    struct gla_state *states;   /* start state is first */

    int num_transitions;
    struct gla_transition *transitions;
};

struct gla_transition
{
    char *term;  /* if NULL, then the term is EOF */
    struct gla_state *dest_state;
};

struct gla_state
{
    bool is_final;

    union {
        struct nonfinal_info {
            struct intfa *intfa;
            int num_transitions;
            struct gla_transition *transitions;
        } nonfinal;

        struct final_info {
            int transition_offset; /* 1-based -- 0 is "return" */
        } final;
    } d;
};

/*
 * IntFA
 */

struct intfa_state;
struct intfa_transition;

struct intfa
{
    int num_states;
    struct intfa_state *states;    /* start state is first */

    int num_transitions;
    struct intfa_transition *transitions;
};

struct intfa_transition
{
    int ch_low;
    int ch_high;
    struct intfa_state *dest_state;
};

struct intfa_state
{
    char *final;  /* NULL if not final */
    int num_transitions;
    struct intfa_transition *transitions;
};

struct grammar
{
    char         **strings;

    int num_rtns;
    struct rtn   *rtns;

    int num_glas;
    struct gla   *glas;

    int num_intfas;
    struct intfa *intfas;
};

/*
 * runtime state
 */

struct terminal
{
    char *name;
    int offset;
    int len;
};

struct parse_val;

struct slotarray
{
    struct rtn *rtn;
    int num_slots;
    struct parse_val *slots;
};

struct parse_val
{
    enum {
      PARSE_VAL_EMPTY,
      PARSE_VAL_TERMINAL,
      PARSE_VAL_NONTERM,
      PARSE_VAL_USERDATA
    } type;

    union {
      struct terminal terminal;
      struct slotarray *nonterm;
      char userdata[8];
    } val;
};

/* This structure is the format for every stack frame of the parse stack. */
struct parse_stack_frame
{
    union {
      struct rtn_frame {
        struct rtn            *rtn;
        struct rtn_state      *rtn_state;
        struct rtn_transition *rtn_transition;
      } rtn_frame;

      struct gla_frame {
        struct gla            *gla;
        struct gla_state      *gla_state;
      } gla_frame;

      struct intfa_frame {
        struct intfa          *intfa;
        struct intfa_state    *intfa_state;
      } intfa_frame;
    } f;

    int start_offset;

    enum frame_type {
      FRAME_TYPE_RTN,
      FRAME_TYPE_GLA,
      FRAME_TYPE_INTFA
    } frame_type;
};

#define GET_PARSE_STACK_FRAME(ptr) \
    (struct parse_stack_frame*)((char*)ptr-offsetof(struct parse_stack_frame,f))

/* A bound_grammar struct represents a grammar which has had callbacks bound
 * to it and has possibly been JIT-compiled.  Though JIT compilation is not
 * supported yet, the APIs are in-place to anticipate this feature.
 *
 * At the moment you initialize a bound_grammar structure directly, but in the
 * future there will be a set of functions that do so, possibly doing JIT
 * compilation and other such things in the process. */

struct parse_state;
typedef void (*rule_callback_t)(struct parse_state *state);
typedef void (*terminal_callback_t)(struct parse_state *state,
                                    struct terminal *terminal);
typedef void (*error_char_callback_t)(struct parse_state *state,
                                      int ch);
typedef void (*error_terminal_callback_t)(struct parse_state *state,
                                          struct terminal *terminal);
struct bound_grammar
{
    struct grammar *grammar;
    terminal_callback_t terminal_cb;
    rule_callback_t start_rule_cb;
    rule_callback_t end_rule_cb;
    error_char_callback_t error_char_cb;
    error_terminal_callback_t error_terminal_cb;
};

/* This structure defines the core state of a parsing stream.  By saving this
 * state alone, we can resume a parse from the position where we left off.
 *
 * However, this state can only be resumed in the context of this process
 * with one particular bound_grammar.  To save it for loading into another
 * process, it must be serialized using a different API (which is not yet
 * written). */
struct parse_state
{
    /* The bound_grammar instance this state is being parsed with. */
    struct bound_grammar *bound_grammar;

    /* A pointer that the client can use for their own purposes. */
    void *user_data;

    /* The offset of the next byte in the stream we will process. */
    int offset;

    /* The offset of the beginning of the first terminal that has not yet been
     * yielded to the terminal callback.  This includes all terminals that are
     * currently being used for an as-yet-unresolved lookahead.
     *
     * Put another way, if a client wants to be able to go back to its input
     * buffer and look at the input data for a terminal that was just parsed,
     * it must not throw away any of the data before open_terminal_offset. */
    int open_terminal_offset;

    /* The parse stack is the main piece of state that the parser keeps.
     * There is a stack frame for every RTN, GLA, and IntFA state we are
     * currently in.
     *
     * TODO: The right input can make this grow arbitrarily, so we'll need
     * built-in limits to avoid infinite memory consumption. */
    DEFINE_DYNARRAY(parse_stack, struct parse_stack_frame);

    /* The token buffer stores tokens that have already been used to transition
     * the current GLA, but will be used to transition an RTN (and perhaps
     * other GLAs) when the current GLA hits a final state.  Keeping those
     * terminals here prevents us from having to re-lex them.
     *
     * TODO: If the grammar is LL(k) for fixed k, the token buffer will never
     * need to be longer than k elements long.  If the grammar is LL(*),
     * this can grow arbitrarily depending on the input, and we'll need
     * a way to clamp its maximum length to prevent infinite memory
     * consumption. */
    DEFINE_DYNARRAY(token_buffer, struct terminal);
};

/* Begin or continue a parse using grammar g, with the current state of the
 * parse represented by s.  It is expected that the text in buf represents the
 * input file or stream at offset s->offset.
 *
 * Return values:
 *  - PARSE_STATUS_OK: the entire buffer has been consumed successfully, and
 *    s represents the state of the parse as of the last byte of the buffer.
 *    You may continue parsing this file by calling parse() again with more
 *    data.
 *  - PARSE_STATUS_ERROR: there was a parse error in the input.  The parse
 *    state is as it immediately before the erroneous character or token was
 *    encountered, and can therefore be used again if desired to continue the
 *    parse from that point.  state->offset will reflect how far the parse
 *    proceeded before encountering the error.
 *  - PARSE_STATUS_CANCELLED: a callback that was called inside of parse()
 *    requested that parsing halt.  state is now invalid (this may change
 *    for the better in the future).
 *  - PARSE_STATUS_EOF: all or part of the buffer was parsed successfully,
 *    but a state was reached where no more characters could be accepted
 *    according to the grammar.  state->offset reflects how many characters
 *    were read before parsing reached this state.  The client should call
 *    finish_parse if it wants to receive final callbacks.
 */
enum parse_status {
  PARSE_STATUS_OK,
  PARSE_STATUS_ERROR,
  PARSE_STATUS_CANCELLED,
  PARSE_STATUS_EOF,

  /* The following errors are Only returned by clients using the parse_file
   * interface: */
  PARSE_STATUS_IO_ERROR,  /* Error reading the file, check errno. */
  PARSE_STATUS_PREMATURE_EOF_ERROR,  /* File hit EOF but the grammar wasn't EOF */
};
enum parse_status parse(struct parse_state *state, char *buf, int buf_len);

/* Call this function to complete the parse.  This primarily involves
 * calling all the final callbacks.  Will return false if the parse
 * state does not allow EOF here. */
bool finish_parse(struct parse_state *s);

struct parse_state *alloc_parse_state();
struct parse_state *dup_parse_state(struct parse_state *state);
void free_parse_state(struct parse_state *state);
void init_parse_state(struct parse_state *state, struct bound_grammar *bg);

/* A buffering layer provides the most common use case of parsing a whole file
 * by streaming from a FILE*.  This "struct buffer" will be the parse state's
 * user_data, the client's user_data is inside "struct buffer". */
struct buffer
{
    /* The buffer itself. */
    DEFINE_DYNARRAY(buf, char);

    /* The file offset of the first byte currently in the buffer. */
    int buf_offset;

    /* The number of bytes that have been successfully parsed. */
    int bytes_parsed;

    /* The user_data you passed to parse_file. */
    void *user_data;
};

enum parse_status parse_file(struct parse_state *state, FILE *file, void *user_data);

/*
 * Local Variables:
 * c-file-style: "bsd"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:et:sts=4:sw=4
 */
