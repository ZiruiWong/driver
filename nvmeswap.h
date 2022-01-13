#if !defined(_SSWAP_NVME_H)
#define _SSWAP_NVME_H



int sswap_rdma_read_async(struct page *page, u64 roffset);
int sswap_rdma_read_sync(struct page *page, u64 roffset);
int sswap_rdma_write(struct page *page, u64 roffset);
int sswap_rdma_poll_load(int cpu);

#endif
