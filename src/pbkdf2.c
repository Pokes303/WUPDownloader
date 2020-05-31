/*
 * fast-pbkdf2 - Optimal PBKDF2-HMAC calculation
 * Written in 2015 by Joseph Birr-Pixton <jpixton@gmail.com>
 * Modified in 2020 by Thomas Rohloff <v10lator@myway.de>
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include <pbkdf2.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <coreinit/memory.h>

#include <sha1.h>


/* --- Common useful things --- */
#define MIN(a, b) ((a) > (b)) ? (b) : (a)

static inline void write32_be(uint32_t n, uint8_t out[4])
{
  out[0] = (n >> 24) & 0xff;
  out[1] = (n >> 16) & 0xff;
  out[2] = (n >> 8) & 0xff;
  out[3] = n & 0xff;
}

/* Prepare block (of blocksz bytes) to contain md padding denoting a msg-size
 * message (in bytes).  block has a prefix of used bytes.
 *
 * Message length is expressed in 32 bits (so suitable for sha1, sha256, sha512). */
static inline void md_pad(uint8_t *block, size_t blocksz, size_t used, size_t msg)
{
  OSBlockSet(block + used, 0, blocksz - used - 4);
  block[used] = 0x80;
  block += blocksz - 4;
  write32_be((uint32_t) (msg * 8), block);
}

  typedef struct {                                                            
    SHA1_CTX inner;                                                               
    SHA1_CTX outer;                                                               
  } HMAC_CTX;                                                          
  
  static inline void sha1_extract(SHA1_CTX *restrict ctx, uint8_t *restrict out)
{
  write32_be(ctx->state[0], out);
  write32_be(ctx->state[1], out + 4);
  write32_be(ctx->state[2], out + 8);
  write32_be(ctx->state[3], out + 12);
  write32_be(ctx->state[4], out + 16);
}

static inline void sha1_cpy(SHA1_CTX *restrict out, const SHA1_CTX *restrict in)
{
  out->state[0] = in->state[0];
  out->state[1] = in->state[1];
  out->state[2] = in->state[2];
  out->state[3] = in->state[3];
  out->state[4] = in->state[4];
}

static inline void sha1_xor(SHA1_CTX *restrict out, const SHA1_CTX *restrict in)
{
  out->state[0] ^= in->state[0];
  out->state[1] ^= in->state[1];
  out->state[2] ^= in->state[2];
  out->state[3] ^= in->state[3];
  out->state[4] ^= in->state[4];
}
  
  static inline void HMAC_INIT(HMAC_CTX *ctx,                   
                                      const uint8_t *key, size_t nkey)        
  {                                                                           
    /* Prepare key: */                                                        
    uint8_t k[64];                                                      
                                                                              
    /* Shorten long keys. */                                                  
    if (nkey > 64)                                                      
    {                                                                         
      SHA1Init(&ctx->inner);                                                     
      SHA1Update(&ctx->inner, key, nkey);                                        
      SHA1Final(k, &ctx->inner);                                                 
                                                                              
      key = k;                                                                
      nkey = SHA1_DIGEST_SIZE;                                                         
    }                                                                         
                                                                              
    /* Standard doesn't cover case where blocksz < hashsz. */                 
    assert(nkey <= 64);                                                 
                                                                              
    /* Right zero-pad short keys. */                                          
    if (k != key)                                                             
      OSBlockMove(k, key, nkey, false);                                       
    if (64 > nkey)                                                      
      OSBlockSet(k + nkey, 0, 64 - nkey);                               
                                                                              
    /* Start inner hash computation */                                        
    uint8_t blk_inner[64];                                              
    uint8_t blk_outer[64];                                              
                                                                              
    for (size_t i = 0; i < 64; i++)                                     
    {                                                                         
      blk_inner[i] = 0x36 ^ k[i];                                             
      blk_outer[i] = 0x5c ^ k[i];                                             
    }                                                                         
                                                                              
    SHA1Init(&ctx->inner);                                                       
    SHA1Update(&ctx->inner, blk_inner, sizeof blk_inner);                        
                                                                              
    /* And outer. */                                                          
    SHA1Init(&ctx->outer);                                                       
    SHA1Update(&ctx->outer, blk_outer, sizeof blk_outer);                        
  }                                                                           
                                                                              
  static inline void HMAC_UPDATE(HMAC_CTX *ctx,                 
                                        const void *data, size_t ndata)       
  {                                                                           
    SHA1Update(&ctx->inner, data, ndata);                                        
  }                                                                           
                                                                              
  static inline void HMAC_FINAL(HMAC_CTX *ctx,                  
                                       uint8_t out[SHA1_DIGEST_SIZE])                  
  {                                                                           
    SHA1Final(out, &ctx->inner);                                                 
    SHA1Update(&ctx->outer, out, SHA1_DIGEST_SIZE);                                       
    SHA1Final(out, &ctx->outer);                                                 
  }                                                                           
                                                                              
                                                                              
  /* --- PBKDF2 --- */                                                        
  static inline void PBKDF2_F(const HMAC_CTX *startctx,         
                                     uint32_t counter,                        
                                     const uint8_t *salt, size_t nsalt,       
                                     uint32_t iterations,                     
                                     uint8_t *out)                            
  {                                                                           
    uint8_t countbuf[4];                                                      
    write32_be(counter, countbuf);                                            
                                                                              
    /* Prepare loop-invariant padding block. */                               
    uint8_t Ublock[64];                                                 
    md_pad(Ublock, 64, SHA1_DIGEST_SIZE, 64 + SHA1_DIGEST_SIZE);                    
                                                                              
    /* First iteration:                                                       
     *   U_1 = PRF(P, S || INT_32_BE(i))                                      
     */                                                                       
    HMAC_CTX ctx = *startctx;                                          
    HMAC_UPDATE(&ctx, salt, nsalt);                                    
    HMAC_UPDATE(&ctx, countbuf, sizeof countbuf);                      
    HMAC_FINAL(&ctx, Ublock);                                          
    SHA1_CTX result = ctx.outer;                                                  
                                                                              
    /* Subsequent iterations:                                                 
     *   U_c = PRF(P, U_{c-1})                                                
     */                                                                       
    for (uint32_t i = 1; i < iterations; i++)                                 
    {                                                                         
      /* Complete inner hash with previous U */                               
      sha1_cpy(&ctx.inner, &startctx->inner);                                    
      SHA1Transform(ctx.inner.state, Ublock);                                       
      sha1_extract(&ctx.inner, Ublock);                                            
      /* Complete outer hash with inner output */                             
      sha1_cpy(&ctx.outer, &startctx->outer);                                    
      SHA1Transform(ctx.outer.state, Ublock);                                       
      sha1_extract(&ctx.outer, Ublock);                                            
      sha1_xor(&result, &ctx.outer);                                             
    }                                                                         
                                                                              
    /* Reform result into output buffer. */                                   
    sha1_extract(&result, out);                                                    
  }

void pbkdf2_hmac_sha1(const uint8_t *pw, size_t npw,
                          const uint8_t *salt, size_t nsalt,
                          uint32_t iterations,
                          uint8_t *out, size_t nout)
{
  assert(iterations);
  assert(out && nout);
  
  /* Starting point for inner loop. */
  HMAC_CTX ctx;
  HMAC_INIT(&ctx, pw, npw);
  
  /* How many blocks do we need? */
  uint32_t blocks_needed = (uint32_t)(nout + SHA1_DIGEST_SIZE - 1) / SHA1_DIGEST_SIZE;
  
  for (uint32_t counter = 1; counter <= blocks_needed; counter++)
  {
    uint8_t block[SHA1_DIGEST_SIZE];
    PBKDF2_F(&ctx, counter, salt, nsalt, iterations, block);
    
    size_t offset = (counter - 1) * SHA1_DIGEST_SIZE;
    size_t taken = MIN(nout - offset, SHA1_DIGEST_SIZE);
    OSBlockMove(out + offset, block, taken, false);
  }
}
