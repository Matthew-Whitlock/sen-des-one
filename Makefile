.PHONY: all
.DEFAULT: all
all: register_level_gpio gui_java

register_level_gpio: register_level_gpio.c
	gcc -O3 register_level_gpio.c -o register_level_gpio

gui_java: SeniorD1GUI.java
	javac SeniorD1GUI.java

clean:
	rm register_level_gpio
	rm *.class
