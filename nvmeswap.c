#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "nvmeswap.h"

#include <linux/gfp.h>
#include <linux/bio.h>
#include <linux/swap.h>
#include <linux/mm.h>
#include <linux/kernel_stat.h>
#include <linux/pagemap.h>
#include <linux/swapops.h>
#include <linux/buffer_head.h>
#include <linux/writeback.h>
#include <linux/frontswap.h>
#include <linux/blkdev.h>
#include <linux/uio.h>
#include <linux/sched/task.h>
#include <asm/pgtable.h>
#include <linux/swapfile.h>
#include <linux/swap_slots.h>
#include <linux/swapops.h>
#include <linux/swap_cgroup.h>

/*******************************************************/
int sswap_rdma_write(struct page *page, u64 roffset)
{
    struct writeback_control wbc = {
        .sync_mode = WB_SYNC_NONE,
        .nr_to_write = SWAP_CLUSTER_MAX,
        .range_start = 0,
        .range_end = LLONG_MAX,
        .for_reclaim = 1,
    };

    struct bio *bio;
    int ret;

    VM_BUG_ON_PAGE(!PageSwapCache(page), page);

    ret = 0;
    bio = get_swap_bio(GFP_NOIO, page, end_swap_bio_write);
    if (bio == NULL) {
        set_page_dirty(page);
        unlock_page(page);
        ret = -ENOMEM;
        //goto out;
        return ret;
    }
    bio->bi_opf = REQ_OP_WRITE | REQ_SWAP | wbc_to_write_flags(&wbc);
    bio_associate_blkg_from_page(bio, page);
    count_swpout_vm_event(page);
    set_page_writeback(page);
    unlock_page(page);
    submit_bio(bio);
out:
    return ret;

}
EXPORT_SYMBOL(sswap_rdma_write);

int sswap_rdma_poll_load(int cpu)
{
    //TODO
    
    return 0;
}
EXPORT_SYMBOL(sswap_rdma_poll_load);

int sswap_rdma_read_async(struct page *page, u64 roffset)
{
    struct bio *bio;
    int ret = 0;
    blk_qc_t qc;
    struct gendisk *disk;

    VM_BUG_ON_PAGE(!PageSwapCache(page), page);
    VM_BUG_ON_PAGE(!PageLocked(page), page);
    VM_BUG_ON_PAGE(PageUptodate(page), page);

    //TODO
    bio = get_swap_bio(GFP_KERNEL, page, end_swap_bio_read);
    if (bio == NULL) {
        unlock_page(page);
        ret = -ENOMEM;
        goto out;
    }
    disk = bio->bi_disk;
    /*
     * Keep this task valid during swap readpage because the oom killer may
     * attempt to access it in the page fault retry time check.
     */
    bio_set_op_attrs(bio, REQ_OP_READ, 0);
    /*if (synchronous) {
        bio->bi_opf |= REQ_HIPRI;
        get_task_struct(current);
        bio->bi_private = current;
    }*/
    count_vm_event(PSWPIN);
    bio_get(bio);
    qc = submit_bio(bio);
    /*while (synchronous) {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if (!READ_ONCE(bio->bi_private))
            break;

        if (!blk_poll(disk->queue, qc, true))
            io_schedule();
    }*/
    __set_current_state(TASK_RUNNING);
    bio_put(bio);

out:
    return ret;
}
EXPORT_SYMBOL(sswap_rdma_read_async);

int sswap_rdma_read_sync(struct page *page, u64 roffset)
{
    return sswap_rdma_read_async(page, roffset);
}
EXPORT_SYMBOL(sswap_rdma_read_sync);

static void __exit sswap_dram_cleanup_module(void)
{
    return;
}

static int __init sswap_dram_init_module(void)
{
    return 0;
}

module_init(sswap_dram_init_module);
module_exit(sswap_dram_cleanup_module);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("NVME backend");
