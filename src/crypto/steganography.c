#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "steganography.h"

// 메시지 삽입 함수
int embed_message_in_image(const char *input_image_path, const char *output_image_path, const char *message) {
    // printf("input_image_path: %s\n", input_image_path);
    // printf("output_image_path: %s\n", output_image_path);
    // printf("message: %s\n", message);
    
    // 입력 파일 열기
    FILE *input_image = fopen(input_image_path, "rb");
    if (!input_image) {
        perror("Failed to open input image file");
        return -1;
    }

    // 출력 파일 열기
    FILE *output_image = fopen(output_image_path, "wb");
    if (!output_image) {
        perror("Failed to open output image file");
        fclose(input_image);
        return -1;
    }

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, input_image);
    fwrite(header, sizeof(unsigned char), 54, output_image);

    size_t message_len = strlen(message) + 1;

    unsigned char buffer;
    size_t bit_index = 0;
    size_t message_index = 0;

    while (fread(&buffer, sizeof(unsigned char), 1, input_image)) {
        if (message_index < message_len) {
            buffer = (buffer & ~1) | ((message[message_index] >> bit_index) & 1);
            bit_index++;
            if (bit_index == 8) {
                bit_index = 0;
                message_index++;
            }
        }
        fwrite(&buffer, sizeof(unsigned char), 1, output_image);
    }

    fclose(input_image);
    fclose(output_image);

    return 0;
}

// 메시지 추출 함수
int extract_message_from_image(const char *image_path, char *output_message, size_t max_len) {
    printf("image_path: %s\n", image_path);

    FILE *image = fopen(image_path, "rb");
    if (!image) {
        perror("Failed to open image file");
        return -1;
    }

    // Skip the header (54 bytes for BMP images)
    fseek(image, 54, SEEK_SET);

    unsigned char buffer;
    size_t bit_index = 0;
    size_t message_index = 0;
    output_message[0] = '\0';

    // Read bits from the image and reconstruct the message
    while (fread(&buffer, sizeof(unsigned char), 1, image) && message_index < max_len - 1) {
        output_message[message_index] |= (buffer & 1) << bit_index;
        bit_index++;
        if (bit_index == 8) {
            if (output_message[message_index] == '\0') {
                break; // End of the message
            }
            bit_index = 0;
            message_index++;
            output_message[message_index] = '\0';
        }
    }

    fclose(image);

    return 0;
}
