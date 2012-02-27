#ifndef _DEBUG_LOG_H__
#define _DEBUG_LOG_H__

#include "talk/base/sigslot.h"

class DebugLog : public sigslot::has_slots<> {
 public:
  DebugLog() :
    debug_input_buf_(NULL), debug_input_len_(0), debug_input_alloc_(0),
    debug_output_buf_(NULL), debug_output_len_(0), debug_output_alloc_(0),
    censor_password_(false)
      {}
  char * debug_input_buf_;
  int debug_input_len_;
  int debug_input_alloc_;
  char * debug_output_buf_;
  int debug_output_len_;
  int debug_output_alloc_;
  bool censor_password_;
  
  static bool IsAuthTag(const char * str, size_t len);

  void Input(const char * data, int len);
  void Output(const char * data, int len);
  void DebugPrint(char * buf, int * plen, bool output);
};

#endif

