#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
// String function for output log file
char *string_concat(char *str1, char *str2) {  
   int length=strlen(str1)+strlen(str2)+1;   
   char *result = (char*)malloc(sizeof(char) * length);    
   strcpy(result, str1);    
   strcat(result, str2);   
   return result;  
}
char Indication[15][20] = {"NUM_THREADS", "MAX_X", "MAX_Y", "WIDTH", "HEIGHT", "NUM_FISH", "NUM_SHARK", 
"SIM_STEPS", "STEPS_OF_YEAR", "SHARK_STARVING", "SHARK_BREED_AGE", "FISH_BREED_AGE", "SIM_DELAY", "NUM_FISHERMAN", "AutoPlace"}; 

enum WatorParamter{water, fish, shark, fisherman};
enum WatorParamter1{fish_child=-2, shark_child=-1};

typedef struct NextXY{
   int x;  
   int y;     
} NextXY;
typedef struct WatorParam{
   int NUM_FISH;  
   int NUM_SHARK;
   int NUM_FISHERMAN;
   int STEPS;
   int STEPS_OF_YEAR;
   int FISH_BREED_AGE;
   int SHARK_BREED_AGE; 
   int SHARK_STARVING;  
   int fishStart, fishEnd, sharkStart, sharkEnd, fishStartTmp, sharkStartTmp;
   int rowStart, rowEnd; 
   int fishPrevDeadYTx[1000]; // Tx of the dead fish
   int fishPrevDeadXTx[1000]; // Tx of the dead fish   
   int fishPrevDeadYRx[1000]; // Rx of the dead fish
   int fishPrevDeadXRx[1000]; // Rx of the dead fish
   int logStart;
   int logSharkDead[1000];
   int logFishDead[1000];
   int logFishBreed[1000];
   int logSharkBreed[1000];
   int logSharkEat[1000];    
   int fishermanGotID;  
} WatorParam;
typedef struct Creature_Fish {
   int x;
   int y;
   int age;   
   int id;
} Creature_Fish;
typedef struct Creature_Shark {
   int x;
   int y;
   int id;
   int age;   
   int starve;
} Creature_Shark;
typedef struct Creature_Fisherman {
   int y;
   int id;   
   int score;
   double Temp;
} Creature_Fisherman;

int Map[1000][1000];
int MapFake[1000][1000];
int fishmap[1000][1000];
int fishmapTmp[1000][1000];
int sharkmap[1000][1000];
int sharkmapTmp[1000][1000];
int MapTmp[1000][1000];
int MapTmpTmp[1000][1000];
int MapRollBack[1000][1000];
int fishOldest[1000];
int sharkOlderst[1000];
double fishermanTemp[1000];
int fishermanMap[1000];
int fRollBack[1000][1000];
int sRollBack[1000][1000];
int fmRollBack[1000][1000];
int smRollBack[1000][1000];
int numFishRB;
int numSharkRB;

// Aqua parameters
WatorParam watorParam;
Creature_Fish Fish[1000000];
Creature_Fish FishTmp[1000000]; // For the reordered version
Creature_Shark Shark[1000000];
Creature_Shark SharkTmp[1000000]; // For the reordered version
Creature_Fisherman Fisherman[1000];
Creature_Fish FishRB[1000000];
Creature_Shark SharkRB[1000000];
// Initialize the Wator map
void wator_init(int MAX_X, int MAX_Y, WatorParam watorParam){
   //printf("Step %d of Year 0\n", watorParam.STEPS);
   int i, j;
   for(i=0; i<MAX_X; i++){
      for(j=0; j<MAX_Y; j++){
         Map[i][j] = water;  
         MapFake[i][j] = water; 
         MapTmp[i][j] = water;
         fishmap[i][j] = -5;
         sharkmap[i][j] = -5;
      }
   }       
   // Fish
   int fishx, fishy;
   for(i=0; i<watorParam.NUM_FISH; i++){
      fishx = rand()%MAX_X;
      fishy = rand()%MAX_Y;
      while(MapFake[fishx][fishy] == -1){
         fishx = rand()%MAX_X;
         fishy = rand()%MAX_Y;         
      }
      Fish[i].x = fishx;
      Fish[i].y = fishy;
      Map[fishx][fishy] = fish;
      MapFake[fishx][fishy] = -1;
      MapTmp[fishx][fishy] = 1;      
      Fish[i].age = 0;
      Fish[i].id = i;
      fishmap[fishx][fishy] = i;
      //printf("Fish id = %d at [%d][%d]\n", Fish[i].id, Fish[i].x, Fish[i].y);      
   }
   // Shark
   int sharkx, sharky;
   for(i=0; i<watorParam.NUM_SHARK; i++){
      sharkx = rand()%MAX_X;
      sharky = rand()%MAX_Y;
      while(MapFake[sharkx][sharky] == -1){
         sharkx = rand()%MAX_X;
         sharky = rand()%MAX_Y;         
      }
      Shark[i].x = sharkx;
      Shark[i].y = sharky;
      Map[sharkx][sharky] = shark;
      MapFake[sharkx][sharky] = -1;
      MapTmp[sharkx][sharky] = 1;
      Shark[i].age = 0;
      Shark[i].id = i;
      Shark[i].starve = watorParam.SHARK_STARVING;
      sharkmap[sharkx][sharky] = i;
      //printf("Shark id = %d at [%d][%d]\n", Shark[i].id, Shark[i].x, Shark[i].y);
   }
   int fishermany;
   for(i=0; i<watorParam.NUM_FISHERMAN; i++){
      fishermany = rand()%MAX_Y;
      while(MapFake[MAX_X][fishermany] == -1){
         fishermany = rand()%MAX_Y;         
      }
      Fisherman[i].y = fishermany;
      fishermanMap[fishermany] = fisherman;
      MapFake[MAX_X][fishermany] = -1;
      Fisherman[i].score = 0;
      Fisherman[i].id = i;
      Fisherman[i].Temp = 36.0;
   } 
   watorParam.logSharkDead[0] = 0;
   watorParam.logFishDead[0] = 0;
   watorParam.logFishBreed[0] = 0;
   watorParam.logSharkBreed[0] = 0;
   watorParam.logSharkEat[0] = 0;           
   for(i=1; i<1000; i++){
      watorParam.logSharkDead[i] = -5;
      watorParam.logFishDead[i] = -5;
      watorParam.logFishBreed[i] = -5;
      watorParam.logSharkBreed[i] = -5;
      watorParam.logSharkEat[i] = -5;      
   }
}
void wator_init_Ind(int MAX_X, int MAX_Y, WatorParam watorParam){
   int i, j;
   for(i=0; i<MAX_X; i++){
      for(j=0; j<MAX_Y; j++){
         Map[i][j] = water;  
         MapFake[i][j] = water; 
         MapTmp[i][j] = water;
         fishmap[i][j] = -5;
         sharkmap[i][j] = -5;
      }
   }       
   // Fish
   int fishx, fishy;
   for(i=0; i<watorParam.NUM_FISH; i++){
      Map[Fish[i].x][Fish[i].y] = fish;
      MapFake[Fish[i].x][Fish[i].y] = -1;
      MapTmp[Fish[i].x][Fish[i].y] = 1;      
      Fish[i].age = 0;
      Fish[i].id = i;
      fishmap[Fish[i].x][Fish[i].y] = i;
      //printf("Fish id = %d at [%d][%d]\n", Fish[i].id, Fish[i].x, Fish[i].y);             
   }
   // Shark
   int sharkx, sharky;
   for(i=0; i<watorParam.NUM_SHARK; i++){
      Map[Shark[i].x][Shark[i].y] = shark;
      MapFake[Shark[i].x][Shark[i].y] = -1;
      MapTmp[Shark[i].x][Shark[i].y] = 1;
      Shark[i].age = 0;
      Shark[i].id = i;
      Shark[i].starve = watorParam.SHARK_STARVING;
      sharkmap[Shark[i].x][Shark[i].y] = i;
   }
   int fishermany;
   for(i=0; i<watorParam.NUM_FISHERMAN; i++){
      fishermany = rand()%MAX_Y;
      while(MapFake[MAX_X][fishermany] == -1){
         fishermany = rand()%MAX_Y;         
      }
      Fisherman[i].y = fishermany;
      fishermanMap[fishermany] = fisherman;
      MapFake[MAX_X][fishermany] = -1;
      Fisherman[i].score = 0;
      Fisherman[i].id = i;
      Fisherman[i].Temp = 36.0;
   } 
   watorParam.logSharkDead[0] = 0;
   watorParam.logFishDead[0] = 0;
   watorParam.logFishBreed[0] = 0;
   watorParam.logSharkBreed[0] = 0;
   watorParam.logSharkEat[0] = 0;           
   for(i=1; i<1000; i++){
      watorParam.logSharkDead[i] = -5;
      watorParam.logFishDead[i] = -5;
      watorParam.logFishBreed[i] = -5;
      watorParam.logSharkBreed[i] = -5;
      watorParam.logSharkEat[i] = -5;      
   }
}
void print_wator(int MAX_X, int MAX_Y, WatorParam watorParam){
   int i, j;
   for(i=0; i<MAX_X; i++){
      for(j=0; j<MAX_Y; j++){
         printf("%d ", Map[i][j]); 
      }
      printf("\n");
   }       
}
int accu(int *array, int size){
   int i=0, j=0;
   for(i=0; i<size; i++){
      if(array[i] == 1){
         j++;
      }
   }
   return j;
}
// 0=(-1,+1) 1=(0,+1) 2=(+1,+1) 3=(+1,0) 4=(+1,-1) 5=(0,-1) 6=(-1,-1) 7=(-1,0)
NextXY nextXY(int direction, int currentx, int currenty, int creature, int MAX_X, int MAX_Y){
   NextXY next;
   int i, j, moveCnt=0;
   int nextX[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
   int nextY[8] = {1, 1, 1, 0, -1, -1, -1, 0};   
   int drCnt[8] = {0, 0, 0, 0, 0, 0, 0, 0};
   next.x = currentx + nextX[direction];
   next.y = currenty + nextY[direction];
   if(creature == fish){
   while((next.x >= MAX_X || next.x < 0 || next.y >= MAX_Y || next.y < 0 || Map[next.x][next.y] == creature) && moveCnt < 7 && Map[next.x][next.y] != shark){
      direction = rand()%8;
      next.x = currentx + nextX[direction];
      next.y = currenty + nextY[direction]; 
      drCnt[direction] = 1; 
      moveCnt = accu(drCnt, 8);             
   }    
   if(moveCnt >= 7){
      //printf("Creature Stucked!!\n");
      next.x = currentx;
      next.y = currenty;
   }
   if(Map[next.x][next.y] == creature){
      next.x = currentx;
      next.y = currenty;   
   }
   }
   else if(creature == shark){
   while((next.x >= MAX_X || next.x < 0 || next.y >= MAX_Y || next.y < 0 || Map[next.x][next.y] == creature) && moveCnt < 7){
      direction = rand()%8;
      next.x = currentx + nextX[direction];
      next.y = currenty + nextY[direction]; 
      drCnt[direction] = 1; 
      moveCnt = accu(drCnt, 8);             
   }    
   if(moveCnt >= 7){
      //printf("Creature Stucked!!\n");
      next.x = currentx;
      next.y = currenty;
   }
   if(Map[next.x][next.y] == creature){
      next.x = currentx;
      next.y = currenty;   
   }   
   }
   return next;                 
}
// 0=(-1,+1) 1=(0,+1) 2=(+1,+1) 3=(+1,0) 4=(+1,-1) 5=(0,-1) 6=(-1,-1) 7=(-1,0)
NextXY fnextXY(int direction, int currenty, int MAX_X, int MAX_Y){
   NextXY next;
   int i, j, moveCnt=0;
   int nextY[2] = {-1, 1};
   next.y = currenty + nextY[direction];
   while((next.y >= MAX_Y || next.y < 0 || fishermanMap[next.y] == fisherman) && moveCnt < 1){
      direction = rand()%2;
      next.y = currenty + nextY[direction]; 
      moveCnt++;              
   }    
   if(moveCnt >= 2){
      //printf("Creature Stucked!!\n");
      next.y = currenty;
   }
   return next;                 
}
// Hunting / Dying function
WatorParam HUNT(int huntx, int hunty, int preyType, WatorParam watorParam){
   int i, j, m, n, preyOrder, judge=0;
   if(preyType == fish){
      for(i=watorParam.fishStartTmp; i<=watorParam.fishStartTmp+watorParam.NUM_FISH-1; i++){
         if(Fish[i].x == huntx && Fish[i].y == hunty){
            preyOrder = i;
            //printf("Fish id %d died!\n", Fish[i].id);
            watorParam.logFishDead[0]++;
            watorParam.logFishDead[watorParam.logFishDead[0]] = Fish[i].id;
            watorParam.fishermanGotID = Fish[i].id;            
            judge = 1;
         }
      }
      if(judge==1){
         for(i=preyOrder+1; i<=watorParam.fishStartTmp+watorParam.NUM_FISH-1; i++){
            Fish[i-1] = Fish[i];
         }      
         watorParam.NUM_FISH--;
         judge = 0;
      }      
   }
   else if(preyType == shark){
      for(i=watorParam.sharkStartTmp; i<=watorParam.sharkStartTmp+watorParam.NUM_SHARK-1; i++){
         if(Shark[i].x == huntx && Shark[i].y == hunty){
            preyOrder = i;
            //printf("Shark id %d died!\n", Shark[i].id);
            watorParam.logSharkDead[0]++;
            watorParam.logSharkDead[watorParam.logSharkDead[0]] = Shark[i].id; 
            watorParam.fishermanGotID = Shark[i].id;     
            //printf("how many? %d\n", watorParam.logSharkDead[0]);                   
            judge = 1;
         }
      }
      if(judge==1){
         for(i=preyOrder+1; i<=watorParam.sharkStartTmp+watorParam.NUM_SHARK-1; i++){
            Shark[i-1] = Shark[i];
         }      
         watorParam.NUM_SHARK--;
         judge = 0;
      }      
   }   
   return watorParam;
}
// Breeding Function
WatorParam BREED(int breedx, int breedy, int breedType, int creatureNum, WatorParam watorParam){
   int i, j, m, n, preyOrder;
   if(breedType == shark){
      Shark[creatureNum].age = 0;
      Shark[creatureNum].starve = watorParam.SHARK_STARVING;
      Shark[creatureNum].x = breedx;
      Shark[creatureNum].y = breedy;       
      Shark[creatureNum].id = shark_child;      
      //printf("A new born shark %d id=%d with age %d at [%d][%d]!!\n", creatureNum, Shark[creatureNum].id, Shark[creatureNum].age, breedx, breedy);
   }
   else if(breedType == fish){
      Fish[creatureNum].age = 0;
      Fish[creatureNum].id = fish_child;      
      Fish[creatureNum].x = breedx;
      Fish[creatureNum].y = breedy;       
      //printf(" A new born fish %d id=%d with age %d at [%d][%d]!!\n", creatureNum, Fish[creatureNum].id, Fish[creatureNum].age, breedx, breedy);            
   } 
   return watorParam; 
}
// Simulation of Wator, the stronger the creatures is, the earlier it shall move
WatorParam WatorFunc(int MAX_X, int MAX_Y, WatorParam watorParam){
   // Creatures moving
   int i, j, direction, m, judgeFishHere=0;
   int sharkBreedTimes = 0, fishBreedTimes = 0;
   int sharkDeadTimes = 0, fishDeadTimes = 0;
   int SharkDead[1000000];
   int fishPrevDeadCnt=0;
   NextXY next;
   // general setting
   int year = (watorParam.STEPS - (watorParam.STEPS%watorParam.STEPS_OF_YEAR)) / watorParam.STEPS_OF_YEAR;
   //printf("Step %d of Year %d\n", watorParam.STEPS, year);
   //printf("Current total %d fishes %d sharks\n", watorParam.NUM_FISH, watorParam.NUM_SHARK);  
   if (watorParam.STEPS % watorParam.STEPS_OF_YEAR == 0){
      for(i=watorParam.sharkStart; i<=watorParam.sharkEnd; i++){
         Shark[i].age++;
         //printf("Shark %d's age = %d\n", Shark[i].id, Shark[i].age);
      }
      for(i=watorParam.fishStart; i<=watorParam.fishEnd; i++){
         Fish[i].age++;
         //printf("Fish %d's age = %d\n", Fish[i].id, Fish[i].age);         
      }      
   }
   //printf("!!!!!!!!!!!! fish = %d\n", watorParam.NUM_FISH);
   //Shark Move
   watorParam.fishPrevDeadYTx[0] = 0;
   for(i=watorParam.sharkStart; i<=watorParam.sharkEnd; i++){
      //printf("i = %d\n", i);
      Shark[i].starve--;   
      judgeFishHere = 0;
      //printf("//Shark id=%d starve = %d\n", Shark[i].id, Shark[i].starve);
      if(Shark[i].starve > 0){
         direction = rand()%8;
         next = nextXY(direction, Shark[i].x, Shark[i].y, shark, MAX_X, MAX_Y);
         if(Map[next.x][next.y] == fish){          
            //printf("Shark %d ate the prey!\n", Shark[i].id);  
            watorParam.logSharkEat[0]++;
            watorParam.logSharkEat[watorParam.logSharkEat[0]] = Shark[i].id;                 
            watorParam = HUNT(next.x, next.y, fish, watorParam);
            for(m=watorParam.fishStartTmp; m<=watorParam.fishStartTmp+watorParam.NUM_FISH-1; m++){
               if(Fish[m].x==next.x && Fish[m].y==next.y){
                  judgeFishHere=1;
               }
            }
            if(judgeFishHere==1){
               fishDeadTimes++;            
            }
            else if(judgeFishHere==0){
               //printf("A fish in the previous rank has been killed!\n");
               fishPrevDeadCnt++; // Count the order of the dead fish in the previous rank
               watorParam.fishPrevDeadYTx[0]++; // Count one more dead fish in the previous rank
               watorParam.fishPrevDeadYTx[fishPrevDeadCnt] = next.y;
               watorParam.fishPrevDeadXTx[fishPrevDeadCnt] = next.x;               
            }
            Shark[i].starve = watorParam.SHARK_STARVING;
            //printf("******Shark %d regains starve = %d!\n", Shark[i].id, Shark[i].starve);
         }                 
         Map[next.x][next.y] = shark;         
         // conditioned breeding
         if(year >= 1){ 
            if(Shark[i].age >= watorParam.SHARK_BREED_AGE && watorParam.STEPS%watorParam.STEPS_OF_YEAR ==0 && (next.x!=Shark[i].x || next.y!=Shark[i].y)){
               //printf("Shark %d breeds! \n", Shark[i].id); 
               watorParam.logSharkBreed[0]++;
               watorParam.logSharkBreed[watorParam.logSharkBreed[0]] = Shark[i].id;                        
               sharkBreedTimes++;
               watorParam = BREED(Shark[i].x, Shark[i].y, shark, watorParam.sharkStartTmp+watorParam.NUM_SHARK+sharkBreedTimes-1, watorParam);
               Map[Shark[i].x][Shark[i].y] = shark;
               MapTmp[Shark[i].x][Shark[i].y] = 1;  
               sharkmap[Shark[i].x][Shark[i].y] = watorParam.sharkStartTmp+watorParam.NUM_SHARK+sharkBreedTimes-1;            
            }
            else{
               Map[Shark[i].x][Shark[i].y] = water;      
            }          
         }
         else{
            Map[Shark[i].x][Shark[i].y] = water;
         }        
         int xtmp, ytmp;
         xtmp = Shark[i].x;
         ytmp = Shark[i].y;
         Shark[i].x = next.x;
         Shark[i].y = next.y;
         sharkmap[Shark[i].x][Shark[i].y] = i; 
         sharkmap[xtmp][ytmp] = -5;         
         //printf("Shark %d id=%d at =[%d][%d] next = [%d][%d]\n", i, Shark[i].id, xtmp, ytmp, next.x, next.y);
         MapTmp[next.x][next.y] = 1;
      }
      else if(Shark[i].starve == 0){     
         //printf("Shark %d id=%d died of starving at [%d][%d]!\n", i, Shark[i].id, Shark[i].x, Shark[i].y); 
         watorParam.logSharkDead[0]++;
         watorParam.logSharkDead[watorParam.logSharkDead[0]] = Shark[i].id;         
         watorParam = HUNT(Shark[i].x, Shark[i].y, shark, watorParam);              
         Map[Shark[i].x][Shark[i].y] = water;
      }
   }
   // Hunting and being hunt parameters adjustment
   //watorParam.sharkEnd -= sharkDeadTimes;
   watorParam.fishEnd -= fishDeadTimes;
   
   // Fish Move
   for(i=watorParam.fishStart; i<=watorParam.fishEnd; i++){   
      direction = rand()%8;
      next = nextXY(direction, Fish[i].x, Fish[i].y, fish, MAX_X, MAX_Y);
      if(Map[next.x][next.y] == shark && (next.x != Fish[i].x || next.y != Fish[i].y)){
         direction = rand()%8;      
         next = nextXY(direction, Fish[i].x, Fish[i].y, fish, MAX_X, MAX_Y);         
      }
      Map[next.x][next.y] = fish;       
      // conditioned breeding
      if(year >= 1){ 
         if(Fish[i].age >= watorParam.FISH_BREED_AGE && watorParam.STEPS%watorParam.STEPS_OF_YEAR ==0 && (next.x!=Fish[i].x || next.y!=Fish[i].y)){
            //printf("Fish %d id=%d breeds! among %d to %d\n", i, Fish[i].id, watorParam.fishStart, watorParam.fishEnd);        
            watorParam.logFishBreed[0]++;
            watorParam.logFishBreed[watorParam.logFishBreed[0]] = Fish[i].id;
            fishBreedTimes++;
            watorParam = BREED(Fish[i].x, Fish[i].y, fish, watorParam.fishStartTmp+watorParam.NUM_FISH+fishBreedTimes-1, watorParam);
            Map[Fish[i].x][Fish[i].y] = fish;
            MapTmp[Fish[i].x][Fish[i].y] = 1;
            fishmap[Fish[i].x][Fish[i].y] = watorParam.fishStartTmp+watorParam.NUM_FISH+fishBreedTimes-1;
         }
         else{
            Map[Fish[i].x][Fish[i].y] = water;      
         }          
      }
      else{
         Map[Fish[i].x][Fish[i].y] = water;
      }     
      int xtmp1, ytmp1;
      xtmp1 = Fish[i].x;
      ytmp1 = Fish[i].y;      
      Fish[i].x = next.x;
      Fish[i].y = next.y; 
      MapTmp[next.x][next.y] = 1;
      fishmap[Fish[i].x][Fish[i].y] = i;  
      fishmap[xtmp1][ytmp1] = -5;      
      //printf("Fish %d id=%d at [%d][%d] next = [%d][%d] of %d fishes\n", i, Fish[i].id, xtmp1, ytmp1, next.x, next.y, watorParam.NUM_FISH);        
   } 
   // Modify the total number of creatures
   watorParam.NUM_SHARK += sharkBreedTimes;
   watorParam.NUM_FISH += fishBreedTimes;   
   return watorParam;
}

// Main
int main(int argc, char *argv[])
{
   // Wator local parameters
   int MAX_X, MAX_Y, WIDTH, HEIGHT, NUM_FISH, NUM_SHARK, SIM_STEPS;
   int STEPS_OF_YEAR, SHARK_STARVING, SHARK_BREED_AGE, FISH_BREED_AGE;
   int SIM_DELAY, RANDOM_INIT_PLACE;
   int NUM_THREADS, NUM_TASKS, NUM_FISHERMAN;
   // Iterators
   int i, j, k, c, m, n, x;
   int paramReg;
   int paramTmp[50];
   int basicParamEnd;
   int compareRandom;
   char tmpStr[100];
   double timeStart,timeEnd; // Timer
   timeStart = clock();   
   // Input file formatting
   FILE *inFile; 
   inFile = fopen(argv[1], "r");
   if (!inFile){
      return 1;
   }
   i = 0;
   int fishesx, sharksx, fishesy, sharksy, randomplace;
   while ((c = fgetc(inFile)) != EOF){
      if(i%2 == 0){
         fscanf(inFile, "%s", &tmpStr);
         compareRandom = strcmp(tmpStr, "automaticly-random-the-initial-place-of-creatures(YES:1-NO:0)");
      }
      else if (i%2 == 1){
         fscanf(inFile, "%d", &paramReg);
         if (compareRandom == 0)
         {
            if(paramReg == 1){
               basicParamEnd = (i-1)/2;
               printf("Random position enabled... \n"); 
               randomplace = 1;              
               c = EOF;
            } 
            else if(paramReg == 0){
               basicParamEnd = (i-1)/2;
               printf("Random position disabled... \n");
               randomplace = 0;
               fscanf(inFile, "%s", &tmpStr);
               //printf("%s NUM_FISH = %d\n", tmpStr, paramTmp[5]); 
               for(j=0; j<paramTmp[5]; j++){
                  fscanf(inFile, "%d", &fishesx);
                  fscanf(inFile, "%d", &fishesy);
                  //printf("x = %d y = %d\n", fishesx, fishesy);
                  Fish[j].x = fishesx;
                  Fish[j].y = fishesy;                                   
               } 
               fscanf(inFile, "%s", &tmpStr); 
               //printf("%s NUM_SHARK = %d\n", tmpStr, paramTmp[6]);                       
               for(j=0; j<paramTmp[6]; j++){
                  fscanf(inFile, "%d", &sharksx);
                  fscanf(inFile, "%d", &sharksy);
                  //printf("x = %d y = %d\n", sharksx, sharksy);
                  Shark[j].x = sharksx;
                  Shark[j].y = sharksy;                                   
               }
               c = EOF;
            }            
         }      
         paramTmp[(i-1)/2] = paramReg;         
      }
      i++;
   }
   fclose(inFile); 
   
   // Print out the basic parameters
   NUM_THREADS = paramTmp[0];
   MAX_X = paramTmp[1];   
   MAX_Y = paramTmp[2];  
   WIDTH = paramTmp[3];
   HEIGHT = paramTmp[4];   
   watorParam.NUM_FISH = paramTmp[5];  
   watorParam.NUM_SHARK = paramTmp[6];  
   watorParam.STEPS_OF_YEAR = paramTmp[8]; 
   watorParam.SHARK_STARVING = paramTmp[9];    
   watorParam.SHARK_BREED_AGE = paramTmp[10]; 
   watorParam.FISH_BREED_AGE = paramTmp[11];    
   SIM_STEPS = paramTmp[7];  
   SIM_DELAY = paramTmp[12];
   if(paramTmp[13] > MAX_Y){
      watorParam.NUM_FISHERMAN = MAX_Y;    
   }
   else{
      watorParam.NUM_FISHERMAN = paramTmp[13];     
   }  
   // MPI Parameters
   int rank, rc, len;
   MPI_Status status;
   // Redistributed rows for slaves
   int rowLocal[8];
   int rowStart[8], fishStart[8], sharkStart[8];
   int rowStartRB[8], fishStartRB[8], sharkStartRB[8];   
   int rowEnd[8], fishEnd[8], sharkEnd[8];
   int rowEndRB[8], fishEndRB[8], sharkEndRB[8];
   int fEndTmp, sEndTmp;
   
   // MPI Process
   char hostname[MPI_MAX_PROCESSOR_NAME];
   rc = MPI_Init(&argc, &argv);
   if (rc != MPI_SUCCESS){
     return -1;
   }    
   MPI_Comm_size(MPI_COMM_WORLD, &NUM_TASKS);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Get_processor_name(hostname, &len);
   MPI_Barrier(MPI_COMM_WORLD);    
   
   /*** master ***********************************************************************************************************************************/   
   if (rank == 0){
      FILE *outFile; 
      outFile = fopen(argv[2], "w");      
      for(i=0; i<basicParamEnd; i++){
         printf("%s = %d\n", Indication[i], paramTmp[i]);
      }
      //fprintf(outFile, watorParam.log);
      printf("Map Rule:\n");
      printf("%d: Water\n", water);
      printf("%d: Fish\n", fish);
      printf("%d: Shark\n", shark);
      printf("Total %d slaves\n\n", NUM_TASKS-1);
      if(randomplace == 1){
         wator_init(MAX_X, MAX_Y, watorParam);  
      }   
      else if(randomplace == 0){
         wator_init_Ind(MAX_X, MAX_Y, watorParam);  
      }       
      // Xwindow
      Display *display;
      Window window;      //initialization for a window
      int screen;         //which screen 
      /* open connection with the server */ 
      display = XOpenDisplay(NULL);
      if(display == NULL) {
   	     fprintf(stderr, "cannot open display\n");
         return -1;
      }
      screen = DefaultScreen(display);
      /* set window size */
      int width = WIDTH * MAX_X + 200;
      int height = HEIGHT * (MAX_Y+1); 
      /* set window position */
      int x = 0;
      int y = 0;
      /* border width in pixels */
      int border_width = 0;
      /* create window */
      window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width,
		  BlackPixel(display, screen), WhitePixel(display, screen));	
      /* create graph */
      GC gc;
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
      // Initialization
      watorParam.STEPS = 0; 
      print_wator(MAX_X, MAX_Y, watorParam);
      // Draw the graph
      int p=0,q=0;
      for(m=0; m<MAX_X * WIDTH; m+=WIDTH){
         q=0;
         for(n=0; n<MAX_Y * HEIGHT; n+=HEIGHT){
            XSetForeground (display, gc, Map[p][q] * 0xAABBCC);	
            XFillRectangle (display, window, gc, n, m, WIDTH, HEIGHT); 
            q++;
         }
         p++;
      }              
      // Print out the fisherman
      for(i=0; i<MAX_Y; i++){
         printf("%d ", fishermanMap[i]);   
      } 
      q=0;
      for(n=0; n<MAX_Y * HEIGHT; n+=HEIGHT){
         XSetForeground (display, gc, fishermanMap[q] * 0xAABBCC);		
         XFillRectangle (display, window, gc, n, MAX_X*WIDTH, WIDTH, HEIGHT);  
         q++;  
      }  
      XSetForeground (display, gc, 10 * 0xAABBCC);     
      XFillRectangle (display, window, gc, MAX_X*WIDTH, 0, MAX_X*WIDTH+200, (MAX_Y+1)*WIDTH);
      XSetForeground (display, gc, 0 * 0xAABBCC);      
      XFlush(display);          
      printf("\n");      
      // Do the Wator
      int steps;
      int judgeRollBack;
      judgeRollBack = 0;      
      for(steps=0; steps<SIM_STEPS; steps++){
         usleep(SIM_DELAY*1000);
         for(m=1; m<NUM_TASKS; m++){
            MPI_Send(&steps, 1, MPI_INT, m, 0, MPI_COMM_WORLD);
            //printf("sending to rank %d\n", m);
         }          
         watorParam.STEPS++;
         // Reorder function
         int totalCreature = 0;
         int slaves, slavesCnt;
         slaves = NUM_TASKS - 1;
         for(j=0; j<MAX_X; j++){
            for(k=0; k<MAX_Y; k++){
               MapTmp[j][k] = water;  
               MapRollBack[j][k] = Map[j][k]; 
               smRollBack[j][k] = sharkmap[j][k];
               fmRollBack[j][k] = fishmap[j][k];     
            }
         }
         numFishRB = watorParam.NUM_FISH;
         numSharkRB = watorParam.NUM_SHARK; 
         for(m=0; m<watorParam.NUM_FISH; m++){
            //printf("---Fish[%d] id=%d\n", m, Fish[m].id);
            FishRB[m].x = Fish[m].x;
            FishRB[m].y = Fish[m].y;   
            FishRB[m].id = Fish[m].id;  
            FishRB[m].id = Fish[m].age;                    
         }        
         for(m=0; m<watorParam.NUM_SHARK; m++){
            SharkRB[m].x = Shark[m].x;
            SharkRB[m].y = Shark[m].y;   
            SharkRB[m].id = Shark[m].id;  
            SharkRB[m].id = Shark[m].age;   
            SharkRB[m].starve = Shark[m].starve;                  
         }         
         for(j=0; j<MAX_X; j++){
            for(k=0; k<MAX_Y; k++){
               if(Map[j][k] == fish || Map[j][k] == shark){
                  MapTmp[j][k] = 1;
                  totalCreature++;   
               } 
            }
         }
         printf("Total %d Creatures\n", totalCreature);  
         for(m=0; m<watorParam.NUM_FISH; m++){
            //printf("XXXXXX Fish %d id = %d\n", m, Fish[m].id);
         }         
      //if(judgeRollBack==0){
         int sharkmapCnt=0, fishmapCnt=0;   
         // Calculate the evenly distributed possible number of creatures
         slavesCnt = (totalCreature - (totalCreature%slaves)) / slaves;
         if(totalCreature%slaves != 0){
            int rowAdd = totalCreature%slaves;
            for(j=0; j<slaves; j++){
               if(rowAdd>0){
                  rowLocal[j] = slavesCnt + 1;
                  rowAdd--;
               }           
               else{
                  rowLocal[j] = slavesCnt;
               }
               //printf("slave %d has %d creatures\n", j+1, rowLocal[j]); 
            }         
         }
         else if(totalCreature%slaves == 0){
            for(j=0; j<slaves; j++){
               rowLocal[j] = slavesCnt;
               //printf("slave %d has %d creatures\n", j+1, rowLocal[j]);
            }
         }
         // Reorder the creatures                   
         int newFishOrder=0, newSharkOrder=0; 
         for(j=0; j<MAX_X; j++){
            for(k=0; k<MAX_Y; k++){
               fishmap[j][k] = -5;
               sharkmap[j][k] = -5;               
            }   
         }
         int mapcnt=0;
         for(m=0; m<watorParam.NUM_FISH; m++){
            fishmap[Fish[m].x][Fish[m].y] = mapcnt;
            mapcnt++;
         }
         mapcnt = 0;
         for(m=0; m<watorParam.NUM_SHARK; m++){
            sharkmap[Shark[m].x][Shark[m].y] = mapcnt;
            mapcnt++;
         }                                
         newFishOrder = 0;
         newSharkOrder = 0;           
         //print_wator(MAX_X, MAX_Y, watorParam); 
         /*for(j=0; j<MAX_X; j++){
            for(k=0; k<MAX_Y; k++){
               printf("%d ", fishmap[j][k]);                
            }   
            printf("\n");
         }  */                             
         for(j=0; j<MAX_X; j++){
            for(k=0; k<MAX_Y; k++){
               if(Map[j][k] == fish){
                  FishTmp[newFishOrder] = Fish[fishmap[j][k]];
                  fishmap[j][k] = newFishOrder;
                  newFishOrder++;
               }   
               else if(Map[j][k] == shark){
                  SharkTmp[newSharkOrder] = Shark[sharkmap[j][k]];  
                  sharkmap[j][k] = newSharkOrder;                        
                  newSharkOrder++;   
               }                
            }   
         }                  
         for(m=0; m<watorParam.NUM_FISH; m++){
            Fish[m] = FishTmp[m];
            //printf("XXXXXX Fish %d id = %d\n", m, Fish[m].id);
         }  
         for(m=0; m<watorParam.NUM_SHARK; m++){
            Shark[m] = SharkTmp[m];
         }          
         // Redistribute the rows to slaves
         int rowTmp = 0, slaveOrder=0, rowStartTmp=0, rowEndTmp=0;
         int fishStartTmp=0, fishEndTmp=0, sharkStartTmp=0, sharkEndTmp=0;
         int fishLastX=0, fishLastY=0, sharkLastX=0, sharkLastY=0, fishCnt=0, sharkCnt=0;
         for(j=1; j<slaves; j++){
            rowStart[j]=MAX_X;
            rowEnd[j]=MAX_Y;
            fishStart[j]=watorParam.NUM_FISH+1;
            fishEnd[j]=watorParam.NUM_FISH+1;
            sharkStart[j]=watorParam.NUM_SHARK+1;
            sharkEnd[j]=watorParam.NUM_SHARK+1;                                                            
         }          
         for(j=0; j<MAX_X; j++){
            for(k=0; k<MAX_X; k++){
               if(MapTmp[j][k]==1){
                  rowTmp++;
                  if(slaveOrder < (slaves-1) && slaves > 1){
                     if(rowTmp == rowLocal[slaveOrder]){
                        rowStart[slaveOrder] = rowStartTmp;
                        rowEndTmp = j;
                        rowEnd[slaveOrder] = rowEndTmp;                       
                        // Sensing the fish or the shark with respect to their maps
                        for(m=rowStart[slaveOrder]; m<=rowEnd[slaveOrder]; m++){
                           for(n=0; n<MAX_Y; n++){
                              if(Map[m][n]==fish){
                                 fishLastX = m;
                                 fishLastY = n;
                                 fishCnt++;
                              }
                              else if(Map[m][n]==shark){
                                 sharkLastX = m;   
                                 sharkLastY = n;
                                 sharkCnt++;
                              }
                           }
                        }
                        fishEndTmp = fishmap[fishLastX][fishLastY];
                        sharkEndTmp = sharkmap[sharkLastX][sharkLastY];
                        fishStart[slaveOrder] = fishStartTmp;
                        sharkStart[slaveOrder] = sharkStartTmp;
                        fishEnd[slaveOrder] = fishEndTmp;
                        sharkEnd[slaveOrder] = sharkEndTmp;
                        fishStartTmp += fishCnt;
                        totalCreature -= fishCnt;
                        sharkStartTmp += sharkCnt;
                        totalCreature -= sharkCnt;               
                        printf("//Slave %d from row %d to %d", slaveOrder+1, rowStart[slaveOrder], rowEnd[slaveOrder]);
                        printf(" from fish %d to %d", fishStart[slaveOrder], fishEnd[slaveOrder]);
                        printf(" from shark %d to %d\n", sharkStart[slaveOrder], sharkEnd[slaveOrder]);
                        rowTmp = 0;
                        slaveOrder++;
                        sharkCnt = 0;
                        fishCnt = 0;
                        rowStartTmp = rowEndTmp + 1;
                     }
                  }
                  else if((slaveOrder = (slaves-1)) || slaves == 1){
                     if(slaves == 1 && rowTmp == rowLocal[slaveOrder]){
                        rowStart[slaveOrder] = rowStartTmp;
                        rowEnd[slaveOrder] = MAX_X - 1;
                        if(totalCreature>0){
                           fishEndTmp = watorParam.NUM_FISH-1;
                           sharkEndTmp = watorParam.NUM_SHARK-1;
                           fishStart[slaveOrder] = fishStartTmp;
                           sharkStart[slaveOrder] = sharkStartTmp;
                           fishEnd[slaveOrder] = fishEndTmp;
                           sharkEnd[slaveOrder] = sharkEndTmp;
                           fishStartTmp += fishEndTmp;
                           sharkStartTmp += sharkEndTmp;        
                        } 
                        else{
                           fishStart[slaveOrder] = watorParam.NUM_FISH+1;
                           fishEnd[slaveOrder] = watorParam.NUM_FISH; 
                           sharkStart[slaveOrder] = watorParam.NUM_SHARK+1;
                           sharkEnd[slaveOrder] = watorParam.NUM_SHARK;                            
                        }                                              
                        printf("**Slave %d from row %d to %d", slaveOrder+1, rowStart[slaveOrder], rowEnd[slaveOrder]);
                        printf(" from fish %d to %d", fishStart[slaveOrder], fishEnd[slaveOrder]);
                        printf(" from shark %d to %d\n", sharkStart[slaveOrder], sharkEnd[slaveOrder]);    
                     }
                     else if (slaves != 1 && rowTmp == rowLocal[slaveOrder]){
                        rowStart[slaveOrder] = rowStartTmp;
                        rowEnd[slaveOrder] = MAX_X - 1;                        
                        if(totalCreature>0){
                           fishEndTmp = watorParam.NUM_FISH-1;
                           sharkEndTmp = watorParam.NUM_SHARK-1;
                           fishStart[slaveOrder] = fishStartTmp;
                           sharkStart[slaveOrder] = sharkStartTmp;
                           fishEnd[slaveOrder] = fishEndTmp;
                           sharkEnd[slaveOrder] = sharkEndTmp;
                           fishStartTmp += fishEndTmp;
                           sharkStartTmp += sharkEndTmp;  
                        }         
                        else{
                           fishStart[slaveOrder] = watorParam.NUM_FISH+1;
                           fishEnd[slaveOrder] = watorParam.NUM_FISH; 
                           sharkStart[slaveOrder] = watorParam.NUM_SHARK+1;
                           sharkEnd[slaveOrder] = watorParam.NUM_SHARK;                            
                        }    
                        printf("*Slave %d from row %d to %d", slaveOrder+1, rowStart[slaveOrder], rowEnd[slaveOrder]);
                        printf(" from fish %d to %d", fishStart[slaveOrder], fishEnd[slaveOrder]);
                        printf(" from shark %d to %d\n", sharkStart[slaveOrder], sharkEnd[slaveOrder]);
                     }
                  }
               }
            }
         }
         for(j=0; j<NUM_TASKS-1; j++){
            rowStartRB[j] = rowStart[j];
            rowEndRB[j] = rowEnd[j];
            fishStartRB[j] = fishStart[j];
            fishEndRB[j] = fishEnd[j];
            sharkStartRB[j] = sharkStart[j];   
            sharkEndRB[j] = sharkEnd[j];    
            //printf("++++++++++++++++++++++++Rank %d fish from %d\n", j, fishStartRB[j]);                              
         }            
      //}     
         //printf("\n");       
         // Sending the rearranged row / creatures with respect to task rank 
         int slaveRank;
         for(slaveRank = 1; slaveRank <= slaves; slaveRank++){
            //printf("Sending to slave %d\n", slaveRank);
            //usleep(SIM_DELAY*1000);
            MPI_Send(&rowStart[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
            MPI_Send(&rowEnd[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
            for(m=rowStart[slaveRank-1]; m<=rowEnd[slaveRank-1]; m++){
               for(n=0; n<MAX_Y; n++){
                 MPI_Send(&Map[m][n], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);
               }
            }            
            MPI_Send(&watorParam.STEPS, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);
            MPI_Send(&fishStart[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
            MPI_Send(&fishEnd[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
            for(m=fishStart[slaveRank-1]; m<=fishEnd[slaveRank-1]; m++){
               MPI_Send(&Fish[m].x, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
               MPI_Send(&Fish[m].y, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
               MPI_Send(&Fish[m].id, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);
               //printf("++++++++++++++Master send %d id = %d from %d to %d\n", m, Fish[m].id, fishStart[slaveRank-1], fishEnd[slaveRank-1]);
               MPI_Send(&Fish[m].age, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);            
            }                
            MPI_Send(&sharkStart[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
            MPI_Send(&sharkEnd[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);
            for(m=sharkStart[slaveRank-1]; m<=sharkEnd[slaveRank-1]; m++){
               //printf("--------------Master Shark %d id = %d starve = %d\n", m, Shark[m].id, Shark[m].starve);
               MPI_Send(&Shark[m].x, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
               MPI_Send(&Shark[m].y, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
               MPI_Send(&Shark[m].id, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);
               MPI_Send(&Shark[m].age, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);                
               MPI_Send(&Shark[m].starve, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD); 
            }
            MPI_Send(&watorParam.FISH_BREED_AGE, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);
            MPI_Send(&watorParam.SHARK_BREED_AGE, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);
            MPI_Send(&watorParam.SHARK_STARVING, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD);               
         }  
         int sharkNumTmp=0, fishNumTmp=0;
         watorParam.NUM_FISH = 0; 
         watorParam.NUM_SHARK = 0;
         // Recieve the required results done by slaves to start up the next round
         //usleep(SIM_DELAY*1000);
         // Reinitialize the fishmap and the sharkmap
         for(m=0; m<MAX_X; m++){
            for(n=0; n<MAX_Y; n++){ 
               fishmap[m][n] = -5;
               sharkmap[m][n] = -5; 
               Map[m][n] = water;                             
            }
         }       
         printf("\nXXXXXXXXXXXXXXXXXXXXXX   MASTER   XXXXXXXXXXXXXXXXXXXXXXXXX\n");
         for(slaveRank = 1; slaveRank <= slaves; slaveRank++){
            //printf("Receiving from slave %d\n", slaveRank);
            MPI_Recv(&sharkNumTmp, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);   
            MPI_Recv(&fishNumTmp, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);           
            MPI_Recv(&fishStart[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&fishEnd[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);
            //printf("Master receive %d fishes from slave %d; %d to %d\n", fishNumTmp, slaveRank, fishStart[slaveRank-1], fishEnd[slaveRank-1]);  
            for(m=watorParam.NUM_FISH; m<watorParam.NUM_FISH+fishNumTmp; m++){
               MPI_Recv(&Fish[m].x, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);            
               MPI_Recv(&Fish[m].y, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status); 
               MPI_Recv(&Fish[m].id, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status); 
               MPI_Recv(&Fish[m].age, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);            
            }       
            MPI_Recv(&sharkStart[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&sharkEnd[slaveRank-1], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);
            for(m=watorParam.NUM_SHARK; m<watorParam.NUM_SHARK+sharkNumTmp; m++){
               MPI_Recv(&Shark[m].x, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);            
               MPI_Recv(&Shark[m].y, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status); 
               MPI_Recv(&Shark[m].id, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status); 
               MPI_Recv(&Shark[m].age, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);               
               MPI_Recv(&Shark[m].starve, 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status); 
            }
            //printf("*Master receive %d sharks from slave %d; %d to %d\n", sharkNumTmp, slaveRank, sharkStart[slaveRank-1], sharkEnd[slaveRank-1]);
            watorParam.NUM_SHARK += sharkNumTmp; 
            watorParam.NUM_FISH += fishNumTmp;         
            for(m=0; m<MAX_X; m++){
               for(n=0; n<MAX_Y; n++){ 
                  MPI_Recv(&sharkmapTmp[m][n], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);
                  if(sharkmap[m][n]==-5 && sharkmapTmp[m][n] != -5){
                     sharkmap[m][n] = sharkmapTmp[m][n];
                  }
                  MPI_Recv(&fishmapTmp[m][n], 1, MPI_INT, slaveRank, 0, MPI_COMM_WORLD, &status);
                  if(fishmap[m][n]==-5 && fishmapTmp[m][n] != -5){
                     fishmap[m][n] = fishmapTmp[m][n];   
                  }                              
               }
            }                                                    
         }
         for(m=0; m<watorParam.NUM_FISH; m++){
            //printf("**************Fish %d id = %d at [%d][%d]\n", m, Fish[m].id, Fish[m].x, Fish[m].y);
         }         
         // RollBack Function
         for(m=0; m<MAX_X; m++){
            for(n=0; n<MAX_Y; n++){
               sRollBack[m][n] = 0;
               fRollBack[m][n] = 0;
            }
         }
         judgeRollBack = 0;
         for(m=0; m<watorParam.NUM_FISH; m++){
            if(fRollBack[Fish[m].x][Fish[m].y]==0){
               fRollBack[Fish[m].x][Fish[m].y] = 1;               
            }
            else if(fRollBack[Fish[m].x][Fish[m].y]==1){
               //printf("!!!! Roll Back at [%d][%d] !!!!\n", Fish[m].x, Fish[m].y); 
               //fprintf(outFile, "!!!! Roll Back at [%d][%d] !!!!\n", Fish[m].x, Fish[m].y);                              
               judgeRollBack = 1;
            }
         } 
         for(m=0; m<MAX_X; m++){
            for(n=0; n<MAX_Y; n++){
               printf("%d ", fRollBack[m][n]);
            }
            printf("\n");
         }            
         for(m=0; m<watorParam.NUM_SHARK; m++){
            if(sRollBack[Shark[m].x][Shark[m].y]==0){
               sRollBack[Shark[m].x][Shark[m].y] = 1;
            }
            else if (sRollBack[Shark[m].x][Shark[m].y]==0){
               //printf("!!!! Roll Back at [%d][%d] !!!!\n", Shark[m].x, Shark[m].y);
               //fprintf(outFile, "!!!! Roll Back at [%d][%d] !!!!\n", Shark[m].x, Shark[m].y);               
               judgeRollBack = 1;
            }         
         }  
         if(judgeRollBack == 1){
            fprintf(outFile, "!!!! Roll Back !!!!\n");   
         }  
         int mm;
         mm=0;     
         /*if(judgeRollBack == 1){
            //watorParam.STEPS--;
            //steps--;
            for(m=0; m<watorParam.NUM_FISH; m++){
               if(Fish[m].id>=0){
                  //printf("FishRB[%d] = Fish[%d].id = %d\n", mm, m, Fish[m].id);
                  FishRB[mm].x = Fish[m].x;
                  FishRB[mm].y = Fish[m].y;   
                  FishRB[mm].id = Fish[m].id;  
                  FishRB[mm].age = Fish[m].age;  
                  mm++;
               }                  
            }
            mm=0;
            for(m=0; m<watorParam.NUM_SHARK; m++){
               if(Shark[m].id>=0){            
                  SharkRB[mm].x = Shark[m].x;
                  SharkRB[mm].y = Shark[m].y;   
                  SharkRB[mm].id = Shark[m].id;  
                  SharkRB[mm].age = Shark[m].age;   
                  SharkRB[mm].starve = Shark[m].starve;   
                  mm++;
               }               
            }
            for(m=0; m<watorParam.NUM_FISH; m++){
               Fish[m].x = FishRB[m].x;
               Fish[m].y = FishRB[m].y;   
               Fish[m].id = FishRB[m].id;  
               Fish[m].age = FishRB[m].age;  
               mm++;               
            }
            mm=0;
            for(m=0; m<watorParam.NUM_SHARK; m++){         
               Shark[m].x = SharkRB[m].x;
               Shark[m].y = SharkRB[m].y;   
               Shark[m].id = SharkRB[m].id;  
               Shark[m].age = SharkRB[m].age;   
               Shark[m].starve = SharkRB[m].starve;   
               mm++;               
            }         
            watorParam.NUM_SHARK = numSharkRB;
            watorParam.NUM_FISH = numFishRB;     
            //printf("ROLLBACK %d fishes and %d sharks\n", watorParam.NUM_FISH, watorParam.NUM_SHARK);                               
            for(m=0; m<MAX_X; m++){
               for(n=0; n<MAX_Y; n++){
                  Map[m][n] = MapRollBack[m][n];
                  fishmap[m][n] = fmRollBack[m][n];
                  sharkmap[m][n] = smRollBack[m][n];                  
               }
            } 
            for(j=0; j<NUM_TASKS-1; j++){
               rowStart[j] = rowStartRB[j];
               rowEnd[j] = rowEndRB[j];
               fishStart[j] = fishStartRB[j];
               fishEnd[j] = fishEndRB[j];
               sharkStart[j] = sharkStartRB[j];   
               sharkEnd[j] = sharkEndRB[j];    
               //printf("++++++++++++++++++++++++Rank %d fish from %d\n", j, fishStartRB[j]);                              
            }                             
         //} */
         //else if(judgeRollBack==0){   
         //
         int fishNewID=0, sharkNewID=0, fishOldCnt=0, sharkOldCnt=0, fishAgeTmp=0, sharkAgeTmp=0;
         // Deal with the new born fishes and the oldest fishes
         for(m=0; m<watorParam.NUM_FISH; m++){
            if(Fish[m].id >= 0){
               Map[Fish[m].x][Fish[m].y] = fish;
               if(Fish[m].id > fishNewID){
                  fishNewID = Fish[m].id;               
               }
               if(Fish[m].age >= fishAgeTmp){
                  fishAgeTmp = Fish[m].age;
               }
            }
         }
         for(m=0; m<watorParam.NUM_SHARK; m++){
            if(Shark[m].id >= 0){
               Map[Shark[m].x][Shark[m].y] = shark;
               if(Shark[m].id > sharkNewID){
                  sharkNewID = Shark[m].id;               
               }
               if(Shark[m].age >= sharkAgeTmp){
                  sharkAgeTmp = Shark[m].age;
               }               
            }
         }         
         fishNewID++;
         sharkNewID++;
         for(m=0; m<watorParam.NUM_FISH; m++){
            if(Fish[m].id < 0){
               Map[Fish[m].x][Fish[m].y] = fish;            
               Fish[m].id = fishNewID;
               fishNewID++;
            }
         }
         for(m=0; m<watorParam.NUM_SHARK; m++){
            if(Shark[m].id < 0){
               Map[Shark[m].x][Shark[m].y] = shark;            
               Shark[m].id = sharkNewID;
               sharkNewID++;
            }           
         }           
         int year = (watorParam.STEPS - (watorParam.STEPS%watorParam.STEPS_OF_YEAR)) / watorParam.STEPS_OF_YEAR;
         printf("//// Steps %d of Year %d ////\n", watorParam.STEPS, year);
         printf("//// Current total %d fishes %d sharks ////\n", watorParam.NUM_FISH, watorParam.NUM_SHARK);
         print_wator(MAX_X, MAX_Y, watorParam);
         // Print out the fisherman after they had a move
         int fdirection;
         NextXY fnext;
         for(m=0; m<watorParam.NUM_FISHERMAN; m++){ // Fisherman moves and fish
            fishermanMap[Fisherman[m].y] = water;      
            fdirection = rand()%2;   
            fnext = fnextXY(fdirection, Fisherman[m].y, MAX_X, MAX_Y);
            //printf("fisherman id =%d next = %d\n", Fisherman[m].id, fnext.y);   
            fishermanMap[fnext.y] = fisherman;
            Fisherman[m].y = fnext.y; 
         }         
         for(m=0; m<MAX_Y; m++){
            printf("%d ", fishermanMap[m]);   
         } 
         printf("\n");         
         // Detect the oldest fishes and sharks
         for(m=0; m<watorParam.NUM_FISH; m++){
            if(Fish[m].age == fishAgeTmp){
               printf("**Oldest fish id %d: %d years old\n", Fish[m].id, Fish[m].age);   
            }
         }
         for(m=0; m<watorParam.NUM_SHARK; m++){
            if(Shark[m].age == sharkAgeTmp){
               printf("*Oldest shark id %d: %d years old\n", Shark[m].id, Shark[m].age);                                
            }
         }
         printf("***Fishing! ****\n");
         // Today's condition of each fisherman
         int tempIcrTmp;
         double tempIcr[5] = {-1.0, -0.5, 0, 0.5, 1.0};
         for(m=0; m<watorParam.NUM_FISHERMAN; m++){ // Fisherman conditions
            tempIcrTmp = rand()%5;
            if(Fisherman[m].Temp+tempIcr[tempIcrTmp] <= 40.0 && Fisherman[m].Temp+tempIcr[tempIcrTmp]>=35.0){
               Fisherman[m].Temp+= tempIcr[tempIcrTmp];
               if(Fisherman[m].Temp>= 38.0){
                  printf("Fisherman %d is ill!!! He can't go fishing today!!!\n", Fisherman[m].id);                  
               }
            }
            printf("Fisherman %d temp = %.1f degree C\n", Fisherman[m].id, Fisherman[m].Temp);              
         }         
         // Fisherman's work
         int hooked=0;
         watorParam.fishStart = 0;
         watorParam.sharkStart = 0; 
         watorParam.fishEnd = watorParam.NUM_FISH-1;
         watorParam.sharkEnd = watorParam.NUM_SHARK-1;                 
         for(m=0; m<watorParam.NUM_FISHERMAN; m++){ // Fisherman fishes
            hooked = 0;
            for(n=0; n<MAX_X; n++){
               if(Map[n][Fisherman[m].y] != water && hooked==0 && Fisherman[m].Temp<38.0){ // Well condition of fishing
                  Fisherman[m].score += Map[n][Fisherman[m].y];
                  hooked = Map[n][Fisherman[m].y];
                  Map[n][Fisherman[m].y] = water;    
                  MapTmp[n][Fisherman[m].y] = water;                                  
                  if(hooked == fish){
                     watorParam = HUNT(n, Fisherman[m].y, fish, watorParam);  
                     fishmap[n][Fisherman[m].y] = -5;        
                     fprintf(outFile, "Fisherman %d got a fish id=%d!\n", Fisherman[m].id, watorParam.fishermanGotID);             
                  }
                  else if(hooked == shark){
                     watorParam = HUNT(n, Fisherman[m].y, shark, watorParam);
                     sharkmap[n][Fisherman[m].y] = -5; 
                     fprintf(outFile, "Fisherman %d got a Shark id=%d!\n", Fisherman[m].id, watorParam.fishermanGotID);       
                  }
               }               
            }   
            printf("Fisherman id = %d; score = %d\n", Fisherman[m].id, Fisherman[m].score);               
         }
         print_wator(MAX_X, MAX_Y, watorParam);
         for(m=0; m<MAX_Y; m++){
            printf("%d ", fishermanMap[m]);   
         }   
         printf("\n");
         int stmp=0, ftmp=0;  
         //if(judgeRollBack==0){                  
            printf("After fishing there are %d fishes and %d sharks left\n", watorParam.NUM_FISH, watorParam.NUM_SHARK);
         //}
         //}
         printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
         // Draw the graph
         p=0;
         for(m=0; m<MAX_X * WIDTH; m+=WIDTH){
            q=0;
            for(n=0; n<MAX_Y * HEIGHT; n+=HEIGHT){
               XSetForeground (display, gc, Map[p][q] * 0xAABBCC);
               XFillRectangle (display, window, gc, n, m, WIDTH, HEIGHT); 
               q++;
            }
            p++;
         }     
         q=0;
         for(n=0; n<MAX_Y * HEIGHT; n+=HEIGHT){
            XSetForeground (display, gc, fishermanMap[q] * 0xAABBCC);		
            XFillRectangle (display, window, gc, n, MAX_X*WIDTH, WIDTH, HEIGHT);  
            q++;  
         }  
         XSetForeground (display, gc, 10 * 0xAABBCC);     
         XFillRectangle (display, window, gc, MAX_X*WIDTH, 0, MAX_X*WIDTH+200, (MAX_Y+1)*WIDTH);
         char str1[20], str2[20], str3[20], str4[20], str5[20];
         int xx, xx1, xx2, xx3, xx4, xx5, xx6; 
         XSetForeground (display, gc, 0 * 0xAABBCC); 
         sprintf(str1, "Fish: %d", watorParam.NUM_FISH);
         xx = strlen(str1);
         sprintf(str2, "Shark: %d", watorParam.NUM_SHARK);    
         xx1 = strlen(str2);
         sprintf(str3, "STEPS: %d / YEAR: %d", watorParam.STEPS, year); 
         xx2 = strlen(str3);
         sprintf(str4, "Fish Max Age: %d", fishAgeTmp); 
         xx3 = strlen(str4);  
         sprintf(str5, "Shark Max Age: %d", sharkAgeTmp); 
         xx4 = strlen(str5);                
         XDrawString(display, window, gc, MAX_X*(WIDTH+1), 100, str1, xx);
         XDrawString(display, window, gc, MAX_X*(WIDTH+1), 100+HEIGHT, str2, xx1); 
         XDrawString(display, window, gc, MAX_X*(WIDTH+1), 100+2*HEIGHT, str3, xx2);    
         XDrawString(display, window, gc, MAX_X*(WIDTH+1), 100+3*HEIGHT, str4, xx3); 
         XDrawString(display, window, gc, MAX_X*(WIDTH+1), 100+4*HEIGHT, str5, xx4);                                                                    
         // Sending the Tag to initiate logging
         for(n=1; n<=NUM_TASKS-1;n++){
            MPI_Recv(&watorParam.logFishDead[0], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);  
            for(m=1; m<=watorParam.logFishDead[0]; m++){
               MPI_Recv(&watorParam.logFishDead[m], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);
               fprintf(outFile, "Fish id = %d is Dead!\n", watorParam.logFishDead[m]);
               //printf("Fish id = %d is Dead!\n", watorParam.logFishDead[m]);  
            }
            MPI_Recv(&watorParam.logSharkDead[0], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);  
            for(m=1; m<=watorParam.logSharkDead[0]; m++){
               MPI_Recv(&watorParam.logSharkDead[m], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);
               //fprintf(outFile, "Shark id = %d is Dead!\n", watorParam.logSharkDead[m]);  
            } 
            MPI_Recv(&watorParam.logFishBreed[0], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);  
            for(m=1; m<=watorParam.logFishBreed[0]; m++){
               MPI_Recv(&watorParam.logFishBreed[m], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);
               fprintf(outFile, "Fish id = %d Breeds!\n", watorParam.logFishBreed[m]);  
            } 
            MPI_Recv(&watorParam.logSharkBreed[0], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);  
            for(m=1; m<=watorParam.logSharkBreed[0]; m++){
               MPI_Recv(&watorParam.logSharkBreed[m], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status); 
               fprintf(outFile, "Shark id = %d Breeds!\n", watorParam.logSharkBreed[m]); 
            } 
            MPI_Recv(&watorParam.logSharkEat[0], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);  
            for(m=1; m<=watorParam.logSharkEat[0]; m++){
               MPI_Recv(&watorParam.logSharkEat[m], 1, MPI_INT, n, 0, MPI_COMM_WORLD, &status);
               fprintf(outFile, "Shark id = %d ate the prey!\n", watorParam.logSharkEat[m]);  
            }                       
         }
         //int year = (watorParam.STEPS - (watorParam.STEPS%watorParam.STEPS_OF_YEAR)) / watorParam.STEPS_OF_YEAR;
         fprintf(outFile, "*** Steps %d of year %d Total %d fishes and %d sharks ****\n", watorParam.STEPS, year, watorParam.NUM_FISH, watorParam.NUM_SHARK);
         XFlush(display); 
      } 
      fclose(outFile);     
      printf("XXXXXXXXXXXXXXXXXXX WATOR COMPLETED XXXXXXXXXXXXXXXXXXX\n");
      sleep(2);         
      timeEnd = clock();
		  double gap = (timeEnd-timeStart) / CLOCKS_PER_SEC; 
      printf("Processors : %d\n", NUM_TASKS);         
      printf("Running time : %lf\n", gap);        
   }
   /*** slaves ***********************************************************************************************************************************/
   if(rank != 0){
   int steps1;
   steps1 = 0;
   while(steps1 < SIM_STEPS-1){
      watorParam.logSharkDead[0] = 0;
      watorParam.logFishDead[0] = 0;
      watorParam.logFishBreed[0] = 0;
      watorParam.logSharkBreed[0] = 0;
      watorParam.logSharkEat[0] = 0;   
      for(i=1; i<1000; i++){
         watorParam.logSharkDead[i] = -5;
         watorParam.logFishDead[i] = -5;
         watorParam.logFishBreed[i] = -5;
         watorParam.logSharkBreed[i] = -5;
         watorParam.logSharkEat[i] = -5;      
      }   
      MPI_Recv(&steps1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
      int fishStartTmp, fishEndTmp, sharkStartTmp, sharkEndTmp;
      // Restore the MapTmp for as register
      for(i=0; i<MAX_X; i++){
         for(j=0; j<MAX_Y; j++){ 
            MapTmp[i][j] = water;
            fishmap[i][j] = -5;
            sharkmap[i][j] = -5;
            Map[i][j] = water;
         }
      }       
      // Receiving required data for a branch to do its task
      MPI_Recv(&watorParam.rowStart, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); 
      MPI_Recv(&watorParam.rowEnd, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
      for(m=watorParam.rowStart; m<=watorParam.rowEnd; m++){
         for(n=0; n<MAX_Y; n++){
            MPI_Recv(&Map[m][n], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
         }
      }   
      for(i=0; i<MAX_X; i++){
         for(j=0; j<MAX_Y; j++){ 
            MapTmp[i][j] = Map[i][j];
         }
      }          
      MPI_Recv(&watorParam.STEPS, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);       
      MPI_Recv(&watorParam.fishStart, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);    
      MPI_Recv(&watorParam.fishEnd, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); 
      if(watorParam.fishEnd < 0){
         watorParam.fishEnd = watorParam.fishStart;
         watorParam.fishStart++;
      }
      fishStartTmp = watorParam.fishStart;
      fishEndTmp = watorParam.fishEnd;
      watorParam.NUM_FISH = watorParam.fishEnd - watorParam.fishStart +1;
      watorParam.fishStartTmp = watorParam.fishStart;
      for(m=watorParam.fishStart; m<=watorParam.fishEnd; m++){
         MPI_Recv(&Fish[m].x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);  
         MPI_Recv(&Fish[m].y, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
         MPI_Recv(&Fish[m].id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
         MPI_Recv(&Fish[m].age, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);                               
      }
      MPI_Recv(&watorParam.sharkStart, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);  
      MPI_Recv(&watorParam.sharkEnd, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);  
      if(watorParam.sharkEnd < 0){
         watorParam.sharkEnd = watorParam.sharkStart;
         watorParam.sharkStart++;
      }      
      sharkStartTmp = watorParam.sharkStart;
      sharkEndTmp = watorParam.sharkEnd;
      watorParam.NUM_SHARK = watorParam.sharkEnd - watorParam.sharkStart +1;
      watorParam.sharkStartTmp = watorParam.sharkStart;
      for(m=watorParam.sharkStart; m<=watorParam.sharkEnd; m++){
         MPI_Recv(&Shark[m].x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
         MPI_Recv(&Shark[m].y, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);      
         MPI_Recv(&Shark[m].id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
         MPI_Recv(&Shark[m].age, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
         MPI_Recv(&Shark[m].starve, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
      }
      MPI_Recv(&watorParam.FISH_BREED_AGE, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); 
      MPI_Recv(&watorParam.SHARK_BREED_AGE, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
      MPI_Recv(&watorParam.SHARK_STARVING, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); 
      //printf("-------Rank %d got fish %d to %d id= ", rank, fishStartTmp, fishEndTmp);
      //for(m=fishStartTmp; m<=fishStartTmp+watorParam.NUM_FISH-1; m++){
      //   printf("%d ", Fish[m].id);
      //}
      //printf("\n");                
      //printf("Rank %d\n", rank);
      //printf("Slave %d starts from row %d to row %d\n", rank, watorParam.rowStart, watorParam.rowEnd);
      //printf("From fish %d to %d; shark %d to %d\n", watorParam.fishStart, watorParam.fishEnd, watorParam.sharkStart, watorParam.sharkEnd);     
      // Wator Basic  function for one step
      //print_wator(MAX_X, MAX_Y, watorParam);
      //printf("//////////////////////////////////\n\n");    
      // Receive two rows results form the next rank to prevent rollback
      if(rank != NUM_TASKS-1){
         for(i=0; i<MAX_Y; i++){
            MPI_Send(&Map[watorParam.rowEnd][i], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);            
         }
         for(i=watorParam.rowEnd; i<watorParam.rowEnd+2; i++){
            for(j=0; j<MAX_Y; j++){
               MPI_Recv(&MapTmpTmp[i][j], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status); 
               if(Map[i][j] == water){
                  Map[i][j] = MapTmpTmp[i][j];
               }              
            }
         }
         //printf("Rank %d\n", rank);
         for(i=watorParam.rowEnd; i<watorParam.rowEnd+2; i++){
            for(j=0; j<MAX_Y; j++){
               //printf("%d ", Map[i][j]);               
            }
            //printf("\n");
         }
         // Receive the information of dead fish from the next rank   
         MPI_Recv(&watorParam.fishPrevDeadYRx[0], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);                          
         for(i=1; i<=watorParam.fishPrevDeadYRx[0]; i++){
            MPI_Recv(&watorParam.fishPrevDeadYRx[i], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);   
            MPI_Recv(&watorParam.fishPrevDeadXRx[i], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);             
         }
         if(watorParam.fishPrevDeadYRx[0]>0){            
            //printf("OOOO Rank %d Fish dead: OOOO\n", rank);
            for(i=1; i<=watorParam.fishPrevDeadYRx[0]; i++){
               //printf("--Fish dead at [%d][%d] ", watorParam.fishPrevDeadXRx[i], watorParam.fishPrevDeadYRx[i]);            
               for(j=watorParam.fishStart; j<=watorParam.fishEnd; j++){ // Find the corresponding fish
                  if(Fish[j].x==watorParam.fishPrevDeadXRx[i] && Fish[j].y==watorParam.fishPrevDeadYRx[i]){
                     //printf("is of id %d and order %d: ", Fish[j].id, j);
                     watorParam = HUNT(watorParam.fishPrevDeadXRx[i], watorParam.fishPrevDeadYRx[i], fish, watorParam);
                  }
               }   
            }               
            //watorParam.fishEnd -= watorParam.fishPrevDeadYRx[0];          
            //printf("OOOO Now Rank %d has %d fishes OOOO\n", rank, watorParam.fishEnd-watorParam.fishStart+1);
         }                
         //
      }
      // Do the first row and send the results to the previous rank
      if(rank != 1){
         watorParam.fishEnd = watorParam.fishStart -1;
         watorParam.sharkEnd = watorParam.sharkStart -1;
         for(i=0; i<MAX_Y; i++){
            MPI_Recv(&Map[watorParam.rowStart-1][i], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);            
         }    
         //printf("Row %d: ", watorParam.rowStart-1);
         for(i=0; i<MAX_Y; i++){
            //printf("%d ", Map[watorParam.rowStart-1][i]);            
         }            
         //printf("\n");
         //print_wator(MAX_X, MAX_Y, watorParam);         
         for(i=watorParam.rowStart; i<watorParam.rowStart+2; i++){
            for(j=0; j<MAX_Y; j++){
               if(MapTmp[i][j] == fish){
                  watorParam.fishEnd++;
               }
               else if(MapTmp[i][j] == shark){
                  watorParam.sharkEnd++;
               }            
            }
         }               
         //printf("**Rank %d, fishStart = %d, fishEnd = %d\n", rank, watorParam.fishStart, watorParam.fishEnd);
         watorParam = WatorFunc(MAX_X, MAX_Y, watorParam); 
         //print_wator(MAX_X, MAX_Y, watorParam);       
         for(i=watorParam.rowStart-1; i<watorParam.rowStart+1; i++){
            for(j=0; j<MAX_Y; j++){
               MPI_Send(&Map[i][j], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD);   
            }
         } 
         // Send the information of dead fish not in this rank         
         MPI_Send(&watorParam.fishPrevDeadYTx[0], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD);                            
         for(i=1; i<=watorParam.fishPrevDeadYTx[0]; i++){
            MPI_Send(&watorParam.fishPrevDeadYTx[i], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD);  
            MPI_Send(&watorParam.fishPrevDeadXTx[i], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD);             
         }
         watorParam.fishStart = watorParam.fishEnd+1;
         watorParam.sharkStart = watorParam.sharkEnd+1;
         fEndTmp = watorParam.fishEnd;
         sEndTmp = watorParam.sharkEnd;
         watorParam.fishEnd = fishEndTmp;
         watorParam.sharkEnd = sharkEndTmp; 
         //printf("//rank %d NUM_FISH = %d NUM_SHARK = %d\n", rank, watorParam.NUM_FISH, watorParam.NUM_SHARK);
      }
      // The rest of creatures    
      //watorParam.NUM_FISH = watorParam.fishEnd - fishStartTmp + 1;
      //watorParam.NUM_SHARK = watorParam.sharkEnd - sharkStartTmp + 1;                     
      //printf("/*/* Rank %d fish = %d\n", rank, watorParam.NUM_FISH);
      for(m=watorParam.fishStart; m<=watorParam.fishEnd; m++){
         //printf("**************Rank %d fish id = %d\n", rank, Fish[m].id);                               
      }      
      watorParam = WatorFunc(MAX_X, MAX_Y, watorParam); 
      for(m=watorParam.fishStart; m<=watorParam.fishEnd; m++){
         //printf("**************Rank %d fish id = %d\n", rank, Fish[m].id);                               
      }       
      //printf("\n/// Rank %d rest are fish %d to %d and shark %d to %d ///\n", rank, watorParam.fishStart, watorParam.fishEnd, watorParam.sharkStart, watorParam.sharkEnd);
      //printf("///rank %d NUM_FISH = %d NUM_SHARK = %d\n", rank, watorParam.NUM_FISH, watorParam.NUM_SHARK);     
      // Calculate again the wator parameters 
      //watorParam.NUM_FISH = watorParam.fishEnd - fishStartTmp + 1;
      //watorParam.NUM_SHARK = watorParam.sharkEnd - sharkStartTmp + 1;
      if(rank != NUM_TASKS-1){
         // Send the information of dead fish not in this rank               
         MPI_Send(&watorParam.fishPrevDeadYTx[0], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);                            
         for(i=1; i<=watorParam.fishPrevDeadYTx[0]; i++){
            MPI_Send(&watorParam.fishPrevDeadYTx[i], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);  
            MPI_Send(&watorParam.fishPrevDeadXTx[i], 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);             
         }    
         //printf("/*/* Rand %d fish = %d\n", rank, watorParam.NUM_FISH);
      }      
      if(rank != 1){
         // Receive the information of dead fish from the next rank   
         MPI_Recv(&watorParam.fishPrevDeadYRx[0], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);                          
         for(i=1; i<=watorParam.fishPrevDeadYRx[0]; i++){
            MPI_Recv(&watorParam.fishPrevDeadYRx[i], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);   
            MPI_Recv(&watorParam.fishPrevDeadXRx[i], 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &status);             
         }
         if(watorParam.fishPrevDeadYRx[0]>0){            
            //printf("OOOO Rank %d Fish dead: OOOO\n", rank);
            for(i=1; i<=watorParam.fishPrevDeadYRx[0]; i++){
               //printf("++Fish dead at [%d][%d] ", watorParam.fishPrevDeadXRx[i], watorParam.fishPrevDeadYRx[i]);            
               for(j=fishStartTmp; j<=watorParam.fishEnd; j++){ // Find the corresponding fish
                  if(Fish[j].x==watorParam.fishPrevDeadXRx[i] && Fish[j].y==watorParam.fishPrevDeadYRx[i]){
                     //printf("is of id %d and order %d: ", Fish[j].id, j);
                     watorParam.fishStart = fishStartTmp;
                     //printf(" among %d to %d ", fishStartTmp, watorParam.fishEnd);                     
                     watorParam = HUNT(watorParam.fishPrevDeadXRx[i], watorParam.fishPrevDeadYRx[i], fish, watorParam);
                  }
               }   
            }          
            //printf("\nOOOO Now Rank %d has %d fishes OOOO\n", rank, watorParam.NUM_FISH);
         }      
      }
      // Sending back the done information for wator
      MPI_Send(&watorParam.NUM_SHARK, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      MPI_Send(&watorParam.NUM_FISH, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);    
      // sharks and fishes
      int sendTmp;
      sendTmp = fishStartTmp+watorParam.NUM_FISH-1;
      MPI_Send(&fishStartTmp, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
      MPI_Send(&sendTmp, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      //printf("-------Rank %d sent from fish %d to %d id= ", rank, fishStartTmp, fishStartTmp+watorParam.NUM_FISH-1);
      //for(m=fishStartTmp; m<=fishStartTmp+watorParam.NUM_FISH-1; m++){
      //   printf("%d ", Fish[m].id);
      //}
      //printf("\n");
      for(m=fishStartTmp; m<=fishStartTmp+watorParam.NUM_FISH-1; m++){
         MPI_Send(&Fish[m].x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
         MPI_Send(&Fish[m].y, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
         MPI_Send(&Fish[m].id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
         MPI_Send(&Fish[m].age, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);            
      }       
      sendTmp = sharkStartTmp+watorParam.NUM_SHARK-1;         
      MPI_Send(&sharkStartTmp, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
      MPI_Send(&sendTmp, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      for(m=sharkStartTmp; m<=sharkStartTmp+watorParam.NUM_SHARK-1; m++){
         MPI_Send(&Shark[m].x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
         MPI_Send(&Shark[m].y, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
         MPI_Send(&Shark[m].id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
         MPI_Send(&Shark[m].age, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);                
         MPI_Send(&Shark[m].starve, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
      }
      for(m=0; m<MAX_X; m++){
         for(n=0; n<MAX_Y; n++){ 
            MPI_Send(&sharkmap[m][n], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&fishmap[m][n], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);            
         }
      }
      // Logs
      //MPI_Recv(&watorParam.logStart, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
      //if(watorParam.logStart == 1){
      //printf("Rank %d Starting logging!\n", rank);                   
      MPI_Send(&watorParam.logFishDead[0], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      for(m=1; m<=watorParam.logFishDead[0]; m++){
         MPI_Send(&watorParam.logFishDead[m], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      }
      MPI_Send(&watorParam.logSharkDead[0], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      for(m=1; m<=watorParam.logSharkDead[0]; m++){
         MPI_Send(&watorParam.logSharkDead[m], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      } 
      MPI_Send(&watorParam.logFishBreed[0], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      for(m=1; m<=watorParam.logFishBreed[0]; m++){
         MPI_Send(&watorParam.logFishBreed[m], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      } 
      MPI_Send(&watorParam.logSharkBreed[0], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      for(m=1; m<=watorParam.logSharkBreed[0]; m++){
         MPI_Send(&watorParam.logSharkBreed[m], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      } 
      MPI_Send(&watorParam.logSharkEat[0], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      for(m=1; m<=watorParam.logSharkEat[0]; m++){
         MPI_Send(&watorParam.logSharkEat[m], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
      } 
      //printf("Rank %d end logging! of steps %d\n", rank, steps1);       
      //}                                                             
   }
   }    
   MPI_Barrier(MPI_COMM_WORLD);  
   MPI_Finalize();
   return 0;
}
