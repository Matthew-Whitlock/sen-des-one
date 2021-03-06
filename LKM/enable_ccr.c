#include <linux/module.h>
#include <linux/kernel.h>

void enable_ccr(void *info){ 
   asm volatile ("mcr p15, 0, %0, c9, c14, 0" :: "r" (1));
   asm volatile ("mcr p15, 0, %0, c9, c12, 0\t\n" :: "r" (1));
   asm volatile ("mcr p15, 0, %0, c9, c12, 1\t\n" :: "r" (0x80000000));
}

int init_module(void){
   on_each_cpu(enable_ccr,NULL,0);
   //asm volatile ("mcr p15, 0, %0, c15, c9, 0\n" : : "r" (1));
   printk (KERN_INFO "User-level access to CCR has been turned on.\n");
   return 0;
}

void cleanup_module(void){
}
