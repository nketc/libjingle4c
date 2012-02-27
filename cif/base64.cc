#include <stdlib.h>
#include <string.h>
#include <string>
#include "talk/base/base64.h"
#include "cif/base64.h"

int base64_encoded_test(const char *text) {
  bool b = talk_base::Base64::IsBase64Encoded(std::string(text));
  return b ? 1 : 0;
}

char* base64_encode(const void *text, int len) {
  std::string encoded;
  talk_base::Base64::EncodeFromArray(text, len, &encoded);
  return strdup(encoded.c_str());
}

char* base64_encode_with_buffer(const char *text, int len, char *buffer, int *buflen) {
  std::string encoded;
  talk_base::Base64::EncodeFromArray(text, len, &encoded);
  if (*buflen >= (int)(encoded.length() + 1)) {
    strcpy(buffer, encoded.c_str());
    return buffer;
  }
  *buflen = encoded.length() + 1;
  return NULL;
}

char* base64_decode(const char *text, int *decoded_len) {
  std::string decoded;
  size_t used = 0;
  bool s = talk_base::Base64::DecodeFromArray(text, strlen(text),
                        talk_base::Base64::DO_STRICT, &decoded, &used);
  if (s) {
    *decoded_len = decoded.length();
    char *r = (char *)malloc(decoded.length());
    if (r)
      memcpy(r, decoded.data(), decoded.length());
    return r;
  }
  return NULL;
}

char* base64_decode_with_buffer(const char *text, char *buffer, int *buflen) {
  std::string decoded;
  size_t used = 0;
  bool s = talk_base::Base64::DecodeFromArray(text, strlen(text),
                        talk_base::Base64::DO_STRICT, &decoded, &used);
  if (s) {
    if (*buflen >= (int)decoded.length()) {
      memcpy(buffer, decoded.data(), decoded.length());
      *buflen = decoded.length();
      return buffer;
    } else {
      *buflen = decoded.length();
    }
  }
  return NULL;
}
