#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h> 

//-----------------------------------------------------------------------------------------------------
//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 4096

// Maximum size of virtual memory
#define MAX_MEMSIZE 4ULL*1024*1024*1024 // 4GB

// Size of "physcial memory"
#define MEMSIZE 1024*1024*1024 // 1GB

// Represents a page table entry
typedef unsigned long pte_t;

// Represents a page directory entry
typedef unsigned long pde_t;

#define TLB_ENTRIES 512


//-----------------------------------------------------------------------------------------------------
//Structure to represents TLB
typedef struct tlb {
    /*Assume your TLB is a direct mapped TLB with number of entries as TLB_ENTRIES
    * Think about the size of each TLB entry that performs virtual to physical
    * address translation.
    */
    void* va;
    void* pa;
    
}tlb;

struct tlb tlb_store;



//-----------------------------------------------------------------------------------------------------
void set_physical_mem();
pte_t* translate(pde_t *pgdir, void *va);
int page_map(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *t_malloc(unsigned int num_bytes);
void t_free(void *va, int size);
void put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);
void print_TLB_missrate();
int get_next_avail(int num_pages);

// Helper functions
static unsigned int get_top_bits(unsigned int value,  int num_bits);
static void set_bit_at_index(int* number, int index);
static void clear_bit_at_index(int* number, int index);
static int get_bit_at_index(int number, int index);
unsigned long bitToNum(unsigned long n, unsigned long numBits);
void free_memory();
void print_pages(int page_start, int page_end, int entry_start, int entry_end);
void char_to_bin(char input);
void int_to_bin(int input);
void print_tlb();
void print_bitmap();
void find_in_bitmap(int number, int* i, int* j);
void free_in_bitmap(int number, int* i, int* j);

#endif
