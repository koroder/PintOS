#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdbool.h>
#include <stdint.h>
#include <hash.h>
#include <debug.h>
#include "devices/block.h"
#include "filesys/file.h"

#define STACK_SIZE_MAX  (1 << 23)	// 8MB
#define NOT_APP_PTR NULL
#define NOT_APP_INT -1 

// defines where the page is located currently
struct placed
{
	bool ex;		// executable
	bool mmap;		// memory mapped
	bool swap;		// swap
	bool loaded;	        // loaded from the file
};


// shadow entry corresponding to the page
// together they will constitute the shadow page table
struct shadow_elem
{
	void *uvaddr;		// user virtual address
	struct placed where;    // current location of page
	bool writable;		// whether the page is writable
	
	//variables used for executables
	struct file *f_ex;
	off_t ofs_ex;
	uint32_t read_bytes_ex;
	uint32_t zero_bytes_ex;
	
	//variables for mmaped files
	struct file *f_mm;
	off_t ofs_mm;
	uint32_t read_bytes_mm;

	//variables for swapping space
	block_sector_t sec_no;	//sector number of the swap slot
				//applicable only if page is in swap
	
	struct hash_elem elem;  // will constitute the hash table
};

// structure for hash table of mapids-> filename
struct map_elem
{
     int mapid;				// unique mapid
     void *uvaddr;			// user VA
     int size;				// represents size to be mapped
     struct file *file;		        // file ptr
     struct hash_elem elem; 		// will constitute the hash table
};

unsigned shadow_hash (const struct hash_elem *p_, void *aux UNUSED);
bool shadow_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED);
bool create_shadow_entry_exec (struct file *file, off_t ofs, uint8_t *upage, 
		      	uint32_t read_bytes, uint32_t zero_bytes, bool writable);
struct shadow_elem *shadow_pg_tbl_lookup (struct hash *ht, void *uvaddr);
void demand_page (struct shadow_elem *s);
bool load_frm_exec (struct shadow_elem *s);
bool is_valid_stack_access (void *addr, void *esp);
bool grow_stack (void *addr);

unsigned map_hash (const struct hash_elem *p_, void *aux UNUSED);
bool map_less (const struct hash_elem *a_, const struct hash_elem *b_,void *aux UNUSED);
struct map_elem *map_table_lookup (struct hash *ht, int mapid);
bool create_shadow_entry_mmap (struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes);
bool load_frm_mmap (struct shadow_elem *s);
bool load_frm_swap (struct shadow_elem *s);
void shadow_destructor (struct hash_elem *he, void *aux UNUSED);
void map_destructor (struct hash_elem *he, void *aux UNUSED);

#endif
