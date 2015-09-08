#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

typedef struct complexType
{
	double real, imag;
} Compl;

int xflag = 1;

int rowData[1400][1400];
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
  int NUM_THREADS = atoi(argv[1]); 
  double xrange = xright - xleft;
  double yrange = yright - yleft;  
	/* set window position */
	int x = 0;
	int y = 0;
  int NUM_PROCS = omp_get_num_procs(); 
  struct timeval tv1, tv2;  
  double timeStart, timeEnd;
  gettimeofday(&tv1, NULL);
  timeStart = tv1.tv_sec * 1000000 + tv1.tv_usec;
  
	GC gc; 
  printf("X Window is %sd\n", argv[8]); 
  xflag = strcmp(argv[8], "enable");
  omp_set_num_threads(NUM_THREADS);
  omp_set_nested(1);
  printf("Total %d threads functioning among %d processors\n", NUM_THREADS, NUM_PROCS); 
  int nest = omp_get_nested();
  printf("omp_nested is set to %d\n", nest);
  
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
        
  // Parameters
  Compl z, c;
  int repeats;
  double temp, lengthsq; 
  int i, j;
  int fakewidth; 
  int task;
  int localw = 0;
  int nlocal = 100;  
  int tid;
  int width1;
  int judge=0;
  int cnt;
  
  for(cnt=0; cnt<NUM_THREADS; cnt++){
      rowCnt[cnt] = 0;
      thgap[cnt] = 0;
  }  
                
  #pragma omp parallel num_threads(NUM_THREADS) private(tid, temp, lengthsq, z, c, repeats, i, j)
  { 
     tid = omp_get_thread_num();   
     printf("Thread %d!!\n", tid);
     #pragma omp for schedule(static, 1)      
	   for(i=0; i<width; i++) {
		   for(j=0; j<height; j++) {
         gettimeofday(&thtv1[tid], NULL);
         thtimeStart[tid] = thtv1[tid].tv_sec * 1000000 + thtv1[tid].tv_usec;          
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
           rowCnt[tid]++;
           gettimeofday(&thtv2[tid], NULL);
           thtimeEnd[tid] = thtv2[tid].tv_sec * 1000000 + thtv2[tid].tv_usec;
           thgap[tid] += (thtimeEnd[tid]-thtimeStart[tid]) / CLOCKS_PER_SEC;           
         }
		   }    
     }
     #pragma omp barrier     
  }
  // Draw the graph 
  if(xflag == 0){  
     for(i=0; i<width; i++) {
	      for(j=0; j<height; j++) {
           XSetForeground (display, gc,  1024 * 1024 * (rowData[i][j] % 256));		
	         XDrawPoint (display, window, gc, i, j);
        }        
     }
	   XFlush(display);
  }   
  
  gettimeofday(&tv2, NULL);
  timeEnd = tv2.tv_sec * 1000000 + tv2.tv_usec;
	double gap = (timeEnd-timeStart) / CLOCKS_PER_SEC;  
  printf("OOOOOOO Graph Drawing Done OOOOOO\n");
  printf("Threads : %d\n", NUM_THREADS);         
  printf("Running time : %lf\n", gap);
  printf("\n");
  for(cnt=0; cnt<NUM_THREADS; cnt++){
      printf("Thread %d computed %d points consuming %1f seconds\n", cnt, rowCnt[cnt], thgap[cnt]);
  }
  printf("\n");  
  FILE *outFile;
  outFile = fopen(argv[9], "a");
  fprintf(outFile, "Threads : %d \n", NUM_THREADS);      
  fprintf(outFile, "Running time : %lf\n\n", gap);
  fclose(outFile);          
	sleep(5);
	return 0;
}
