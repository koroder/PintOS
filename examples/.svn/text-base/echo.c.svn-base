#include <stdio.h>
//16655
#include <syscall.h>

int
main (int argc, char **argv)
{
 //  int i;

 //  for (i = 0; i < argc; i++)
 //  printf ("%s ", argv[i]);
 //  printf ("\n");
 //  printf ("manjeetdahiya--");

       mkdir("/abc");
       char *s = "/abc/abc";
       mkdir(s);
       mkdir("/abc/../bcd");
       mkdir("/abc/../def");
       mkdir("/bcd/efg");
       mkdir("abc/./abc/hi");
       mkdir("abc/abc/hi/hello");
       mkdir("abc/abc/hi/../xyz");
       mkdir("abc/abc/xyz/p");

 return EXIT_SUCCESS;
}
