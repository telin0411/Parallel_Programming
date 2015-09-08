#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include<time.h>

int judge(int , int);
const int maxArray = 1000000;
//Function of swapping
int swap(int*a,int*b){
    int c;
    c=*a;
    *a=*b;
    *b=c;
    return 0;
}

int main(int argc, char *argv[]){
   
    int numtasks, rank, len, rc, c, var, procSize;
    int cnt = 0;
    int size, judgePhase, oddevenTask;
    int value[maxArray];
    char hostname[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status; 
    // Parameters for timing measurement
    double timeStart, timeEnd; // Whole program
    double timeIOS, timeIOE, timeCommS, timeCommE, timeCompS, timeCompE; // Start-End time    
    double gapIO=0, gapComm=0, gapComp=0; // Gap time
    double recvgapIO, recvgapComm, recvgapComp;             
    //Declare the file and read in
timeIOS = clock();
    FILE *inFile;
    int tmp1[maxArray]; 
    int tmp[maxArray];
    inFile = fopen(argv[1], "r");
    if (!inFile){
        return 1;
    }
    //Display the file
    int i, k;
    int j = 0;
    fscanf(inFile, "%d", &var);
    size = var;
    for (i=0; i<size; i++){
        if ((c = fgetc(inFile)) != EOF){
            fscanf(inFile, "%d", &var);
            tmp[i] = var; 
        }
    }
    /*
    for (i=0; i<size; i++){
        tmp1[i] = tmp[i];
    }*/ 
    fclose(inFile);    
    
    //MPI processes
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS){
        printf("MPI initialization failed\n");
        return -1;
    }
   	if( rank == 0 ){
		    timeStart = clock();
    }          
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(hostname, &len);
    MPI_Barrier(MPI_COMM_WORLD);                
    //Allocate the input values
    int fakeSize;
    oddevenTask = numtasks % 2;
    if (size%numtasks == 0){
        procSize = size / numtasks;       
    }
    else {
        fakeSize = size - size%numtasks;
        procSize = fakeSize / numtasks;
        procSize += 1;
        int remainder;
        remainder = procSize * numtasks - size;
        
        for (i=size; i<procSize*numtasks; i++){
            tmp[i] = -1;
        }
        /*
        for (i=0; i<size; i++){
            tmp[i+remainder] = tmp1[i];
        }
        for (i=0; i<remainder; i++){
            tmp[i] = -1;
        }  */                      
    }
if( rank == 0 ){
    timeIOE = clock();
    gapIO += (timeIOE - timeIOS);
}       
    //printf("Number of tasks= %d; My rank= %d; Running on%s\n", numtasks, rank, hostname);
   	MPI_Barrier(MPI_COMM_WORLD);
    //Scatter the values evenly to each task
    MPI_Scatter(&tmp, procSize, MPI_INT, &value, procSize, MPI_INT, 0, MPI_COMM_WORLD);
    //printf("Rank %d recieved : ", rank);
    /*for (k=0; k<procSize; k++){
        printf("%d ", value[k]);    
    }*/
    //printf("\n");  
      
    //MPI communicators sending and recieving values process
    for (i=0; i<procSize*numtasks; i++){   
        judgePhase = i % 2;
        //Even phases case1
        if (i%2==0 && procSize%2==0){
timeCompS = clock();         
            for (j=0; j<procSize; j++){
                if(j%2==0){
                    if (value[j]>value[j+1]){
                        swap(&value[j], &value[j+1]);                                                   
                    }                
                }    
            }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);            
        }
        //Even phases case2
        else if (i%2==0 && procSize%2==1){       
            if(rank%2==0 && (rank != judge(numtasks, judgePhase))){
timeCompS = clock();             
                for (k=0; k<procSize-1; k++){
                    if (k%2==0){
                        if (value[k]>value[k+1]){
                            swap(&value[k], &value[k+1]);
                        }    
                    }                   
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                  
                if (numtasks>1){
timeCommS = clock();                
                    MPI_Send(&value[procSize-1], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
                    MPI_Recv(&value[procSize], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);
                    if (value[procSize]<value[procSize-1]){
                        value[procSize-1] = value[procSize];
                    }
timeCommE = clock();
gapComm += (timeCommE - timeCommS);                    
                }                             
            }
            else if (rank%2==1 && (rank != judge(numtasks, judgePhase))){
timeCompS = clock(); 
                for (k=1; k<procSize; k++){               
                    if (k%2==1){
                        if (value[k]>value[k+1]){
                            swap(&value[k], &value[k+1]);
                        }    
                    }                    
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                 
                if (numtasks>1){
timeCommS = clock();                                
                    MPI_Recv(&value[procSize], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);
                    MPI_Send(&value[0], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
                    if (value[procSize]>value[0]){
                        value[0] = value[procSize];
                    }
timeCommE = clock();
gapComm += (timeCommE - timeCommS);                     
                }                
            }                      
            else if ((rank == judge(numtasks, judgePhase)) && (oddevenTask==1)){
timeCompS = clock();             
                for (k=0; k<procSize-1; k++){
                    if (k%2==0){
                        if (value[k]>value[k+1]){
                            swap(&value[k], &value[k+1]);
                        }    
                    }
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                                     
            }                      
        }
        //Odd phases case1
        else if (i%2==1 && procSize%2==1){
            if(rank%2==1 && rank != 0 && (rank != judge(numtasks, judgePhase))){
timeCompS = clock();             
                for (k=0; k<procSize-1; k++){
                    if (k%2==0){
                        if (value[k]>value[k+1]){
                            swap(&value[k], &value[k+1]);
                        }    
                    }                    
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                 
                if (numtasks>1){
timeCommS = clock();                                 
                    MPI_Send(&value[procSize-1], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
                    MPI_Recv(&value[procSize], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);
                    if (value[procSize]<value[procSize-1]){
                        value[procSize-1] = value[procSize];
                    }
timeCommE = clock();
gapComm += (timeCommE - timeCommS);                    
                }                            
            }
            else if (rank%2==0 && rank != 0){
timeCompS = clock();            
                for (k=1; k<procSize; k++){
                    if (k%2==1){
                        if (value[k]>value[k+1]){
                            swap(&value[k], &value[k+1]);
                        }    
                    }                    
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                 
                if (numtasks>1){
timeCommS = clock();                                
                    MPI_Recv(&value[procSize], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);
                    MPI_Send(&value[0], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
                    if (value[procSize]>value[0]){
                        value[0] = value[procSize];
                    }
timeCommE = clock();
gapComm += (timeCommE - timeCommS);                    
                }                
            }                      
            else if (rank == 0){
timeCompS = clock();            
                for (k=1; k<procSize; k++){
                    if (k%2==1){
                        if (value[k]>value[k+1]){
                            swap(&value[k], &value[k+1]);
                        }    
                    }
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                                         
            }
            else if ((rank == judge(numtasks, judgePhase)) && rank%2==1){
timeCompS = clock();            
                for (k=0; k<procSize-1; k++){
                    if (k%2==0){
                        if (value[k]>value[k+1]){
                            swap(&value[k], &value[k+1]);
                        }    
                    }
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                                        
            }                                                 
        }
        //Odd phases case2
        else if (i%2==1 && procSize%2==0){
            if(rank != 0 && rank != numtasks-1){
timeCompS = clock();            
                for (k=1; k<procSize-1; k++){
                    if (k%2==1){
                        if (value[k]>value[k+1]){
                            swap(&value[k], &value[k+1]);
                        }    
                    }                          
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                
                if (numtasks>1){
timeCommS = clock();                                 
                    MPI_Send(&value[procSize-1], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
                    MPI_Recv(&value[procSize], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);
                    if (value[procSize]<value[procSize-1]){
                        value[procSize-1] = value[procSize];
                    }
                    MPI_Send(&value[0], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
                    MPI_Recv(&value[procSize+1], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);
                    if (value[0]<value[procSize+1]){
                        value[0] = value[procSize+1];
                    }
timeCommE = clock();
gapComm += (timeCommE - timeCommS);                     
                }                   
            }                     
            else if (rank == 0){
timeCompS = clock();            
                for (k=1; k<procSize-1; k++){
                    if (value[k]>value[k+1]){
                        swap(&value[k], &value[k+1]);
                    }                        
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                 
                if (numtasks>1){
timeCommS = clock();                                
                    MPI_Send(&value[procSize-1], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
                    MPI_Recv(&value[procSize], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);
                    if (value[procSize]<value[procSize-1]){
                        value[procSize-1] = value[procSize];
                    }
timeCommE = clock();
gapComm += (timeCommE - timeCommS);                     
                }                                        
            }
            else if (rank == numtasks-1){
timeCompS = clock();         
                for (k=1; k<procSize-1; k++){
                    if (value[k]>value[k+1]){
                        swap(&value[k], &value[k+1]);
                    }
                }
timeCompE = clock();
gapComp += (timeCompE - timeCompS);                 
                if (numtasks>1){
timeCommS = clock();                               
                    MPI_Send(&value[0], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
                    MPI_Recv(&value[procSize], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);
                    if (value[0]<value[procSize]){
                        value[0] = value[procSize];
                    } 
timeCommE = clock();
gapComm += (timeCommE - timeCommS);                     
                }                                        
            }                                                 
        }                             
    }     
    MPI_Gather(&value[0], procSize, MPI_INT, &tmp[0], procSize, MPI_INT, 0, MPI_COMM_WORLD);
    for (i=1; i<numtasks; i++){
        if (rank == i){
            MPI_Send(&gapComm, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&gapComp, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);                         
        }
        if (rank == 0){
            MPI_Recv(&recvgapComm, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&recvgapComp, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status); 
            gapComm += recvgapComm;
            gapComp += recvgapComp;                         
        }
    }   
    
    if (rank == 0){
timeIOS = clock();    
        printf("Sorted array listed below:\n");
        FILE *outFile = fopen(argv[2], "w");
        for (i=0; i<procSize*numtasks; i++){
            if (tmp[i] >= 0){
                printf("%d ", tmp[i]);            
                fprintf(outFile, "%d ", tmp[i]);
            }
        }
        printf("\n");
        fclose(outFile);
    }      

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize(); 
    
    //Record the time elapsed
   	if( rank == 0 ){
		    timeEnd = clock();
		    double gap = (timeEnd-timeStart) / CLOCKS_PER_SEC;
		    //printf("Running time : %lf\n", gap);
        //time_t t;
        //char  *nowTime = ctime(&t);
        //FILE *outFile = fopen(argv[3], "a");
        /*fprintf(outFile, "Schedule: %s", nowTime);
        fprintf(outFile, "Numbers : %d / Processors : %d\n", size, numtasks);         
        fprintf(outFile, "Running time : %lf\n", gap);       
        fprintf(outFile, "\n");
        fclose(outFile);*/
        printf("Numbers : %d / Processors : %d\n", size, numtasks);         
        printf("Running time : %lf\n", gap);       
        printf("\n");        
timeIOE = clock();
gapIO += (timeIOE - timeIOS);                    
	  }
     
    // Record the timing intervals analysis
if (rank == 0){    
    double totalTime, IOPer, CommPer, CompPer;
    gapIO = gapIO / CLOCKS_PER_SEC;
    gapComm = gapComm / CLOCKS_PER_SEC / numtasks;
    gapComp = gapComp / CLOCKS_PER_SEC / numtasks;
    totalTime = gapIO + gapComm + gapComp;             
    IOPer = gapIO / totalTime * 100;
    CommPer = gapComm / totalTime * 100;
    CompPer = gapComp / totalTime * 100; 
    //printf("Numbers: %d / Processors: %d \n", size, numtasks);         
    //printf("Input/Output time: %1f - %1f percent\n", gapIO, IOPer);      
    //printf("Communication time: %1f - %1f percent\n", gapComm, CommPer);
    //printf("Computing time: %1f - %1f percent\n", gapComp, CompPer);       
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