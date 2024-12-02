#ifndef BASE64_H
#define BASE64_H

char* base64_encode(const unsigned char* buffer, size_t length);
char* encode_file_to_base64(const char* filepath);

#endif // BASE64_H
