#include "filesys/filesys.h"
//7227
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
  flush_cache();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
//////////////////////////////////////////////////////////////////////////////////////////////
bool
filesys_create (const char *file_name, off_t initial_size) 
{
  char *name = give_name((char *)file_name);
  struct dir *dir=give_dir_parent((char *)file_name);  
  
  if(name==NULL || dir==NULL) 
  {
	dir_close (dir);
	return false;
  }
  block_sector_t inode_sector = 0;
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size,0,0)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *path_name)
{ 
   char *name = give_name((char *)path_name);
   struct dir *dir=give_dir_parent((char *)path_name);
   struct file *f = NULL;   
   struct inode *inode = NULL;
   
   if(name==NULL || dir==NULL) 
        {
          dir_close (dir);
	  return NULL;
	}
   if (strcmp(name,"/")==0)
   {
       dir_close (dir);
       dir=dir_open_root();
       f = (struct file *)dir;     
   }
  else if (strcmp(name,".")==0)
   {
       f = (struct file *)dir;     
   }
   else if (strcmp(name,"..")==0)
   {
       inode = dir_get_inode(dir);
       block_sector_t bp = inode_get_parent(inode);
       inode = inode_open(bp);
       if(!inode_get_status(inode))
     		        return NULL;
       dir_close(dir);
       dir = dir_open(inode);        
       f = (struct file *)dir;     
   }
   else
   {
   	if(!dir_lookup (dir, name, &inode))
   	{
            dir_close (dir);
	    return NULL;
  	} 
   	dir_close (dir);
   
        if(inode_get_status(inode)) 
        {  
            dir = dir_open(inode);     
            if(dir==NULL)    
                 return NULL;    
            f = (struct file *)dir;   
        }
   	else
  	{
     	    f = file_open (inode);
  	}
   }
   return f;
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *path_name) 
{
 // printf("path=%s\n",path_name);
  char *name = give_name((char *)path_name);
  struct dir *dir=give_dir_parent((char *)path_name);
  struct inode *inode;
  bool success = false;
  if(name==NULL || dir==NULL) 
        {
            dir_close(dir);
	     return false;
	}
  if(!dir_lookup (dir, name, &inode))
     	{
            dir_close(dir);
	     return false;
	}
  if(inode_get_status(inode)) 
  {
          struct dir_entry e;
          off_t ofs;
          for (ofs = 0; inode_read_at (inode, &e, sizeof e, ofs) == sizeof e; ofs += sizeof e) 
               if (e.in_use)
                        {
                            dir_close (dir);
    	         	      return false;
    	         	}
    	  success = dir_remove (dir, name); 
    	  dir_close (dir); 
  } 
  else
  {
          success = dir_remove (dir, name);
             dir_close (dir); 
  }
 // free(name);
  return success;
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  /////////////////////////////////////////////////////////////////////////////////////////////////////
  if (!dir_create (ROOT_DIR_SECTOR, 16,0,1))
  /////////////////////////////////////////////////////////////////////////////////////////////////////
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
