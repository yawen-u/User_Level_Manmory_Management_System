#include "my_vm.h"

#define numPages (MAX_MEMSIZE / PGSIZE) // 262144 pages in total
#define entrySize 4

#define dir_mask (1023 << 22)
#define vpn_mask (1023 << 12)
#define offset_mask 1023

char** phys_mem;
int* phys_bitmap;

unsigned int current_va = 0;

unsigned int curr_pdi;
unsigned int curr_pti;
pde_t* page_dir;
pde_t* pde;
int pte;

tlb* TLB_list[TLB_ENTRIES];
int curr_tlb_index = 0;
int miss_count = 0;
int translation_count = 0;
bool first_invoke = true;

// Lock 
pthread_mutex_t mutex;

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating
    

    // Allocate for physical frames/pages
    // phys_mem[PFN][offset]
    phys_mem = (char **)malloc(numPages * sizeof(char *));
    for (int i = 0; i < numPages; i++) {
        phys_mem[i] = (char *)malloc(PGSIZE * sizeof(char));
    }


    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them
    unsigned int bitmap_size = numPages / 32;
    phys_bitmap = malloc(bitmap_size * sizeof(int));
    set_bit_at_index( &(phys_bitmap[0]), 0); // The first bit we left unused for mapping purposes

    // initialize tlb
    for (int i = 0; i < TLB_ENTRIES; i++) {
        TLB_list[i] = (tlb *)malloc(sizeof(tlb));
    }
    curr_tlb_index = 0;
    
}

//-----------------------------------------------------------------------------------------------------

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */

int
add_TLB(void *va, void *pa){

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    TLB_list[curr_tlb_index]->va = va;
    TLB_list[curr_tlb_index]->pa = pa;
    curr_tlb_index++;

    return -1;
}

//-----------------------------------------------------------------------------------------------------

/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va){
    /* Part 2: TLB lookup code here */
    for (int i = 0; i < curr_tlb_index; i++) {
        if (TLB_list[i]->va == va) {
            return (pte_t *)(&(TLB_list[i]->pa));
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------------------------------

/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate(){
    double miss_rate = 0;   

    /*Part 2 Code here to calculate and print the TLB miss rate*/
    if (translation_count == 0) {
        printf("No translation was performed.\n");
        return;
    }

    double miss_count_double = (double) miss_count;
    double translation_count_double = (double) translation_count;

    miss_rate = miss_count_double / translation_count_double;

    printf("Miss count: %d | Tranlation count: %d\n", miss_count, translation_count);


    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}

//-----------------------------------------------------------------------------------------------------

/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/

// |      directoryIndex      ||   vpn   ||     offset      |
//           10 bits             10 bits        12 bits

pte_t *translate(pde_t *pgdir, void *va) {
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *

    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */
    pthread_mutex_lock(&mutex);
    pte_t* phys_addr;
    pte_t* phys_addr_page;
    translation_count++;

    unsigned int tlb_check_mask = (dir_mask) | (vpn_mask);
    unsigned int va_check = (unsigned int)va & tlb_check_mask;

    // PART II: check virtual address exists in TLB
    phys_addr = check_TLB( (void*)va_check);
    if (phys_addr != NULL) {

        unsigned int offset_index = (unsigned int)va & offset_mask;
        phys_addr = (pte_t*) (((unsigned int) phys_addr) + offset_index);
        
        return phys_addr;
    } 
    miss_count++;


    // PART I:
    unsigned int dir_index;
    unsigned int vpn_index;
    unsigned int offset_index;
    unsigned int inner_level;
    unsigned int pte;

    // Get the indexes 
    dir_index = ((unsigned int)va & dir_mask) >> 22;
    vpn_index = ((unsigned int)va & vpn_mask) >> 12;
    offset_index = (unsigned int)va & offset_mask;

    // Check Page Directory
    if (phys_mem[0][dir_index * entrySize] == 0){

        return NULL;
    }

    else{

        inner_level = phys_mem[0][dir_index * entrySize];

        // Check Page Table
        if (phys_mem[inner_level][vpn_index * entrySize] == 0){

            return NULL;
        }

        else{

            pte = phys_mem[inner_level][vpn_index * entrySize];

            phys_addr = (pte_t *) (&(phys_mem[pte][offset_index]) ) ;
            phys_addr_page = (pte_t *) &(phys_mem[pte][0]);

            add_TLB((void*)va_check, (void*)phys_addr_page); // Since we miss, we will add the page to the TLB

            //printf("Physical address in translate: %x, respective vz: %x\n", (unsigned int) phys_addr, (unsigned int)va);

            pthread_mutex_unlock(&mutex);
            
            return phys_addr;
        }
    }

}

// //-----------------------------------------------------------------------------------------------------

// /*
// The function takes a page directory address, virtual address, physical address
// as an argument, and sets a page table entry. This function will walk the page
// directory to see if there is an existing mapping for a virtual address. If the
// virtual address is not present, then a new entry will be added
// */
int page_map(pde_t *pgdir, void *va, void *pa){

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    unsigned int dir_index;
    unsigned int vpn_index;
    unsigned int offset_index;
    unsigned int inner_level;

    dir_index = ((unsigned int)va & dir_mask) >> 22;
    vpn_index = ((unsigned int)va & vpn_mask) >> 12;
    offset_index = ((unsigned int)va & offset_mask);


    // Walk the Page Directory now
    if (phys_mem[0][dir_index * 4] == 0){

        // Initialize new page table
        phys_mem[0][dir_index * 4] = get_next_avail(1);
        if (dir_index = 0) {
            curr_pti = 1;
        } else {
            curr_pti = 0;
        }
         // it's a new page; so we are reseting the current page table index to 1;

        return curr_pti; // pt index
    }

    else{

        curr_pti++;

        return curr_pti;
    }

}

// //-----------------------------------------------------------------------------------------------------

// // Traverse the page table to find the next available page
int get_next_avail(int num_pages) {

    //Find the next free continuious n virtual pages
    bool page_found = false;
    bool skip = false;

    // For Bitmap
    int a = 0;
    int b = 0;

    for (int i = 1; i < numPages; i++) {
        for (int j = 0; j < num_pages; j++) {
            if (phys_mem[i+j][0] == 0) {
                page_found = true;
            } else {
                page_found = false;
                skip = true;
            }
        }

        if (skip == true) {
            skip = false;
            continue;
        }

        // When the free chunk is found, mark them as used.
        if (page_found) {
            for (int x = 0 ; x < num_pages; x++) {
                phys_mem[i+x][0] = 1;
            }

            find_in_bitmap(i, &a, &b);

            // Mark bits in Bitmap as occupied
            for (int k = 0; k < num_pages; k++){

                set_bit_at_index( &(phys_bitmap[a]), (b + k) );    
            }

            // Generate a virtual address according to the values of a (i) and b (j)
            current_va = ((a * 32) + b) * PGSIZE;
            
            return i;
        }
    }


    return -1;
}

//-----------------------------------------------------------------------------------------------------

/* Function responsible for allocating pages
and used by the benchmark
*/

// This method returns the user a virtual address and initializes its physical memory
void *t_malloc(unsigned int num_bytes) {

    pthread_mutex_lock(&mutex);

    unsigned int dir_index;
    unsigned int vpn_index;
    unsigned int offset_index;
    unsigned int inner_level;

    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */

    if (phys_mem == NULL) {
        set_physical_mem();
    }

   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */

    // The current page directory index tracker
    if ( (curr_pdi != 0) && ((curr_pti + 1) % PGSIZE) == 0 ){

        curr_pdi++;
        curr_pti = 0;
    }

    int num_pages_to_allocate = num_bytes / PGSIZE;
    if (num_bytes % PGSIZE != 0) {
        num_pages_to_allocate++;
    }

    unsigned int va = ((curr_pdi << 22) & dir_mask);

    void* pa;

    vpn_index = page_map(page_dir, (void*)va, pa);


    // Find free data pages in physical memory
    int free_page_index;
    free_page_index = get_next_avail(num_pages_to_allocate);
    if (free_page_index == -1) {
        printf ("Memory Full / The memory requested is too big. \n");
        return NULL;
    }
    pte = free_page_index;

    // Updated locations of new va
    dir_index = ((unsigned int)current_va & dir_mask) >> 22;
    vpn_index = ((unsigned int)current_va & vpn_mask) >> 12;
    offset_index = ((unsigned int)current_va & offset_mask);


    inner_level = phys_mem[0][dir_index * 4];
    phys_mem[inner_level][vpn_index * 4] = pte;


    va = current_va;

    pthread_mutex_unlock(&mutex);

    return (void*)va;
}

//-----------------------------------------------------------------------------------------------------

// /* Responsible for releasing one or more memory pages using virtual address (va)
// */
void t_free(void *va, int size) {

    pthread_mutex_lock(&mutex);
    // Part II : take it off the tlb
    for (int i = 0; i < curr_tlb_index; i++) {
        if (TLB_list[i]->va == va) {
            TLB_list[i]->va = 0;
            TLB_list[i]->pa = 0;
            curr_tlb_index--;
        }
    }

    // /* Part 1: Free the page table entries starting from this virtual address
    //  * (va). Also mark the pages free in the bitmap. Perform free only if the 
    //  * memory from "va" to va+size is valid.
    //  *
    // */

    unsigned int dir_index;
    unsigned int vpn_index;
    unsigned int offset_index;
    unsigned int inner_level;
    unsigned int data_level;
    pte_t* pa;

    // For Bitmap
    int a = 0;
    int b = 0;

    dir_index = ((unsigned int)va & dir_mask) >> 22;
    vpn_index = ((unsigned int)va & vpn_mask) >> 12;
    offset_index = ((unsigned int)va & offset_mask);
    pa = translate(NULL, va);    

    // Check if valid
    if (pa == NULL){
        return;
    }

    // Check how many pages to free
    int num_pages_to_free = (size - offset_index) / PGSIZE;
    if (size % PGSIZE != 0) {
        num_pages_to_free++;
    }


    inner_level = phys_mem[0][dir_index * 4];


    // Update BitMap
    unsigned int free_va = (unsigned int) va;

    free_in_bitmap(free_va, &a, &b);

    // Mark bits in Bitmap as free
    for (int k = 0; k < num_pages_to_free; k++){

        clear_bit_at_index( &(phys_bitmap[a]), (b + k) );
    }


    // Free Data Page(s)
    data_level = phys_mem[inner_level][vpn_index * 4];

    for (int i = 0; i < num_pages_to_free; i++){    

        
        for (int j = 0; j < PGSIZE; j++){

            phys_mem[data_level + i][j] = 0;
        }
    }

    // Free Page Table Entry
    phys_mem[inner_level][vpn_index * 4] = 0;

    pthread_mutex_unlock(&mutex);    
}

//-----------------------------------------------------------------------------------------------------

/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
 * The function returns 0 if the put is successfull and -1 otherwise.
*/
void put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
     * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     * than one page. Therefore, you may have to find multiple pages using translate()
     * function.
     */
    pte_t* pa;
    pa = translate(NULL, va);

    if (pa == NULL){
        return;
    }
    if (first_invoke) {
        pa = translate(NULL, va);
        first_invoke = false;
    }

    //printf("phys_addr: %p\n", pa);
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < size; i++ ) {
        //printf("i: %d\n", i);
        memcpy(pa, val, size);
    }
    
    pthread_mutex_unlock(&mutex);

    return;
}

//-----------------------------------------------------------------------------------------------------

/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */
    pte_t* pa;
    pa = translate(NULL, va);

    if (pa == NULL){
        return;
    }

    pthread_mutex_lock(&mutex);
    memcpy(val, pa, size);
    pthread_mutex_unlock(&mutex);

    return;
}

//-----------------------------------------------------------------------------------------------------

/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */
    int x, y, val_size = sizeof(int);
    int i, j, k;
    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            unsigned int a, b, c = 0;
            for (k = 0; k < size; k++) {
                int address_a = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
                int address_b = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));
                get_value( (void *)address_a, &a, sizeof(int));
                get_value( (void *)address_b, &b, sizeof(int));
                // printf("Values at the index: %d, %d, %d, %d, %d\n", 
                //     a, b, size, (i * size + k), (k * size + j));
                c += (a * b);
            }
            int address_c = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            // printf("This is the c: %d, address: %x!\n", c, address_c);
            put_value((void *)address_c, (void *)&c, sizeof(int));
        }
    }
}


//-----------------------------------------------------------------------------------------------------
// Helper Functions

static unsigned int get_top_bits(unsigned int value,  int num_bits){

    //Implement your code here
    unsigned int limit = (8 * sizeof(unsigned int)) - 1;
    unsigned int result = 0;
    unsigned int current = 0;

    // Traverse bit by bit
    for (int i=0; i < num_bits; i++){

        current = (value >> (limit - i)) & 1;

        result = result | (current << ( (num_bits - 1) - i) );
    }

    return result;
}


static void set_bit_at_index(int* number, int index){

    *number |= 1UL << index;
    return;
}

static void clear_bit_at_index(int* number, int index){

    *number &= ~(1UL << index);
    return;
}


static int get_bit_at_index(int number, int index){

    unsigned int result = 0;

    result = (number >> index) & 1;

    return result;
}

unsigned long bitToNum(unsigned long n, unsigned long numBits) { 

    return (((1 << numBits) - 1) & n); 
}


void free_memory(){

    for (int i = 0; i < numPages; i++) {
        free(phys_mem[i]);
    }

    free(phys_mem);
    free(phys_bitmap);
}


void print_pages(int page_start, int page_end, int entry_start, int entry_end){

    int counter = 0;

    for (int i = page_start; i <= page_end; i++){

        for (int j = entry_start; j <= entry_end; j++){

            for (int k = 0; k < 4; k++){

                char_to_bin(phys_mem[i][(j *4) + k]);

                counter++;
                if (counter == 4){
                    counter = 0;
                    printf("\n--------------------------------------------------------\n");
                }

            }

        }

        printf("\n\n\n");
    }
}

void char_to_bin(char input){

    printf(" | ");
    for (int j = 0; j < 8; j++){

        printf("%d", !!((input << j) & 0x80));
    }
    printf(" | ");
}

void int_to_bin(int input){

    printf(" | ");
    for (int j = 31; j >= 0; j--){

        printf("%d", !!((input >> j) & 1));
    }
    printf(" | ");
}

void print_tlb() {
    
    for (int i = 0; i < curr_tlb_index; i++) {
        printf("TLB[%d]-> (va : %p)    (pa : %p)\n", i, TLB_list[i]->va, TLB_list[i]->pa);
    }
}

void print_bitmap(){

    printf("Printing Bitmap: \n");

    for (int i = 0; i < 10; i++) {

        int_to_bin(phys_bitmap[i]);
        printf("\n");
    }

}

void find_in_bitmap(int number, int* i, int* j){

    for (int x = 0; x < (numPages / 32); x++){

        for (int y = 0; y < 32; y++){

            if ( ((32 * x) + y) == number){

                *i = x;
                *j = y;
                return;
            }
        }
    }
}

void free_in_bitmap(int number, int* i, int* j){

    for (int x = 0; x < (numPages / 32); x++){

        for (int y = 0; y < 32; y++){

            if ( (((32 * x) + y) * PGSIZE) == number){

                *i = x;
                *j = y;
                return;
            }
        }
    }
}