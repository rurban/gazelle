#include <gazelle/Grammar.hh>
#include <gazelle/grammar.h>
#include <gazelle/bc_read_stream.h>
#include <exception>
#include <string.h>

using namespace gazelle;

// Construct a new uninitialized grammar with optional name
Grammar::Grammar(const char *name) : grammar_(NULL) {
  name_ = name ? strdup(name) : NULL;
}

Grammar::~Grammar() {
  if (name_) {
    free(name_);
    name_ = NULL;
  }
  if (grammar_) {
    gzl_free_grammar(grammar_);
    grammar_ = NULL;
  }
}


bool Grammar::loadFile(const char *path) {
  bc_read_stream *stream = bc_rs_open_file(path);
  if (!stream)
    return false;
  return loadBitCodeStream(stream, true);
}


bool Grammar::loadData(const void *data, size_t len) {
  // Note: bc_rs_open_mem does not (yet) support length
  bc_read_stream *stream = bc_rs_open_mem((const char*)data);
  if (!stream)
    return false;
  return loadBitCodeStream(stream, true);
}


bool Grammar::loadBitCodeStream(bc_read_stream *stream, bool closeStream) {
  if (grammar_)
    gzl_free_grammar(grammar_);
  grammar_ = gzl_load_grammar(stream);
  if (closeStream)
    bc_rs_close_stream(stream);
  // Note: the current implementation of gzl_load_grammar either succeeds or
  // calls exit() with a value of 1. Ugly but true.
  return !!grammar_;
}
