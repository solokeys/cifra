
#include "prp.h"
#include "modes.h"
#include "bitops.h"
#include "blockwise.h"

#include <string.h>

/* CBC */
void cf_cbc_init(cf_cbc *ctx, const cf_prp *prp, void *prpctx, uint8_t iv[CF_MAXBLOCK])
{
  ctx->prp = prp;
  ctx->prpctx = prpctx;
  memcpy(ctx->block, iv, prp->blocksz);
}

void cf_cbc_encrypt(cf_cbc *ctx, const uint8_t *input, uint8_t *output, size_t blocks)
{
  uint8_t buf[CF_MAXBLOCK];
  size_t nblk = ctx->prp->blocksz;

  while (blocks--)
  {
    xor_bb(buf, input, ctx->block, nblk);
    ctx->prp->block(ctx->prpctx, cf_prp_encrypt, buf, ctx->block);
    memcpy(output, ctx->block, nblk);
    input += nblk;
    output += nblk;
  }
}

void cf_cbc_decrypt(cf_cbc *ctx, const uint8_t *input, uint8_t *output, size_t blocks)
{
  uint8_t buf[CF_MAXBLOCK];
  size_t nblk = ctx->prp->blocksz;

  while (blocks--)
  {
    ctx->prp->block(ctx->prpctx, cf_prp_decrypt, input, buf);
    xor_bb(output, buf, ctx->block, nblk);
    memcpy(ctx->block, input, nblk);
    input += nblk;
    output += nblk;
  }
}

/* CTR */
void cf_ctr_init(cf_ctr *ctx, const cf_prp *prp, void *prpctx, uint8_t nonce[CF_MAXBLOCK])
{
  ctx->prp = prp;
  ctx->prpctx = prpctx;
  ctx->nkeymat = 0;
  memcpy(ctx->nonce, nonce, prp->blocksz);
}

static void next_block(uint8_t *block, size_t nb)
{
  nb--;
  while (1)
  {
    if (++block[nb] != 0)
      return;
    if (nb == 0)
      return;
    nb--;
  }
}

static void ctr_next_block(void *vctx, uint8_t *out)
{
  cf_ctr *ctx = vctx;
  ctx->prp->block(ctx->prpctx, cf_prp_encrypt, ctx->nonce, out);
  next_block(ctx->nonce, ctx->prp->blocksz);
}

void cf_ctr_cipher(cf_ctr *ctx, const uint8_t *input, uint8_t *output, size_t bytes)
{
  cf_blockwise_xor(ctx->keymat, &ctx->nkeymat,
                   ctx->prp->blocksz,
                   input, output, bytes,
                   ctr_next_block,
                   ctx);
}
