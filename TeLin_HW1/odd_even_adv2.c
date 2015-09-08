#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include <stdlib.h> 
#include <time.h>

const int maxArray = 300000000;
int judge(int, int);
int value[300000000];  
int tmp[300000000];
    
int compare (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

int swap(int*a,int*b){
    int c;
    c=*a;
    *a=*b;
    *b=c;
    return 0;
}
int main(int argc, char *argv[]){
   
    //Parameters for MPI
    int numtasks, rank, len, rc, nlocal;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status;   
    int cnt = 0;
    //Parameters for others
    int size, judgePhase, c, var;
    //int value[maxArray];  
    //int tmp[maxArray]; 
    double timeStart,timeEnd;         
    //Declare the file and read in
    if( rank == 0 ){    
       timeStart = clock();
    }        
    FILE *inFile;
    inFile = fopen(argv[1], "r");
    if (!inFile){
        return 1;
    }
    //Display and store the file
    int i, k;
    int j = 0;
    fscanf(inFile, "%d", &var);
    size = var;
    //int size2 = 2 * size;
    //int *tmp;
    //int *value;
    //tmp = (int*) malloc(size2*sizeof(int));    
    //value = (int*) malloc(size2*sizeof(int));     
    for (i=0; i<size; i++){
        if ((c = fgetc(inFile)) != EOF){
            fscanf(inFile, "%d", &var);
            tmp[i] = var; 
        }
    } 
    fclose(inFile);
    
    //MPI processes
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS){
        //printf("MPI initialization failed\n");
        return -1;
    }
    else {
        //printf("MPI initiated successfully\n");   
    }      
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(hostname, &len);
    MPI_Barrier(MPI_COMM_WORLD);     
    //Allocate the input values
    int fakeSize;
    if (size%numtasks == 0){
        nlocal = size / numtasks;       
    }
    else {
        fakeSize = size - size%numtasks;
        nlocal = fakeSize / numtasks;
        nlocal += 1;
        int remainder;
        remainder = nlocal * numtasks - size;        
        for (i=size; i<nlocal*numtasks; i++){
            tmp[i] = -1;
        }                      
    }       
   	MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&nlocal, 1, MPI_INT, 0, MPI_COMM_WORLD); 
        
    //Scatter the values evenly to each task
    MPI_Scatter(&tmp, nlocal, MPI_INT, &value, nlocal, MPI_INT, 0, MPI_COMM_WORLD);
    //printf("%d\n", numtasks);
    //MPI communicators sending and recieving values process
    for (i=0; i<numtasks; i++){
        judgePhase = i % 2;
        //Even phases
        if (i%2 == 0){
            if ((rank%2 == 0) && (rank != judge(numtasks, judgePhase))){
                if (numtasks > 1){            
                    MPI_Send(&value[0], nlocal, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
                    MPI_Recv(&value[nlocal], nlocal, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);   
                    qsort (value, 2*nlocal, sizeof(int), compare);   
                }                                           
            }
            else if ((rank%2==1) && (rank != judge(numtasks, judgePhase))) {
                if (numtasks > 1){                      
                    MPI_Recv(&value[nlocal], nlocal, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);
                    MPI_Send(&value[0], nlocal, MPI_INT, rank-1, 0, MPI_COMM_WORLD);                
                    qsort (value, 2*nlocal, sizeof(int), compare); 
                    for (k=0; k<nlocal; k++){
                        swap(&value[k], &value[k+nlocal]);
                    }                  
                }                                  
            }
            else if ((numtasks == 1) && (rank == 0)) {
                    qsort (value, nlocal, sizeof(int), compare);            
            }            
        }
        //Odd phases    
        else if (i%2 == 1) {
            if ((rank%2 == 1) && (rank != judge(numtasks, judgePhase))){    
                if (numtasks > 1){                   
                    MPI_Send(&value[0], nlocal, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
                    MPI_Recv(&value[nlocal], nlocal, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);
                    qsort (value, 2*nlocal, sizeof(int), compare);  
                }                                            
            }
            else if (rank%2==0 && (rank != 0)){
                if (numtasks > 1) {                  
                    MPI_Recv(&value[nlocal], nlocal, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);
                    MPI_Send(&value[0], nlocal, MPI_INT, rank-1, 0, MPI_COMM_WORLD);               
                    qsort (value, 2*nlocal, sizeof(int), compare);
                    for (k=0; k<nlocal; k++){
                        swap(&value[k], &value[k+nlocal]);
                    } 
                }                                           
            }             
        }             
    } 
    MPI_Barrier(MPI_COMM_WORLD);    
    MPI_Gather(&value[0], nlocal, MPI_INT, &tmp[0], nlocal, MPI_INT, 0, MPI_COMM_WORLD);
        
    // Display the output and store them into an ouput file
    if (rank == 0){   
        printf("Sorted array listed below:\n");
        FILE *outFile = fopen(argv[2], "w");         
        for (i=0; i<nlocal*numtasks; i++){
            if (tmp[i] >= 0){
                printf("%d ", tmp[i]);            
                fprintf(outFile, "%d ", tmp[i]);
            }
        }
        printf("\n");
        fclose(outFile);
    }          
    MPI_Barrier(MPI_COMM_WORLD);
    
    //free(value);
    //free(tmp); 
    MPI_Finalize(); 
   
    //Record the time elapsed
   	if( rank == 0 ){
        timeEnd = clock();
		    double gap = (timeEnd-timeStart) / CLOCKS_PER_SEC;
		    //printf("Running time : %lf\n", gap);
        //time_t t;
        //char  *nowTime = ctime(&t);
        //FILE *outFile = fopen(argv[3], "a");
        /*
        fprintf(outFile, "Schedule: %s", nowTime);
        fprintf(outFile, "Numbers : %d / Processors : %d\n", size, numtasks);         
        fprintf(outFile, "Running time : %lf\n", gap);       
        fprintf(outFile, "\n");
        */
        printf("Numbers : %d / Processors : %d\n", size, numtasks);         
        printf("Running time : %lf\n", gap);       
        printf("\n");        
        //fclose(outFile);                  
	  }

    return 0;
}

int judge(int size, int judgePhase){
    if (size%2 == 0){
        if (judgePhase == 0){
            return size;
        }
        else {
            size-=1;
            return size;
        }
    }
    else{
        if (judgePhase == 0){
            size-=1;
            return size;
        }
        else {
            return size;
        }
    }
}