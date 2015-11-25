#include "filesys/inode.h"
//6616
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "cache.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

#define N_DIRECT 122
#define N_IN_DIRECT 128
#define N_DOUBLY_DIRECT 16384

#define OFS_DIRECT 122*BLOCK_SECTOR_SIZE
#define OFS_IN_DIRECT (OFS_DIRECT + 128*BLOCK_SECTOR_SIZE)
#define OFS_DOUBLY_DIRECT (OFS_IN_DIRECT + 16384*BLOCK_SECTOR_SIZE) // 128*128 = 16384

#define INDEX_DIRECT 121
#define INDEX_IN_DIRECT 122
#define INDEX_DOUBLY_DIRECT 123
//UNUSED 8 byte

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start[N_DIRECT + 2];  /* First data sector. */
    off_t length;                       /* File size in bytes. */
    int isdir;                          /* if the inode corresponds to dirctory*/
    block_sector_t parent;              /* sector number of parent inode*/
    unsigned magic;                     /* Magic number. */
   // uint32_t unused[2];               /* Not used. */
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    int isdir;                          /* if the inode corresponds to dirctory*/
    block_sector_t parent;		/* sector number of parent inode*/
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);

  uint32_t sec;
  off_t pos_dd;
  void *data_link = NULL;
  void *data_link_dd = NULL;

  struct inode_disk *data = malloc(sizeof(struct inode_disk));
  cache_read(inode->sector, data, BLOCK_SECTOR_SIZE, 0);

  off_t length = data->length;

  if (pos >= length)
  {
    free(data);
    return -1;
  }
  // assuming offset 0 means start[0]?? direct linking
  else if (pos < OFS_DIRECT)
  {
    block_sector_t ret = data->start[pos/BLOCK_SECTOR_SIZE];
    free(data);
    return ret;
  }
  // indirect linking
  else if (OFS_DIRECT <= pos && pos < OFS_IN_DIRECT)
  {

    pos = (pos - OFS_DIRECT)/BLOCK_SECTOR_SIZE;
    data_link = malloc (BLOCK_SECTOR_SIZE);

    cache_read(data->start[INDEX_IN_DIRECT], data_link, BLOCK_SECTOR_SIZE, 0);
    sec = *((uint32_t *)data_link + pos); 
    
    free(data);
    free(data_link);
    
    return sec;
  }
  // doubly direct linking
  else if (OFS_IN_DIRECT <= pos && pos < OFS_DOUBLY_DIRECT)
  {
    data_link = malloc (BLOCK_SECTOR_SIZE);
    data_link_dd = malloc (BLOCK_SECTOR_SIZE);

    pos_dd = (pos - OFS_DIRECT) % (OFS_IN_DIRECT - OFS_DIRECT);
    pos_dd = pos_dd/BLOCK_SECTOR_SIZE;

    pos = (pos - OFS_DIRECT) / (OFS_IN_DIRECT - OFS_DIRECT);
    pos = pos/BLOCK_SECTOR_SIZE;

    cache_read(data->start[INDEX_DOUBLY_DIRECT], data_link_dd, BLOCK_SECTOR_SIZE, 0);
    sec = *((uint32_t *)data_link_dd + pos);

    cache_read(sec, data_link, BLOCK_SECTOR_SIZE, 0);
    sec = *((uint32_t *)data_link + pos_dd);

    free(data);
    free(data_link);
    free(data_link_dd);

    return sec;

  }
  else
  {
    printf("undefine state: byte_to_sector\n");
    free(data);
    return -1;
  }
  
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length,block_sector_t sec, int isdir)
{
  int i,j,k;
 // printf("block sector = %d,%d\n",sector,isdir);
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

 static char zeros[BLOCK_SECTOR_SIZE];
 disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->isdir = isdir;
      disk_inode->parent = sec;
      disk_inode->magic = INODE_MAGIC;

      if (sectors == 0)
      {
        if (free_map_allocate (sectors, &disk_inode->start[0]))
        {
          cache_write(sector,disk_inode,BLOCK_SECTOR_SIZE,0);
          success = true; 
        }

      }      
      else if (sectors > 0) 
      {
        for (i=0; i < (int)sectors && i < N_DIRECT; i++)
        {
          if (free_map_allocate (1, &disk_inode->start[i])) 
          {
            cache_write(disk_inode->start[i], zeros, BLOCK_SECTOR_SIZE, 0);
            if (i == (int)sectors -1) success = true; 
          }
          
        }
        if (i < (int)sectors && free_map_allocate (1, &disk_inode->start[INDEX_IN_DIRECT]))
        {
          j=0;
          uint32_t *temp = malloc (BLOCK_SECTOR_SIZE);
          for (; i < (int)sectors && i < N_IN_DIRECT + N_DIRECT; i++)
          {
             if (free_map_allocate (1, (temp + j)))
             {
                cache_write(*(block_sector_t *)(temp + j), zeros, BLOCK_SECTOR_SIZE, 0);
                if (i == (int)sectors -1) success = true; 
             }

             j++; 
          }
          cache_write(disk_inode->start[INDEX_IN_DIRECT], temp, BLOCK_SECTOR_SIZE, 0);
          free(temp);
        }
        if (i < (int)sectors && free_map_allocate (1, &disk_inode->start[INDEX_DOUBLY_DIRECT]))
        {
          j=0;
          k=0;
          uint32_t *temp1 = malloc (BLOCK_SECTOR_SIZE);
          uint32_t *temp2 = malloc (BLOCK_SECTOR_SIZE);

          for (j=0; j < N_IN_DIRECT && i < (int)sectors && i < N_IN_DIRECT+N_DIRECT+N_DOUBLY_DIRECT; j++)
          {
            if (free_map_allocate (1,(temp1+j)))
            {
              for (k=0; k < N_IN_DIRECT && i<(int)sectors && i < N_IN_DIRECT+N_DIRECT+N_DOUBLY_DIRECT; k++,i++)
              {
                if (free_map_allocate (1, (temp2 + k)))
                {
                  cache_write(*(block_sector_t *)(temp2 + k), zeros, BLOCK_SECTOR_SIZE, 0);
                  if (i == (int)sectors -1) success = true; 
                }
              }
              cache_write(*(block_sector_t *)(temp1 + j), temp2, BLOCK_SECTOR_SIZE, 0);
            }
          }
          cache_write(disk_inode->start[INDEX_DOUBLY_DIRECT], temp1, BLOCK_SECTOR_SIZE, 0);
          free(temp1);
          free(temp2);
        }
        cache_write(sector,disk_inode,BLOCK_SECTOR_SIZE,0);
      }
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;
  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  //block_read (fs_device, inode->sector, &inode->data);
  struct inode_disk *data = malloc(sizeof(struct inode_disk));
  cache_read(inode->sector, data, BLOCK_SECTOR_SIZE, 0);
  inode->isdir= data->isdir;
  inode->parent = data->parent;
  free(data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  int i,j,k;
  struct inode_disk *data = malloc(sizeof(struct inode_disk));
  cache_read(inode->sector, data, BLOCK_SECTOR_SIZE, 0);

  off_t length = data->length;

  size_t sectors = bytes_to_sectors (length);
  

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);

          //-------------------------------------------//
          if (sectors == 0)
          {
            free_map_release (data->start[0], sectors);
          }
          else if (sectors > 0) 
          {
            for (i=0; i < (int)sectors && i < N_DIRECT; i++)
            {
              free_map_release (data->start[i], 1);          
            }
            if (i < (int)sectors)
            {
              j=0;
              uint32_t *temp = malloc (BLOCK_SECTOR_SIZE);
              cache_read(data->start[INDEX_IN_DIRECT], temp, BLOCK_SECTOR_SIZE, 0);

              for (; i < (int)sectors && i < N_IN_DIRECT + N_DIRECT; i++)
              {
                free_map_release (*(block_sector_t *)(temp + j), 1); 
                j++; 
              }
              free_map_release (data->start[INDEX_IN_DIRECT], 1);
              free (temp);
            }
            if (i < (int)sectors)
            {
              j=0;
              k=0;
              uint32_t *temp1 = malloc (BLOCK_SECTOR_SIZE);
              uint32_t *temp2 = malloc (BLOCK_SECTOR_SIZE);

              cache_read(data->start[INDEX_DOUBLY_DIRECT], temp1, BLOCK_SECTOR_SIZE, 0);

              for (j=0; j < N_IN_DIRECT && i < (int)sectors && i < N_IN_DIRECT+N_DIRECT+N_DOUBLY_DIRECT; j++)
              {
                cache_read((block_sector_t)(temp1+j), temp2, BLOCK_SECTOR_SIZE, 0);
                for (k=0; k < N_IN_DIRECT && i<(int)sectors && i < N_IN_DIRECT+N_DIRECT+N_DOUBLY_DIRECT; k++,i++)
                {
                  free_map_release (*(block_sector_t *)(temp2 + k), 1); 
                }
                free_map_release (*(block_sector_t *)(temp1 + j), 1); 
              }
              free_map_release (data->start[INDEX_DOUBLY_DIRECT], 1); 
              free(temp1);
              free(temp2);
            }
          } 
          //-------------------------------------------//
        }

      
      free (inode);
    }
  free(data);
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  //uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cache_read(sector_idx, buffer+ bytes_read, chunk_size, sector_ofs);
     
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  //free (bounce);

  return bytes_read;
}

bool inode_grow (off_t size, off_t offset, struct inode_disk *id)
{
  int i,j,k;
  size_t start_sec = bytes_to_sectors(id->length);
  size_t end_sec = bytes_to_sectors(offset + size);
  static char zeros[BLOCK_SECTOR_SIZE];
  bool success = false;

  if (start_sec == end_sec)
  {
    id->length = offset + size;
    return true;
  }

  for (i=start_sec; i < (int)end_sec && i < N_DIRECT; i++)
  {
    if (free_map_allocate (1, &id->start[i])) 
    {
      cache_write(id->start[i], zeros, BLOCK_SECTOR_SIZE, 0);
      if (i == (int)end_sec -1) success = true;
    }
          
  }
  if (i < (int)end_sec)
  {
    j=0;

    if (i == N_DIRECT)
    {
      free_map_allocate (1, &id->start[INDEX_IN_DIRECT]);
    }
    
    uint32_t *temp = malloc(BLOCK_SECTOR_SIZE);
    if (start_sec > N_DIRECT)
    {
      cache_read(id->start[INDEX_IN_DIRECT], temp, BLOCK_SECTOR_SIZE, 0);
      j = start_sec - N_DIRECT;
    }

    for (; i < (int)end_sec && i < N_IN_DIRECT + N_DIRECT; i++)
    {
      if (free_map_allocate (1, (temp + j)))
      {
        cache_write(*(block_sector_t *)(temp + j), zeros, BLOCK_SECTOR_SIZE, 0);
        if (i == (int)end_sec -1) success = true; 
      }
      j++;
    }
    cache_write(id->start[INDEX_IN_DIRECT], temp, BLOCK_SECTOR_SIZE, 0);
    free(temp);
  }
  if (i < (int)end_sec && free_map_allocate (1, &id->start[INDEX_DOUBLY_DIRECT]))
  {
    j=0;
    k=0;
    uint32_t *temp1 = malloc (BLOCK_SECTOR_SIZE);
    uint32_t *temp2 = malloc (BLOCK_SECTOR_SIZE);

    for (j=0; j < N_IN_DIRECT && i < (int)end_sec && i < N_IN_DIRECT+N_DIRECT+N_DOUBLY_DIRECT; j++)
    {
      if (free_map_allocate (1,(temp1+j)))
      {
        for (k=0; k < N_IN_DIRECT && i<(int)end_sec && i < N_IN_DIRECT+N_DIRECT+N_DOUBLY_DIRECT; k++,i++)
        {
          if (free_map_allocate (1, (temp2 + k)))
          {
            cache_write(*(block_sector_t *)(temp2 + k), zeros, BLOCK_SECTOR_SIZE, 0);
            if (i == (int)end_sec -1) success = true; 
          }
        }
        cache_write(*(block_sector_t *)(temp1 + j), temp2, BLOCK_SECTOR_SIZE, 0);
      }
    }
    cache_write(id->start[INDEX_DOUBLY_DIRECT], temp1, BLOCK_SECTOR_SIZE, 0);
    free(temp1);
    free(temp2);
  }
  id->length = offset + size;
  return success;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
 
  //printf("block sector = %d,%d\n",inode->sector, offset);

  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  if (inode->deny_write_cnt)
    return 0;

  struct inode_disk *id = malloc(sizeof(struct inode_disk));
  cache_read(inode->sector, id, BLOCK_SECTOR_SIZE, 0);

  if ( (size + offset) > id->length)
  {
    if(!inode_grow (size, offset, id))
    {
      free (id);
      return 0;
    }
    cache_write(inode->sector, id, BLOCK_SECTOR_SIZE, 0);
  } 

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cache_write(sector_idx,buffer + bytes_written,chunk_size,sector_ofs);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  //free (bounce);
  free (id);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)  
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  struct inode_disk *data = malloc(BLOCK_SECTOR_SIZE);
  cache_read(inode->sector, data, BLOCK_SECTOR_SIZE, 0);

  off_t length = data->length;
  free(data);

  return length;
}
block_sector_t
inode_get_sec (struct inode *inode) 
{
  return inode->sector;
}
block_sector_t
inode_get_parent (struct inode *inode) 
{
  return inode->parent;
}
int
inode_get_status (struct inode *inode) 
{
  return inode->isdir;
}
int
inode_get_removed (struct inode *inode) 
{
  return inode->removed;
}




