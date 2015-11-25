/* halt.c
//9656

   Simple program to test whether running a user program works.
 	
   Just invokes a system call that shuts down the OS. */

#include <syscall.h>

int
main (void)
{
//  int child = exec("echo");
//  wait(child);
  int check = create("manjeet", 12);
  if(check)
    printf("create sucess \n");
  check = open("manjeet");
  if(check)
  printf("open sucess \n");
  close(check);
  return 11;
//  exit(229);
//  wait(1);
  //halt();
  /* not reached */
}
