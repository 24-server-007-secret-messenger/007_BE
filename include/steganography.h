#ifndef STEGANOGRAPHY_H
#define STEGANOGRAPHY_H

// JPEG 이미지에 암호화된 파일 숨기기
int embed_message_in_image(const char *input_image_path, const char *output_image_path, const char *message);

// JPEG 이미지에서 숨겨진 파일 추출하기
int extract_message_from_image(const char *image_path, char *output_message, size_t max_len);

#endif // STEGANOGRAPHY_H
