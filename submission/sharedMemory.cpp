
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

// g++ sharedMemory.cpp -std=c++11 -lpthread

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

struct thread_data_for_merge {
    int threadId;
    long int *resultArray;
    long int *list;
    long int listSize;
    long int partitionSize;
    long int numThreads;
    long int minPivot;
    long int maxPivot;
    long int exchangeSize;
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
    for (long int i = 0; i < numThreads; i++) {
        if (i == numThreads - 1) {
            sectionSize = listSize - (partitionSize * (numThreads - 1));
        } else {
            sectionSize = partitionSize;
        }
        // use a sliding window approach
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
        // cout << "left: " << left << "right: " << right << "\n";
        size = right - left + 1;
        totalSize = totalSize + size;
        // sections[i] = new section(left, size);
        struct section sec;
        sec.startIndex = left;
        sec.size = size;
        sections[i] = sec;
        // cout << "processor: " << id << " section: " << sections[i].startIndex << " " << sections[i].size << " left: " << left << " right: " << right << "\n";
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
    long int mergedSize = sections[0].size;
    // for (int i = 0; i < numThreads; i++) {
    //     cout << "thread: " << id << " start: " << sections[i].startIndex << " size: " << sections[i].size << "\n";
    // }
    for (int i = 1; i < numThreads; i++) {
        currentPartition = list + sections[i].startIndex;
        newPartition = (long int *) malloc((mergedSize + sections[i].size) * sizeof(long int));
        merge(currentMerge, currentMerge + mergedSize, currentPartition, currentPartition + sections[i].size, newPartition);
        currentMerge = newPartition;
        mergedSize = mergedSize + sections[i].size;
    }
    threadArgs->resultArray = currentMerge;
    threadArgs->mergeSize = mergedSize;

    //close the thread
    pthread_exit(NULL);
}

void *mergeOnThread(void *threadData) {
    // create a new vector
    struct thread_data_for_merge *threadArgs;
    threadArgs = (struct thread_data_for_merge *) threadData;
    int id = threadArgs->threadId;
    long int *resultArray = threadArgs->resultArray;
    long int *list = threadArgs->list;
    long int listSize = threadArgs->listSize;
    long int partitionSize = threadArgs->partitionSize;
    long int numThreads = threadArgs->numThreads;
    long int minPivot = threadArgs->minPivot;
    long int maxPivot = threadArgs->maxPivot;

    struct section sections[numThreads];
    // find all exchange sections from each partition for this particular processor
    long int left;
    long int right;
    bool leftStop;
    bool rightStop;
    long int size;
    long int totalSize = 0;
    long int sectionSize;
    for (long int i = 0; i < numThreads; i++) {
        if (i == numThreads - 1) {
            sectionSize = listSize - (partitionSize * (numThreads - 1));
        } else {
            sectionSize = partitionSize;
        }
        // use a sliding window approach
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
        // cout << "left: " << left << "right: " << right << "\n";
        size = right - left + 1;
        totalSize = totalSize + size;
        // sections[i] = new section(left, size);
        struct section sec;
        sec.startIndex = left;
        sec.size = size;
        sections[i] = sec;
        // cout << "processor: " << id << " section: " << sections[i].startIndex << " " << sections[i].size << " left: " << left << " right: " << right << "\n";
    }
    // cout << "processor: " << id << " total size: " << totalSize << "\n";
    // merge sections
    // resultArray = (long int *) realloc(resultArray, totalSize * sizeof(long int));
    //assume first partition is always merged 
    long int * currentMerge = list + sections[0].startIndex;
    long int * currentPartition;
    long int * newPartition;
    long int mergedSize = sections[0].size;
    for (int i = 1; i < numThreads; i++) {
        currentPartition = list + sections[i].startIndex;
        newPartition = (long int *) malloc((mergedSize + sections[i].size) * sizeof(int));
        merge(currentMerge, currentMerge + mergedSize, currentPartition, currentPartition + sections[i].size, newPartition);
        currentMerge = newPartition;
        mergedSize = mergedSize + sections[i].size;
    }
    threadArgs->resultArray = currentMerge;
    threadArgs->exchangeSize = mergedSize;
    // cout << "processor: " << id << " result array start: " << resultArray[0] << " \n";
    
    // for (int k = 0; k < mergedSize; k++) {
    //     cout << "thread: " << id << ", value = " << threadArgs->resultArray[k] << "\n";
    // }
    // long int sectionSize;
    // long int item;
    // // merge partitions together and sort elements as they are inserted
    // for (long int i = 0; i < numThreads; i++) {
    //     if (i == numThreads - 1) {
    //         sectionSize = listSize - (partitionSize * (numThreads - 1));
    //     } else {
    //         sectionSize = partitionSize;
    //     }
    //     for (long int j = 0; j < sectionSize; j++) {
    //         // cout << "access array item: " << (i * partitionSize) + j << "\n";
    //         item = list[(i * partitionSize) + j];
    //         // perform a sort as we merge
    //         if (item > minPivot && item <= maxPivot) {
    //             // insert item at beginning
    //             resultArray->insert(resultArray->begin(), item);
    //             // always keep vector sorted
                
                
    //         }
    //     }
    // }
    // sort(resultArray->begin(), resultArray->end());
    // cout << "thread exit: processor " << id << "\n";
    //close the thread
    pthread_exit(NULL);
}

int main() {
    // generate a random array of 1 million integers
    // use time as the seed since it is a number that changes all the time
    // default_random_engine generator;
    srand(time(NULL));
    
    const long int listSize = 32000000;
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
        // threadData[i].listStartIndex = i * partitionSize;
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

    // cout << "\nAfter Phase 1:\n";
    // for(int i = 0; i < listSize; i++) {
    //     cout << array[i] << " ";
    // };
    // cout << "\n";


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
        // cout << regularSamplePivots[i-1] << " ";
    }

    // stop the clock for Phase 2
    clock_t phase2Stop = clock();
    
    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 3 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // exchange partitions with each processor based on regular sample pivots
    // loop through list size_t
    // but know when to compare to a certain pivot

    // cout << "pivots: ";
    // for (int i = 0; i < numThreads - 1; i++) {
    //     cout << regularSamplePivots[i] << " ";
    // }
    // // cout << partitionSize << "\n";
    // cout << "\n";

    pthread_t phase3Threads [numThreads];
    struct phase_3_thread_data phase3ThreadData[numThreads];
    // let each thread merge its parts based on the pivots
    
    // std::vector<std::vector<int>> v(10, std::vector<int>(5));

    for (long int i = 0; i < numThreads; i++) {
        // segment list into (list size / number of threads) partitions, pass each partition to a thread to sort it
        // processorResults[i] = vector<long int>(0);
        // thread arguments
        phase3ThreadData[i].threadId = i;
        phase3ThreadData[i].numThreads = numThreads;
        phase3ThreadData[i].list = array;
        phase3ThreadData[i].listSize = listSize;
        phase3ThreadData[i].partitionSize = partitionSize;

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
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
    }

    // join all the threads together to sync end of phase 1 with beginning of phase 2
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        pthread_join(phase3Threads[i], NULL);
    }

    

    // cout << "\nAfter Phase 3\n";
    // for (long int i = 0; i < numThreads; i++) {
    //     cout << "processor: " << i << " exchange size" << phase3ThreadData[i].exchangeSize << "\n";
    // }
    // cout << "\n";


    // stop the clock for Phase 3
    clock_t phase3Stop = clock();


    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 4 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // merger the vectors back to the main array memory slots
    // this allows us to skip the merge step for phase 4, since
    // the exchanged partitions or sorted in place right next to each other
    // by each process

    pthread_t phase4Threads [numThreads];
    struct phase_4_thread_data phase4ThreadData[numThreads];
    // let each thread merge its parts based on the pivots

    for (long int i = 0; i < numThreads; i++) {
        // segment list into (list size / number of threads) partitions, pass each partition to a thread to sort it
        // processorResults[i] = vector<long int>(0);
        // thread arguments
        // for (int j = 0; j < numThreads; j++) {
        //     cout << "start: " << phase3ThreadData[i].sections[j].startIndex << " size: " << phase3ThreadData[i].sections[j].size << "\n";
        // }
        phase4ThreadData[i].threadId = i;
        phase4ThreadData[i].sections = phase3ThreadData[i].sections;
        phase4ThreadData[i].list = array;
        phase4ThreadData[i].numThreads = numThreads;

        pthread_create(&phase4Threads[i], NULL, mergePartitions, (void *) &phase4ThreadData[i]);
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
    }

    // join all the threads together to sync
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        pthread_join(phase4Threads[i], NULL);
    }

    // cout << "\nfinal sorted list\n";
    // concatenate arrays together
    long int counter = 0;
    for (long int i = 0; i < numThreads; i++) {
        // cout << "processor: " << i << " exchange size" << phase3ThreadData[i].exchangeSize << "\n";
        for (int j = 0; j < phase4ThreadData[i].mergeSize; j++) {
            // cout << phase4ThreadData[i].resultArray[j] << " ";
            array[counter] = phase4ThreadData[i].resultArray[j];
            counter++;
        }
    }
    // cout << "\n";

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


