# APD - Tema 1
# Octombrie 2021

build:
	@echo "Building..."
	@gcc -o tema1_par tema1.c genetic_algorithm.c -lm -Wall -Werror -lpthread
	@echo "Done"

build_debug:
	@echo "Building debug..."
	@gcc -o tema1_par tema1.c genetic_algorithm.c -lm -Wall -Werror -O0 -g3 -DDEBUG -lpthread
	@echo "Done"

clean:
	@echo "Cleaning..."
	@rm -rf tema1
	@echo "Done"
