/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

#include <string.h>
#include <glib.h>

#include "rsa.h"
#include "utils.h"

/* Forward compatibility functions if libssl < 1.1.0. */

#if OPENSSL_VERSION_NUMBER < 0x10100000L

int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d)
{
   /* If the fields n and e in r are NULL, the corresponding input
    * parameters MUST be non-NULL for n and e.  d may be
    * left NULL (in case only the public key is used).
    */
   if ((r->n == NULL && n == NULL)
       || (r->e == NULL && e == NULL))
       return 0;
   if (n != NULL) {
       BN_free(r->n);
       r->n = n;
   }
   if (e != NULL) {
       BN_free(r->e);
       r->e = e;
   }
   if (d != NULL) {
       BN_free(r->d);
       r->d = d;
   }
   return 1;
}

void RSA_get0_key(const RSA *r,
                 const BIGNUM **n, const BIGNUM **e, const BIGNUM **d)
{
   if (n != NULL)
       *n = r->n;
   if (e != NULL)
       *e = r->e;
   if (d != NULL)
       *d = r->d;
}

#endif

RSA*
private_key_to_pub(RSA *priv)
{
    RSA *pub = RSA_new();
    const BIGNUM *n, *e;

    RSA_get0_key (priv, &n, &e, NULL);
    RSA_set0_key (pub, BN_dup(n), BN_dup(e), NULL);

    return pub;
}


GString* public_key_to_gstring(const RSA *rsa)
{
    GString *buf = g_string_new(NULL);
    unsigned char *temp;
    char *coded;
    const BIGNUM *n, *e;

    RSA_get0_key (rsa, &n, &e, NULL);

    gsize len = BN_num_bytes(n);
    temp = malloc(len);
    BN_bn2bin(n, temp);
    coded = g_base64_encode(temp, len);
    g_string_append (buf, coded);
    g_string_append_c (buf, ' ');
    g_free(coded);
    
    len = BN_num_bytes(e);
    temp = realloc(temp, len);
    BN_bn2bin(e, temp);
    coded = g_base64_encode(temp, len);
    g_string_append (buf, coded);
    g_free(coded);

    free(temp);
    
    return buf;
}

void
public_key_append_to_gstring(const RSA *rsa, GString *buf)
{
    unsigned char *temp;
    char *coded;
    const BIGNUM *n, *e;

    RSA_get0_key (rsa, &n, &e, NULL);

    gsize len = BN_num_bytes(n);
    temp = malloc(len);
    BN_bn2bin(n, temp);
    coded = g_base64_encode(temp, len);
    g_string_append (buf, coded);
    g_string_append_c (buf, ' ');
    g_free(coded);
    
    len = BN_num_bytes(e);
    temp = realloc(temp, len);
    BN_bn2bin(e, temp);
    coded = g_base64_encode(temp, len);
    g_string_append (buf, coded);
    g_free(coded);

    free(temp);
}

RSA* public_key_from_string(char *str)
{
    char *p;
    unsigned char *num;
    gsize len;
    if (!str)
        return NULL;

    if ( !(p = strchr(str, ' ')) )
        return NULL;
    *p = '\0';

    RSA *key = RSA_new();
    BIGNUM *n = NULL, *e = NULL;

    num = g_base64_decode(str, &len);
    n = BN_bin2bn(num, len, NULL);
    if (!n)
        goto err;
    g_free(num);
    
    num = g_base64_decode(p+1, &len);
    e = BN_bin2bn(num, len, NULL);
    if (!e)
        goto err;
    g_free(num);

    RSA_set0_key (key, n, e, NULL);

    *p = ' ';
    return key;
err:
    *p = ' ';
    RSA_free (key);
    if (n)
        BN_free (n);
    if (e)
        BN_free (e);
    g_free(num);
    return NULL;
}

unsigned char *
private_key_decrypt(RSA *key, unsigned char *data, int len, int *decrypt_len)
{
    int size;
    unsigned char *buf;

    size = RSA_size(key);
    buf = g_malloc(size);
    *decrypt_len = RSA_private_decrypt(len, data, buf, key, RSA_PKCS1_PADDING);

    return buf;
}

unsigned char *
public_key_encrypt(RSA *key, unsigned char *data, int len, int *encrypt_len)
{
    int size;
    unsigned char *buf;

    size = RSA_size(key);
    buf = g_malloc(size);
    *encrypt_len = RSA_public_encrypt(len, data, buf, key, RSA_PKCS1_PADDING);

    return buf;
}

char *
id_from_pubkey (RSA *pubkey)
{
    GString *buf;
    unsigned char sha1[20];
    char *id = g_malloc(41);

    buf = public_key_to_gstring (pubkey);
    calculate_sha1 (sha1, buf->str);
    sha1_to_hex (sha1, id);
    g_string_free (buf, TRUE);

    return id;
}

RSA *
generate_private_key(u_int bits)
{
	RSA *private = NULL;

	private = RSA_generate_key(bits, 35, NULL, NULL);
	if (private == NULL)
		g_error ("rsa_generate_private_key: key generation failed.");
	return private;
}
