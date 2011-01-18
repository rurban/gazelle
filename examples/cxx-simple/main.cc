/*
A simple example of using the C++ interface of the runtime library.

First, compile the grammar:
gzlc json.gzl

Then build this program:
c++ -o main -I../../runtime/include -L../../runtime -lgazelle main.cc && ./main

And run it:
./main

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <err.h>
#include <string>

#include <gazelle/Parser.hh>
#include <gazelle/Grammar.hh>

#define DLOG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__ );
const char *statusstr(gzl_status status);
inline static int mymin(int a, int b) { return (a < b) ? a : b; }

// our parser subclass
class MyParser : public gazelle::Parser {
  const char *source_;
  size_t sourceLen_;
 public:
  void setSource(const char *source, size_t len) {
    source_ = source;
    sourceLen_ = len;
  }
  
  int indentLevel() { return (stackDepth()-1)*2; }

  void onStartRule(gzl_rtn_frame *frame, const char *name) {
    DLOG("%*sonStartRule: \"%s\"", indentLevel(), "", name);
  }

  void onEndRule(gzl_rtn_frame *frame, const char *name) {
    DLOG("%*sonEndRule: \"%s\"", indentLevel(), "", name);
  }

  void onTerminal(gzl_terminal *terminal) {
    DLOG("%*sonTerminal: \"%s\"", indentLevel(), "", terminal->name);
  }

  void onUnknownTransitionError(int ch) {
    DLOG("%*sonUnknownTransitionError: from character '%c' at input:%zu:%zu[%zu]",
         indentLevel(), "", ch, line(), column(), offset());
  }

  void onUnexpectedTerminalError(gzl_terminal *terminal) {
    // extract source line where the error occured
    size_t start_offset = terminal->offset.byte -
                          mymin(30,terminal->offset.byte);
    while (start_offset < terminal->offset.byte &&
           source_[start_offset++] != '\n');
    const char *p = source_ + start_offset;
    size_t len, end = terminal->offset.byte;
    while (1) {
      if (end == sourceLen_) {
        len = mymin(sourceLen_ - start_offset, 60);
        break;
      } else if (source_[++end] == '\n') {
        len = mymin(end-start_offset, 60);
        break;
      }
    }
    int error_offset = terminal->offset.byte - start_offset;
    char *source_line = (char*)malloc((len+1)*sizeof(char));
    memcpy(source_line, p, len);
    DLOG("error: unexpected terminal '%s' -- aborting (input:%zu:%zu[%zu])\n"
         "  %s\n"
         "  %*s",
         terminal->name,
         terminal->offset.line, terminal->offset.column, terminal->offset.byte,
         source_line,
         error_offset, "^");
    free(source_line);
  }
};


int main(int argc, char *argv[]) {
  MyParser parser;
  gazelle::Grammar grammar;
  if (!grammar.loadFile("./json.gzc"))
    return 1;
  parser.setGrammar(&grammar);

  // load source from a file
  FILE *f = fopen("input.json", "r");
  std::string source1;
  char buf[4097];
  while (!feof(f)) {
    fread(buf, 1, 4096, f);
    source1.append(buf);
  }
  fclose(f);

  // keep a reference to the complete source so we can print pretty errors
  const char *source = source1.c_str();
  parser.setSource(source, source1.size());

  // parse complete source
  gzl_status status = parser.parse(source, 0, true);

  DLOG("status: %s", statusstr(status));
  return status == GZL_STATUS_OK ? 0 : 1;
}


// return a string representation of a gzl status code
const char *statusstr(gzl_status status) {
  switch (status) {
    case GZL_STATUS_OK: return "GZL_STATUS_OK";
    case GZL_STATUS_ERROR: return "GZL_STATUS_ERROR";
    case GZL_STATUS_CANCELLED: return "GZL_STATUS_CANCELLED";
    case GZL_STATUS_HARD_EOF: return "GZL_STATUS_HARD_EOF";
    case GZL_STATUS_RESOURCE_LIMIT_EXCEEDED:
      return "GZL_STATUS_RESOURCE_LIMIT_EXCEEDED";
    case GZL_STATUS_IO_ERROR: return "GZL_STATUS_IO_ERROR";
    case GZL_STATUS_PREMATURE_EOF_ERROR:
      return "GZL_STATUS_PREMATURE_EOF_ERROR";
    default: return "unknown";
  }
}
