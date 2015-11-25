#include "userprog/syscall.h"
//14406
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
///////////////////////////////////////////////////////////////////////////////
#include "filesys/directory.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
///////////////////////////////////////////////////////////////////////////////
#include "process.h"
#include "pagedir.h"
#include "devices/input.h"

//#define DEBUG
#include "debug_helper.h"

extern struct lock filesys_lock;

int is_valid_address(void* add);
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  if(f)
  {
    stack_address_check(f->esp);

    int sys_call_no = get_nth_arg_int(f->esp, 0);

    DPRINTF("system call! %d\n", sys_call_no);
    switch(sys_call_no)
    {
      case SYS_HALT:
        power_off();
        break;
      case SYS_EXIT:
        {
          int status = get_nth_arg_int(f->esp, 1);
          thread_current()->exit_status = status;
          process_terminate();
          return;
        }
        break;
      case SYS_EXEC:
        {
          char* file_name = (char*)get_nth_arg_ptr(f->esp, 1);
          user_string_add_range_check_and_terminate(file_name); 
          int tid = sys_exec(file_name);
          DPRINTF("exec %s, tid = %d\n", file_name, tid);
          f->eax = tid;
        }
        return;
      case SYS_WAIT:
        {
          int pid = get_nth_arg_int(f->esp, 1);
          int ret = process_wait(pid);
          DPRINTF("wait for pid %d by %d return %d\n", pid, thread_current()->tid ,ret);
          f->eax = ret;
        }
        return;
      case SYS_CREATE:
        {
          char* file_name = (char*)get_nth_arg_ptr(f->esp, 1); 
          user_string_add_range_check_and_terminate(file_name);
          int size = get_nth_arg_int(f->esp, 2);
          DPRINTF("sys_create(%s,%d)\n", file_name, size);
          f->eax = sys_create(file_name, size);
        }
        return;
      case SYS_REMOVE:
        {
          char* file_name = (char*)get_nth_arg_ptr(f->esp, 1); 
          user_string_add_range_check_and_terminate(file_name); 
          DPRINTF("sys_remove(%s)\n", file_name);
          f->eax = sys_remove(file_name);
        }
        return;
      case SYS_OPEN:
        {
          char* file_name = (char*)get_nth_arg_ptr(f->esp, 1); 
          user_string_add_range_check_and_terminate(file_name); 
          DPRINTF("sys_open(%s)\n", file_name);
          f->eax = sys_open(file_name);
        }
        return; 
      case SYS_FILESIZE:
        {
          int fd = get_nth_arg_int(f->esp, 1);
          DPRINTF("sys_filesize(%d)\n", fd);
          f->eax = sys_filesize(fd);
        }
        return;
      case SYS_READ:
        {
          int fd = get_nth_arg_int(f->esp, 1);
          char* buf = (char*)get_nth_arg_ptr(f->esp, 2); 
          int size = get_nth_arg_int(f->esp, 3);
          user_add_range_check_and_terminate(buf, size);
          DPRINTF("sys_read(%d,%s,%d)\n", fd, buf, size);
          f->eax = sys_read(fd, buf, size);
        }
        return;
      case SYS_WRITE:
        {
          int fd = get_nth_arg_int(f->esp, 1);
          char* buf = (char*)get_nth_arg_ptr(f->esp, 2); 
          int size = get_nth_arg_int(f->esp, 3);
          user_add_range_check_and_terminate(buf, size);
          DPRINTF("sys_write(%d,%s,%d)\n", fd, buf, size);
          f->eax = sys_write(fd, buf, size);
        }
        return;
      case SYS_SEEK:
        {
          int fd = get_nth_arg_int(f->esp, 1);
          unsigned pos = get_nth_arg_int(f->esp, 2);
          DPRINTF("sys_seek(%d,%d)\n", fd, pos);
          sys_seek(fd, pos);
        }
        return;
      case SYS_TELL:
        {
          int fd = get_nth_arg_int(f->esp, 1);
          DPRINTF("sys_tell(%d)\n", fd);
          f->eax = sys_tell(fd);
        }
        return;
      case SYS_CLOSE:
        {
          int fd = get_nth_arg_int(f->esp, 1);
          DPRINTF("sys_close(%d)\n", fd);
          sys_close(fd);
        }
        return;
///////////////////////////////////////////////////////////////////////////////////
      case SYS_CHDIR:
      	{
      	      char* path_name = (char*)get_nth_arg_ptr(f->esp, 1); 
              user_string_add_range_check_and_terminate(path_name);   
              DPRINTF("sys_chdir %s\n", path_name);
              f->eax = sys_chdir(path_name); 
      	}
      	return;
      case SYS_MKDIR:
      	{
      	      char* path_name = (char*)get_nth_arg_ptr(f->esp, 1); 
              user_string_add_range_check_and_terminate(path_name);   
              DPRINTF("sys_mkdir %s\n", path_name);
              f->eax = sys_mkdir(path_name);      	       
      	}
      	return;
      case SYS_READDIR:
      	{
      	      int fd = get_nth_arg_int(f->esp, 1);
      	      char* name = (char*)get_nth_arg_ptr(f->esp, 2); 
              user_string_add_range_check_and_terminate(name); 
              DPRINTF("sys_mkdir %d,%s\n", fd,name);
              f->eax = sys_readdir(fd,name);                   	
      	}
      	return;
      case SYS_ISDIR:
      	{
      	      int fd = get_nth_arg_int(f->esp, 1);
              DPRINTF("sys_isdir(%d)\n", fd);
              f->eax=sys_inumber(fd);      	
      	}
      	return;
      case SYS_INUMBER:
      	{
      	      int fd = get_nth_arg_int(f->esp, 1);
              DPRINTF("sys_inumber(%d)\n", fd);
              f->eax = sys_inumber(fd);      	
      	}
      	return;  
 ////////////////////////////////////////////////////////////////////////////////////       
      /*
      case SYS_MMAP:
      case SYS_MUNMAP:
      */
      default:
        thread_exit();
        break;
    }
  }
  else
    thread_exit();
}

// returns nth int value from stack
// it terminates if the address is
// invalid
int get_nth_arg_int(void* esp, int n)
{
  user_add_range_check_and_terminate((char*)((int*)esp+n),4); 
  return *((int*)esp + n);
}

// returns nth ptr value from the stack
// terminates the process if the stack address and the 
// ptr value are invalid
void* get_nth_arg_ptr(void* esp, int n)
{
  user_add_range_check_and_terminate((char*)((int*)esp+n),4); 
  user_add_range_check_and_terminate((char*)(*((int*)esp + n)), 1); 
  return (void*)(*((int*)esp + n));
}

// check for a range of address for validity
// the address should be below PHYS_BASE and
// should be mapped in the page_dir
// Range: [start, start+size-1]
//
// return 0/1 if invalid or valid
// * does not teminate the process
// * It checks at the page boundary only
int user_add_range_check(char* start, int size)
{
  unsigned ptr;

  for(ptr = (unsigned)start; ptr < (unsigned)(start+size); 
      ptr = ptr + (PGSIZE - ptr % PGSIZE))  // jump to last entry of a page
    if(!is_valid_address((void*)ptr))
      return 0;

  return 1;
}

//  helper function, it checks for validity of range of addresses
//  and terminates also if range is invalid
//
void user_add_range_check_and_terminate(char* start, int size)
{
  if(!user_add_range_check(start, size))
  {
    thread_current()->exit_status = -1;
    process_terminate();
  }
}

// checks for validity of user string
// terminates the process if invalid
void user_string_add_range_check_and_terminate(char* str)
{
  char* tmp = str;
  user_add_range_check_and_terminate(tmp, 1);
  while(*tmp) // loop untill NULL is found
  {
    ++tmp;
    user_add_range_check_and_terminate(tmp, 1); // check and terminate
  }
}

//  checks for valid address
//  add should not be NULL
//      should be below PHYS_BASE
//      should be mapped in the the page_dir
int is_valid_address(void* add)
{
  if(!add)
    return 0;
  if(add >= (void*)PHYS_BASE)
    return 0;
  if(!pagedir_get_page(thread_current()->pagedir, add))
    return 0;
  
  return 1;
}

// check if the stack is valid
void stack_address_check(void* esp)
{
  if(!is_valid_address(esp) || (int*)esp >= ((int*)PHYS_BASE - 2))
  {
      thread_current()->exit_status = -1;
      process_terminate();
  }
}

// prints the exit code and terminates the process
void process_terminate(void)
{
  struct thread *t = thread_current ();
  printf ("%s: exit(%d)\n", t->name, t->exit_status);  
  thread_exit();
}


int sys_exec(char* filename)
{
  tid_t tid = process_execute(filename);
  if(tid == -1) // process execute failed, return -1
    return -1;
  else  // tid is valid
  {
    intr_disable();
    thread_block(); // block myself, until child wakes me up 
                    // with the exec_status, telling me if the elf load was successful
    intr_enable();

    struct thread* child = get_thread_from_tid(tid);
   
    if(child)
    {
      // exec_status will be -1 if load failed, hence we return -1
      // in such case
      tid = child->exec_status;
      child->parent_waiting_exec = 0;
      // child had blocked itself, unblock it
      thread_unblock(child);
    }
    return tid;
  }
}

int sys_open(char* file_name)
{
  if(!*file_name)  // empty string check
    return -1;
  struct thread *t = thread_current ();
  int i = 2;
  for(; i < FDTABLESIZE; ++i) // find the first free FD
  {
    if(!t->fd_table[i])
    {
      lock_acquire(&filesys_lock);
      struct file* fi = filesys_open(file_name);
      if(fi)
        t->fd_table[i] = fi;
      lock_release(&filesys_lock);
      if(fi)
        return i;
      else
        return -1;
    }
  }
  return -1;
}

int sys_create(char* file_name, int size)
{
  int ret;
  if(!*file_name)  // empty string check
    return 0;
  lock_acquire(&filesys_lock);
  ret = filesys_create(file_name, size);
  lock_release(&filesys_lock);
  return ret;
}

int sys_remove(char* file_name)
{
  int ret;
  if(!*file_name)  // empty string check
    return 0;
  lock_acquire(&filesys_lock);
  ret = filesys_remove(file_name);
  lock_release(&filesys_lock);
  return ret;
}

void sys_close(int fd)
{
  if(fd >= 0 && fd < FDTABLESIZE) // is valid FD?
  {
    struct thread *t = thread_current ();
    struct file* fi = t->fd_table[fd];
    if(fi)
    {
      lock_acquire(&filesys_lock);
      file_close(fi);
      lock_release(&filesys_lock);
      t->fd_table[fd] = 0;
    }
  }
}

int sys_write(int fd, void *buffer, unsigned size)
{
  if(fd == STDIN_FILENO)
    return 0;
  else if(fd == STDOUT_FILENO)
  {
    putbuf(buffer, size);
    return size;
  }
  else if(fd < FDTABLESIZE && fd > 1)
  {
    struct thread* cur = thread_current ();
    struct file* fi = cur->fd_table[fd];
    if(fi)
    {
      int ret;
      lock_acquire(&filesys_lock);
      ret = file_write(fi, buffer, size);
      lock_release(&filesys_lock);
      return ret;
    }
  }
  return 0;
}

int sys_read(int fd, void* buffer, unsigned size)
{
  if(fd == STDOUT_FILENO)
    return 0;
  else if(fd == STDIN_FILENO)
  {
    unsigned i = 0;
    char* buf = (char*) buffer;
    for(; i < size; ++i)
      buf[i] = input_getc();
    return 0;
  }
  else if(fd < FDTABLESIZE && fd > 1)
  {
    struct thread *cur = thread_current ();
    struct file* fi = cur->fd_table[fd];
    if(fi)
    {
      int ret;
      lock_acquire(&filesys_lock);
      ret = file_read(fi, buffer, size);
      lock_release(&filesys_lock);
      return ret;
    }
  }
  return 0;
}

int sys_filesize(int fd)
{
  if(fd >= 0 && fd < FDTABLESIZE)
  {
    struct thread *t = thread_current ();
    struct file* fi = t->fd_table[fd];
    if(fi)
    {
      int ret;
      lock_acquire(&filesys_lock);
      ret = file_length(fi);
      lock_release(&filesys_lock);
      return ret;
    }
  }
  return -1;
}

void sys_seek(int fd, unsigned pos)
{
  if(fd >= 0 && fd < FDTABLESIZE)
  {
    struct thread *t = thread_current ();
    struct file* fi = t->fd_table[fd];
    if(fi)
    {
      lock_acquire(&filesys_lock);
      file_seek(fi, pos);
      lock_release(&filesys_lock);
    }
  }
}

unsigned sys_tell(int fd)
{
  if(fd >= 0 && fd < FDTABLESIZE)
  {
    struct thread *t = thread_current ();
    struct file* fi = t->fd_table[fd];
    if(fi)
    {
      unsigned ret;
      lock_acquire(&filesys_lock);
      ret = file_tell(fi);
      lock_release(&filesys_lock);
      return ret;
    }
  }
  return -1;
}
////////////////////////////////////////////////////////////////////////////////////
int sys_mkdir(char *path_name)
{
  char *name;
  block_sector_t b=0;
  struct inode *ip;
  lock_acquire(&filesys_lock);
  struct dir *dir = give_dir_parent(path_name); 
  name = give_name(path_name);

  if(name==NULL || dir==NULL) 
       {lock_release(&filesys_lock);
  	return 0;}
  	
  if(!free_map_allocate (1,&b))	
  	{lock_release(&filesys_lock);
  	return 0;}
  ip = dir_get_inode(dir);	
  if(!dir_create (b,16,inode_get_sec(ip),1))
  	{lock_release(&filesys_lock);
  	return 0;}
  if(!dir_add (dir, name, b))
  	{lock_release(&filesys_lock);
  	return 0;}	
  dir_close(dir);

  lock_release(&filesys_lock);
  // free(name);
  return 1;  
}
int sys_chdir(char *path_name)
{
     char *name=give_name(path_name) ;
     struct inode *ip1;
     struct inode *inode;
     struct dir *dir = give_dir_parent(path_name) ;  
     struct thread *t = thread_current();
     
     if(name==NULL || dir==NULL) 
  	return 0;
     if (strcmp(name,"..")==0)
       {
       
       inode = dir_get_inode(dir);
       block_sector_t bp = inode_get_parent(inode);
       inode = inode_open(bp);
       if(!inode_get_status(inode))
     		        return NULL;
       dir_close(dir);
       dir = dir_open(inode); 
       t->cwd = dir; 
       return 1;            
       }
     
     if(!dir_lookup (dir, name, &ip1))
     	return 0;  
     	       
     dir_close(dir);
     if(!inode_get_status(ip1))
     	return 0;
     		        
     dir = dir_open(ip1);     
     if(dir==NULL)    
        return 0;
     
     
     dir_close(t->cwd);
     t->cwd = dir;   
     //free(name);     
     return 1;
}
int sys_isdir(int fd)
{
    if(fd >= 0 && fd < FDTABLESIZE) // is valid FD?
    {
          struct thread *t = thread_current ();
          struct file* fi = t->fd_table[fd];
          if(fi)
          {
                struct dir *dir = (struct dir *)fi;
                struct inode *inode = dir_get_inode(dir);
                return inode_get_status(inode);
          }
  }
 return 0;
}
int sys_inumber(int fd)
{
    if(fd >= 0 && fd < FDTABLESIZE)
    {
          struct thread *t = thread_current ();
          struct file* fi = t->fd_table[fd];
          if(fi)
          {
                return inode_get_sec( file_get_inode(fi));
          }
  }
 return -1;
}
int sys_readdir(int fd,char *name)
{
   if(fd >= 0 && fd < FDTABLESIZE)
    {
          struct thread *t = thread_current ();
          struct file* fi = t->fd_table[fd];
          if(fi)
          {
                struct dir *dir = (struct dir *)fi;
                struct inode *inode = dir_get_inode(dir);
                if(inode_get_status(inode))
                {
        	      char *s = malloc(15); 
                      bool succ = dir_readdir (dir, s);
                      memcpy(name,s,15);
			return succ;
                }                
          }
    }
 return 0;       
}

