/*
// Pin 14 is an output pin, for exciting the probe.
// Pin 15 is used to trigger sending the voltage to the measurement equipment.
// Pin 4 is a GPIOclk pin for triggering the ADC.
// Pins 6, 13, 19, 26, 12, 16, 20, and 21 are the ADC output DB pins (in order.)
// Pin 5 is the EOC pin of the ADC.
*/

#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/io.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#ifdef DEBUG
#include <errno.h>
#include <string.h>
#endif

#define BCM2708_PERI_BASE  0x3F000000
#define GPIO_BASE          (BCM2708_PERI_BASE + 0x200000)
#define INTERUPT_BASE      (BCM2708_PERI_BASE + 0x00B000)
#define CLK_BASE           (BCM2708_PERI_BASE + 0x101000) //Clock control actually starts at 0x70 past this!
#define CLK_OFFSET (0x1C) //0x70/4, so the number of unsigned ints to increment by.
#define DMA_BASE           (BCM2708_PERI_BASE + 0x007000)

#define GET_CYCLE(v) asm volatile ("mrc p15, 0, %0, c9, c13, 0":"=r" (v));

#define PWM_CLK_OFFSET  (0xC)
#define CLK_PSWD			(0x5A << 24)
#define CLK_PSWD_MASK   (0xFF << 24)
#define CLK_ENABLE      (1 << 4)
#define CLK_BUSY        (1 << 7)
#define CLK_PLLD_SRC    (6)
#define CLK_FLIP        (1<<8)

volatile unsigned *gpio,*gpset,*gpclr,*gpin,*clk,*intrupt, *dma, *dma_dest;
volatile unsigned dma_cntrl[8] __attribute__ ((aligned (256)));
volatile unsigned dma_src_tmp;
int m_pagemap_fd;


uintptr_t phys_addr(const volatile void * where){
   uint64_t frameinfo;
   ssize_t res = pread(m_pagemap_fd, &frameinfo, sizeof(frameinfo), (((uintptr_t)where) / 4096) * sizeof(frameinfo));
#ifdef DEBUG
   fprintf(stderr, "Reading at offset %u\n", ((uintptr_t)where/4096));
   if(res != sizeof(frameinfo)){ 
      fprintf(stderr, "Error: pread() failed (%d): %s\n", errno, strerror(errno));
      return 0;
   }
   if(((frameinfo >> 55) & 0xFBF) != 0x10c){
      fprintf(stderr, "Error: Page not present\n");
      return 0;
   }
#endif
   return (uintptr_t)(frameinfo * 4096) + ((uintptr_t)where % 4096);
}

int setup() {
  //pid_t pid = getpid();
  //char pagemap_file[50];
  //sprintf(pagemap_file, "/proc/%d/pagemap", pid);
  char *pagemap_file = "/proc/self/pagemap\0";
  int m_pagemap_fd = open(pagemap_file, O_RDWR);
#ifdef DEBUG
  //fprintf(stderr, "PID: %d\n", pid);
  if(m_pagemap_fd < 0) {
    fprintf(stderr, "Mem open error\n");
    return(0);
  }
#endif
  
  int memfd;
  void *gpio_map,*clk_map,*int_map, *dma_map;

  memfd = open("/dev/mem",O_RDWR|O_SYNC);
#ifdef DEBUG
  if(memfd < 0) {
    printf("Mem open error\n");
    return(0);
  }
#endif

  gpio_map = mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,GPIO_BASE);

  clk_map =  mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,CLK_BASE);

  int_map =  mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,INTERUPT_BASE);

  dma_map =  mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,DMA_BASE);

  close(memfd);

#ifdef DEBUG
  if(gpio_map == MAP_FAILED  || clk_map == MAP_FAILED 
     || clk_map == MAP_FAILED || dma_map == MAP_FAILED) {
    printf("Map failed\n");
    return(0);
  }
#endif
  
  //interrupt pointer
  intrupt = (volatile unsigned *)int_map;
  
  //timer pointer
  clk = (((volatile unsigned *)clk_map) + CLK_OFFSET);

  //GPIO pointers
  gpio = (volatile unsigned *)gpio_map;
  gpset = gpio + 7;     // set bit register offset 28
  gpclr = gpio + 10;    // clr bit register
  gpin = gpio + 13;     // read all bits register

  dma = (volatile unsigned *)dma_map;

  //Now that we've set up the register access stuff
  //for convenience, we can also setup pin 4 
  //to output the GPIO clock output
  //Function select register for 4 are in the 
  //first register of gpio
  //Mask function selects of pin 4 to 0
  *gpio &= ~(7<<12);
  //Now set pin 4 to alt function 0
  *gpio |= 4<<12;

  //Now set pin 14 and 15 to output, for exciting the transducer 
  *(gpio+1) &= ~(7<<12);
  *(gpio+1) |= (1<<12);
  *(gpio+1) &= ~(7<<15);
  *(gpio+1) |= (1<<15);
   

  //Set the ADC input pins as inputs.
  // Pins 6, 13, 19, 26, 12, 16, 20, and 21 are the ADC output DB pins (in order.)
  // Pin 5 is the EOC pin of the ADC.
  *gpio &= ~(0x3F << 15); //5/6 as inputs
  *(gpio+1) &= ~(0x3F << 6); //12/13
  *(gpio+1) &= ~(0x7 << 18); //16
  *(gpio+1) &= ~(0x7 << 27); //19
  *(gpio+2) &= ~(0x3F); //20/21
  *(gpio+2) &= ~(0x7 << 18); //26


  //Now we'll configure DMA stuff
  dma_dest = (unsigned *)malloc(10000*sizeof(unsigned));
  dma_dest[0] = 0;
  dma_src_tmp = 1<<5;
  //Apparently the control value has to be 256-bit aligned in memory, and this
  //is how you do that. (?)
  dma_cntrl[0] = 1<<4; //Read the same location and write w/ offset of 32 bits
  dma_cntrl[1] = phys_addr(&dma_src_tmp);
  dma_cntrl[2] = phys_addr(dma_dest);
  dma_cntrl[3] = 1<<4; //10,000 unsigned ints = 40000 bytes
  dma_cntrl[4] = 0;
  dma_cntrl[5] = 0;
  dma_cntrl[6] = 0;
  dma_cntrl[7] = 0;

  unsigned tmp = *gpin;

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
#ifdef DEBUG
    if(sav132 == 0) {
      printf("Tried enabling interrupts when they weren't disabled. Something is wonky.\n");
      return(0);
    }
#endif

    *(intrupt+132) = sav132;    // restore saved interrupts
    *(intrupt+133) = sav133;
    *(intrupt+134) = sav134;
    sav132 = 0;                 // indicates interrupts enabled
  }
  return(1);
}

int main(){
   unsigned timer1, timer2;
   //format: GPIO pins 0, timer0, GPIO pins 1, timer1, ...
   setup();

   //disable interrupts while we do this.
   //while(!interrupts(0));

   //Configure and enable ADC clock
   //First, turn off the gpio clock.
   *clk &= ~CLK_PSWD_MASK;
   *clk &= ~CLK_ENABLE;
	*clk |= CLK_PSWD;
#ifdef DEBUG
  fprintf(stderr, "Waiting on gpioclk... "); 
#endif
	while((*clk & CLK_BUSY)){
      //Do nothing - we're waiting for the clock to safely stop.
   }
#ifdef DEBUG
  fprintf(stderr, "done.\n"); 
#endif

   //Now configure clock scaling frequency
   *(clk+1) = CLK_PSWD | (250<<12); //~1.8MHz
   *clk =     CLK_PSWD | CLK_PLLD_SRC;
   *clk =     CLK_PSWD | CLK_PLLD_SRC | CLK_ENABLE;
   
   //Now configure input reads from DMA
   int dma_to_use = 0;
   while(dma_to_use < 15 && (*(dma+dma_to_use*0x100) & 1)) dma_to_use += 1;
   if(dma_to_use == 15) return 111;
#ifdef DEBUG
   //fprintf(stderr, "%u\n", phys_addr(dma_cntrl));
   fprintf(stderr, "Using DMA %u\n", dma_to_use);
#endif
   *(dma + dma_to_use*0x100 + 1) = phys_addr(dma_cntrl);
   *(dma + dma_to_use*0x100) |= 1;

   //Wait a couple cycles to be sure the clock & DMA start
   GET_CYCLE(timer1);
   GET_CYCLE(timer2);
   while(timer2 - timer1 < 1400){
      GET_CYCLE(timer2);
   }


   //ADC clock is now running. Lets excite the transducer.
   *gpset = 1<<14;
   GET_CYCLE(timer1);
   GET_CYCLE(timer2);
   while(timer2 - timer1 < 600){
      GET_CYCLE(timer2);
   }
   *gpclr = 1<<14;
   //Now wait before enabling voltage to the measurement system.
   while(timer2 - timer1 < 24000){
      GET_CYCLE(timer2);
   }
   *gpset = 1<<15;

   //re-enable interrupts now that the time-sensitive part is done.
   //interrupts(1);
 
   //Now we just need to wait for the dma to finish.
#ifdef DEBUG
   fprintf(stderr, "Waiting on DMA... "); 
#endif
   GET_CYCLE(timer1);
   GET_CYCLE(timer2);
   while((*(dma+dma_to_use*0x100) & 1) && timer2-timer1 < 125000) GET_CYCLE(timer2);
   //disable dma in case it didn't finish on its own.
   *(dma+dma_to_use*0x100) &= ~1;

#ifdef DEBUG
   fprintf(stderr, "done\n");
#endif
   
   //Disable voltage to the measurement system.
   *gpclr = 1<<15;

   //Now disable ADC clock
   *clk = CLK_PSWD | CLK_PLLD_SRC;
   
   //Pull measurements from input array
   unsigned *actual_measurements = (unsigned*)malloc(sizeof(unsigned)*600);
   unsigned num_measures = 0;
   for(int i = 0; i < 10000; i++){
      if(dma_dest[i] & 1<<5){
         printf("Here\n");
         actual_measurements[num_measures] = dma_dest[i];
         i++;
         while((i < 10000) && (dma_dest[i] & 1<<5)){
            actual_measurements[num_measures] = dma_dest[i];
            i++;
         }
         
         num_measures++;
      }
   }

  // Convert gpio pins to actual value.
  // Pins 6, 13, 19, 26, 12, 16, 20, and 21 are the ADC output DB pins (int order.)
   for(int i = 0; i < num_measures; i++){
      unsigned converted_val = ((actual_measurements[i] >> 6) & 1);
      converted_val |= (((actual_measurements[i] >> 13) & 1) << 1);
      converted_val |= (((actual_measurements[i] >> 19) & 1) << 2);
      converted_val |= (((actual_measurements[i] >> 26) & 1) << 3);
      converted_val |= (((actual_measurements[i] >> 12) & 1) << 4);
      converted_val |= (((actual_measurements[i] >> 16) & 1) << 5);
      converted_val |= (((actual_measurements[i] >> 20) & 1) << 6);
      converted_val |= (((actual_measurements[i] >> 21) & 1) << 7);
      
      printf("%u", ((actual_measurements[i] >> 6) & 1));
      printf("%u", ((actual_measurements[i] >> 13) & 1));
      printf("%u", ((actual_measurements[i] >> 19) & 1));
      printf("%u", ((actual_measurements[i] >> 26) & 1));
      printf("%u", ((actual_measurements[i] >> 12) & 1));
      printf("%u", ((actual_measurements[i] >> 16) & 1));
      printf("%u", ((actual_measurements[i] >> 20) & 1));
      printf("%u -> ", ((actual_measurements[i] >> 21) & 1));
      
      actual_measurements[i] = converted_val;

      double voltage = (double)converted_val/255 * 2;

      //For now, just print the value & timestamp.
      printf("%4.3f\n", voltage);
   }

   printf("Total measurements: %u\n", num_measures);
   
   close(m_pagemap_fd);

   return 0;
}
