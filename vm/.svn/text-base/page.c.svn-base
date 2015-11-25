#include <string.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "frame.h"
#include "page.h"
#include "swap.h"


static bool install_page (void *upage, void *kpage, bool writable);

/* Returns a hash value for page p. */
unsigned
shadow_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct shadow_elem *p = hash_entry (p_, struct shadow_elem, elem);
  return hash_bytes (&p->uvaddr, sizeof p->uvaddr);
}

/* Returns true if page a precedes page b. */
bool
shadow_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
  const struct shadow_elem *a = hash_entry (a_, struct shadow_elem, elem);
  const struct shadow_elem *b = hash_entry (b_, struct shadow_elem, elem);

  return a->uvaddr < b->uvaddr;
}

/* Returns the page containing the given virtual address,
   or a null pointer if no such page exists. */
struct shadow_elem *
shadow_pg_tbl_lookup (struct hash *ht, void *uvaddr)
{
  struct shadow_elem s;
  struct hash_elem *e;

  s.uvaddr = uvaddr;
  e = hash_find (ht, &s.elem);
  return e != NULL ? hash_entry (e, struct shadow_elem, elem) : NULL;
}

unsigned
map_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct map_elem *p = hash_entry (p_, struct map_elem, elem);
  return hash_bytes (&p->mapid, sizeof p->mapid);
}

/* Returns true if page a precedes page b. */
bool
map_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
  const struct map_elem *a = hash_entry (a_, struct map_elem, elem);
  const struct map_elem *b = hash_entry (b_, struct map_elem, elem);

  return a->mapid < b->mapid;
}

/* Returns the page containing the given virtual address,
   or a null pointer if no such page exists. */
struct map_elem *
map_table_lookup (struct hash *ht, int mapid)
{
  struct map_elem s;
  struct hash_elem *e;

  s.mapid = mapid;
  e = hash_find (ht, &s.elem);
  return e != NULL ? hash_entry (e, struct map_elem, elem) : NULL;
}

// destructs and frees the sahdow page table corresponding to the process
void shadow_destructor (struct hash_elem *he, void *aux UNUSED)
{
  struct shadow_elem *s = hash_entry(he, struct shadow_elem, elem);
  struct thread *t = thread_current();
  if(!(s->where).ex && !(s->where).mmap)
  {
   if ((s->where).swap)
       {
           swap_free (s);
       }
   else
       {
          void *kadd = pagedir_get_page (t->pagedir, (void *)s->uvaddr);
          frame_free(kadd);
       }

  }
  if((s->where).ex)
  {
     if((s->where).loaded)
     {
        if ((s->where).swap)
        {
            swap_free (s);
        }
        else
        {
            void *kadd = pagedir_get_page (t->pagedir, (void *)s->uvaddr);
            frame_free(kadd);
        }
      }
  }
}

// destructs and frees the map table corresponding to the process
void map_destructor (struct hash_elem *he, void *aux UNUSED)
{
  struct map_elem *m = hash_entry(he, struct map_elem, elem);
  free (m);
}


//creates a shadow entry corresponding to executable in shadow page table
bool create_shadow_entry_exec (struct file *file, off_t ofs, uint8_t *upage, 
		      	uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  struct shadow_elem *s;   	  	
  s = malloc ( sizeof (struct shadow_elem));
  s->uvaddr = (void *) upage;
  
  // sets the location variables
  (s->where).ex = true;
  (s->where).mmap = false;
  (s->where).swap = false;
  (s->where).loaded = false;

  s->f_ex = file;
  s->ofs_ex = ofs;               // sets the offset
  s->read_bytes_ex = read_bytes; // stores read bytes
  s->zero_bytes_ex = zero_bytes; // stores zero bytes
  s->writable = writable;

  s->f_mm = NOT_APP_PTR;
  s->ofs_mm = NOT_APP_INT;
  s->read_bytes_mm = NOT_APP_INT;

  struct thread *t = thread_current ();
  if( hash_insert (&t->shadow_pg_tbl, &s->elem) != NULL )
	return false;

  return true;

}

// creates the shadow page table entry corresponding to the memory mapped files
bool create_shadow_entry_mmap (struct file *file, off_t ofs, uint8_t *upage, 
            uint32_t read_bytes)
{
  struct shadow_elem *s;    
  s = malloc ( sizeof (struct shadow_elem));
  s->uvaddr = (void *) upage;
  
  // sets the location variables
  (s->where).ex = false;
  (s->where).mmap = true;
  (s->where).swap = false;
  (s->where).loaded = false;

  s->f_ex = NOT_APP_PTR;
  s->ofs_ex = NOT_APP_INT;
  s->read_bytes_ex = NOT_APP_INT;
  s->zero_bytes_ex = NOT_APP_INT;
  s->writable = NOT_APP_INT;

  s->f_mm = file;                 // stores file ptr
  s->ofs_mm = ofs;                // stores the offset
  s->read_bytes_mm = read_bytes;  // stores the read bytes

  struct thread *t = thread_current ();
  if( hash_insert (&t->shadow_pg_tbl, &s->elem) != NULL )
    return false;

  return true;

}

// load page depending upon where it is placed
void demand_page (struct shadow_elem *s)
{
  // loads from executable
  if ((s->where).ex && !(s->where).loaded )
    load_frm_exec (s);
  // loads from mmap
  else if ((s->where).mmap)
    load_frm_mmap (s);
  // loads from swap space
  else if ((s->where).swap)
    load_frm_swap (s);
    
}

//load the page from executable file
bool load_frm_exec (struct shadow_elem *s)
{
	
  // reading in variables
  struct file *file = s->f_ex; 
  off_t ofs = s->ofs_ex;
  uint8_t *upage = (uint8_t *)s->uvaddr;
  uint32_t read_bytes = s->read_bytes_ex; 
  uint32_t zero_bytes = s->zero_bytes_ex;
  bool writable = s->writable;

  size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
  size_t page_zero_bytes = PGSIZE - page_read_bytes;

  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
    /* Get a frame of memory. */
  uint8_t *kpage = frame_allocator (PAL_USER);
  if (kpage == NULL)
      return false;

  lock_acquire(&filesys_lock);
  file_seek (file, ofs);
  if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
  {
        // if not successful in reading, free the frame
        frame_free (kpage);
        lock_release(&filesys_lock);
        return false; 
  }
  lock_release(&filesys_lock);

  // set to the zero the remaining bytes of page
  memset (kpage + page_read_bytes, 0, page_zero_bytes);

  //insert uvaddr in the element of ftable_list
  insert_vaddr (kpage, upage); 

  /* Add the page to the process's address space. */
  if (!install_page (upage, kpage, writable)) 
  {
       // if not successful in reading, free the frame
       frame_free (kpage);
       return false; 
  }

  // set location variables
  (s->where).loaded = true;
  return true;
}

//load the page from mmaped file
bool load_frm_mmap (struct shadow_elem *s)
{
        
        // read in variables
        struct file *file = s->f_mm; 
        off_t ofs = s->ofs_mm;
        uint8_t *upage = (uint8_t *)s->uvaddr;
        uint32_t read_bytes = s->read_bytes_mm; 

        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        lock_acquire(&filesys_lock);
        file_seek (file, ofs);
        lock_release(&filesys_lock);
        /* Get a frame of memory. */
        uint8_t *kpage = frame_allocator (PAL_USER);
        if (kpage == NULL)
                 return false;      // for swapping

        /* Load this page. */
        lock_acquire(&filesys_lock);
        if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
            // if not successful in reading, free the frame
            frame_free (kpage);
            lock_release(&filesys_lock);
            return false; 
        }
        lock_release(&filesys_lock);

        // set the remaining bytes of pages to zero
        memset (kpage + page_read_bytes, 0, page_zero_bytes);

        //insert uvaddr in the element of ftable_list
        insert_vaddr (kpage, upage); 

        /* Add the page to the process's address space. */
        if (!install_page (upage, kpage, true)) 
        {
           // if not successful in installing, free the frame
           frame_free (kpage);
           return false; 
        }
        // set the location variable
        (s->where).loaded = true;
        return true;
}

// load the page from swap spacw
bool load_frm_swap (struct shadow_elem *s)
{
    /* get a frame of memrory */
    uint8_t *kpage = frame_allocator (PAL_USER);
    if (kpage == NULL)
      return false;

    // read in from swap
    swap_remove (kpage, s->sec_no);

    // insert vaddr corresponding to the kpage in
    // frame table    
    insert_vaddr (kpage, s->uvaddr);

    // install the page
    if (!install_page (s->uvaddr, kpage, true)) 
    {
      // if not successful in installing, free the frame
      frame_free (kpage);
      return false; 
    }

    // set the location variable
    (s->where).swap = false;
    return true;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

// heuristic to check whether it is a valid stack access
//--------?? make sure for the kernal faulting addrss ==
// need not do this, address is verified before accessing
bool is_valid_stack_access (void *addr, void *esp)
{
  bool valid = false;
  // 1. check that esp is within 32 bytes of faulting address
  // 2. checks that stack size dont grow above maximum size
  if ( ( addr >= (esp - 32) ) && (PHYS_BASE - pg_round_down (addr)) <= STACK_SIZE_MAX )
    valid = true;

  return valid;
}

// grow user's stack, address
// validity is verified earlier
bool grow_stack (void *addr)
{
  /* get in physical frame */
  uint8_t *stk_pg = frame_allocator (PAL_USER);

  if (stk_pg == NULL)
     return false;
  insert_vaddr (stk_pg, pg_round_down( addr));

  // installing the page
  if (!install_page (pg_round_down(addr), stk_pg, true)) 
  {
    // if not successful in installing, free the frame
    frame_free (stk_pg);
    return false; 
  }

  return true;  
}

