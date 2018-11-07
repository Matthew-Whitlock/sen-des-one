register_level_gpio: register_level_gpio.c
	gcc -O3 register_level_gpio.c -o register_level_gpio

clean:
	rm register_level_gpio
