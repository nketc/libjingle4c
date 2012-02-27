#ifndef __CIF_BASE64_H__
#define __CIF_BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif

int base64_encoded_test(const char *text);
char* base64_encode(const void *text, int len);
/*buflen was used by both input and output. if buffer cannot cap for encoded
 *result, buflen was set that size.
 */
char* base64_encode_with_buffer(const char *text, int len, char *buffer, int *buflen);
char* base64_decode(const char *text, int *decoded_len);
/*buflen was used by both input and output. if sucess decoded length was set to buflen,
 *and buffer was return. if fail, NULL was return, can check buflen to identify whether
 *decode fail or buffer cannot cap.
 */
char* base64_decode_with_buffer(const char *text, char *buffer, int *buflen);

#ifdef __cplusplus
}
#endif

#endif