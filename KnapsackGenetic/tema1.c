#include <stdlib.h>
#include "genetic_algorithm.h"
#include<pthread.h>
#include<stdio.h>

int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	int cores = atoi(argv[3]);

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, argc, argv)) {
		return 0;
	}

	var *parameters = calloc(cores, sizeof(var));
	pthread_t threads[cores];
    pthread_barrier_t barrier;
  	int r;
	pthread_barrier_init(&barrier, NULL, cores);
	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));

	
	for(int i=0;i<cores;i++)
	{
		parameters[i].thread_id = i;
		parameters[i].current_generation  = current_generation;
		parameters[i].next_generation = next_generation;

		parameters[i].objects = objects;
		parameters[i].object_count = object_count;
		parameters[i].sack_capacity = sack_capacity;
		parameters[i].generations_count = generations_count;
		parameters[i].nr_threads = cores;
		parameters[i].barrier = &barrier;
		r = pthread_create(&threads[i], NULL, run_genetic_algorithm,&parameters[i]);
		if (r) {
		    printf("Eroare la crearea thread-ului");
		exit(-1);
	    }
	}
	for(int i=0;i<cores;i++)
	{
		pthread_join(threads[i],NULL);
	}
	pthread_barrier_destroy(&barrier);
	free(objects);
	free_generation(current_generation);
	free_generation(next_generation);
	free(current_generation);
	free(next_generation);
	free(parameters);

	return 0;
}
