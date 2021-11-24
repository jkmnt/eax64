#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "crypt64.h"

#include "vectors.h"

static struct
{
    uint32_t key[4];
} xtea_rt;

void xtea_install_key(const uint8_t *key)
{
    memcpy(xtea_rt.key, key, 16);
}


uint64_t xtea_ecb(uint64_t block)
{
   uint32_t sum = 0;
   uint32_t delta = 0x9E3779B9;

   uint32_t v0 = block;
   uint32_t v1 = block >> 32;

   for (int i = 0; i < 32; i++)
   {
       uint32_t r;

       r = ((v1<<4) ^ (v1>>5)) + v1;
       r ^= sum + xtea_rt.key[sum & 3];
       v0 += r;

       sum += delta;

       r = ((v0<<4) ^ (v0>>5)) + v0;
       r ^= sum + xtea_rt.key[(sum >> 11) & 3];
       v1 += r;
   }

   return ((uint64_t)v1 << 32) | v0;
}


extern void xtea_install_key(const uint8_t *key);
extern uint64_t xtea_ecb(uint64_t block);
extern void xtea_clear(void);

void print64(uint64_t q)
{
    printf("%08x%08x", (uint32_t)(q >> 32), (uint32_t)q);
}

void print_dump(const void *data, int len)
{
    const uint8_t *p = data;
    int col = 0;
    const int max_cols = 8;

    while (col < len)
    {
        if (!(col % max_cols))
            printf("\n%08x:", col);
        printf(" %02x", *p);
        p++;
        col++;
    }
    printf("\n");
}

static uint64_t xtea_enc(crypt64_cipher_t *ctx, uint64_t q)
{
    return xtea_ecb(q);
}

static void test_vector(const xtea_eax_testvector_t *v)
{
    crypt64_cipher_t enc =
    {
        .ecb = xtea_enc,
    };

    crypt64_eax_t ctx;

    xtea_install_key(v->key);

    crypt64_eax_init(&ctx, &enc, v->nonce, v->noncelen);

    uint8_t pt[256];

    for (int i = 0; i < v->headerlen; i++)
        crypt64_eax_auth_header(&ctx, v->header[i]);

    for (int i = 0; i < v->ctlen; i++)
        crypt64_eax_auth_ct(&ctx, v->ct[i]);

    for (int i = 0; i < v->ctlen; i++)
    {
        pt[i] = crypt64_eax_decrypt_ct(&ctx, i, v->ct[i]);
    }

    uint64_t local_tag = crypt64_eax_digest(&ctx);

    if (memcmp(pt, v->pt, v->ptlen) != 0)
    {
        print_dump(pt, v->ptlen);
        print_dump(v->pt, v->ptlen);
        printf("decrypt fail\n");
        exit(-1);
    }

    if (memcmp(&local_tag, v->tag, v->taglen) != 0)
    {
        print_dump(v->tag, v->taglen);
        print_dump(&local_tag, 8);
        printf("auth fail\n");
        exit(-1);
    }
}


int main(void)
{

    for (int i = 0; i < sizeof(xtea_eax_testvectors) / sizeof(xtea_eax_testvectors[0]); i++)
        test_vector(&xtea_eax_testvectors[i]);

    printf("Ok");
    return 0;
}
