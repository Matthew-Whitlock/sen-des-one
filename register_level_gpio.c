/*
// Pin 12 is an output pin, for exciting the probe.
// Pin 4 is a GPIOclk pin for triggering the ADC.
// Pins 6, 13, 19, 26, 12, 16, 20, and 21 are the ADC output DB pins (int order.)
// Pin 5 is the EOC pin of the ADC.
*/

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
#define PWM_BASE           (BCM2708_PERI_BASE + 0x20C000)

#define GET_CYCLE(v) asm volatile ("mrc p15, 0, %0, c9, c13, 0":"=r" (v));

#define PWM_CLK_OFFSET  (0xC)
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

  clk_map =  mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,CLK_BASE);

  int_map =  mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,memfd,INTERUPT_BASE);

  close(memfd);

  if(gpio_map == MAP_FAILED  || clk_map == MAP_FAILED 
     || clk_map == MAP_FAILED) {
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
  //for convenience, we can also setup pin 4 
  //to output the GPIO clock output
  //Function select register for 4 are in the 
  //first register of gpio
  //Mask function selects of pin 4 to 0
  *gpio &= ~(7<<12);
  //Now set pin 4 to alt function 0
  *gpio |= 4<<12;

  //Now set pin 2 to output, for exciting the transducer 
  *(gpio) &= ~(7<<3);
  *(gpio) |= (1<<3);


  //Set the ADC input pins as inputs.
  // Pins 6, 13, 19, 26, 12, 16, 20, and 21 are the ADC output DB pins (int order.)
  // Pin 5 is the EOC pin of the ADC.
  *gpio &= ~(0x3F << 15); //5/6 as inputs
  *(gpio+1) &= ~(0x3F << 6); //12/13
  *(gpio+1) &= ~(0x7 << 18); //16
  *(gpio+1) &= ~(0x7 << 27); //19
  *(gpio+2) &= ~(0x3F); //20/21
  *(gpio+2) &= ~(0x7 << 18); //26


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
   unsigned timer1, timer2, measurements = 0, tmp_measure, tmp_measure_two;
   //format: GPIO pins 0, timer0, GPIO pins 1, timer1, ...
   unsigned *measurements_arr = (unsigned*)malloc(200*sizeof(unsigned));
   setup();

   //disable interrupts while we do this.
   while(!interrupts(0));

   //Configure and enable ADC clock
   //First, turn off the gpio clock.
   *clk &= ~CLK_PSWD_MASK;
   *clk &= ~CLK_ENABLE;
	*clk |= CLK_PSWD;
	while((*clk & CLK_BUSY)){
      //Do nothing - we're waiting for the clock to safely stop.
   }

   //Now configure clock scaling frequency
   *(clk+1) = CLK_PSWD | (278<<12); //~1.8MHz
   *clk =     CLK_PSWD | CLK_PLLD_SRC;
   *clk =     CLK_PSWD | CLK_PLLD_SRC | CLK_ENABLE;
   
   //Wait a couple cycles to be sure the clock starts
   GET_CYCLE(timer1);
   GET_CYCLE(timer2);
   while(timer2 - timer1 < 1400){
      GET_CYCLE(timer2);
   }


   //ADC clock is now running. Lets excite the transducer.
   *gpset = 1<<12;
   *gpclr = 1<<12;
   
   GET_CYCLE(timer1);
   GET_CYCLE(timer2);
   //Now we just need to take lots of measurements.
   while((timer2 - timer1 < 55000)){
      tmp_measure = *gpin;
      while(!(tmp_measure & (1<<5))) tmp_measure = *gpin; //Wait for EOC bit to be 1
      tmp_measure_two = *gpin;
      while(tmp_measure_two & 1<<5){ //Take the last measurement available.
         tmp_measure = tmp_measure_two;
         tmp_measure_two = *gpin;
      }

      measurements_arr[measurements*2] = tmp_measure;
      GET_CYCLE(measurements_arr[measurements*2 + 1]);
      measurements++;
      GET_CYCLE(timer2);
   }
   
   //re-enable interrupts now that the time-sensitive part is done.
   interrupts(1);


  // Convert gpio pins to actual value.
  // Pins 6, 13, 19, 26, 12, 16, 20, and 21 are the ADC output DB pins (int order.)
   for(int i = 0; i < measurements; i++){
      unsigned converted_val = ((measurements_arr[2*i] >> 6) & 1);
      converted_val |= (((measurements_arr[2*i] >> 13) & 1) << 1);
      converted_val |= (((measurements_arr[2*i] >> 19) & 1) << 2);
      converted_val |= (((measurements_arr[2*i] >> 26) & 1) << 3);
      converted_val |= (((measurements_arr[2*i] >> 12) & 1) << 4);
      converted_val |= (((measurements_arr[2*i] >> 16) & 1) << 5);
      converted_val |= (((measurements_arr[2*i] >> 20) & 1) << 6);
      converted_val |= (((measurements_arr[2*i] >> 21) & 1) << 7);
      
      printf("%u", ((measurements_arr[2*i] >> 6) & 1));
      printf("%u", ((measurements_arr[2*i] >> 13) & 1));
      printf("%u", ((measurements_arr[2*i] >> 19) & 1));
      printf("%u", ((measurements_arr[2*i] >> 26) & 1));
      printf("%u", ((measurements_arr[2*i] >> 12) & 1));
      printf("%u", ((measurements_arr[2*i] >> 16) & 1));
      printf("%u", ((measurements_arr[2*i] >> 20) & 1));
      printf("%u -> ", ((measurements_arr[2*i] >> 21) & 1));
      
      measurements_arr[2*i] = converted_val;

      double voltage = (double)converted_val/255 * 2;

      //For now, just print the value & timestamp.
      printf("%4.3f @ %u\n", voltage, measurements_arr[2*i + 1] - measurements_arr[1]);
   }

   printf("Total measurements: %u\n", measurements);

   //Now disable ADC clock
   *clk = CLK_PSWD | CLK_PLLD_SRC;
   
   return 0;
}
