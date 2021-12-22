/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
   cache_to_cache_transfer = memory_transactions = interventions = invalidations = flushes = BusRdX = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
// void Cache::Access(ulong addr,uchar op)
// {
// 	currentCycle++;/*per cache global counter to maintain LRU order 
// 			among cache ways, updated on every cache access*/
        	
// 	if(op == 'w') writes++;
// 	else          reads++;
	
// 	cacheLine * line = findLine(addr);
// 	if(line == NULL)/*miss*/
// 	{
// 		if(op == 'w') writeMisses++;
// 		else readMisses++;

// 		cacheLine *newline = fillLine(addr);
//    		if(op == 'w') newline->setFlags(DIRTY);    
		
// 	}
// 	else
// 	{
// 		/**since it's a hit, update LRU and update dirty flag**/
// 		updateLRU(line);
// 		if(op == 'w') line->setFlags(DIRTY);
// 	}
// }

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getFlags() == MODIFIED || victim->getFlags() == SM) writeBack(addr);
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(int proc)
{ 
	printf("============ Simulation results (Cache %d) ============\n",proc);
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
   printf("01. number of reads:				%d\n",reads);
   printf("02. number of read misses:			%d\n",readMisses);
   printf("03. number of writes:				%d\n",writes);
   printf("04. number of write misses:			%d\n",writeMisses);
   printf("05. total miss rate:				%.2f%c\n",float((readMisses + writeMisses))/(reads+writes)*100,'%');
   printf("06. number of writebacks:			%d\n",writeBacks);
   printf("07. number of cache-to-cache transfers:		%d\n",cache_to_cache_transfer);
   printf("08. number of memory transactions:		%d\n",memory_transactions);
   printf("09. number of interventions:			%d\n",interventions);
   printf("10. number of invalidations:			%d\n",invalidations);
   printf("11. number of flushes:				%d\n",flushes);
   printf("12. number of BusRdX:				%d\n",BusRdX);
}

bool verbose = true;
void Cache::MSI(int processor_no,ulong addr,char op,Cache** multicore_caches,int num_processors)
{
   currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
   
   
	if(op == 'w') writes++;
	else if(op =='r')        reads++;
	
	if((op == 'r')|| (op =='w'))
   {
      cacheLine * line = findLine(addr);
      if(line == NULL)/*miss*/
      {
         //printf("\n Miss \n");
         //Now cache is in Invalid State
         
         if(op == 'w') {
            writeMisses++;
            memory_transactions+=1;
         }
         else {
            readMisses++;
            memory_transactions+=1;
         }
         //Processor write
         cacheLine *newline = fillLine(addr);
            if(op == 'w') {
               //Invalid -> Modified (BusRdX)
               BusRdX++;
               
               newline->setFlags(MODIFIED);
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                  {
                     //Invalidate
                      
                     multicore_caches[i]->MSI(i,addr,'i',multicore_caches,num_processors);
                  }
               }    
            }

            //Processor read
            else if(op == 'r')
            {
               
               //Invalid -> Shared
               newline->setFlags(SHARED);
               for(int i=0;i<num_processors;i++)
               {
                  //make shared
                  multicore_caches[i]->MSI(i,addr,'s',multicore_caches,num_processors);
               }
            }
         
      }
      else
      {
         /**since it's a hit, update LRU and update dirty flag**/
         updateLRU(line);
         //Processor write
         //printf("\n hit\n");
         if(op == 'w') {
            
            if(line->getFlags() == MODIFIED)
            {
               //Modified -> Modified (No Snoop request)
            }

            else if(line->getFlags() == SHARED)
            {
               //Shared -> Modified (BusRdX)
               BusRdX++;
               memory_transactions+=1;
               line->setFlags(MODIFIED);
               //Invalidate others
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                  {
                     
                     //invalidations++;
                     multicore_caches[i]->MSI(i,addr,'i',multicore_caches,num_processors);
                  }
                     
               }
            }
           
         }
         //Processor read
         else if(op =='r')
         {
            if(line->getFlags() == SHARED)
            {
               //Shared -> Shared (No snoops)
               line->setFlags(SHARED);
               
            }
            else if(line->getFlags() == MODIFIED)
            {
               //Modified -> Modified (No snoops)
            }
         }
      }
   }

   else if((op == 's') || (op =='i'))
   {
      cacheLine * line = findLine(addr);
      if(line == NULL)
      {
         //Invalid state and so no action for BusRd and BusRdX
      }
      else
      {
         //BusRdX -> i
         if(op == 'i')
         {  
            if(line->getFlags() == MODIFIED)
            {
               // Modified -> Invalid (flush)
               line->invalidate();
               flushes++;
               invalidations++;
               writeBacks++;
               memory_transactions+=1;
            }

            else if(line->getFlags() == SHARED)
            {
               //Shared -> Invalid
               line->invalidate();
               invalidations++;
            }
         }

         //BusRd -> s
         else if(op =='s')
         {
            if(line->getFlags() == SHARED)
            {
               //Shared -> Shared
            }

            else if(line->getFlags() == MODIFIED)
            {
               //Modified -> Shared (flush)
               line->setFlags(SHARED);
               flushes++;
               interventions++; //Number of interventions (refers to the total number of times that E/M transits to Shared states.
               writeBacks++;
               memory_transactions+=1;
            }
         }
      }

   }
}


void Cache::MESI(int processor_no,ulong addr,char op,Cache** multicore_caches,int num_processors)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else if (op == 'r')  reads++;

   //printf("\n HERE\n");
	if((op == 'r') || (op =='w'))
   {
      cacheLine * line = findLine(addr);
      //printf("\n HERE1\n");
      if(line == NULL)/*miss*/
      {
         //Cache is now in Invalid State
         if(op == 'w') {
            writeMisses++;
            memory_transactions++;
         }
         else {
            readMisses++;
            memory_transactions++;
         }
         //Check for data in other caches
         bool copy_exists = false;
         for(int i=0;i<num_processors;i++)
         {
            if(i!=processor_no)
            {
               cacheLine *line1 = multicore_caches[i]->findLine(addr);
              
               if((line1!=NULL))
               {
                  copy_exists = true;
                  break;
               }
            }
               
         }

         //printf("\n HERE2\n");
         cacheLine *newline = fillLine(addr);
         
         //Processor write
         if(op == 'w')
         {
            //BusRdX -> i
            //Invalid -> Modified(BusRdX)
            newline->setFlags(MODIFIED);
            BusRdX++;
            
            //Testing                 ///////////WWWWWHHYYYYYY
            //Invalid -> Modified (BusRdx) increments cache to cache transfer and write backs
            if(copy_exists)
            {
               cache_to_cache_transfer++;
               memory_transactions--;
            }
            //Testing ends

            for(int i=0;i<num_processors;i++)
            {
               if(i!=processor_no)
                  multicore_caches[i]->MESI(i,addr,'i',multicore_caches,num_processors);
            }

         }

         //Processor Read
         else if(op =='r')    
         {            
            if(copy_exists)
            {
               //Invalid -> Shared
               //BusRd -> s
               memory_transactions--;
               cache_to_cache_transfer++;
               newline->setFlags(SHARED);
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                  {
                     multicore_caches[i]->MESI(i,addr,'s',multicore_caches,num_processors);
                  }
               }
            }
            else
            {
               //Invalid -> Exclusive
               newline->setFlags(EXCLUSIVE);
               //testing
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                     multicore_caches[i]->MESI(i,addr,'s',multicore_caches,num_processors);
               }
               //testing ends
            }
         }
         
      }
      else
      {
         /**since it's a hit, update LRU and update dirty flag**/
         updateLRU(line);
         //Processor Write
         if(op == 'w')
         {
            if(line->getFlags() == SHARED)
            {
               //Shared -> MODIFIED (BusUpgr -> u)
               //Invalidate others
               line->setFlags(MODIFIED);
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                  {
                     multicore_caches[i]->MESI(i,addr,'u',multicore_caches,num_processors);
                  }
               }

            }
            else if(line->getFlags() == MODIFIED)
            {
               //MODIFIED -> MODIFIED
            }

            else if(line->getFlags() == EXCLUSIVE)
            {
               //EXCLUSIVE -> MODIFIED (No need to invalidate)
               line->setFlags(MODIFIED);
            }
         }

         //Processor Read
         else if(op =='r')
         {
            if(line->getFlags() == SHARED)
            {
               //Shared -> Shared
            }
            else if(line->getFlags() == MODIFIED)
            {
               //Modified -> Modified
            }
            else if(line->getFlags() == EXCLUSIVE)
            {
               //Exclusive -> Exclusive
            }
         }
      }
   }

   else if((op =='i') || (op =='s') || (op =='u'))
   {
      cacheLine * line = findLine(addr);
      if(line == NULL)
      {
         //Invalid state and so no action for BusRd, BusUpgr and BusRdX
      }
      else
      {
         //BusRdX -> i
         if(op == 'i')
         {
            //Shared (BusRdX snoop received) -> Invalid (Flushopt)
            if(line->getFlags() == SHARED)
            {
               line->invalidate();
               invalidations++;
               cache_to_cache_transfer++;
               
            }
            //Modified (BusRdX snoop received) -> Invalid (flush)
            else if(line->getFlags() == MODIFIED)
            {
               line->invalidate();
               invalidations++;
               flushes++;
               writeBacks++;
               memory_transactions++;
               //testing starts
               //cache_to_cache_transfer++;
               //testing ends
            }
            //Exclusive (BusRdX Snoop received) -> Invalid (flushopt)
            else if(line->getFlags() == EXCLUSIVE)
            {
               line->invalidate();
               invalidations++;
               cache_to_cache_transfer++;
               
            }
         }
         //BusRd -> s
         else if(op == 's')
         {
            //Shared -> Shared (flushopt)
            if(line->getFlags() == SHARED)
            {
               line->setFlags(SHARED);
               //cache_to_cache_transfer++; //already done in invalid -> shared state
            }
            //Modified -> Shared (flush)
            else if(line->getFlags() == MODIFIED)
            {
               line->setFlags(SHARED);
               interventions++; //Number of interventions (refers to the total number of times that E/M transits to Shared states.
               flushes++;
               writeBacks++;
               memory_transactions++;
            }
            //Exclusive -> Shared (flushopt)
            else if(line->getFlags() == EXCLUSIVE)
            {
               line->setFlags(SHARED);
               //cache_to_cache_transfer++;  //already done in invalid -> shared state
               interventions++; //Number of interventions (refers to the total number of times that E/M transits to Shared states.
            }
         }
         //BusUpgr -> u
         else if(op == 'u')
         {
            //Shared -> Invalid 
            if(line->getFlags() == SHARED)
            {
               line->invalidate();
               invalidations++;
            }
         }
      }
   }
	
}


void Cache::Dragon(int processor_no,ulong addr,char op,Cache** multicore_caches,int num_processors)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else if(op == 'r') reads++;
	
   if((op =='r') || (op =='w'))
   {
      cacheLine * line = findLine(addr);
      if(line == NULL)/*miss*/
      {
         //Now Cache is in invalid states
         if(op == 'w'){
            writeMisses++;
            memory_transactions++;
         } 
         else{
            readMisses++;
            memory_transactions++;
         } 

         cacheLine *newline = fillLine(addr);
         //Processor write
         //Check for data in other caches
         bool copy_exists = false;
         for(int i=0;i<num_processors;i++)
         {
            if(i!=processor_no)
            {
               cacheLine *line1 = multicore_caches[i]->findLine(addr);
               
               if((line1!=NULL))
               {
                  copy_exists = true;
                  break;
               }
            }
               
         }
         if(op == 'w') 
         {
            if(copy_exists)
            {
               //Invalid -> SM (BusRd,BusUpgr)
               newline->setFlags(SM);
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                  {
                     //BusRd -> s
                     //BusUpdate -> u
                     multicore_caches[i]->Dragon(i,addr,'s',multicore_caches,num_processors);
                     multicore_caches[i]->Dragon(i,addr,'u',multicore_caches,num_processors);
                  }
               }

            }
            else
            {
               //Invalid -> Modified (BusRd)
               newline->setFlags(MODIFIED);
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                  {
                     //BusRd -> s
                     multicore_caches[i]->Dragon(i,addr,'s',multicore_caches,num_processors);
                  }

               }
            }
         }

         //Processor Read
         else if(op =='r')
         {
            if(copy_exists)
            {
               //Invalid -> Sc (BusRd)
               newline->setFlags(SC);
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                  {
                     //BusRd -> s
                     multicore_caches[i]->Dragon(i,addr,'s',multicore_caches,num_processors);
                  }
               }
            }  
            else
            {
               //Invalid -> Exclusive (BusRd)
               newline->setFlags(EXCLUSIVE);
               for(int i=0;i<num_processors;i++)
               {
                  if(i!=processor_no)
                  {
                     //BusRd -> s
                     multicore_caches[i]->Dragon(i,addr,'s',multicore_caches,num_processors);
                  }
               }
            }
         }
         
      }
      else
      {
         /**since it's a hit, update LRU and update dirty flag**/
         updateLRU(line);
         //Processor write
         //Check for data in other caches
         bool copy_exists = false;
         for(int i=0;i<num_processors;i++)
         {
            if(i!=processor_no)
            {
               cacheLine *line1 = multicore_caches[i]->findLine(addr);
               
               if((line1!=NULL))
               {
                  copy_exists = true;
                  break;
               }
            }
               
         }

         if(op == 'w')
         {
            if(line->getFlags() == EXCLUSIVE)
            {
               //EXCLUSIVE -> Modified (PrWr/-)
               line->setFlags(MODIFIED);
            }
           
            else if(line->getFlags() == SC)
            {
               //Sc -> Sm (PrWr/BusUpd(C))

               if(copy_exists)
               {
                  line->setFlags(SM);
                  for(int i=0;i<num_processors;i++)
                  {
                     if(i!=processor_no)
                     {
                        //BusUpd -> u
                        multicore_caches[i]->Dragon(i,addr,'u',multicore_caches,num_processors);
                     }
                  }
               }
               else
               {
                  //Sc -> M (PrWr/BusUpd(!C))
                  line->setFlags(MODIFIED);
                  for(int i=0;i<num_processors;i++)
                  {
                     if(i!=processor_no)
                     {
                        //BusUpd -> u
                        multicore_caches[i]->Dragon(i,addr,'u',multicore_caches,num_processors);
                     }
                  }
               }
               
            }

            else if(line->getFlags() == SM)
            {
               
               if(copy_exists)
               {
                  //Sm -> Sm (PrWr/BusUpd(C))
                  line->setFlags(SM);
                  for(int i=0;i<num_processors;i++)
                  {
                     if(i!=processor_no)
                     {
                        //BusUpd -> u
                        multicore_caches[i]->Dragon(i,addr,'u',multicore_caches,num_processors);
                     }
                  }
               }
               else
               {
                  //Sm -> M (PrWr/BusUpd(!C))
                  line->setFlags(MODIFIED);
                  for(int i=0;i<num_processors;i++)
                  {
                     if(i!=processor_no)
                     {
                        //BusUpd -> u
                        multicore_caches[i]->Dragon(i,addr,'u',multicore_caches,num_processors);
                     }
                  }
               }
               
            }

            else if(line->getFlags() == MODIFIED)
            {
               //Modified -> Modified (No snoop msgs)
               line->setFlags(MODIFIED);
            }
         }
         //Processor read
         else if(op == 'r')
         {
            if(line->getFlags() == EXCLUSIVE)
            {
               //Exclusive -> Exclusive (No Snoop msgs)
               line->setFlags(EXCLUSIVE);
            }
            else if(line->getFlags() == SC)
            {
               // Sc -> Sc (No Snoop msgs)
               line->setFlags(SC);
            }
            else if(line->getFlags() == SM)
            {
               //Sm -> Sm (No Snoop msgs)
               line->setFlags(SM);
            }
            else if(line->getFlags() == MODIFIED)
            {
               // modified -> modified (No Snoop msgs)
               line->setFlags(MODIFIED);
            }
         }
      }  
   }
   else if((op =='s') || (op =='u'))
   {
      cacheLine * line = findLine(addr);
      if(line == NULL)
      {
         //Invalid state and so no action for BusRd, BusUpdate
      }
      else 
      {
         
         if(op == 's')
         {
            //s -> BusRd
            if(line->getFlags() == EXCLUSIVE)
            {
               // Exclusive -> Sc
               line->setFlags(SC);
               interventions++;
            }
            else if(line->getFlags() == MODIFIED)
            {  
               //Modified -> Sm
               line->setFlags(SM);
               flushes++;
               interventions++;
               // writeBacks++;
               // memory_transactions++;
            }
            else if(line->getFlags() == SC)
            {
               //Sc -> Sc
               line->setFlags(SC);
            }
            else if(line->getFlags() == SM)
            {
               //SM -> SM
               line->setFlags(SM);
               flushes+=1;
               // writeBacks++;
               // memory_transactions++;
            }
         }

         else if(op == 'u')
         {
            if(line->getFlags() == EXCLUSIVE)
            {
               //Not possible
            }
            else if(line->getFlags() == MODIFIED)
            {
               //Not possible
            }
            else if(line->getFlags() == SC)
            {
               // Sc -> Sc
               //updates
               // writeBacks++;
               // memory_transactions++;
              
               
            }
            else if(line->getFlags() == SM)
            {
               //Sm -> Sc
               line->setFlags(SC);
               
               //updates
               // writeBacks++;
               // memory_transactions++;
               
               
            }
         }
      }
   }
	
}
