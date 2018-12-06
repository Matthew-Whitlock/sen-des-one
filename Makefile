.PHONY: all
.DEFAULT: all
all: register_level_gpio gui_java

register_level_gpio: register_level_gpio.c
	gcc -O3 register_level_gpio.c -o register_level_gpio

gui_java: SeniorD1GUI.java
	javac -cp ".:./jfreechart-1.0.19/lib/jfreechart-1.0.19.jar:./jfreechart-1.0.19/lib/jcommon-1.0.23.jar" SeniorD1GUI.java

clean:
	rm register_level_gpio
	rm *.class
