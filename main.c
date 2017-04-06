// Authors: Sam Mustipher and Noah Sarkey
/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

// Globals
int npages;
int nframes;
struct disk *disk;
char *virtmem;
char *physmem;
char *algorithm; // string that is algorithm
int *frame_table; // array of ints for the frame table
int *frame_count; // for LRU, initialize to 0's
int currentFrames = 0;
int fifoframe = 0; 
int framecounter = 0;

// Second Chance
int *ref;
int refptr = 0;


// Data Needed
int diskreads = 0;
int diskwrites = 0;
int numfaults = 0;

/*
int findpage(int page){
	int i;
	for(i = 0; i < nframes; i++){
		if(page == frame_table[i]){
			return i;
		}
	}
	return -1;
}*/

void printData(){
	printf("Disk Reads: %d\n", diskreads);
	printf("Disk Writes: %d\n", diskwrites);
	printf("Number Faults: %d\n", numfaults);
}

int findLFU(){
	int i;
	int pos;
	int minimum;
	i = 0;
	pos = 0;
	minimum = frame_count[0];


	for(i = 0; i < nframes; i++){
		if(frame_count[i] < minimum){
			minimum = frame_count[i];
			pos = i;
		}
	}

	return pos;
}

int findsc(){

}

void print_frames(){
	int i;
	printf("FRAME TABLE: \n");
	for(i = 0; i < nframes; i++){
		printf("%d\n", frame_table[i]);
	}
}

void page_fault_handler( struct page_table *pt, int page )
{
	// printf("Page fault on page: %d \n\n", page);
	numfaults+=1;

	// filling up the frame table linearly
	int i;
	int permissions = -1;
	int *bits = &permissions;
	int myframe = -1;
	int *currFrame = &myframe;

	page_table_get_entry(pt, page, currFrame, bits);  
	// printf("Current Frame: %d\n", myframe);

	for(i = 0; i < nframes; i++){
		if(frame_table[i] == -1){ // if the current frame has no data, place page data in frame table
			frame_table[i] = page;
			frame_count[i] = frame_count[i]+1;
			page_table_set_entry(pt, page, i, PROT_READ);
			disk_read(disk, page, &physmem[i*PAGE_SIZE]);
			diskreads+=1;
			return;
		}

		if(permissions == 1){ // if the file only has read permissions set write
			page_table_set_entry(pt, page, myframe, PROT_READ|PROT_WRITE);
			return; 
		}
	}

	// print_frames();


	if(!strcmp(algorithm, "rand")){ // rand algorithm 
		// printf("RAND\n"); // testing
		// int frame = findpage(page);
		int randframe = rand()%nframes;
		// printf("Rand Frame: %d\n", randframe);
		page_table_get_entry(pt, frame_table[randframe], currFrame, bits);

		if(permissions > 1){ // if the current frame already has write permissions then write to disk
			disk_write(disk,frame_table[randframe],&physmem[myframe*PAGE_SIZE]);
			diskwrites+=1;
		}

		disk_read(disk, page, &physmem[myframe*PAGE_SIZE]);
		diskreads+=1;
		page_table_set_entry(pt, page, myframe, PROT_READ);
		page_table_set_entry(pt, frame_table[randframe], myframe, 0);
		frame_table[randframe] = page;
		return;
	}
	else if(!strcmp(algorithm, "fifo")){
		// Follow the comments from above
		// printf("fifo\n");
		if (fifoframe >= nframes){
			fifoframe = 0;
		}

		// printf("Frame: %d\nAbout to get page: %d\n", fifoframe, frame_table[fifoframe]);
		page_table_get_entry(pt, frame_table[fifoframe], currFrame, bits);

		if(permissions > 1){ // if the current frame already has write permissions write to disk
			disk_write(disk, frame_table[fifoframe], &physmem[myframe*PAGE_SIZE]);
			diskwrites+=1;
		}

		disk_read(disk, page, &physmem[myframe*PAGE_SIZE]);
		diskreads+=1;
		page_table_set_entry(pt, page, myframe, PROT_READ);
		page_table_set_entry(pt, frame_table[fifoframe], myframe, 0);
		frame_table[fifoframe] = page;
		// printf("Setting %d to Page: %d\n", fifoframe, page);
		fifoframe+=1;// increment the frame
		return;
	}
	else if(!strcmp(algorithm, "custom")){
		// printf("RAND\n"); // testing
		// int frame = findpage(page);
		int LFUframe = findLFU();
		// printf("Rand Frame: %d\n", randframe);

		page_table_get_entry(pt, frame_table[LFUframe], currFrame, bits);

		if(permissions > 1){ // if the current frame already has write permissions then write to disk
			disk_write(disk,frame_table[LFUframe],&physmem[myframe*PAGE_SIZE]);
			diskwrites+=1;
		}

		disk_read(disk, page, &physmem[myframe*PAGE_SIZE]);
		diskreads+=1;
		page_table_set_entry(pt, page, myframe, PROT_READ);
		page_table_set_entry(pt, frame_table[LFUframe], myframe, 0);
		frame_table[LFUframe] = page;
		if(permissions == 1){
			frame_count[LFUframe] = nframes;
		}
		if(permissions == 3){
			frame_count[LFUframe] = frame_count[LFUframe] - 1;
		}
		
		return;	
	}
	else if(!strcmp(algorithm, "sc")){

	}
	else{
		printf("Invalid Algorithm\n");
		exit(1);
	}
	// page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE);
	
	// printf("page fault on page #%d\n",page);
	// exit(1);
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|lru|custom> <sort|scan|focus>\n");
		return 1;
	}

	srand(time(NULL));

	npages = atoi(argv[1]); // number of pages
	// printf("%d\n", npages);
	nframes = atoi(argv[2]); // number of frames
	const char *program = argv[4];

	// getting the given algorithm
	algorithm = argv[3];

	// setting up the frame table
	int i;
	frame_table = (int *)malloc(sizeof(int)*nframes);
	for(i = 0; i < nframes; i++){
		frame_table[i] = -1;
		// printf("%d\n", frame_table[i]);
	}

	frame_count = (int *)malloc(sizeof(int)*nframes);
	for(i = 0; i < nframes; i++){
		frame_count[i] = 0;
	}


	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	virtmem = page_table_get_virtmem(pt);

	physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);

	printData();

	return 0;
}
