#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>
#include <math.h>
#include <time.h>

typedef struct complexType
{
	double real, imag;
} Compl;

int xflag = 1;
double timeStart,timeEnd; 
int rowData[1400][1400]; 

int main(int argc, char *argv[])
{
	Display *display;
	Window window;      //initialization for a window
	int screen;         //which screen
 
 	/* set window size */
	int width = atoi(argv[6]);
	int height = atoi(argv[7]);
  int xleft = atoi(argv[2]);
  int yleft = atoi(argv[4]);
  int xright = atoi(argv[3]);
  int yright = atoi(argv[5]); 
  int NUM_THREADS = atoi(argv[1]); 
  double xrange = xright - xleft;
  double yrange = yright - yleft;  
	/* set window position */
	int x = 0;
	int y = 0;
  int NUM_PROCS = omp_get_num_procs();
  struct timeval tv1, tv2;
  gettimeofday(&tv1, NULL);

	GC gc; 
  xflag = strcmp(argv[8], "enable");
  omp_set_num_threads(NUM_THREADS);
  omp_set_nested(1); 
  int nest = omp_get_nested();
          
  // Parameters
  Compl z, c;
  int repeats;
  double temp, lengthsq; 
  int i, j;
  int fakeheight; 
  int task;
  int localw = 0;
  int nlocal = 100;  
  int tid;
  int width1;
  int judgecnt=0;
  int localStart, localEnd;
  int slaveReturn;   
  MPI_Status status;
  MPI_Request req;
          
  // MPI Process
  int numtasks, rank, len, rc;
  char hostname[MPI_MAX_PROCESSOR_NAME];   
  //MPI processes
  rc = MPI_Init(&argc, &argv);
  if (rc != MPI_SUCCESS){
    return -1;
  }    
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name(hostname, &len);
  MPI_Barrier(MPI_COMM_WORLD);  
  
  fakeheight = height - height % numtasks;
  nlocal = fakeheight / numtasks;       
  localStart = rank * nlocal;
  localEnd = localStart + nlocal;
  
  if(rank == (numtasks-1) && height % numtasks != 0){
      localEnd = height;
  }  
  
  if(rank == 0){
     timeStart = clock(); 
     printf("X Window is %sd\n", argv[8]); 
     printf("Total %d HW processors\n", NUM_PROCS); 
     printf("MPI %d nodes, each node creates %d threads \n", numtasks, NUM_THREADS);
     printf("omp_nested is set to %d\n\n", nest);   
  }  
  else{
     printf("Rank %d has %d columns to comnpute starting from column %d\n", rank, nlocal, localStart);
  }
        
  #pragma omp parallel num_threads(NUM_THREADS) private(tid, temp, lengthsq, z, c, repeats, i, j)
  { 
     tid = omp_get_thread_num();   
     printf("Rank %d -- Thread %d starts computing... \n", rank, tid);
     #pragma omp for schedule(static, 1)      
	   for(i=0; i<width; i++) {
		   for(j=localStart; j<localEnd; j++) {
			   z.real = 0.0;
		     z.imag = 0.0;
			   c.real = xleft + (double)i * (xrange/(double)width);
			   c.imag = yleft + (double)j * (yrange/(double)height);
			   repeats = 0;
			   lengthsq = 0.0;

			   while(repeats < 100000 && lengthsq < 4.0) {
				   temp = z.real*z.real - z.imag*z.imag + c.real;
				   z.imag = 2*z.real*z.imag + c.imag;
				   z.real = temp;
			 	   lengthsq = z.real*z.real + z.imag*z.imag; 
			 	   repeats++;
			   }
         #pragma omp critical
         {    
           rowData[i][j] = repeats;
         }
		   }    
     }     
  }  
  #pragma omp barrier  
  //MPI_Barrier(MPI_COMM_WORLD);
  
  if(rank != 0){     
     slaveReturn = rank;
     MPI_Isend(&slaveReturn, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &req);
	   for(i=0; i<width; i++) {
		   for(j=localStart; j<localEnd; j++) {
          MPI_Send(&rowData[i][j], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
       }
     }   
  }
  if(rank == 0){
     double ctStart, ctEnd;
     ctStart = clock();     
     judgecnt++;  
     while(judgecnt < numtasks){
         MPI_Irecv(&slaveReturn, 1, MPI_INT, judgecnt, 0, MPI_COMM_WORLD, &req);
         MPI_Wait(&req, &status);
         int rowBuff, rowBuff1;
         rowBuff = slaveReturn * nlocal;
         if(slaveReturn != numtasks-1){
            rowBuff1 = rowBuff + nlocal;
         }
         else{
            rowBuff1 = height;
         }
         printf("rowBuff = %d from slave %d\n", rowBuff, slaveReturn);
	       for(i=0; i<width; i++) {
		        for(j=rowBuff; j<rowBuff1; j++) {
               MPI_Recv(&rowData[i][j], 1, MPI_INT, slaveReturn, 0, MPI_COMM_WORLD, &status); 
            }
         }
         printf("Slave %d communicaiton done! \n", slaveReturn);                          
         judgecnt++;
     }  
     ctEnd = clock();
     double comgap = (ctEnd - ctStart) / CLOCKS_PER_SEC;
     if (xflag == 0){
	      //open connection with the server
	      display = XOpenDisplay(NULL);
	      if(display == NULL) {
		       fprintf(stderr, "cannot open display\n");
		       return -1;
	      }     
        screen = DefaultScreen(display);
	      //border width in pixels
	      int border_width = 0;
        //create window 
        window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width,
        BlackPixel(display, screen), WhitePixel(display, screen));	
        //create graph 
        XGCValues values;
        long valuemask = 0;	
        gc = XCreateGC(display, window, valuemask, &values);
	      //XSetBackground (display, gc, WhitePixel (display, screen));
	      XSetForeground (display, gc, BlackPixel (display, screen));
	      XSetBackground(display, gc, 0X0000FF00);
	      XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);	
	      //map(show) the window
	      XMapWindow(display, window);
	      XSync(display, 0);     
        // Draw the graph 
        for(i=0; i<width; i++) {
	         for(j=0; j<height; j++) {
              XSetForeground (display, gc,  1024 * 1024 * (rowData[i][j] % 256));		
	            XDrawPoint (display, window, gc, i, j);
           }        
        }
        XFlush(display);  
     }   
     gettimeofday(&tv2, NULL);
     double gap_sec = tv2.tv_sec - tv1.tv_sec;
	   double gap = (gap_sec * 1000000 + tv2.tv_usec - tv1.tv_usec) / 1000000;  
     printf("\nOOOOOOO Graph Drawing Done OOOOOO\n");
     printf("Total Threads : %d\n", numtasks*NUM_THREADS);         
     printf("Running time : %1f\n", gap);   
     printf("Communication time : %1f = %1f%\n", comgap, 100 * comgap / gap); 
     FILE *outFile;
     outFile = fopen(argv[9], "a");
     fprintf(outFile, "Threads : %d / Processors : %d\n", NUM_THREADS, numtasks);      
     fprintf(outFile, "Running time : %lf\n\n", gap);
     fclose(outFile);                     
  }   
       
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();  
           
	sleep(5);
	return 0;
}