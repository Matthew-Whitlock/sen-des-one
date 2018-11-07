#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define BCM2708_PERI_BASE  0x3F000000
#define GPIO_BASE          (BCM2708_PERI_BASE + 0x200000)
#define INTERUPT_BASE      (BCM2708_PERI_BASE + 0x00B000)
#define CLK_BASE           (BCM2708_PERI_BASE + 0x101000) //Clock control actually starts at 0x70 past this!
#define CLK_OFFSET (0x1C) //0x70/4, so the number of unsigned ints to increment by.

#define GET_CYCLE(v) asm volatile ("mrc p15, 0, %0, c9, c13, 0":"=r" (v));

#define CLK_PSWD			(0x5A << 24)
#define CLK_PSWD_MASK   (0xFF << 24)
#define CLK_ENABLE      (1 << 4)
#define CLK_BUSY        (1 << 7)
#define CLK_PLLD_SRC    (6)
#define CLK_FLIP        (1<<8)

volatile unsigned *gpio,*gpset,*gpclr,*gpin,*clk,*intrupt;

int setup() {
  int memfd;
  void *gpio_map,*clk_map,*int_map;

  memfd = open("/dev/mem",O_RDWR|O_SYNC);
  if(memfd < 0) {
    printf("Mem open error\n");
    return(0);
  }

  gpio_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,GPIO_BASE);

  clk_map = mmap(NULL,4096,PROT_READ|PROT_WRITE, MAP_SHARED,memfd,CLK_BASE);

  int_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,INTERUPT_BASE);

  close(memfd);

  if(gpio_map == MAP_FAILED  || clk_map == MAP_FAILED || clk_map == MAP_FAILED) {
    printf("Map failed\n");
    return(0);
  }
  
  //interrupt pointer
  intrupt = (volatile unsigned *)int_map;
  
  //timer pointer
  clk = (((volatile unsigned *)clk_map) + CLK_OFFSET);

  //GPIO pointers
  gpio = (volatile unsigned *)gpio_map;
  gpset = gpio + 7;     // set bit register offset 28
  gpclr = gpio + 10;    // clr bit register
  gpin = gpio + 13;     // read all bits register

   

  //Now that we've set up the register access stuff
  //For convenience, we can also setup pins 4,5,&6 
  //to output the GPIO output.
  //Function select registers for 4/5/6 are in the 
  //first register of gpio
  //Mask function selects of the 3 to 0
  *gpio &= ~(511<<12);
  //Now set each to alt function 0
  *gpio |= 4<<12;
  //*gpio |= 4<<15;
  *gpio |= 4<<18;

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
   unsigned timer1, timer2; 
   setup();
    
   //Configure and enable the clock outputs. Clock 0 is for the transducer, clock 2 is for the ADC
	
   //First, turn off all gpio clocks.
	*clk     = CLK_PSWD;
	*(clk+4) = CLK_PSWD;
	while((*clk & CLK_BUSY) || (*(clk+4) & CLK_BUSY)){
      //Do nothing - we're waiting for the clocks to safely stop.i
   }

   //Now configure clock scaling frequencies
   *(clk+1) = CLK_PSWD | (203<<12); //2.46MHz
   *(clk+5) = CLK_PSWD | (263<<12); //1.90MHz //TODO: Could this be higher?
   
   
   *clk =     CLK_PSWD | CLK_PLLD_SRC;
   *(clk+4) = CLK_PSWD | CLK_PLLD_SRC;
   
   *clk =     CLK_PSWD | CLK_PLLD_SRC | CLK_ENABLE;
   *(clk+4) = CLK_PSWD | CLK_PLLD_SRC | CLK_ENABLE;

   //Wait for 8 pulses to be sent.
   GET_CYCLE(timer1);
   GET_CYCLE(timer2);
   while(timer2 - timer1 < 3902){
      GET_CYCLE(timer2);
   }

   //Now disable transducer clock
   *clk =     CLK_PSWD | CLK_PLLD_SRC;
   
   //Now we just need to take lots of measurements.

   
   
   
   *(clk+4) = CLK_PSWD | CLK_PLLD_SRC;
}
