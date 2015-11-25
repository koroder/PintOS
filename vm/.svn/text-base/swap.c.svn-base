#include <stdio.h>
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "threads/interrupt.h"
#include "swap.h"

//initializes the swap block and table
void swap_init (void)
{
	b = block_get_role (BLOCK_SWAP);
	if (b == NULL)
		PANIC ("can't get swap block");

	int no_page = ( block_size(b) / SEC_PER_PG );

	// false if swap is free, true otherwise
	swap_table = bitmap_create(no_page);
	lock_init(&swap_lock);

}

//allocates a swap slot
block_sector_t swap_allocate ( void *kaddr )
{
	lock_acquire(&swap_lock);
	int i=0;
	int swap_no = bitmap_scan_and_flip (swap_table,0,1,false);

	if ( swap_no == (int)BITMAP_ERROR )
		return -1;

	
	for ( i = 0; i < SEC_PER_PG; i++ )
		block_write ( b, swap_no*SEC_PER_PG+ i, kaddr + i*BLOCK_SECTOR_SIZE );
	
	lock_release(&swap_lock);
	return (swap_no*SEC_PER_PG);
}

//removes page from swap
void swap_remove ( void *kaddr, block_sector_t sec_no )
{
	lock_acquire(&swap_lock);
	int i=0;
	bitmap_flip (swap_table, sec_no/SEC_PER_PG);

	for ( i = 0; i < SEC_PER_PG; i++ )
		block_read ( b, sec_no+ i, kaddr + i*BLOCK_SECTOR_SIZE );
		
        lock_release(&swap_lock);
}

// will perform eviction of frame and free the frame,
// so that a frame can then be allocated to the caller process
bool eviction (void)
{
	//returns the frame that needs to be evcited
	enum intr_level old_level;
	struct frame *frame_ev = eviction_clock ();	
	void *kaddr = frame_ev->phy_frame;
	tid_t tid = frame_ev->tid;
	
	old_level = intr_disable ();
	struct thread *t = get_thread_by_tid (tid);
	struct shadow_elem *s = shadow_pg_tbl_lookup (&t->shadow_pg_tbl, 
								pg_round_down(frame_ev->uvaddr));

	// allocate shadow entry (if necessary) and populate
	// the frame to be evcited lies in the stack
	
	if (s == NULL)
	{
		s = malloc (sizeof(struct shadow_elem));
		s->uvaddr = frame_ev->uvaddr;
		
  		// sets the location variable
  		(s->where).ex = false;
  		(s->where).mmap = false;
  		(s->where).swap = true;
  		(s->where).loaded = false;

  		s->f_ex = NOT_APP_PTR;;
		s->ofs_ex = NOT_APP_INT;
		s->read_bytes_ex = NOT_APP_INT;
		s->zero_bytes_ex = NOT_APP_INT;
		s->writable = true;

		s->f_mm = NOT_APP_PTR;
		s->ofs_mm = NOT_APP_INT;
		s->read_bytes_mm = NOT_APP_INT;

		pagedir_clear_page (t->pagedir, s->uvaddr);
		
		intr_set_level (old_level);
		block_sector_t sec = swap_allocate (kaddr);
		
		old_level = intr_disable ();
		s->sec_no = sec;
		if ((int)s->sec_no == -1)
			return false;
                
		if( hash_insert (&t->shadow_pg_tbl, &s->elem) != NULL)
			return false;
		intr_set_level (old_level);
	}
	// if frame to be evicted corresponds to mmap
	else if ((s->where).mmap) 
	{
		// finding the kernel address corresponding to user VA
		uint32_t *x = lookup_page(t->pagedir, s->uvaddr, false);
		uint32_t y = *(x) & PTE_ADDR;
		void *kadd = ptov ((uintptr_t)y);
         
		// if the page is dirty write back to the file....
		if((pagedir_is_dirty (t->pagedir, s->uvaddr) || pagedir_is_dirty (t->pagedir, kadd))
		 									&& (s->writable)) 
                {
         	  	(s->where).loaded = false;
              		(s->where).swap = false;   
         	  	pagedir_clear_page (t->pagedir, s->uvaddr);
              		lock_acquire(&filesys_lock);
              		file_write_at((s->f_mm),s->uvaddr,s->read_bytes_mm,s->ofs_mm);
                        lock_release(&filesys_lock);
                    
                }
                // esle simply evict it
         	else
         	{
          		(s->where).loaded = false;
         		(s->where).swap = false;
          		pagedir_clear_page (t->pagedir, s->uvaddr);	         	
     		}
     		intr_set_level (old_level);
        } 
        // if the frame to be evicted corresponds to the executable
	else if ((s->where).ex)
	{
		// finding the kernel address corresponding to user VA
		uint32_t *x = lookup_page(t->pagedir, s->uvaddr, false);
		uint32_t y = *(x) & PTE_ADDR;
		void *kadd = ptov ((uintptr_t)y);

		// if the page is dirty move it to the swap space....
		if((pagedir_is_dirty (t->pagedir, s->uvaddr) || pagedir_is_dirty (t->pagedir, kadd) )
            										 && (s->writable)) 
		{
			(s->where).swap = true;
			pagedir_clear_page (t->pagedir, s->uvaddr);
			intr_set_level (old_level);
			
			block_sector_t sec = swap_allocate (kaddr);
		
		        old_level = intr_disable ();						
			s->sec_no = sec;
			if ((int)s->sec_no == -1)
				return false;
			intr_set_level (old_level);
		}
		// else simply evict it
		else
		{
			(s->where).loaded = false;
			(s->where).swap = false;
			pagedir_clear_page (t->pagedir, s->uvaddr);
		        intr_set_level (old_level);	
		}
	}
	// if the frame to be evicted corresponds to the stack page
	// that has initially been moved to the swap space and then moved
	// in to the RAM.
	else if (!(s->where).ex && !(s->where).mmap)
	{
		(s->where).swap = true;
		pagedir_clear_page (t->pagedir, s->uvaddr);
		intr_set_level (old_level);
		
		block_sector_t sec = swap_allocate (kaddr);
		
		old_level = intr_disable ();						
		s->sec_no = sec;
		if ((int)s->sec_no == -1)
			return false;
		intr_set_level (old_level);
	}
	// should never reach here -- undefined state
	else
	{
		printf("undefine state\n");
	}
	// free the corresponding frame
	frame_free (frame_ev->phy_frame);
        return true;
}

// implementing the clock algorithm for eviction
struct frame *eviction_clock (void)
{

	struct list_elem *f;
  	struct frame *ret;
  	struct thread *t;
 	ret = NULL;
 	lock_acquire (&ftable_lock);
        for (f = &clock_ptr->elem; ; f = list_next (f))
        {
 	     // checks for the list tail
 	     if(!(f != NULL && f->prev != NULL && f->next == NULL))
             {
                      ret = list_entry (f, struct frame, elem);
                      t = get_thread_by_tid (ret->tid);
                      bool access = pagedir_is_accessed ( t->pagedir, ret->uvaddr);
           
                      // if the page has access_bit = 1 then set it to 0
                      if (access)
            	      {
                             pagedir_set_accessed (t->pagedir, ret->uvaddr, false);
            	      }
                      // else evict the page
                      else
                      {
              	             // move the clock ptr next to evicting frame
                             clock_ptr = list_entry(list_next(f), struct frame, elem);
                             lock_release (&ftable_lock);
                	     return ret ;  
                      }
                      // if reach the list end move to beginning - clock aspect
                      if (f == list_end (&ftable_list))
                      {
        		     f = list_begin (&ftable_list);
                      }
              }
              // if reach the list end move to beginning - clock aspect
              else
              {
                      f=list_begin(&ftable_list);
              }
        }
        // should panic if returns here
        // meaning cannot find the frame to evict
        PANIC ("eviction_clcok error");
        return NULL; //the function should never reach here
}

// to free the swap space
void swap_free (struct shadow_elem *s)
{
	block_sector_t sec = s->sec_no;
	bitmap_set (swap_table, sec/SEC_PER_PG, false);
}
