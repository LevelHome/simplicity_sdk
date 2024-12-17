#include "ringbufs.h"
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IS_RBUFS(b) ((b) != NULL && (b)->buff != NULL && (b)->size > 0)

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

int32_t rbufsInit(rbufs_t *rBuf, void *aData, size_t aDataLength)
{
    if (rBuf == NULL || aData == NULL || aDataLength == 0)
    {
        return SL_STATUS_FAIL;
    }

    memset((void *)rBuf, 0x00, sizeof(*rBuf));

    rBuf->size = aDataLength;
    rBuf->buff = aData;

    return SL_STATUS_OK;
}

size_t rbufsWrite(rbufs_t *rBuf, const void *aData, size_t wBytes)
{
    size_t         nCopyBytes, nFreeBytes;
    const uint8_t *d = aData;

    if (!IS_RBUFS(rBuf) || aData == NULL || wBytes == 0)
    {
        return SL_STATUS_FAIL;
    }

    /* Calculate maximum number of bytes available to write */
    nFreeBytes = rbufsGetFreeSize(rBuf);
    wBytes     = MIN(nFreeBytes, wBytes);
    if (wBytes == 0)
    {
        return 0;
    }

    /* Write data to linear part of buffer */
    nCopyBytes = MIN(rBuf->size - rBuf->w, wBytes);
    memcpy(&rBuf->buff[rBuf->w], d, nCopyBytes);
    rBuf->w += nCopyBytes;
    wBytes -= nCopyBytes;

    /* Write data to beginning of buffer (overflow part) */
    if (wBytes > 0)
    {
        memcpy(rBuf->buff, &d[nCopyBytes], wBytes);
        rBuf->w = wBytes;
    }

    /* Check end of buffer */
    if (rBuf->w >= rBuf->size)
    {
        rBuf->w = 0;
    }

    return nCopyBytes + wBytes;
}

size_t rbufsRead(rbufs_t *rBuf, void *aData, size_t rBytes)
{
    size_t   nCopyBytes, nBytes;
    uint8_t *d = aData;

    if (!IS_RBUFS(rBuf) || aData == NULL || rBytes == 0)
    {
        return 0;
    }

    /* Calculate maximum number of bytes available to read */
    nBytes = rbufsGetByteCount(rBuf);
    rBytes = MIN(nBytes, rBytes);
    if (rBytes == 0)
    {
        return 0;
    }

    /* Read data from linear part of buffer */
    nCopyBytes = MIN(rBuf->size - rBuf->r, rBytes);
    memcpy(d, &rBuf->buff[rBuf->r], nCopyBytes);
    rBuf->r += nCopyBytes;
    rBytes -= nCopyBytes;

    /* Read data from beginning of buffer (overflow part) */
    if (rBytes > 0)
    {
        memcpy(&d[nCopyBytes], rBuf->buff, rBytes);
        rBuf->r = rBytes;
    }

    /* Check end of buffer */
    if (rBuf->r >= rBuf->size)
    {
        rBuf->r = 0;
    }

    return nCopyBytes + rBytes;
}

size_t rbufsGetFreeSize(rbufs_t *rBuf)
{
    size_t size, w, r;

    if (!IS_RBUFS(rBuf))
    {
        return 0;
    }

    w = rBuf->w;
    r = rBuf->r;
    if (w == r)
    {
        size = rBuf->size;
    }
    else if (r > w)
    {
        size = r - w;
    }
    else
    {
        size = rBuf->size - (w - r);
    }

    return size - 1;
}

size_t rbufsGetByteCount(rbufs_t *rBuf)
{
    size_t w, r, size;

    if (!IS_RBUFS(rBuf))
    {
        return 0;
    }

    w = rBuf->w;
    r = rBuf->r;
    if (w == r)
    {
        size = 0;
    }
    else if (w > r)
    {
        size = w - r;
    }
    else
    {
        size = rBuf->size - (r - w);
    }
    return size;
}

void *rbufsGetBlockReadAddress(rbufs_t *rBuf)
{
    if (!IS_RBUFS(rBuf))
    {
        return NULL;
    }

    return &rBuf->buff[rBuf->r];
}

size_t rbufsGetBlockReadLength(rbufs_t *rBuf)
{
    size_t w, r, len;

    if (!IS_RBUFS(rBuf))
    {
        return 0;
    }

    w = rBuf->w;
    r = rBuf->r;
    if (w > r)
    {
        len = w - r;
    }
    else if (r > w)
    {
        len = rBuf->size - r;
    }
    else
    {
        len = 0;
    }

    return len;
}

void *rbufsGetBlockWriteAddress(rbufs_t *rBuf)
{
    if (!IS_RBUFS(rBuf))
    {
        return NULL;
    }

    return &rBuf->buff[rBuf->w];
}

size_t rbufsGetBlockWriteLength(rbufs_t *rBuf)
{
    size_t w, r, len;

    if (!IS_RBUFS(rBuf))
    {
        return 0;
    }

    w = rBuf->w;
    r = rBuf->r;
    if (w >= r)
    {
        len = rBuf->size - w;
        /*
         * When read pointer is 0,
         * maximal length is one less as if too many bytes
         * are written, buffer would be considered empty again (r == w)
         */
        if (r == 0)
        {
            /*
             * Cannot overflow:
             * - If r is not 0, statement does not get called
             * - rBuf->size cannot be 0 and if r is 0, len is greater 0
             */
            --len;
        }
    }
    else
    {
        len = r - w - 1;
    }

    return len;
}

size_t rbufsSkip(rbufs_t *rBuf, size_t len)
{
    size_t nBytes;

    if (!IS_RBUFS(rBuf) || len == 0)
    {
        return 0;
    }

    nBytes = rbufsGetByteCount(rBuf);
    len    = MIN(len, nBytes);
    rBuf->r += len;
    if (rBuf->r >= rBuf->size)
    {
        rBuf->r -= rBuf->size;
    }

    return len;
}

#ifdef __cplusplus
}
#endif
