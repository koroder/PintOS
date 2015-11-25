#ifndef USERPROG_SYSCALL_H
//32306
#define USERPROG_SYSCALL_H

#include <stdint.h>

void syscall_init (void);

int get_nth_arg_int(void* esp, int n);
void* get_nth_arg_ptr(void* esp, int n);
int user_add_range_check(char* start, int size);
void stack_address_check(void* esp);

int user_add_range_check(char* start, int size);
void user_add_range_check_and_terminate(char* start, int size);
void process_terminate(void);
void user_string_add_range_check_and_terminate(char* str);
void user_string_add_range_check_and_terminate1(char* str);
int get_user(const char *uaddr);
int put_user(char *udst, char byte);

int sys_exec(char* filename);
int sys_open(char* file_name);
void sys_close(int fd);
int sys_write(int fd, void *buffer, unsigned size);
int sys_read(int fd, void* buffer, unsigned size);
int sys_filesize(int fd);
void sys_seek(int fd, unsigned pos);
unsigned sys_tell(int fd);
int sys_remove(char* file_name);
int sys_create(char* file_name, int size);
/////////////////////////////////////////////////////////////////////////////
int sys_mkdir(char *path_name);
int sys_chdir(char *path_name);
int sys_isdir(int fd);
int sys_inumber(int fd);
int sys_readdir(int fd,char *name);
//////////////////////////////////////////////////////////////////////////////

void process_terminate(void);

#endif /* userprog/syscall.h */
