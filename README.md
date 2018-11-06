# Senior Design One - Ultrasonic NDT w/ Raspberry Pi

This is the instruction set for configuring the Raspberry Pi to run - starting from a fresh, updated install of Raspbian.

### Install touchscreen drivers.

1. Run `git clone https://github.com/goodtft/LCD-show.git`
2. `chmod -R 755 LCD-show`
3. `cd LCD-show`
4. *IMPORTANT* Edit LCD5-show to remove the `reboot` command at the end of the file.
5. `./LCD5-show`
6. `sudo rm /etc/inittab` *Skipping this will prevent booting!*
7. `reboot`

And you should be good to go!

### Giving user access to the cycle count register

This is the most precise, and fastest-to-use clock available. First we will need to build and install a Loadable Kernel Module (LKM).

1. We need access to the kernel headers before we continue. If you have insalled a custom kernel, you should know where these are and should modify the makefile accordingly.  Otherwise, run `sudo apt install raspberrypi-kernel raspberrypi-kernel-headers`  
2. In the LKM directory, run `make`
3. (Optional) Test that the module loads correctly. Run `sudo insmod enable_ccr.ko` then `dmesg | tail`. You should see the line "User-level access to CCR has been turned on."
4. Next we need to get the module to load every time we boot. Copy the enable\_ccr.ko file to the folder `/lib/modules/4.14.62-v7+/kernel/drivers`. Modify the `4.14.62-v7+` folder to the name of your kernel, if needed.
5. Edit `/etc/modules` to add the line `enable_ccr` at the end.
6. Run `sudo depmod`
7. Verify that the module is being loaded by rebooting and running the command `lsmod | grep enable_ccr`. If you see no output, there is an error you will need to debug.


#### Configuring the RPI for a constant 1.2GHz operation

This is vital to get accurate readings based on the cycle count - if each cycle is a variable amount of time, cycle count is useless!

1. Edit the file `/etc/init.d/raspi-config`. Change the line that looks something like `echo "ondemand" > $SYS_CPUFREQ_GOVERNOR` to  `echo "performance" > $SYS_CPUFREQ_GOVERNOR`
2. Verify that the change took. Reboot and run `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq` - you should see 1200000.

### Configuring the bootline to section off CPU 2

Coming up next.
