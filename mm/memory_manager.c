#include <mm/include/memory_manager.h>
#include <include/bootp.h>
#include <mm/include/frame_alloc.h>
#include <mm/include/obj_alloc.h>
#include <include/mem.h>
#include <mm/include/paging.h>
#include <mm/include/kmalloc.h>
#include <tty/include/tty.h>

uint64_t memory_length = 0; //total quantity of physical memory
uint64_t mmap_size = 0;
uint64_t mmap_last_ind = 0;
uint8_t mmap_pool_id; //pool id of the memory map (set later)
bool mman_ready = false;

bool init_mm(struct leokernel_boot_params bootp) {
    //set the global variables
    mmap_size = bootp.map_size;
    mmap_pool_id = 0;
    memory_length = 0;
    mmap_pool_id = 0;
    mmap_last_ind = 0;

    //use the memory map to count how much memory is available
    for (uint64_t i = 0; i < bootp.map_size; i++) {
        leokernel_memory_descriptor_t *entry = bootp.map + i;

        if (!DESCRIPTOR_VALID(entry->flags)) {
            break;
        }

        memory_length += entry->pages * PAGE_SIZE;
        mmap_last_ind++;
    }

    //init physical frame allocator
    if (!init_frame_alloc(bootp.frame_bitmap)) {
        return false;
    }

    //init object allocator
    if (!obj_alloc_init(&mmap_pool_id)) {
        return false;
    }

    //transfer the memory map into the pool
    for (uint64_t i = 0; i < mmap_last_ind; i++) {
        leokernel_memory_descriptor_t *entry = bootp.map + i;
        obj_pool_put(mmap_pool_id, (void *) entry, i);
    }

    //clear the memory that was previously used to store the memory map
    memclear(bootp.map, bootp.map_size * sizeof(leokernel_memory_descriptor_t));

    //free the frames that were previously allocted to the memory map
    for (uint32_t i = 0; i < (bootp.map_size * sizeof(leokernel_memory_descriptor_t)) / PAGE_SIZE; i++) {
        kfree_frame((void *) bootp.map + i * PAGE_SIZE);
    }

    //init kernel heap
    if (!init_kmalloc()) {
        return false;
    }

    mman_ready = true;
    return true;
}

//initialize a memory descriptor
void init_descriptor(leokernel_memory_descriptor_t *descr, void *virtual, void *physical, uint32_t pages, uint8_t flags, uint8_t type) {
    descr->virtual_address = (uint64_t) virtual;
    descr->physical_address = (uint64_t) physical;
    descr->pages = pages;
    descr->flags = flags;
    descr->type = type;
}

/*
Allocate a set of pages (one or more).
This function takes the requested number of frames and map them in the virtual memory in the first available page address.
This function returns the address of the allocated memory or null if:
- a bad number of pages is requested
- if the system doesn't have enough virtual or physical memory
- if an operation with the memory map or page table went wrong
*/
void *kalloc_page(uint32_t n) {
    if (n == 0 || n > ALLOC_MAX_PAGES) {
        return null;
    }

    uint64_t descr_ind;
    leokernel_memory_descriptor_t *entry = find_available_virtual_memory(n, &descr_ind);
    if (!entry) {return null;}
    void *frames[n];
    
    if (!kalloc_frames_array(n, frames)) {
        return null;
    }
    
    leokernel_memory_descriptor_t new_entry;
    init_descriptor(&new_entry, (void *) entry->virtual_address, frames[0], 0, LEOKERNEL_MEMORY_MAP_VALID, kernel_reserved);
    void *base_address = (void *) new_entry.virtual_address; //base virtual address of the memory we're allocating
    uint32_t m = 0;

    while(m < n) {
        if (m > 0 && frames[m - 1] + PAGE_SIZE != frames[m]) {
            new_entry.flags |= LEOKERNEL_MEMORY_MAP_HAS_NEXT;
            if (!obj_pool_put(mmap_pool_id, (void *) &new_entry, mmap_last_ind)) {kfree_frames_array(n, frames);}
            mmap_last_ind++;
            init_descriptor(&new_entry, (void *) new_entry.virtual_address + m * PAGE_SIZE, frames[m], 0, LEOKERNEL_MEMORY_MAP_VALID, kernel_reserved);
        }

        if (!map_page((void *)(base_address + m * PAGE_SIZE), frames[m])) {kfree_frames_array(n, frames);}
        new_entry.pages++;
        entry->virtual_address += PAGE_SIZE;
        entry->pages--;
        m++;

        if (m == n) {
            if (!obj_pool_put(mmap_pool_id, (void *) &new_entry, mmap_last_ind)) {kfree_frames_array(n, frames);}
            mmap_last_ind++;
        }
    }

    //if this descriptor is now empty, delete it from the map
    if (entry->pages == 0) {
        if (descr_ind == mmap_size - 1) { //if it was the last, jut remove it
            memclear(entry, sizeof(leokernel_memory_descriptor_t));
        } else { //otherwise move every descriptor after this a position back
            memmove((void *) entry, (void *)(entry + 1), (mmap_last_ind - descr_ind - 1) * sizeof(leokernel_memory_descriptor_t));
        }

        mmap_last_ind--;
    }

    //get the pointer of the first entry of the map to operate on them directly
    leokernel_memory_descriptor_t *first_entry;
    obj_pool_get(mmap_pool_id, (void *) &first_entry, 0);
    order_map(first_entry, mmap_last_ind);
    return base_address;
}

bool kfree_page(void *base) {
    if (base == null || (uint64_t) base >= memory_length) {
        return false;
    }

    leokernel_memory_descriptor_t *next = null;
    uint32_t ind = 0;

    //linear search, to be upgraded to a log n search
    for (uint64_t i = 0; i < mmap_size; i++) {
        leokernel_memory_descriptor_t *entry;

        if (!obj_pool_get(mmap_pool_id, (void **) &entry, i)) {
            break;
        }

        //check if we reached the end of the map, in that case exit the loop and return null
        if (!DESCRIPTOR_VALID(entry->flags)) {
            return false;
        }

        if (entry->virtual_address == (uint64_t) base) {
            next = entry;
            ind = i;
            break;
        }
    }

    if (next == null) {
        return false;
    }

    while(next != null && DESCRIPTOR_VALID(next->flags)) {
        leokernel_memory_descriptor_t *tmp = next;

        if (DESCRIPTOR_HAS_NEXT(tmp->flags)) {
            next = tmp + 1;
        } else {
            next = null;
        }

        for (uint32_t i = 0; i < tmp->pages; i++) {
            if (!kfree_frame((void *)(tmp->physical_address + i * PAGE_SIZE))) {/*???*/}
        }

        //remove_descriptor(ind);
        /*we don't have to increase ind because when the descriptor with index ind is removed, the other descriptors will move down and ind
        will automatically point to the next descriptor*/

        //edit the descriptor to make it describe available memory without removing it (in the case of multiple descriptors all of them will be edited)
        uint8_t flags = LEOKERNEL_MEMORY_MAP_AVAIL | LEOKERNEL_MEMORY_MAP_VALID | LEOKERNEL_MEMORY_MAP_TRANS_UNK;
        init_descriptor(tmp, (void *) tmp->virtual_address, null, tmp->pages, flags, usable);
    }

    leokernel_memory_descriptor_t *first_entry;
    obj_pool_get(mmap_pool_id, (void **) &first_entry, 0);
    compress_map(first_entry, mmap_last_ind);

    return true;
}

//scans the memory map to find the required amount of virtual memory
leokernel_memory_descriptor_t *find_available_virtual_memory(uint32_t num_pages, uint64_t *descr_ind) {
    for (uint64_t i = 0; i < mmap_size; i++) {
        leokernel_memory_descriptor_t *entry;

        if (!obj_pool_get(mmap_pool_id, (void **) &entry, i)) {
            break;
        }

        //check if we reached the end of the map, in that case exit the loop and return null
        if (!DESCRIPTOR_VALID(entry->flags)) {
            break;
        }

        //if this descriptor is marked as available and it refers a number of page greater or equal than the number of page we need return its pointer
        if (entry->pages >= num_pages && DESCRIPTOR_AVAILABLE(entry->flags) && entry->type == usable) {
            *descr_ind = i;
            return entry;
        }

    }

    return null;
}

/*
Maps a virtual address into a physical address.
*/
bool kmap_page(void *virtual, void *physical) {
    uint64_t page_address = (uint64_t) virtual & 0xFFFFFFFFFFFFF000;
    uint64_t frame_address = (uint64_t) physical & 0xFFFFFFFFFFFFF000;
    leokernel_memory_descriptor_t *referenced_entry = null;
    leokernel_memory_descriptor_t new_entry;

    init_descriptor(&new_entry, (void *) page_address, (void *) frame_address, 1, LEOKERNEL_MEMORY_MAP_VALID, kernel_reserved);

    //search for the descriptor which describes the page we want to map
    for (uint64_t i = 0; i < mmap_size; i++) {
        leokernel_memory_descriptor_t *entry;

        if (!obj_pool_get(mmap_pool_id, (void **) &entry, i)) {
            return false;
        }

        //check if we reached the end of the map, in that case exit the loop and return null
        if (!DESCRIPTOR_VALID(entry->flags)) {
            return false;
        }

        if (page_address >= entry->virtual_address && page_address < entry->virtual_address + entry->pages * PAGE_SIZE && entry->pages >= 1) {
            referenced_entry = entry;
            break;
        }
    }

    //if the virtual address we want to map is already in the memory map we have to modify it
    if (referenced_entry) {
        if (referenced_entry->pages == 1) {
            referenced_entry->physical_address = frame_address;
            referenced_entry->flags = LEOKERNEL_MEMORY_MAP_VALID;
            referenced_entry->type = kernel_reserved;
        } else {
            leokernel_memory_descriptor_t before, after;
            uint32_t pages_before = (page_address - referenced_entry->virtual_address) / PAGE_SIZE;
            uint32_t pages_after = ((referenced_entry->virtual_address + (referenced_entry->pages - 1) * PAGE_SIZE) - page_address) / PAGE_SIZE;
            init_descriptor(&before, (void *) referenced_entry->virtual_address, (void *) null, pages_before, referenced_entry->flags, referenced_entry->type);
            init_descriptor(&after, (void *)(page_address + PAGE_SIZE), (void *) null, pages_after, referenced_entry->flags, referenced_entry->type);

            if (page_address > referenced_entry->virtual_address) {
                if (!obj_pool_put(mmap_pool_id, (void *) &before, mmap_last_ind)) {return false;}
                mmap_last_ind++;
            }

            if (page_address < referenced_entry->virtual_address + referenced_entry->pages * PAGE_SIZE) {
                if (!obj_pool_put(mmap_pool_id, (void *) &after, mmap_last_ind)) {return false;}
                mmap_last_ind++;
            }
        }
    }
    
    if (!obj_pool_put(mmap_pool_id, (void *) &new_entry, mmap_last_ind)) {return false;}
    mmap_last_ind++;
    leokernel_memory_descriptor_t *first_entry;
    obj_pool_get(mmap_pool_id, (void **) &first_entry, 0);
    order_map(first_entry, mmap_last_ind);
    return map_page((void *) page_address, (void *) frame_address);
}

/*
This function orders the memory map by virtual address.
The ordering is made with selection sort algorithm (to improve).
*/
void order_map(leokernel_memory_descriptor_t *map, uint64_t size) {
	if (size < 2) {
		return;
	}

	for (uint64_t i = 0; i < size - 1; i++) {
		leokernel_memory_descriptor_t *entry = map + i;
		leokernel_memory_descriptor_t *selected = entry;

        if (!DESCRIPTOR_VALID(entry->flags)) {
            break;
        }

		for (uint64_t j = i + 1; j < size; j++) {
			leokernel_memory_descriptor_t *candidate = map + j;

			if (candidate->virtual_address < selected->virtual_address && DESCRIPTOR_VALID(candidate->flags)) {
				selected = candidate;
			}
		}

		leokernel_memory_descriptor_t tmp;
		memcpy(&tmp, entry, sizeof(leokernel_memory_descriptor_t));
		memmove(entry, selected, sizeof(leokernel_memory_descriptor_t));
		memmove(selected, &tmp, sizeof(leokernel_memory_descriptor_t));
	}
}

/*
This function tries to reduce the number of entries in the memory map since there's a limited number of entries available.
This function merges two or more descriptors if they're compatible.
This function works at its max when is called on a ordered map (by virtual address).
Two or more descriptors are compatible if the memory area they refer is:
- contiguous in virtual memory
- are the same type (usable)
- have the same flags
*/
void compress_map(leokernel_memory_descriptor_t *map, uint64_t size) {
	uint64_t i;

	for (i = 0; i < size - 1; i++) {
		leokernel_memory_descriptor_t *entry = map + i;

		if (!DESCRIPTOR_VALID(entry->flags)) {
			break;
		}

        //compress only descriptors that refer to free memory
        if (entry->type != usable || !DESCRIPTOR_AVAILABLE(entry->flags)) {
            continue;
        }

		uint64_t n = 0; //number of descriptors to be merged
		uint64_t pages = entry->pages; //total number of pages of the new descriptor
		leokernel_memory_descriptor_t *candidate = entry + 1;

		while(candidate->type == entry->type && candidate->flags == entry->flags && candidate->virtual_address == (candidate - 1)->virtual_address + PAGE_SIZE * (candidate - 1)->pages) {
			pages += candidate->pages;
			candidate += 1;
			n++;
		}

		if (n > 0) {
			entry->pages = pages;
            memclear((void *)(leokernel_memory_descriptor_t *)(entry + 1), n * sizeof(leokernel_memory_descriptor_t));
			memmove((void *)(leokernel_memory_descriptor_t *)(entry + 1), (void *)(leokernel_memory_descriptor_t *)(entry + n + 1), (size - n - i - 1) * sizeof(leokernel_memory_descriptor_t));
            mmap_last_ind -= n;
        }
	}
}

void print_descriptor(leokernel_memory_descriptor_t *d) {
    printf("0x%016lX -> 0x%016lX %d ", d->virtual_address, d->physical_address, d->pages);

    if (DESCRIPTOR_VALID(d->flags)) {
        printf("v ");
    }

    if (DESCRIPTOR_AVAILABLE(d->flags)) {
        printf("a ");
    }

    if (DESCRIPTOR_HAS_NEXT(d->flags)) {
        printf("hn ");
    }

    if (DESCRIPTOR_TRANS_UNK(d->flags)) {
        printf("tu ");
    }

    printf("\n");
}

void print_map() {
    for (uint64_t i = 0; i < mmap_size; i++) {
        leokernel_memory_descriptor_t *entry;
        if (!obj_pool_get(mmap_pool_id, (void **) &entry, i)) {break;}
        if (!DESCRIPTOR_VALID(entry->flags)) {break;}
        print_descriptor(entry);
        for (long i = 0; i < 10000000; i++) {}
    }
}