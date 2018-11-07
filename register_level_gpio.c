#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BCM2708_PERI_BASE	0x3F000000
#define GPIO_BASE  			(BCM2708_PERI_BASE + 0x200000)
#define INTERUPT_BASE 		(BCM2708_PERI_BASE + 0x00B000)
#define CLK_BASE			(BCM2708_PERI_BASE + 0x101000) //Clock control actually starts at 0x70 past this!
#define CLK_OFFSET (0x1D) //0x74/4, so the number of unsigned ints to increment by.

volatile unsigned *gpio,*gpset,*gpclr,*gpin,*clk,*intrupt;

int setup() {
  int memfd;
  void *gpio_map,*timer_map,*int_map;

  memfd = open("/dev/mem",O_RDWR|O_SYNC);
  if(memfd < 0) {
    printf("Mem open error\n");
    return(0);
  }

  gpio_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,GPIO_BASE);

  clk_map = mmap(NULL,4096,PROT_READ|PROT_WRITE, MAP_SHARED,memfd,TIMER_BASE);

  int_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,INT_BASE);

  close(memfd);

  if(gpio_map == MAP_FAILED  || timer_map == MAP_FAILED || clk_map == MAP_FAILED) {
    printf("Map failed\n");
    return(0);
  }
  
  //interrupt pointer
  intrupt = (volatile unsigned *)int_map;
  
  //timer pointer
  clk = (volatile unsigned *)timer_map + CLK_OFFSET;

  //GPIO pointers
  gpio = (volatile unsigned *)gpio_map;
  gpset = gpio + 7;     // set bit register offset 28
  gpclr = gpio + 10;    // clr bit register
  gpin = gpio + 13;     // read all bits register

  return(1);
}

int interrupts(int flag) {
  static unsigned int sav132 = 0;
  static unsigned int sav133 = 0;
  static unsigned int sav134 = 0;

  if(flag == 0) {    // disable
    if(sav132 != 0) {
      // Interrupts already disabled so avoid printf
      return(0);
    }

    if( (*(intrupt+128) | *(intrupt+129) | *(intrupt+130)) != 0) {
      //printf("Pending interrupts\n");  // may be OK but probably
      return(0);                         // better to wait for the
    }                                    // pending interrupts to
                                         // clear

    sav134 = *(intrupt+134);
    *(intrupt+137) = sav134;
    sav132 = *(intrupt+132);  // save current interrupts
    *(intrupt+135) = sav132;  // disable active interrupts
    sav133 = *(intrupt+133);
    *(intrupt+136) = sav133;
  } else { // flag = 1 enable
    if(sav132 == 0) {
      printf("Tried enabling interrupts when they weren't disabled. Something is wonky.\n");
      return(0);
    }

    *(intrupt+132) = sav132;    // restore saved interrupts
    *(intrupt+133) = sav133;
    *(intrupt+134) = sav134;
    sav132 = 0;                 // indicates interrupts enabled
  }
  return(1);
}

int main(){
	//TODO: This.
}