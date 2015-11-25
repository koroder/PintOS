#include "threads/malloc.h"
#include "frame.h"
#include "swap.h"

// allocates a physical frame and
// creates its corresponding list_entry
// in frame table.
void *frame_allocator(enum palloc_flags flag)
{
	void *frame = palloc_get_page (flag);
        
	if (frame == NULL) // eviction
	{
		// if we get a NULL frame call the eviction algorithm
		// this will free the frame, which can then be used
		eviction();
		frame = palloc_get_page (flag);
		if (frame == NULL)
			PANIC ("cannot allocate frame after eviction");
	}

	// struct alias_proc *temp1 = malloc (sizeof(struct alias_proc));
	// temp1->tid = thread_current()->tid;

	struct frame *temp2 = malloc(sizeof(struct frame));
	temp2->phy_frame = frame;
	//list_init (&temp2->tid_s);
	temp2->tid = thread_current()->tid;
	if (!frame_first)
	{
	         lock_acquire (&ftable_lock);
	         list_push_back (&ftable_list, &temp2->elem);
	         lock_release (&ftable_lock);
             frame_first = true;
		 	 clock_ptr = temp2;		// sets the clock ptr to initial frame
	}
	else
	{
    		lock_acquire(&ftable_lock);
    		list_insert (&clock_ptr->elem, &temp2->elem);
    		lock_release(&ftable_lock);
	}


	return frame;
}

// removes frame entry from
// the frame table
void frame_free (void *frame)
{
	struct frame *temp;
	
	// freeing the structure corresponding to frame in ftable list
	lock_acquire (&ftable_lock);
	temp = get_elem_by_frame (frame);

	if(temp == NULL)
		return;

	list_remove (&temp->elem);
	free (temp);
	lock_release (&ftable_lock);
   
	// freeing the page designated by frame.
	palloc_free_page (frame);
	return;
}

// returns the element corresponding to frame
// in frame table
struct frame *get_elem_by_frame (void *frame)
{
        struct list_elem *f;
        struct frame *ret;
  
        ret = NULL;
        for (f = list_begin (&ftable_list); f != list_end (&ftable_list); f = list_next (f))
        {
             ret = list_entry (f, struct frame, elem);
             if (ret->phy_frame == frame)
                   return ret;
        }
      
        return NULL;
}

// inserts uvaddr field corresponding to frame
// in frame table  
void insert_vaddr (void *frame, void *uvaddr)
{
	struct frame *temp = get_elem_by_frame (frame);
	temp->uvaddr = uvaddr;
	return;
}
