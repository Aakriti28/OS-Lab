#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "processInfo.h"

void welcome(void)
{
  printf(1, "I am child in welcome function\n");
  welcomeDone();
}

int main(void)
{

  int ret1 = fork();
  if(ret1 == 0)
    {
      printf(1, "I am child with no welcome\n"); 
    }
  else if (ret1 > 0)
    {
      wait();
      printf(1, "Parent reaped first child\n");
     
      welcomeFunction(&welcome);
      
      int ret2 = fork();
      if (ret2 == 0)
  {
    printf(1, "I have returned from welcome function\n");
    exit();
  }
      else if (ret2 > 0)
  {
    wait();
    printf(1, "Parent reaped second child\n");
  }
    }

    
  printf(1, "Hello, world!\n");
  helloYou("Calling from XV6");
  struct processInfo info;
    int pid;
    printf(1, "Total Number of Active Processes: %d\n", getNumProc());
    printf(1, "Maximum PID: %d\n\n", getMaxPid());
    
    printf(1, "PID\tPPID\tSIZE\tNumber of Context Switch\n");
    for(int i=1; i<=getMaxPid(); i++)
    {
        pid = i;
        if(getProcInfo(pid, &info) == 0)
    printf(1, "%d\t%d\t%d\t%d\n", pid, info.ppid, info.psize, info.numberContextSwitches);
  }
  helloYou("Welcome to XV6");
  exit();

}
