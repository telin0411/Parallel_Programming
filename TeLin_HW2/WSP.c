#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_t *threads;

// City map's Structures
struct cityMap {
    int citySize; // Specify the map size
    int route[30][30]; // Store the city distances
    int cityStart;  
    int numthreads;
    int threadid;
};
typedef struct cityMap cityMap;
cityMap Map;

// For optimal solutions storaging
int lbPublic[20];
int OptPath[20][20];

// Print the map
void PrintMap(void *MapTmp){
    // Pointer Transformation
    cityMap Map, *MapTmp1;
    MapTmp1 = (cityMap *) MapTmp;
    Map = *MapTmp1; 
    int i, j; 
    for (i=0; i<Map.citySize; i++){
        for (j=0; j<Map.citySize; j++){
                printf("%d ", Map.route[i][j]);
        }
        printf("\n");
    }   
}
// Transform the map
cityMap MapTransform(void *MapTmp){
    cityMap Map, *MapTmp1;
    MapTmp1 = (cityMap *) MapTmp;
    Map = *MapTmp1; 
    int s = Map.cityStart;
    cityMap MapT;
    MapT = Map;
    int S[20];
    S[0] = Map.cityStart-1;
    int x = 1, i, j;
    for(i=0; i<Map.citySize; i++){
        if(i != Map.cityStart-1 && x<Map.citySize){
            S[x] = i;
            x++;
        }
    }
    printf("\n");
    for(i=0; i<Map.citySize; i++){
        for(j=0; j<Map.citySize; j++){
            MapT.route[i][j] = Map.route[S[i]][S[j]];         
        }
    }   
    return MapT;
}
// Global parameters
int F[20][624288][20];
int M[20][624288][20];  
double gap = 0;
int Timer = 0;
cityMap Map1[30];
double timeStart,timeEnd;

// WSP Function using Dynamic Programming
void *WSP_Func_Init(void *MapTmp){
    cityMap Map, *MapTmp1;
    MapTmp1 = (cityMap *) MapTmp;
    Map = *MapTmp1; 
    printf("############## Start at %d By thread %d ##############\n", Map.cityStart, Map.threadid);       
    int i, j, min, k, temp;
    int S;
    S = Map.cityStart-1;    
    pthread_mutex_lock (&mutex);
        if(Map.threadid == 0){
            timeStart = clock();
        }
    pthread_mutex_unlock (&mutex);    
    for(i=0; i<Map.citySize; i++){
        Map.route[i][0] = 0;
        F[i][0][S] = Map.route[i][0];
    }
    int b;
    b = pow(2, Map.citySize-1);
    for(i=1; i<b-1; i++){
        for(j=1; j<Map.citySize; j++){
            if( ((int)pow(2, j-1) & i) == 0){
                min = 1000000;
                for(k=1; k<Map.citySize; k++){
                    if( (int)pow(2, k-1) & i ){
                        temp = Map.route[j][k] + F[k][i-(int)pow(2, k-1)][S];
                        if(temp < min){
                            min = temp;
                            F[j][i][S] = min;
                            M[j][i][S] = k;
                        }
                    }
                }
            } 
        }
    }
    min = 1000000;
    for(k=1; k<Map.citySize; k++){
        temp = Map.route[0][k] + F[k][b-1 - (int)pow(2, k-1)][S];
        if(temp < min){
            min = temp;
            F[0][b-1][S] = min;
            M[0][b-1][S] = k;
        }
    }
    int OptLength;
    OptLength = F[0][b-1][S];
    printf("Optimal Length = %d\n", F[0][b-1][S]);
    if(OptLength <= lbPublic[S]){
        lbPublic[S] = OptLength;
        printf("lbPublic = %d\n", lbPublic[S]);
    }
    printf("%d", Map.cityStart);
    int next = 1;
    OptPath[S][0] = Map.cityStart;
    for(i=b-1, j=0; i>0;){
        j = M[j][i][S];
        i = i - (int)pow(2, j-1);
        if(j >= Map.cityStart){
            printf(" => %d", j+1);
            OptPath[S][next] = j+1;
            next++;
        }
        else{
            printf(" => %d", j);
            OptPath[S][next] = j;
            next++;            
        }
    }
    printf("\n");
    if(Map.numthreads != 1){
        if(Map.threadid != 0){ 
            pthread_mutex_lock(&mutex);        
            Timer++;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);            
        }
        else if(Map.threadid == 0){  
            pthread_mutex_lock(&mutex); 
            while(Timer < Map.numthreads-1){        
                pthread_cond_wait(&cond, &mutex); 
            }  
                timeEnd = clock();
                gap += ((timeEnd-timeStart) / CLOCKS_PER_SEC) / Map.numthreads;           
                printf("Running time : %lf\n", gap);    
                pthread_mutex_unlock(&mutex);
        }     
    }   
    else{
        pthread_mutex_lock(&mutex);
        timeEnd = clock();
        gap += ((timeEnd-timeStart) / CLOCKS_PER_SEC) / Map.numthreads;           
        printf("Running time : %lf\n", gap);    
        pthread_mutex_unlock(&mutex);    
    }
    pthread_exit(NULL);
}
// WSP execution function
int WSP_Func(void *MapTmp, int NUM_THREADS, int threadcnt){
    cityMap Map, *MapTmp1;
    MapTmp1 = (cityMap *) MapTmp;
    Map = *MapTmp1;
    int i, j, rc;
    pthread_attr_t attr;    
    pthread_cond_init(&cond, NULL);    
    pthread_attr_init(&attr); 
    pthread_mutex_init(&mutex, NULL);     
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);    
    threads = (pthread_t *) malloc(NUM_THREADS*sizeof(pthread_t));
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);  
    void *status;
    Map.numthreads = NUM_THREADS;
    for(i=threadcnt; i<threadcnt+NUM_THREADS+1; i++){
        Map1[i] = Map;
        Map1[i].cityStart = i+1;
        if(Map1[i].cityStart != 1){
            Map1[i] = MapTransform(&Map1[i]);
        }
    }
    int mapOrder;
    int threadorder;
    threadorder = threadcnt;
    mapOrder = 0;
    while (mapOrder<NUM_THREADS){
        printf("In WSP: creating thread %ld\n", mapOrder);  
        Map1[threadorder].threadid = mapOrder;     
        rc = pthread_create(&threads[mapOrder], &attr, WSP_Func_Init, (void *) &Map1[threadorder]);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }    
        mapOrder++; 
        threadorder++;               
    }
    mapOrder = 0;
    while (mapOrder<NUM_THREADS){    
        rc = pthread_join(threads[mapOrder], &status);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
        else{
            threadcnt++;
        }
        mapOrder++; 
    }   
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&mutex);         
    free(threads);            
    return threadcnt;
}
// Main Block
int main(int argc, char *argv[]){
    
    // Local parameters
    int var; // Registers for reading in file
    int i, j, m, n; // For for loop iterations
    int c; // For file judging
    //double timeStart,timeEnd;     
    // Parameters for pthread
    int NUM_THREADS, NUM_THREADS_TMP;
    int threadcnt;
    // Initialize the pthread variables
    NUM_THREADS = atoi(argv[1]);
    NUM_THREADS_TMP = NUM_THREADS;
    printf("\n\n######################### WSP Start! ###################################\n");
    printf("OOOOOOOOOOOOOOOOOOOOOOOOO Total %d threads functioning! OOOOOOOOOOOOOOOO\n\n", NUM_THREADS);    
    // Read in the city map file
    FILE *inFile;
    inFile = fopen(argv[2], "r");
    if (!inFile){
        return 1;
    }
    fscanf(inFile, "%d", &var);   
    Map.citySize = var;
    printf("There are %d cities in the map. \n", Map.citySize); 
    for (i=0; i<Map.citySize; i++){
        for (j=0; j<Map.citySize; j++){
            if ((c = fgetc(inFile)) != EOF){
                fscanf(inFile, "%d", &var);
                 Map.route[i][j] = var; 
            }
        }
    } 
    fclose(inFile);
    
    PrintMap(&Map);
    // Initialize the lowerbound Public
    for(i=0; i<Map.citySize; i++){       
        lbPublic[i] = 10000000;
    }
    printf("\n");
    // Map functioning
    for(i=0; i<Map.citySize; i++){
        for(j=0; j<Map.citySize; j++){
            if(Map.route[i][j] == -1 && i != j){
                Map.route[i][j] = 100000;
            }
            else if(Map.route[i][j] == -1 && i == j){
                Map.route[i][j] = 0;
            }
        }
    }
    if (NUM_THREADS > Map.citySize){
        NUM_THREADS = Map.citySize;
    }       
    // Thread function distributer
    threadcnt = 0;
    while(threadcnt < Map.citySize){
        Timer = 0;
        threadcnt = WSP_Func(&Map, NUM_THREADS, threadcnt);
        if (Map.citySize-threadcnt < NUM_THREADS){
            NUM_THREADS = Map.citySize-threadcnt;          
        }
    } 
    // I/O        
    if(threadcnt == Map.citySize){
        int minLB = 10000000, order;
        for(i=0; i<Map.citySize; i++){
            if(lbPublic[i] <= minLB){
                minLB = lbPublic[i];
                order = i;
            }
        }
        FILE *outFile;
        outFile = fopen(argv[3], "a");
        fprintf(outFile, "Problem Size = %d cities\n", Map.citySize);
        fprintf(outFile, "Total functioning threads = %d\n", NUM_THREADS_TMP);        
        printf("\n################ Optimal Solution ################\n");
        fprintf(outFile, "################ Optimal Solution ################\n");
        printf("%d:", minLB);
        fprintf(outFile, "%d:", minLB);
        for(i=0; i<Map.citySize-1; i++){
            printf("%d,", OptPath[order][i]);
            fprintf(outFile, "%d,", OptPath[order][i]);            
        }        
        printf("%d\n\n", OptPath[order][Map.citySize-1]);   
        fprintf(outFile, "%d\n", OptPath[order][Map.citySize-1]);                         
        printf("Running time : %lf\n", gap);  
        fprintf(outFile, "Running time : %lf\n\n", gap);          
        fclose(outFile);    
    }  
         
    /* Last thing that main() should do */
    pthread_exit(NULL);
}