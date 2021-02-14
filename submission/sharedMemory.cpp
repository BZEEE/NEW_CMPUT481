
#include "mpi.h"
#include <iostream>
#include <time.h>
#include <cstdlib>
#include <pthread.h>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <random>


// g++ sharedMemory.cpp -std=c++11  -lpthread -o psrs

using namespace std;

struct section {
    long int startIndex;
    long int size;
};

struct thread_data {
    int threadId;
    long int *list;
    long int listSize;
};

struct phase_3_thread_data {
    int threadId;
    struct section * sections;
    long int numThreads;
    long int *list;
    long int listSize;
    long int partitionSize;
    long int minPivot;
    long int maxPivot;
};

struct phase_4_thread_data {
    int threadId;
    struct section * sections;
    long int * list;
    long int numThreads;
    long int *resultArray;
    long int mergeSize;
};

int compare(const void* a, const void* b) {
    // returns a negative integer if the first argument is less than the second
    // returns a positive integer if the first argument is greater than the second
    // returns zero if they are equal
	const long int* x = (long int*) a;
	const long int* y = (long int*) b;

	if (*x > *y) {
        return 1;
    }	
	else if (*x < *y) {
        return -1;
    } else {
        return 0;
    }
}

void *sortOnThread(void *threadData) {
    // get args passed to thread
    struct thread_data *threadArgs;
    threadArgs = (struct thread_data *) threadData;
    int id = threadArgs->threadId;
    long int* array = threadArgs->list;
    long int listSize = threadArgs->listSize;
    // perform quick sort on partitioned data using quicksort, sorts in place
    qsort(array, listSize, sizeof(long int), compare);
    //close the thread
    pthread_exit(NULL);
}

void *exchangePartitions(void *threadData) {
    // find all exchange sections from each partition for this particular processor
    struct phase_3_thread_data *threadArgs;
    threadArgs = (struct phase_3_thread_data *) threadData;
    int id = threadArgs->threadId;
    long int *list = threadArgs->list;
    long int listSize = threadArgs->listSize;
    long int partitionSize = threadArgs->partitionSize;
    long int numThreads = threadArgs->numThreads;
    long int minPivot = threadArgs->minPivot;
    long int maxPivot = threadArgs->maxPivot;

    struct section * sections = (struct section *) malloc(numThreads * sizeof(struct section));
    long int left;
    long int right;
    bool leftStop;
    bool rightStop;
    long int size;
    long int totalSize = 0;
    long int sectionSize;
    // get the partition from each processor using the minPivot and maxPivot value ranges
    for (long int i = 0; i < numThreads; i++) {
        // if the total size of the array is not evenly divisible by the number of processors
        // then the partition from the last array is a different size from the other partitions
        if (i == numThreads - 1) {
            sectionSize = listSize - (partitionSize * (numThreads - 1));
        } else {
            sectionSize = partitionSize;
        }
        // use a sliding window approach
        // have a pointer to the left side and the right side of the partition window
        // traverse the left pointer to the right until it is greater than minPivot 
        // traverse the right pointer to the left until it is less than than maxPivot 
        // or unless the left and right pointer have crossed
        left = (i * partitionSize);
        right = (i * partitionSize) + sectionSize - 1;
        leftStop = false;
        rightStop = false;
        while ( left <= right && !(leftStop && rightStop) ) {
            if (list[left] > minPivot) {
                leftStop = true;
            } else {
                left++;
            }
            if (list[right] <= maxPivot) {
                rightStop = true;
            } else {
                right--;
            }
        }
        // the left and right range is a section that this processor will merge with other sections
        size = right - left + 1;
        totalSize = totalSize + size;
        struct section sec;
        sec.startIndex = left;
        sec.size = size;
        sections[i] = sec;
    }
    threadArgs->sections = sections;

    //close the thread
    pthread_exit(NULL);
}

void *mergePartitions(void *threadData) {
    struct phase_4_thread_data *threadArgs;
    threadArgs = (struct phase_4_thread_data *) threadData;
    int id = threadArgs->threadId;
    struct section * sections = threadArgs->sections;
    long int numThreads = threadArgs->numThreads;
    long int * list = threadArgs->list;
    long int * currentMerge = list + sections[0].startIndex;
    long int * currentPartition;
    long int * newPartition;
    // assume first exchange section is always merged, this handles the case where we only have one processor (thread)
    long int mergedSize = sections[0].size;
    // for (p) number of processors there will be p sections we need to merge in this thread
    for (int i = 1; i < numThreads; i++) {
        currentPartition = list + sections[i].startIndex;
        newPartition = (long int *) malloc((mergedSize + sections[i].size) * sizeof(long int));
        merge(currentMerge, currentMerge + mergedSize, currentPartition, currentPartition + sections[i].size, newPartition);
        currentMerge = newPartition;
        mergedSize = mergedSize + sections[i].size;
    }
    // return the merged arrays and the total size of the merged arrays
    threadArgs->resultArray = currentMerge;
    threadArgs->mergeSize = mergedSize;

    //close the thread
    pthread_exit(NULL);
}

int main() {
    // use time as the seed since it is a number that changes all the time
    // default_random_engine generator;
    srand(time(NULL));
    
    const long int listSize = 10000000;
    long int *array = new long int[listSize];
    long int MAX_NUM = 2147483647;
    for (long int i = 0; i < listSize; i++) {
        array[i] = ((unsigned long int) (rand() * rand() * rand())) % MAX_NUM;
        // array[i] = ((unsigned long int) (rand() * rand() * rand())) % 50;
    }

    // display unsorted list
    // cout << "\nBefore:\n";
    // for(int i = 0; i < listSize; i++) {
    //     cout << array[i] << " ";
    // };
    // cout << "\n";

    // set parameters
    const long int numThreads = 1;
	pthread_t phase1Threads [numThreads];
    struct thread_data phase1ThreadData[numThreads];
    // set start and end partition indices
    long int *start = &array[0];
    long int partitionSize = floor(listSize / numThreads);

    // start the clock
    clock_t startClock = clock(); 

    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 1 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // pass (n/p) items to each processor, create a thread for each process
    for (long int i = 0; i < numThreads; i++) {
        // segment list into (list size / number of threads) partitions, pass each partition to a thread to sort it
        phase1ThreadData[i].threadId = i;
        phase1ThreadData[i].list = &array[i * partitionSize];
        // the list and number of processors isnt always evenly divisible
        // have to account for the last array
        if (i == numThreads - 1) {
            phase1ThreadData[i].listSize = listSize - (i * partitionSize);
        } else {
            phase1ThreadData[i].listSize = partitionSize;
        };
        pthread_create(&phase1Threads[i], NULL, sortOnThread, (void *) &phase1ThreadData[i]);
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        // pthread_join(phase1Threads[i], NULL);
    }

    // join all the threads together to sync end of phase 1 with beginning of phase 2
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        pthread_join(phase1Threads[i], NULL);
    }

    // stop the clock for Phase 1
    clock_t phase1Stop = clock();


    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 2 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // pick (number of processors - 1) pivots from each thread partition

    long int sampleSize = numThreads * numThreads;
    long int gatheredRegularSample[sampleSize];
    long int partitionPivotSeparation = floor(partitionSize / numThreads);
    for (long int i = 0; i < numThreads; i++) {
        for (long int j = 0; j < numThreads; j++) {
            // create a smaller array with all the pivot values from the partitioned array
            gatheredRegularSample[(i * numThreads) + j] = array[(i * partitionSize) + (j * partitionPivotSeparation)];
        }
    }
    // sort the smaller pivot array, sorts in place
    qsort(gatheredRegularSample, sampleSize, sizeof(long int), compare);

    // pick new pivots from pivot Array
    long int regularSamplePivots[numThreads-1];
    long int regularSamplePivotSeparation = floor(sampleSize / numThreads);
    for (long int i = 1; i < numThreads; i++) {
        regularSamplePivots[i-1] = gatheredRegularSample[i * regularSamplePivotSeparation];
    }

    // stop the clock for Phase 2
    clock_t phase2Stop = clock();
    
    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 3 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // exchange partitions with each processor based on regular sample pivots
    pthread_t phase3Threads [numThreads];
    struct phase_3_thread_data phase3ThreadData[numThreads];
    // let each thread determine its exchange sections based on the pivots

    for (long int i = 0; i < numThreads; i++) {
        // thread arguments
        phase3ThreadData[i].threadId = i;
        phase3ThreadData[i].numThreads = numThreads;
        phase3ThreadData[i].list = array;
        phase3ThreadData[i].listSize = listSize;
        phase3ThreadData[i].partitionSize = partitionSize;

        // set the min and max pivot range for this processor
        if (i == 0) {
            phase3ThreadData[i].minPivot = -1;
            phase3ThreadData[i].maxPivot = regularSamplePivots[i];
        } else if (i == numThreads - 1) {
            phase3ThreadData[i].minPivot = regularSamplePivots[i-1];
            phase3ThreadData[i].maxPivot = MAX_NUM;
        } else {
            phase3ThreadData[i].minPivot = regularSamplePivots[i-1];
            phase3ThreadData[i].maxPivot = regularSamplePivots[i];
        }

        pthread_create(&phase3Threads[i], NULL, exchangePartitions, (void *) &phase3ThreadData[i]);
    }

    // join all the threads together to sync for phase 3
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in the group before moving on to phase 4
        pthread_join(phase3Threads[i], NULL);
    }


    // stop the clock for Phase 3
    clock_t phase3Stop = clock();


    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 4 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // merge the exchanged sections together for each processor
    pthread_t phase4Threads [numThreads];
    struct phase_4_thread_data phase4ThreadData[numThreads];

    // let each thread merge its parts based on sections from phase 3
    for (long int i = 0; i < numThreads; i++) {
        phase4ThreadData[i].threadId = i;
        phase4ThreadData[i].sections = phase3ThreadData[i].sections;
        phase4ThreadData[i].list = array;
        phase4ThreadData[i].numThreads = numThreads;

        pthread_create(&phase4Threads[i], NULL, mergePartitions, (void *) &phase4ThreadData[i]);
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
    }

    // join all the threads together to sync
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in order to concatenate the merged partitions from each processor
        pthread_join(phase4Threads[i], NULL);
    }

    // concatenate arrays together
    long int counter = 0;
    for (long int i = 0; i < numThreads; i++) {
        for (int j = 0; j < phase4ThreadData[i].mergeSize; j++) {
            array[counter] = phase4ThreadData[i].resultArray[j];
            counter++;
        }
    }

    // stop the clock for Phase 4
    clock_t phase4Stop = clock();

    // Calculating total time taken by the program. 
    cout << "Time taken for Phase 1: " << fixed  << double(phase1Stop - startClock) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 2: " << fixed  << double(phase2Stop - phase1Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 3: " << fixed  << double(phase3Stop - phase2Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 4: " << fixed  << double(phase4Stop - phase3Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl; 
    cout << "Total time: " << fixed  << double(phase4Stop - startClock) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;  

    // print sorted array
    // cout << "\nAfter:\n";
    // for(int i = 0; i < listSize; i++) {
    //     cout << array[i] << " ";
    // };
    // cout << "\n";


    cout << "\nTest Correctness:\n";
    for(long int i = 0; i < listSize - 1; i++) {
        if (array[i] > array[i+1]) {
            cout << "false\n";
        }
    };
    cout << "\n";

    pthread_exit(NULL); // last thing that main should do

  
    
    // return 0;
}


