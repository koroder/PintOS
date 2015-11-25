#ifndef FS_CACHE_H
#define FS_CACHE_H

#include <stdbool.h>
#include "threads/synch.h"
#include "devices/block.h"

#define CACHE_SIZE 64

struct bcache_entry
{
	bool valid;		// does the buffer contain vailid data
	bool dirty;		// is the buffer dirty
	bool access;     	// is the buffer accessed
	int op_cnt;		// no of operations(read or write) count
	bool used;

	block_sector_t sec;     // sector no of disk

	void *buffer;	        // buffer containing data

	struct lock block_lock; // per cache block lock

};

struct bcache_entry *buffer_cache[CACHE_SIZE];	// buffer cache

struct lock bcache_lock;	// global bcache lock

// helper sruct to implement clock algorithm
struct clock_helper
{
	block_sector_t sec;
	struct list_elem elem;
};

struct list clock_list;	// list to implement clock algorithm
bool first;
struct clock_helper *clock_ptr;

// structure to show entry that is during eviction
struct evicting_entry
{
	block_sector_t sec;
	struct list_elem elem;	
};

struct list evict_list;	// list of sectors that are being evicted currently

// structure to help in read ahead
struct read_ahead_entry
{
	block_sector_t sec;
	struct list_elem elem;	
};

struct list read_ahead_list;

void binit(void);
struct bcache_entry *bget(block_sector_t sec);
struct bcache_entry *entry_to_evict (void);
struct clock_helper *get_helper_by_sec (block_sector_t sec);
struct bcache_entry *get_bcache_by_sec (block_sector_t sec);
void cache_read(block_sector_t sec, void *buffer, int chunk_size, int offset);
void cache_write(block_sector_t sec,const void *buffer, int chunk_size, int offset);
bool search_evict (block_sector_t sec);
void flush_cache (void);
void read_ahed (void *aux);
void read_ahead_helpr (block_sector_t sec);
void timer_flsh (void *aux);
void read_ahead (void *aux);
void read_ahead_helper (block_sector_t sec);
void timer_flush (void *aux);

#endif /* filesys/cache.h */
