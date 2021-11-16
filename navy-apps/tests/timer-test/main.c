#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <stdint.h>

int NDL_Init(uint32_t flags);
void NDL_Quit();
uint32_t NDL_GetTicks();

int main(){
  NDL_Init(0);

  __uint64_t time;
  __uint64_t usec = 500000;
  struct timeval tv;
  printf("Hello. %d\n", sizeof(struct timeval));
  while (1) {
    gettimeofday(&tv, NULL);
    time = tv.tv_sec * 1000000 + tv.tv_usec;
    while(time < usec) {
      gettimeofday(&tv, NULL);
      time = tv.tv_sec * 1000000 + tv.tv_usec;
    };
    //rtc = io_read(AM_TIMER_RTC);
    //printf("%d-%d-%d %02d:%02d:%02d GMT (", rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second);
    
    printf("Oh, 0.5s have been lost...\n");
    usec += 500000;
  }

  
  //NDL_Quit();
}
