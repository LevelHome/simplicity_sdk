#ifndef RINGBUFS_H
#define RINGBUFS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct rbufs
{
    uint8_t *buff; /*!< Pointer to buffer data */
    size_t   size; /*!< Size of buffer data */
    size_t   r;    /*!< Next read pointer */
    size_t   w;    /*!< Next write pointer */
} rbufs_t;

int32_t rbufsInit(rbufs_t *rBuf, void *aData, size_t aDataLength);

size_t rbufsWrite(rbufs_t *rBuf, const void *aData, size_t wBytes);

size_t rbufsRead(rbufs_t *rBuf, void *aData, size_t rBytes);

size_t rbufsGetFreeSize(rbufs_t *rBuf);

size_t rbufsGetByteCount(rbufs_t *rBuf);

void *rbufsGetBlockReadAddress(rbufs_t *rBuf);

size_t rbufsGetBlockReadLength(rbufs_t *rBuf);

void *rbufsGetBlockWriteAddress(rbufs_t *rBuf);

size_t rbufsGetBlockWriteLength(rbufs_t *rBuf);

size_t rbufsSkip(rbufs_t *rBuf, size_t len);

#endif // RINGBUFS_H
