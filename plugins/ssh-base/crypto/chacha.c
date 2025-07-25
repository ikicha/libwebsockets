/*
chacha-merged.c version 20080118
D. J. Bernstein
Public domain.
*/

#include <libwebsockets.h>
#include "lws-ssh.h"

#include <string.h>
#include <stdlib.h>

struct chacha_ctx {
	u_int input[16];
};

#define CHACHA_MINKEYLEN 	16
#define CHACHA_NONCELEN		8
#define CHACHA_CTRLEN		8
#define CHACHA_STATELEN		(CHACHA_NONCELEN+CHACHA_CTRLEN)
#define CHACHA_BLOCKLEN		64

typedef unsigned char u8;
typedef unsigned int u32;

typedef struct chacha_ctx chacha_ctx;

#define U8C(v) (v##U)
#define U32C(v) (v##U)

#define U8V(v) ((u8)((v) & U8C(0xFF)))
#define U32V(v) ((u32)(v) & U32C(0xFFFFFFFF))

#define ROTL32(v, n) \
  (U32V((v) << (n)) | ((v) >> (32 - (n))))

#define U8TO32_LITTLE(p) \
  (((u32)((p)[0])      ) | \
   ((u32)((p)[1]) <<  8) | \
   ((u32)((p)[2]) << 16) | \
   ((u32)((p)[3]) << 24))

#define U32TO8_LITTLE(p, v) \
  do { \
    (p)[0] = U8V((v)      ); \
    (p)[1] = U8V((v) >>  8); \
    (p)[2] = U8V((v) >> 16); \
    (p)[3] = U8V((v) >> 24); \
  } while (0)

#define ROTATE(v,c) (ROTL32(v,c))
#define XOR(v,w) ((v) ^ (w))
#define PLUS(v,w) (U32V((v) + (w)))
#define PLUSONE(v) (PLUS((v),1))

#define QUARTERROUND(a,b,c,d) \
  a = PLUS(a,b); d = ROTATE(XOR(d,a),16); \
  c = PLUS(c,d); b = ROTATE(XOR(b,c),12); \
  a = PLUS(a,b); d = ROTATE(XOR(d,a), 8); \
  c = PLUS(c,d); b = ROTATE(XOR(b,c), 7);

static const char sigma[17] = "expand 32-byte k";
static const char tau[17] = "expand 16-byte k";

void
chacha_keysetup(chacha_ctx *x,const u8 *k,u32 kbits)
{
  const char *constants;

  x->input[4] = U8TO32_LITTLE(k + 0);
  x->input[5] = U8TO32_LITTLE(k + 4);
  x->input[6] = U8TO32_LITTLE(k + 8);
  x->input[7] = U8TO32_LITTLE(k + 12);
  if (kbits == 256) { /* recommended */
    k += 16;
    constants = sigma;
  } else { /* kbits == 128 */
    constants = tau;
  }
  x->input[8] = U8TO32_LITTLE(k + 0);
  x->input[9] = U8TO32_LITTLE(k + 4);
  x->input[10] = U8TO32_LITTLE(k + 8);
  x->input[11] = U8TO32_LITTLE(k + 12);
  x->input[0] = U8TO32_LITTLE(constants + 0);
  x->input[1] = U8TO32_LITTLE(constants + 4);
  x->input[2] = U8TO32_LITTLE(constants + 8);
  x->input[3] = U8TO32_LITTLE(constants + 12);
}

void
chacha_ivsetup(chacha_ctx *x, const u8 *iv, const u8 *counter)
{
  x->input[12] = counter == NULL ? 0 : U8TO32_LITTLE(counter + 0);
  x->input[13] = counter == NULL ? 0 : U8TO32_LITTLE(counter + 4);
  x->input[14] = U8TO32_LITTLE(iv + 0);
  x->input[15] = U8TO32_LITTLE(iv + 4);
}

void
chacha_encrypt_bytes(chacha_ctx *x,const u8 *m,u8 *c,u32 bytes)
{
  u32 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
  u32 j0, j1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14, j15;
  u8 *ctarget = NULL;
  u8 tmp[64];
  u_int i;

  if (!bytes) return;

  j0 = x->input[0];
  j1 = x->input[1];
  j2 = x->input[2];
  j3 = x->input[3];
  j4 = x->input[4];
  j5 = x->input[5];
  j6 = x->input[6];
  j7 = x->input[7];
  j8 = x->input[8];
  j9 = x->input[9];
  j10 = x->input[10];
  j11 = x->input[11];
  j12 = x->input[12];
  j13 = x->input[13];
  j14 = x->input[14];
  j15 = x->input[15];

  for (;;) {
    if (bytes < 64) {
      for (i = 0;i < bytes;++i) tmp[i] = m[i];
      m = tmp;
      ctarget = c;
      c = tmp;
    }
    x0 = j0;
    x1 = j1;
    x2 = j2;
    x3 = j3;
    x4 = j4;
    x5 = j5;
    x6 = j6;
    x7 = j7;
    x8 = j8;
    x9 = j9;
    x10 = j10;
    x11 = j11;
    x12 = j12;
    x13 = j13;
    x14 = j14;
    x15 = j15;
    for (i = 20;i > 0;i -= 2) {
      QUARTERROUND( x0, x4, x8,x12)
      QUARTERROUND( x1, x5, x9,x13)
      QUARTERROUND( x2, x6,x10,x14)
      QUARTERROUND( x3, x7,x11,x15)
      QUARTERROUND( x0, x5,x10,x15)
      QUARTERROUND( x1, x6,x11,x12)
      QUARTERROUND( x2, x7, x8,x13)
      QUARTERROUND( x3, x4, x9,x14)
    }
    x0 = PLUS(x0,j0);
    x1 = PLUS(x1,j1);
    x2 = PLUS(x2,j2);
    x3 = PLUS(x3,j3);
    x4 = PLUS(x4,j4);
    x5 = PLUS(x5,j5);
    x6 = PLUS(x6,j6);
    x7 = PLUS(x7,j7);
    x8 = PLUS(x8,j8);
    x9 = PLUS(x9,j9);
    x10 = PLUS(x10,j10);
    x11 = PLUS(x11,j11);
    x12 = PLUS(x12,j12);
    x13 = PLUS(x13,j13);
    x14 = PLUS(x14,j14);
    x15 = PLUS(x15,j15);

    x0 = XOR(x0,U8TO32_LITTLE(m + 0));
    x1 = XOR(x1,U8TO32_LITTLE(m + 4));
    x2 = XOR(x2,U8TO32_LITTLE(m + 8));
    x3 = XOR(x3,U8TO32_LITTLE(m + 12));
    x4 = XOR(x4,U8TO32_LITTLE(m + 16));
    x5 = XOR(x5,U8TO32_LITTLE(m + 20));
    x6 = XOR(x6,U8TO32_LITTLE(m + 24));
    x7 = XOR(x7,U8TO32_LITTLE(m + 28));
    x8 = XOR(x8,U8TO32_LITTLE(m + 32));
    x9 = XOR(x9,U8TO32_LITTLE(m + 36));
    x10 = XOR(x10,U8TO32_LITTLE(m + 40));
    x11 = XOR(x11,U8TO32_LITTLE(m + 44));
    x12 = XOR(x12,U8TO32_LITTLE(m + 48));
    x13 = XOR(x13,U8TO32_LITTLE(m + 52));
    x14 = XOR(x14,U8TO32_LITTLE(m + 56));
    x15 = XOR(x15,U8TO32_LITTLE(m + 60));

    j12 = PLUSONE(j12);
    if (!j12)
      j13 = PLUSONE(j13);
      /* stopping at 2^70 bytes per nonce is user's responsibility */

    U32TO8_LITTLE(c + 0,x0);
    U32TO8_LITTLE(c + 4,x1);
    U32TO8_LITTLE(c + 8,x2);
    U32TO8_LITTLE(c + 12,x3);
    U32TO8_LITTLE(c + 16,x4);
    U32TO8_LITTLE(c + 20,x5);
    U32TO8_LITTLE(c + 24,x6);
    U32TO8_LITTLE(c + 28,x7);
    U32TO8_LITTLE(c + 32,x8);
    U32TO8_LITTLE(c + 36,x9);
    U32TO8_LITTLE(c + 40,x10);
    U32TO8_LITTLE(c + 44,x11);
    U32TO8_LITTLE(c + 48,x12);
    U32TO8_LITTLE(c + 52,x13);
    U32TO8_LITTLE(c + 56,x14);
    U32TO8_LITTLE(c + 60,x15);

    if (bytes <= 64) {
      if (bytes < 64) {
        for (i = 0;i < bytes;++i) ctarget[i] = c[i];
      }
      x->input[12] = j12;
      x->input[13] = j13;
      return;
    }
    bytes -= 64;
    c += 64;
    m += 64;
  }
}

struct lws_cipher_chacha {
	struct chacha_ctx ccctx[2];
};

#define K_1(_keys) &((struct lws_cipher_chacha *)_keys->cipher)->ccctx[0]
#define K_2(_keys) &((struct lws_cipher_chacha *)_keys->cipher)->ccctx[1]

int
lws_chacha_activate(struct lws_ssh_keys *keys)
{
	if (keys->cipher) {
		free(keys->cipher);
		keys->cipher = NULL;
	}

	keys->cipher = malloc(sizeof(struct lws_cipher_chacha));
	if (!keys->cipher)
		return 1;

	memset(keys->cipher, 0, sizeof(struct lws_cipher_chacha));

	/* uses 2 x 256-bit keys, so 512 bits (64 bytes) needed */
	chacha_keysetup(K_2(keys), keys->key[SSH_KEYIDX_ENC], 256);
	chacha_keysetup(K_1(keys), &keys->key[SSH_KEYIDX_ENC][32], 256);

	keys->valid = 1;
	keys->full_length = 1;
	keys->padding_alignment = 8; // CHACHA_BLOCKLEN;
	keys->MAC_length = POLY1305_TAGLEN;

	return 0;
}

void
lws_chacha_destroy(struct lws_ssh_keys *keys)
{
	if (keys->cipher) {
		free(keys->cipher);
		keys->cipher = NULL;
	}
}

uint32_t
lws_chachapoly_get_length(struct lws_ssh_keys *keys, uint32_t seq,
			  const uint8_t *in4)
{
        uint8_t buf[4], seqbuf[8];

	/*
	 * When receiving a packet, the length must be decrypted first.  When 4
	 * bytes of ciphertext length have been received, they may be decrypted
	 * using the K_1 key, a nonce consisting of the packet sequence number
	 * encoded as a uint64 under the usual SSH wire encoding and a zero
	 * block counter to obtain the plaintext length.
	 */
        POKE_U64(seqbuf, seq);
	chacha_ivsetup(K_1(keys), seqbuf, NULL);
        chacha_encrypt_bytes(K_1(keys), in4, buf, 4);

	return PEEK_U32(buf);
}

/*
 * chachapoly_crypt() operates as following:
 * En/decrypt with header key 'aadlen' bytes from 'src', storing result
 * to 'dest'. The ciphertext here is treated as additional authenticated
 * data for MAC calculation.
 * En/decrypt 'len' bytes at offset 'aadlen' from 'src' to 'dest'. Use
 * POLY1305_TAGLEN bytes at offset 'len'+'aadlen' as the authentication
 * tag. This tag is written on encryption and verified on decryption.
 */
int
chachapoly_crypt(struct lws_ssh_keys *keys, u_int seqnr, u_char *dest,
    const u_char *src, u_int len, u_int aadlen, u_int authlen, int do_encrypt)
{
        u_char seqbuf[8];
        const u_char one[8] = { 1, 0, 0, 0, 0, 0, 0, 0 }; /* NB little-endian */
        u_char expected_tag[POLY1305_TAGLEN], poly_key[POLY1305_KEYLEN];
        int r = 1;

        /*
         * Run ChaCha20 once to generate the Poly1305 key. The IV is the
         * packet sequence number.
         */
        memset(poly_key, 0, sizeof(poly_key));
        POKE_U64(seqbuf, seqnr);
        chacha_ivsetup(K_2(keys), seqbuf, NULL);
        chacha_encrypt_bytes(K_2(keys),
            poly_key, poly_key, sizeof(poly_key));

        /* If decrypting, check tag before anything else */
        if (!do_encrypt) {
                const u_char *tag = src + aadlen + len;

                poly1305_auth(expected_tag, src, aadlen + len, poly_key);
                if (lws_timingsafe_bcmp(expected_tag, tag, POLY1305_TAGLEN)) {
                        r = 2;
                        goto out;
                }
        }

        /* Crypt additional data */
        if (aadlen) {
                chacha_ivsetup(K_1(keys), seqbuf, NULL);
                chacha_encrypt_bytes(K_1(keys), src, dest, aadlen);
        }

        /* Set Chacha's block counter to 1 */
        chacha_ivsetup(K_2(keys), seqbuf, one);
        chacha_encrypt_bytes(K_2(keys), src + aadlen, dest + aadlen, len);

        /* If encrypting, calculate and append tag */
        if (do_encrypt) {
                poly1305_auth(dest + aadlen + len, dest, aadlen + len,
                    poly_key);
        }
        r = 0;
 out:
        lws_explicit_bzero(expected_tag, sizeof(expected_tag));
        lws_explicit_bzero(seqbuf, sizeof(seqbuf));
        lws_explicit_bzero(poly_key, sizeof(poly_key));
        return r;
}

int
lws_chacha_decrypt(struct lws_ssh_keys *keys, uint32_t seq,
		   const uint8_t *ct, uint32_t len, uint8_t *pt)
{
	return chachapoly_crypt(keys, seq, pt, ct, len - POLY1305_TAGLEN - 4, 4,
			 POLY1305_TAGLEN, 0);
}

int
lws_chacha_encrypt(struct lws_ssh_keys *keys, uint32_t seq,
		   const uint8_t *ct, uint32_t len, uint8_t *pt)
{
	return chachapoly_crypt(keys, seq, pt, ct, len - 4, 4, 0, 1);
}

