#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "nvmeswap.h"

/*========buffer============*/
/*
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
*/

static struct bio *get_swap_bio(gfp_t gfp_flags,
				struct page *page, bio_end_io_t end_io)
{
	struct bio *bio;

	bio = bio_alloc(gfp_flags, 1);
	if (bio) {
		struct block_device *bdev;

		bio->bi_iter.bi_sector = map_swap_page(page, &bdev);
		bio_set_dev(bio, bdev);
		bio->bi_iter.bi_sector <<= PAGE_SHIFT - 9;
		bio->bi_end_io = end_io;

		bio_add_page(bio, page, PAGE_SIZE * hpage_nr_pages(page), 0);
	}
	return bio;
}

static struct void end_swap_bio_write(struct bio *bio)
{
	struct page *page = bio_first_page_all(bio);

	if (bio->bi_status) {
		SetPageError(page);
		/*
		 * We failed to write the page out to swap-space.
		 * Re-dirty the page in order to avoid it being reclaimed.
		 * Also print a dire warning that things will go BAD (tm)
		 * very quickly.
		 *
		 * Also clear PG_reclaim to avoid rotate_reclaimable_page()
		 */
		set_page_dirty(page);
		pr_alert("Write-error on swap-device (%u:%u:%llu)\n",
			 MAJOR(bio_dev(bio)), MINOR(bio_dev(bio)),
			 (unsigned long long)bio->bi_iter.bi_sector);
		ClearPageReclaim(page);
	}
	end_page_writeback(page);
	bio_put(bio);
}

static void swap_slot_free_notify(struct page *page)
{
	struct swap_info_struct *sis;
	struct gendisk *disk;
	swp_entry_t entry;

	/*
	 * There is no guarantee that the page is in swap cache - the software
	 * suspend code (at least) uses end_swap_bio_read() against a non-
	 * swapcache page.  So we must check PG_swapcache before proceeding with
	 * this optimization.
	 */
	if (unlikely(!PageSwapCache(page)))
		return;

	sis = page_swap_info(page);
	if (!(sis->flags & SWP_BLKDEV))
		return;

	/*
	 * The swap subsystem performs lazy swap slot freeing,
	 * expecting that the page will be swapped out again.
	 * So we can avoid an unnecessary write if the page
	 * isn't redirtied.
	 * This is good for real swap storage because we can
	 * reduce unnecessary I/O and enhance wear-leveling
	 * if an SSD is used as the as swap device.
	 * But if in-memory swap device (eg zram) is used,
	 * this causes a duplicated copy between uncompressed
	 * data in VM-owned memory and compressed data in
	 * zram-owned memory.  So let's free zram-owned memory
	 * and make the VM-owned decompressed page *dirty*,
	 * so the page should be swapped out somewhere again if
	 * we again wish to reclaim it.
	 */
	disk = sis->bdev->bd_disk;
	entry.val = page_private(page);
	if (disk->fops->swap_slot_free_notify && __swap_count(entry) == 1) {
		unsigned long offset;

		offset = swp_offset(entry);

		SetPageDirty(page);
		disk->fops->swap_slot_free_notify(sis->bdev,
				offset);
	}
}

static void end_swap_bio_read(struct bio *bio)
{
	struct page *page = bio_first_page_all(bio);
	struct task_struct *waiter = bio->bi_private;

	if (bio->bi_status) {
		SetPageError(page);
		ClearPageUptodate(page);
		pr_alert("Read-error on swap-device (%u:%u:%llu)\n",
			 MAJOR(bio_dev(bio)), MINOR(bio_dev(bio)),
			 (unsigned long long)bio->bi_iter.bi_sector);
		goto out;
	}

	SetPageUptodate(page);
	swap_slot_free_notify(page);
out:
	unlock_page(page);
	WRITE_ONCE(bio->bi_private, NULL);
	bio_put(bio);
	if (waiter) {
		blk_wake_io_task(waiter);
		put_task_struct(waiter);
	}
}


/*========buffer============*/

static inline int nvme_swap_get_req()
{
    
}

int sswap_rdma_write(struct page *page, u64 roffset)
{
	struct writeback_control wbc = {
		.sync_mode = WB_SYNC_NONE,
		.nr_to_write = SWAP_CLUSTER_MAX,
		.range_start = 0,
		.range_end = LLONG_MAX,
		.for_reclaim = 1,
	};


	
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

	count_vm_event(PSWPIN);
	bio_get(bio);
	qc = submit_bio(bio);
	__set_current_state(TASK_RUNNING);
	bio_put(bio);

	SetPageUptodate(page);
	unlock_page(page);
	return 0;
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