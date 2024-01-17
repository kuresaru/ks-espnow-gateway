//
// Created by kuresaru on 2023/12/14.
//

#include <string.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

void encrypt_aes(const uint8_t *key, const uint8_t *in, int in_len, uint8_t *out, int *out_len) {
    int flen;
    uint8_t iv[16];
    EVP_CIPHER_CTX *ctx;

    RAND_bytes(iv, 16);
    memcpy(out, iv, 16);

    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_set_padding(ctx, EVP_PADDING_PKCS7);
    EVP_EncryptUpdate(ctx, out + 16, out_len, in, in_len);
    EVP_EncryptFinal_ex(ctx, out + 16 + *out_len, &flen);
    *out_len += flen + 16;
    EVP_CIPHER_CTX_free(ctx);
}

void decrypt_aes(const uint8_t *key, const uint8_t *in, int in_len, uint8_t *out, int *out_len) {
    int flen;
    EVP_CIPHER_CTX *ctx;
    uint8_t iv[16];
    memcpy(iv, in, 16);
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_set_padding(ctx, EVP_PADDING_PKCS7);
    EVP_DecryptUpdate(ctx, out, out_len, in + 16, in_len - 16);
    EVP_DecryptFinal_ex(ctx, out + *out_len, &flen);
    *out_len += flen;
    EVP_CIPHER_CTX_free(ctx);
}

void calculate_md5(uint8_t *md5, const uint8_t *key, const uint8_t *data, int len) {
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, key, 16);
    MD5_Update(&ctx, data, len);
    MD5_Final(md5, &ctx);
}