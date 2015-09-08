#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

#define DRAW_TAG 1
#define FB_TAG 2
#define TER_TAG 3

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
  timeStart = clock();
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
  int judgeRow, cnt;  
	/* set window position */
	int x = 0;
	int y = 0;
  int rowData[1800];
    
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
  int i, j;
  int row = 0;
  
  for(cnt=0; cnt<numtasks; cnt++){
     rowCnt[cnt] = 0;
     thgap[cnt] = 0;
  }  
        
  // Master task      
  if (rank == 0){
     // X window process
     if (xflag == 0){
	      /* open connection with the server */ 
	      display = XOpenDisplay(NULL);
	      if(display == NULL) {
		       fprintf(stderr, "cannot open display\n");
		       return -1;
	      }    
        screen = DefaultScreen(display);
	      /* border width in pixels */
	      int border_width = 0;
        /* create window */
        window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width,
        BlackPixel(display, screen), WhitePixel(display, screen));	
        /* create graph */
        XGCValues values;
        long valuemask = 0;	
        gc = XCreateGC(display, window, valuemask, &values);
	      //XSetBackground (display, gc, WhitePixel (display, screen));
	      XSetForeground (display, gc, BlackPixel (display, screen));
	      XSetBackground(display, gc, 0X0000FF00);
	      XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);	
   	    /* map(show) the window */
	      XMapWindow(display, window);
	      XSync(display, 0);
	   } 
     // Assigning tasks
     int count = 0;
     for(i=1; i<numtasks; i++){
        MPI_Isend(&row, 1, MPI_INT, i,DRAW_TAG, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &status);
        row++;
        count++;        
     }
     do{
        MPI_Irecv(&rowData, height+1, MPI_INT, MPI_ANY_SOURCE, FB_TAG, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &status);  
        int slaveReturn;
        slaveReturn = status.MPI_SOURCE;
        count--;
        if(row < width){
           MPI_Isend(&row, 1, MPI_INT, slaveReturn, DRAW_TAG, MPI_COMM_WORLD, &req);
           MPI_Wait(&req, &status);
           row++;
           count++;        
        } 
        else{
           MPI_Isend(&row, 1, MPI_INT, slaveReturn, TER_TAG, MPI_COMM_WORLD, &req);
           MPI_Wait(&req, &status);            
        }
        int rowNum = rowData[height];
        if(xflag == 0){
           for(j=0; j<height; j++){
               XSetForeground (display, gc,  1024 * 1024 * (rowData[j] % 256));	
               XDrawPoint (display, window, gc, rowNum, j);            
           }
        }
        
     } while(count > 0);       
     if(xflag == 0){
	     XFlush(display);
     }       
     // Timing measurement  
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
  // Slave tasks
  else{
     MPI_Irecv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &req);  
     MPI_Wait(&req, &status);
     while(status.MPI_TAG == DRAW_TAG){
        rowData[height] = row;
        for(j=0; j<height; j++) {
           gettimeofday(&thtv1[rank], NULL);
           thtimeStart[rank] = thtv1[rank].tv_sec * 1000000 + thtv1[rank].tv_usec;          
           z.real = 0.0;
		       z.imag = 0.0;
	      	 c.real = xleft + (double)row * (xrange/(double)width);
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
           rowData[j] = repeats;   
           rowCnt[rank]++;
           gettimeofday(&thtv2[rank], NULL);
           thtimeEnd[rank] = thtv2[rank].tv_sec * 1000000 + thtv2[rank].tv_usec;
           thgap[rank] += (thtimeEnd[rank]-thtimeStart[rank]) / CLOCKS_PER_SEC;                      
        }               
        MPI_Isend(&rowData, height+1, MPI_INT, 0, FB_TAG, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &status); 
        MPI_Irecv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &status);                          
     }
  }
  printf("Rank %d computed %d points consuming %1f seconds\n", rank, rowCnt[rank], thgap[rank]);
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();  

	sleep(10);
	return 0;
}
