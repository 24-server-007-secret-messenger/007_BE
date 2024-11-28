#ifndef STEGANOGRAPHY_H
#define STEGANOGRAPHY_H

// JPEG 이미지에 암호화된 파일 숨기기
void hide_message_in_jpeg(const char *input_jpeg, const char *output_jpeg, const char *file_to_hide);

// JPEG 이미지에서 숨겨진 파일 추출하기
void extract_message_from_jpeg(const char *input_jpeg, const char *output_file);

#endif // STEGANOGRAPHY_H
