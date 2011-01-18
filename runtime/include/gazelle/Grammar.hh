#ifndef GAZELLE_CXX_GRAMMAR_H_
#define GAZELLE_CXX_GRAMMAR_H_

#include <stdlib.h>

struct gzl_grammar;
struct bc_read_stream;

namespace gazelle {

/**
 * Represents a language grammar
 */
class Grammar {
 public:
  // Construct a new uninitialized grammar with optional name
  explicit Grammar(const char *name=NULL);
  virtual ~Grammar();

  // The gzl_grammar struct (or NULL if not yet initialized)
  inline gzl_grammar *grammar() { return grammar_; }

  // Name of this grammar (or NULL if not named)
  inline const char *name() { return name_; }

  // Load grammar definition from .gzc file at |path|. Returns true on success.
  bool loadFile(const char *path);

  // Load grammar definition from BitCode data. Returns true on success.
  bool loadData(const void *data, size_t len=0);

  // Load grammar definition from BitCode input stream. Returns true on success.
  bool loadBitCodeStream(bc_read_stream *stream, bool closeStream=false);

 protected:
  gzl_grammar *grammar_;
  char *name_;
};

}  // namespace gazelle
#endif  // GAZELLE_CXX_GRAMMAR_H_
