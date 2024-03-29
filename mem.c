/******************************************************************************
 * FILENAME: mem.c
 * AUTHOR:   cherin@cs.wisc.edu <Cherin Joseph>
 * DATE:     20 Nov 2013
 * PROVIDES: Contains a set of library functions for memory allocation
 * MODIFIED BY:  Katie Anderson, section: 002
 * *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

/* this structure serves as the header for each block */
typedef struct block_hd{
  /* The blocks are maintained as a linked list */
  /* The blocks are ordered in the increasing order of addresses */
  struct block_hd* next;

  /* size of the block is always a multiple of 4 */
  /* ie, last two bits are always zero - can be used to store other information*/
  /* LSB = 0 => free block */
  /* LSB = 1 => allocated/busy block */

  /* For free block, block size = size_status */
  /* For an allocated block, block size = size_status - 1 */

  /* The size of the block stored here is not the real size of the block */
  /* the size stored here = (size of block) - (size of header) */
  int size_status;

}block_header;

/* Global variable - This will always point to the first block */
/* ie, the block with the lowest address */
block_header* list_head = NULL;


/* Function used to Initialize the memory allocator */
/* Not intended to be called more than once by a program */
/* Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated */
/* Returns 0 on success and -1 on failure */
int Mem_Init(int sizeOfRegion)
{
  int pagesize;
  int padsize;
  int fd;
  int alloc_size;
  void* space_ptr;
  static int allocated_once = 0;
  
  if(0 != allocated_once)
  {
    fprintf(stderr,"Error:mem.c: Mem_Init has allocated space during a previous call\n");
    return -1;
  }
  if(sizeOfRegion <= 0)
  {
    fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
    return -1;
  }

  /* Get the pagesize */
  pagesize = getpagesize();

  /* Calculate padsize as the padding required to round up sizeOfRegio to a multiple of pagesize */
  padsize = sizeOfRegion % pagesize;
  padsize = (pagesize - padsize) % pagesize;

  alloc_size = sizeOfRegion + padsize;

  /* Using mmap to allocate memory */
  fd = open("/dev/zero", O_RDWR);
  if(-1 == fd)
  {
    fprintf(stderr,"Error:mem.c: Cannot open /dev/zero\n");
    return -1;
  }
  space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (MAP_FAILED == space_ptr)
  {
    fprintf(stderr,"Error:mem.c: mmap cannot allocate space\n");
    allocated_once = 0;
    return -1;
  }
  
  allocated_once = 1;
  
  /* To begin with, there is only one big, free block */
  list_head = (block_header*)space_ptr;
  list_head->next = NULL;
  /* Remember that the 'size' stored in block size excludes the space for the header */
  list_head->size_status = alloc_size - (int)sizeof(block_header);
  
  return 0;
}


/* Function for allocating 'size' bytes. */
/* Returns address of allocated block on success */
/* Returns NULL on failure */
/* Here is what this function should accomplish */
/* - Check for sanity of size - Return NULL when appropriate */
/* - Round up size to a multiple of 4 */
/* - Traverse the list of blocks and allocate the first free block which can accommodate the requested size */
/* -- Also, when allocating a block - split it into two blocks when possible */
/* Tips: Be careful with pointer arithmetic */
void* Mem_Alloc(int size)
{
	if (size < 1) return NULL;
	size = 1 + ((size - 1)/4);
	size = size * 4;
		 
	if (list_head != NULL){
		
		block_header* head_node = list_head; 

		while (head_node != NULL){
			/*get curr header size*/
			int node_size = head_node->size_status;

				/*there is space to add*/
				if (node_size%2 == 0 && (size+sizeof(block_header)) < node_size){					

					/*twocases*/
					if ((node_size - size - sizeof(block_header)) > 0){
													
							block_header* temp_node = NULL;

							char *raw_tmp_node = ((char*)head_node) + sizeof(block_header) + size;
							temp_node = (block_header*)raw_tmp_node;
							temp_node->next = head_node->next;
							temp_node->size_status = (head_node->size_status - sizeof(block_header) - size);
							head_node->size_status = size + 1;
							
							head_node->next = temp_node;
						}
						else{
							/*if at end, and not enough space for another header*/
							/*keep current size_status - do not reduce current */
							if (head_node->next != NULL) head_node->size_status = size + 1;
							
						}

					
					/*return location of (head_node_location+sizeof(block_header))*/
					return head_node + sizeof(block_header);	
				}/*end if*/
				
				else{
					/*headersize%2 equaled 1, so it's used - move on*/
					head_node = head_node->next;
				}
		}/*end while*/
	}/*end if*/
	return NULL;






}

/* Function for freeing up a previously allocated block */
/* Argument - ptr: Address of the block to be freed up */
/* Returns 0 on success */
/* Returns -1 on failure */
/* Here is what this function should accomplish */
/* - Return -1 if ptr is NULL */
/* - Return -1 if ptr is not pointing to the first byte of a busy block */
/* - Mark the block as free */
/* - Coalesce if one or both of the immediate neighbours are free */
int Mem_Free(void *ptr)
{
	
	if (ptr == NULL) return -1;
		
	if (list_head != NULL){
		
		block_header* head_node = list_head; 
		block_header* prev_node = NULL;
		/*points to where beginning of block should be*/
		void *temp_ptr = ((char*)ptr) - sizeof(block_header);

		while (head_node != NULL){
			if (head_node == temp_ptr){
				/*busy block?*/
				if (head_node->size_status%2 == 1){
					/*set free*/
					head_node->size_status = head_node->size_status-1;
					/*if not last block*/
					/*if (head_node->next !=NULL){
						//if there is a little extra space in the middle, add it in to curr block
						if (((block_header*)((char*)head_node) + sizeof(block_header) + head_node->size_status) != head_node->next){
							head_node->size_status = (int)(
((unsigned int)(head_node->size_status)) + 
(((unsigned int*)(head_node->next)) - 
(((unsigned int*)head_node) + ((unsigned int)sizeof(block_header)) + ((unsigned int)(head_node->size_status)))));
						}
					}*/
					
					/*coalesce to right*/
					if (head_node->next != NULL){
						/*not used?*/
						if (head_node->next->size_status%2 == 0){
							head_node->size_status = head_node->size_status + head_node->next->size_status + sizeof(block_header);
							head_node->next = head_node->next->next;
						}/*if used, or end, do nothing*/
					}
					/*coalesce to left*/
					if (prev_node != NULL){
						/*not used?*/
						if (prev_node->size_status%2 == 0){
							prev_node->size_status = prev_node->size_status + head_node->size_status + sizeof(block_header);
							prev_node->next = head_node->next;
						}/*if used, or beginning, do nothing*/
					}
				return 0; /*successfully freed block*/
				}/*end if busy block*/
				return -1; /*found ptr, but already free*/
			}/*end if - keep searching*/
			
			prev_node = head_node;
			head_node = head_node->next;
		}/*end while loop*/
	}/*end if - did not find ptr or list_head NULL*/
	return -1;





}

/* Function to be used for debug */
/* Prints out a list of all the blocks along with the following information for each block */
/* No.      : Serial number of the block */
/* Status   : free/busy */
/* Begin    : Address of the first useful byte in the block */
/* End      : Address of the last byte in the block */
/* Size     : Size of the block (excluding the header) */
/* t_Size   : Size of the block (including the header) */
/* t_Begin  : Address of the first byte in the block (this is where the header starts) */
void Mem_Dump()
{
  int counter;
  block_header* current = NULL;
  char* t_Begin = NULL;
  char* Begin = NULL;
  int Size;
  int t_Size;
  char* End = NULL;
  int free_size;
  int busy_size;
  int total_size;
  char status[5];

  free_size = 0;
  busy_size = 0;
  total_size = 0;
  current = list_head;
  counter = 1;
  fprintf(stdout,"************************************Block list***********************************\n");
  fprintf(stdout,"No.\tStatus\tBegin\t\tEnd\t\tSize\tt_Size\tt_Begin\n");
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  while(NULL != current)
  {
    t_Begin = (char*)current;
    Begin = t_Begin + (int)sizeof(block_header);
    Size = current->size_status;
    strcpy(status,"Free");
    if(Size & 1) /*LSB = 1 => busy block*/
    {
      strcpy(status,"Busy");
      Size = Size - 1; /*Minus one for ignoring status in busy block*/
      t_Size = Size + (int)sizeof(block_header);
      busy_size = busy_size + t_Size;
    }
    else
    {
      t_Size = Size + (int)sizeof(block_header);
      free_size = free_size + t_Size;
    }
    End = Begin + Size;
    fprintf(stdout,"%d\t%s\t0x%08lx\t0x%08lx\t%d\t%d\t0x%08lx\n",counter,status,(unsigned long int)Begin,(unsigned long int)End,Size,t_Size,(unsigned long int)t_Begin);
    total_size = total_size + t_Size;
    current = current->next;
    counter = counter + 1;
  }
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  fprintf(stdout,"*********************************************************************************\n");

  fprintf(stdout,"Total busy size = %d\n",busy_size);
  fprintf(stdout,"Total free size = %d\n",free_size);
  fprintf(stdout,"Total size = %d\n",busy_size+free_size);
  fprintf(stdout,"*********************************************************************************\n");
  fflush(stdout);
  return;
}
