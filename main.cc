/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

	
	//****************************************************//
	//**printf("===== Simulator configuration =====\n");**//
	//*******print out simulator configuration here*******//
	//****************************************************//

 
	//*********************************************//
        //*****create an array of caches here**********//
	//*********************************************//	
	Cache** multicore_caches = new Cache*[num_processors]; //IMPORTANT dont use default constructor here.
	
	for(int i=0;i<num_processors;i++)
	{
		multicore_caches[i] = new Cache(cache_size,cache_assoc,blk_size);
	
	}
	pFile = fopen (fname,"r");
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//
	char str[2];
	unsigned long int addr;
	int proc;
	char rw; 
    while(fscanf(pFile, "%d %s %lx", &proc, str, &addr) != EOF)
    {
        
        rw = str[0];
        // if (rw == 'r')
		// {
		// 	printf("%s %lx %d\n", "read", addr, proc);           // Print and test if file is read correctly
		// 	//multicore_caches[proc].Access(addr,rw);
		// }
            
        // else if (rw == 'w')
		// {
		// 	printf("%s %lx %d\n", "write", addr, proc);          // Print and test if file is read correctly
		// 	//multicore_caches[proc].Access(addr,rw);
		// }
            
		if(protocol == 0) //MSI
		{
			multicore_caches[proc]->MSI(proc,addr,rw,multicore_caches,num_processors);
		}
		else if(protocol == 1)//MESI
		{
			multicore_caches[proc]->MESI(proc,addr,rw,multicore_caches,num_processors);
		}
		else if(protocol == 2)//Dragon
		{
			multicore_caches[proc]->Dragon(proc,addr,rw,multicore_caches,num_processors);
		}
        /*************************************
                  Add cache code here
    //     **************************************/
	//    for(int i=0;i<num_processors;i++)
	// 	{
	// 	multicore_caches[i]->printStats(i);
	// 	}
	// 	printf("\n");
    }

	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//
	printf("\n===== 506 Personal information =====");
	printf("\nFirstName: Ramachandran LastName: Sekanipuram Srikanthan");
	printf("\nUnityID: rsekani");
	printf("\n===== 506 SMP Simulator configuration =====");
	printf("\nL1_SIZE: %d",cache_size);
	printf("\nL1_ASSOC: %d",cache_assoc);
	printf("\nL1_BLOCKSIZE: %d",blk_size);
	printf("\nNUMBER OF PROCESSORS: %d",num_processors);
	
	if(protocol == 0) //MSI
	{
		printf("\nCOHERENCE PROTOCOL: MSI");
	}
	else if(protocol == 1)//MESI
	{
		printf("\nCOHERENCE PROTOCOL: MESI");
	}
	else if(protocol == 2)//Dragon
	{
		printf("\nCOHERENCE PROTOCOL: Dragon");
	}

	printf("\nTRACE FILE: trace/%s\n",fname);

	for(int i=0;i<num_processors;i++)
	{
		multicore_caches[i]->printStats(i);
	}
	delete(multicore_caches);
	
}
