/*
*#author:chenzhengqiang
*#start-date:2015/6/25
*#desc:simple AES encrypt,decrypt using openssl
*/
#ifndef _C_AES_H_
#define _C_AES_H_
#include<sys/types.h>
#include<stdint.h>
static  const size_t FIXED_AES_ENCRYPT_SIZE=64;
static  const size_t FIXED_AES_DECRYPT_SIZE=64;

static const int AES_ENDCRYPT_SIZE=128;

void generate_simple_aes_key( unsigned char *aes_key_buf, size_t buf_size );
void generate_aes_encrypt_string(unsigned char *encrypt_string,const char *src_string);
void obtain_aes_decrypt_string(unsigned char *decrypt_string, const uint8_t *encrypt_string );
void print_aes_encrypt_string(const unsigned char *encrypt_string, size_t encrypt_size);
#endif
