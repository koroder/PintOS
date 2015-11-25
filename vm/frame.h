#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include <stdbool.h>
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

void *frame_allocator(enum palloc_flags );
void frame_free (void *);
void free_list (struct list *);
void insert_vaddr (void *, void * );

//defining structure of processes which share the same phy frame (for sharing purpose)
struct alias_proc
{
	tid_t tid;
	struct list_elem elem;
};

//defining structure for an element of frame table
struct frame
{
	void *phy_frame;	// stores return value of palloc_get_page()
	void *uvaddr;	        // user virtual address corresponding to the physical frame
	//struct list tid_s;	// list of processes using the frame (for sharing purpose)
	tid_t tid;              // tid of the process which is allowed to use this physical frame

	struct list_elem elem;	// this will constitute the element of list
}; 

struct frame *get_elem_by_frame (void *);

struct list ftable_list;	// global list of frame table
struct lock ftable_lock;	// lock for the frame table
struct frame *clock_ptr;	// clock pointer - used for implementing the clock algorithm
bool frame_first;		// check whether the frame inserted is first...
				// ... clock ptr is initially set to this frame

#endif /* vm/frame.h */
