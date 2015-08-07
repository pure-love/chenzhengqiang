/*
*@author:chenzhengqiang
*@company:swwy
*@start-date:2015/6/25
*@modified-date:
*@desc:simple implementation of AES's encrypt and decrypt using openssl
*/

#include "aes.h"
#include <cstdlib>
#include <openssl/aes.h>
#include <cstring>
#include <cstdio>
#include <stdint.h>

/*
*#args:
*#returns:void
*#desc:simple algorithm to generate the aes key
*/
void generate_simple_aes_key( unsigned char *aes_key_buf, size_t buf_size )
{   
    if( buf_size != AES_BLOCK_SIZE )
    {
        exit(EXIT_FAILURE);
    }
    
    for ( size_t index=0; index < buf_size; ++index ) 
    {
        aes_key_buf[index] = 98 + index%2+1;
    }
}


/*
*#args:
*#returns:void
*#desc:as the function name described,generate the aes encrypt string using openssl
*/
void generate_aes_encrypt_string(unsigned char *encrypt_string,const char *src_string)
{
    AES_KEY aes;
    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE];        // in
    unsigned char aes_input_string[FIXED_AES_ENCRYPT_SIZE];
    memset(aes_input_string,0,sizeof(aes_input_string));
    memset(iv,0,sizeof(iv));
    strncpy((char*)aes_input_string, src_string, strlen(src_string));
    // generate simple AES 128-bit key
    generate_simple_aes_key(key,AES_BLOCK_SIZE);
    // Set encryption key
    if (AES_set_encrypt_key(key, AES_ENDCRYPT_SIZE, &aes) < 0) 
    {
        fprintf(stderr, "failed to set encryption key in AES\n");
        exit(EXIT_FAILURE);
    }
    AES_cbc_encrypt(aes_input_string, encrypt_string, FIXED_AES_ENCRYPT_SIZE, &aes, iv, AES_ENCRYPT);
}


/*
*#args:
*#returns:void
*#desc:as the function name described,obtain the aes decrypt string using openssl
*/
void obtain_aes_decrypt_string(unsigned char *decrypt_string, const uint8_t *encrypt_string )
{
    AES_KEY aes;
    unsigned char key[AES_BLOCK_SIZE];
    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv,0,sizeof(iv));
    // generate the simple AES 128-bit key
    generate_simple_aes_key(key,AES_BLOCK_SIZE);
    if (AES_set_decrypt_key(key, AES_ENDCRYPT_SIZE, &aes) < 0) 
    {
        fprintf(stderr, "failed to set decryption key in AES\n");
        exit(EXIT_FAILURE);
    }
    AES_cbc_encrypt(encrypt_string, decrypt_string, FIXED_AES_ENCRYPT_SIZE, &aes, iv, 
            AES_DECRYPT);	
}



void print_aes_encrypt_string(const unsigned char *encrypt_string, size_t encrypt_size)
{
    for (size_t index =0; index < encrypt_size; ++index ) 
    {
            printf("%x%x", (encrypt_string[index] >> 4 ) & 0xf, encrypt_string[index] & 0xf);    
    }
    printf("\n");
}

