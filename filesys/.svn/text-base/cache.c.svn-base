#include  <string.h>
#include "cache.h"
#include "threads/malloc.h"
#include "filesys.h"
#include "threads/thread.h"
#include "devices/timer.h"

void binit(void)
{
	int i;
	
	lock_init (&bcache_lock);
	list_init (&clock_list);
	list_init (&evict_list);
	list_init (&read_ahead_list);
	first = true;

	for (i=0; i < CACHE_SIZE; i++)
	{
		buffer_cache[i] = malloc (sizeof(struct bcache_entry));
		buffer_cache[i]->valid = false;
		buffer_cache[i]->dirty = false;
		buffer_cache[i]->access = false;
		buffer_cache[i]->op_cnt = 0;
		buffer_cache[i]->sec = -1;
		buffer_cache[i]->used = false;
		buffer_cache[i]->buffer = malloc (BLOCK_SECTOR_SIZE);

		lock_init (&buffer_cache[i]->block_lock);
	}

	// thread for write ahead
	thread_create ("read_ahead", PRI_DEFAULT, read_ahed, NULL);
	// thread for timer flush
	thread_create ("timer_flush", PRI_DEFAULT, timer_flsh, NULL);
}

struct bcache_entry *bget(block_sector_t sec)
{

	int i;
	struct clock_helper *temp;

	lock_acquire (&bcache_lock);

	bool in_evict = search_evict(sec);
	if (in_evict) thread_yield();

	for (i=0; i < CACHE_SIZE; i++)
	{
		if( buffer_cache[i]->sec == sec)
		{
			lock_acquire (&buffer_cache[i]->block_lock);
			buffer_cache[i]->op_cnt++;
			lock_release(&bcache_lock);
			lock_release (&buffer_cache[i]->block_lock);
			return buffer_cache[i];
		}
	}

	for (i=0; i < CACHE_SIZE; i++)
	{
		if(! buffer_cache[i]->used)
		{
			lock_acquire (&buffer_cache[i]->block_lock);
			buffer_cache[i]->sec = sec;
			buffer_cache[i]->op_cnt++;
			buffer_cache[i]->used = true;

			temp = malloc (sizeof(struct clock_helper));
			temp->sec = sec;
			if (first)
			{
				clock_ptr = temp;
				list_push_back (&clock_list, &temp->elem);
				first = false;
			}
			else
				list_insert (&clock_ptr->elem, &temp->elem);

			lock_release(&bcache_lock);
			lock_release (&buffer_cache[i]->block_lock);
			return buffer_cache[i];
		}
	}

	// not found, needs eviction
	struct bcache_entry *to_evict = entry_to_evict();

	lock_acquire (&to_evict->block_lock);
	block_sector_t sec_old = to_evict->sec;
	to_evict->sec = sec;

	temp = get_helper_by_sec (sec_old);
	temp->sec = sec;

	lock_release(&bcache_lock);

	if (to_evict->dirty)
	{
		struct evicting_entry *e = malloc (sizeof(struct evicting_entry));
		e->sec = sec_old;
		list_push_back (&evict_list, &e->elem);
		block_write (fs_device, sec_old, to_evict->buffer);
		list_remove (&e->elem);
		free(e);
	}

	block_read (fs_device, to_evict->sec, to_evict->buffer);
	to_evict->op_cnt++;

	lock_release(&to_evict->block_lock);
	return to_evict;

}

struct bcache_entry *entry_to_evict (void)
{
	struct list_elem *f;
  	struct bcache_entry *ret;
  	struct clock_helper *ch;
  
 	ret = NULL;
        for (f = &clock_ptr->elem; ; f = list_next (f))
        {
 	     // checks for the list tail
 	     if(!(f != NULL && f->prev != NULL && f->next == NULL))
             {
                      ch = list_entry (f, struct clock_helper, elem);
                      ret = get_bcache_by_sec (ch->sec);
                      bool access = ret->access;
           
                      // if the page has access_bit = 1 then set it to 0
                      if (access)
            	      {
                             ret->access = false;
            	      }
                      // else evict the page
                      else if (ret->op_cnt > 0)
                      	continue;
                      else
                      {
              	             // move the clock ptr next to evicting frame
                             clock_ptr = list_entry(list_next(f), struct clock_helper, elem);
                             return ret ;  
                      }
                      // if reach the list end move to beginning - clock aspect
                      if (f == list_end (&clock_list))
                      {
        		     		f = list_begin (&clock_list);
                      }
              }
              // if reach the list end move to beginning - clock aspect
              else
              {
                      f=list_begin(&clock_list);
              }
        }
        // should panic if returns here
        // meaning cannot find the frame to evict
        PANIC ("eviction_clcok error");
        return NULL; //the function should never reach here
}

void read_ahed (void *aux UNUSED)
{
	thread_exit ();
	return;
}
void timer_flsh (void *aux UNUSED)
{
	thread_exit ();
	return;
}
void read_ahead_helpr (block_sector_t sec UNUSED)
{
	return;
}

struct clock_helper *get_helper_by_sec (block_sector_t sec)
{
  struct list_elem *f;
  struct clock_helper *ret;
  
  ret = NULL;
  for (f = list_begin (&clock_list); f != list_end (&clock_list); f = list_next (f))
    {
      ret = list_entry (f, struct clock_helper, elem);
      if (ret->sec == sec)
        return ret;
    }
      
  return NULL;
}

struct bcache_entry *get_bcache_by_sec (block_sector_t sec)
{
	int i;
	for (i=0; i < CACHE_SIZE; i++)
	{
		if( buffer_cache[i]->sec == sec)
			return buffer_cache[i];
	}
	return NULL;
}

void cache_read(block_sector_t sec, void *buffer, int chunk_size, int offset)
{
      read_ahead_helpr (sec+1);

      struct bcache_entry *be = bget (sec);
      memcpy(buffer,be->buffer + offset,chunk_size);
      lock_acquire(&be->block_lock);
      be->access  = true;
      be-> op_cnt --;
      lock_release(&be->block_lock);      
}

void cache_write(block_sector_t sec, const void *buffer, int chunk_size, int offset)
{
 	  struct bcache_entry *be = bget (sec);
 	  memcpy(be->buffer+offset, buffer, chunk_size);
 	  lock_acquire(&be->block_lock);
      be->access  = true;
      be->dirty = true;
      be-> op_cnt --;
      lock_release(&be->block_lock); 
}

bool search_evict (block_sector_t sec)
{
  struct list_elem *f;
  struct evicting_entry *ret;
  
  ret = NULL;
  for (f = list_begin (&evict_list); f != list_end (&evict_list); f = list_next (f))
    {
      ret = list_entry (f, struct evicting_entry, elem);
      if (ret->sec == sec)
        return true;
    }
      
  return false;
}

void flush_cache (void)
{
	int i;
	for (i=0; i < CACHE_SIZE; i++)
	{
		if (buffer_cache[i]->dirty && buffer_cache[i] != NULL)
		{
			block_write (fs_device, buffer_cache[i]->sec, 
									buffer_cache[i]->buffer);
		}
	}
}

void read_ahead_helper (block_sector_t sec)
{
	struct read_ahead_entry *rae = malloc (sizeof(struct read_ahead_entry));
    rae->sec = sec + 1;
    list_push_back (&read_ahead_list, &rae->elem);
}

void read_ahead (void *aux UNUSED)
{
	while (true)
  	{
  		while (!list_empty (&read_ahead_list))
     	{
       		struct list_elem *f = list_pop_front (&read_ahead_list);
       		struct read_ahead_entry *ret = list_entry (f, struct read_ahead_entry, elem);
       		bget (ret->sec);
       		free (ret);	
     	}
    }
}

void timer_flush (void *aux UNUSED)
{
	while (true)
  	{
    	timer_sleep (5);
    	flush_cache ();
  	}
}
