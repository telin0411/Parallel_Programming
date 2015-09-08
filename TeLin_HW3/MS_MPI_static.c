#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

typedef struct complexType
{
	double real, imag;
} Compl;

int xflag = 1;
double timeStart,timeEnd; 
int rowCnt[20];
struct timeval thtv1[20], thtv2[20];
double thtimeStart[20], thtimeEnd[20];
double thgap[20];

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
  double xrange = xright - xleft;
  double yrange = yright - yleft;  
	/* set window position */
	int x = 0;
	int y = 0;
  int rowData[1400][1400];
 
  timeStart = clock(); 
	GC gc; 
  printf("X Window is %sd\n", argv[8]); 
  xflag = strcmp(argv[8], "enable"); 
      
  // MPI Process
  int numtasks, rank, len, rc, nlocal, localw;
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
 
  // Parameters
  Compl z, c;
  int repeats;
  double temp, lengthsq;
  MPI_Status status;
  MPI_Request req;  
  int i, j, slaveReturn;
  int task;
  int judgecnt=0, cnt;

  int fakewidth;
  fakewidth = width - width % numtasks;
  nlocal = fakewidth / numtasks;
  MPI_Bcast(&nlocal, 1, MPI_INT, 0, MPI_COMM_WORLD);
  for(cnt=0; cnt<numtasks; cnt++){
     rowCnt[cnt] = 0;
     thgap[cnt] = 0;
  }
        
  // Master task      
  if (rank == 0){                   
     localw = 0;
     for(task=1; task<numtasks; task++){
        MPI_Send(&localw, 1, MPI_INT, task, 0, MPI_COMM_WORLD);
        localw += nlocal;
     }
     printf("Total %d slaves functioning\n", numtasks-1);
     printf("Each contains %d rows to compute\n", nlocal);
     printf("Master %d computes from %d!\n", rank, localw);     
	   /* draw points */
	   //sleep(1);  
	   for(i=localw; i<width; i++) {
		   for(j=0; j<height; j++) {
         gettimeofday(&thtv1[rank], NULL);
         thtimeStart[rank] = thtv1[rank].tv_sec * 1000000 + thtv1[rank].tv_usec;             
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
         rowData[i][j] = repeats;
         rowCnt[rank]++;
         gettimeofday(&thtv2[rank], NULL);
         thtimeEnd[rank] = thtv2[rank].tv_sec * 1000000 + thtv2[rank].tv_usec;
         thgap[rank] += (thtimeEnd[rank]-thtimeStart[rank]) / CLOCKS_PER_SEC;          
		   }
	   }
     printf("Rank %d computed %d points consuming %1f seconds\n", rank, rowCnt[rank], thgap[rank]);       
     judgecnt++;  
 	   //sleep(1);
     while(judgecnt < numtasks){
         MPI_Recv(&slaveReturn, 1, MPI_INT, judgecnt, 0, MPI_COMM_WORLD, &status);
         int rowBuff;
         rowBuff = (slaveReturn - 1) * nlocal;
         printf("rowBuff = %d from slave %d\n", rowBuff, slaveReturn);
	       for(i=rowBuff; i<rowBuff+nlocal; i++) {
		        for(j=0; j<height; j++) {
               MPI_Recv(&rowData[i][j], 1, MPI_INT, slaveReturn, 0, MPI_COMM_WORLD, &status); 
            }
         }                 
         judgecnt++;
     }
     printf("judgecnt = %d\n", judgecnt);
     if (xflag == 0 && judgecnt >= numtasks){
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
        for(i=0; i<width; i++) {
	         for(j=0; j<height; j++) {
              XSetForeground (display, gc,  1024 * 1024 * (rowData[i][j] % 256));		
	            XDrawPoint (display, window, gc, i, j);
           }        
        }
        XFlush(display);       
     }
      
     timeEnd = clock();  
	   double gap = (timeEnd-timeStart) / CLOCKS_PER_SEC;  
     printf("OOOOOOO Graph Drawing Done OOOOOO\n"); 
     printf("Processors : %d\n", numtasks);         
     printf("Running time : %lf\n", gap);     
     FILE *outFile;
     outFile = fopen(argv[9], "a");
     fprintf(outFile, "Processors : %d\n", numtasks);      
     fprintf(outFile, "Running time : %lf\n\n", gap);
     fclose(outFile);               
  } 
  // Slave task
  else{
     MPI_Recv(&localw, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);  
	   /* draw points */
     printf("Slave %d has %d computations from %d to do!\n",rank,  nlocal, localw);  
	   for(i=localw; i<localw+nlocal; i++) {
		   for(j=0; j<height; j++) {
         gettimeofday(&thtv1[rank], NULL);
         thtimeStart[rank] = thtv1[rank].tv_sec * 1000000 + thtv1[rank].tv_usec;          
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
         rowData[i][j] = repeats;
         rowCnt[rank]++;
         gettimeofday(&thtv2[rank], NULL);
         thtimeEnd[rank] = thtv2[rank].tv_sec * 1000000 + thtv2[rank].tv_usec;
         thgap[rank] += (thtimeEnd[rank]-thtimeStart[rank]) / CLOCKS_PER_SEC;          
		   }
	   }
     printf("Rank %d computed %d points consuming %1f seconds\n", rank, rowCnt[rank], thgap[rank]);    
     slaveReturn = rank;
     MPI_Send(&slaveReturn, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	   for(i=localw; i<localw+nlocal; i++) {
		   for(j=0; j<height; j++) {
          MPI_Send(&rowData[i][j], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
       }
     }               
  }  
    
  MPI_Barrier(MPI_COMM_WORLD);  
  MPI_Finalize();    
	sleep(5);
	return 0;
}
