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

### Configuring the bootline to section off CPU 2

Coming up next.
