#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))


int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 3) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));
	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int start, int end, int sack_capacity)
{
	int weight;
	int profit;

	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;
		for (int j = 0; j <generation[i].chromosome_length ; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {

		res = first->sum - second->sum; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}


void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}


void *run_genetic_algorithm(void *par)
{
	int count, cursor;
	var *par1=(var*)par;
	int generations_count = par1->generations_count;
    int object_count = par1->object_count;
    int sack_capacity = par1->sack_capacity;
	int nr_threads = par1->nr_threads;
	int thread_id = par1->thread_id;
	individual *current_generation = par1->current_generation;
	individual *next_generation = par1->next_generation;
	sack_object *objects = par1->objects;

	individual *tmp = NULL;

	int start = thread_id * (double)object_count / nr_threads;
	int end = min((thread_id + 1) * (double)object_count / nr_threads, object_count);

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = start; i < end; ++i) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = object_count;
	}
	pthread_barrier_wait(par1->barrier);

	// iterate for each generation
	for (int k = 0; k < generations_count; ++k) {
		
		
		cursor = 0;

		// compute fitness and sort by it
		int start1 = thread_id * (double)object_count / nr_threads;
		int end1 = min((thread_id + 1) * (double)object_count / nr_threads, object_count);
		compute_fitness_function(objects, current_generation, start1, end1, sack_capacity);
		
		pthread_barrier_wait(par1->barrier);
		int start2 = thread_id * (double)object_count / nr_threads;
		int end2 = min((thread_id + 1) * (double)object_count / nr_threads, object_count);
		for(int i = start2; i < end2; i++)
		{
				int suma = 0;
				for (int j = 0; j < current_generation[i].chromosome_length; ++j) {
					suma+=current_generation[i].chromosomes[j];
				}
				current_generation[i].sum=suma;
		}
		pthread_barrier_wait(par1->barrier);	
		if(thread_id==0){
			qsort(current_generation, object_count, sizeof(individual), cmpfunc);
		}
		pthread_barrier_wait(par1->barrier);
		// keep first 30% children (elite children selection)
		count = object_count * 3 / 10;
		int start3 = thread_id * (double)count / nr_threads;
		int end3 = min((thread_id + 1) * (double)count / nr_threads, count);
		for (int i = start3; i < end3; ++i) {
			copy_individual(current_generation + i, next_generation + i);
		}
		pthread_barrier_wait(par1->barrier);
		cursor = count;
		// mutate first 20% children with the first version of bit string mutation
		count = object_count * 2 / 10;
		int start4 = thread_id * (double)count / nr_threads;
		int end4 = min((thread_id + 1) * (double)count / nr_threads, count);
		for (int i = start4; i < end4; ++i) {
			copy_individual(current_generation + i, next_generation + cursor + i);
			if(thread_id==0){
			mutate_bit_string_1(next_generation + cursor + i, k);
			}
		}
		pthread_barrier_wait(par1->barrier);
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = object_count * 2 / 10;
		int start5 = thread_id * (double)count / nr_threads;
		int end5 = min((thread_id + 1) * (double)count / nr_threads, count);
		for (int i = start5; i < end5; ++i) {
			copy_individual(current_generation + i + count, next_generation + cursor + i);
			mutate_bit_string_2(next_generation + cursor + i, k);
		}
		pthread_barrier_wait(par1->barrier);
		cursor += count;

		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = object_count * 3 / 10;

		if (count % 2 == 1) {
			if (thread_id == 0){
			copy_individual(current_generation + object_count - 1, next_generation + cursor + count - 1);
			}
			count--;
		}
		count /=2;
		int start6 = thread_id * (double)count / nr_threads;
		int end6 = min((thread_id + 1) * (double)count / nr_threads, count);
		for (int i = start6; i < end6; i ++) {
			crossover(current_generation + 2*i, next_generation + cursor + 2*i, k);
		}
		pthread_barrier_wait(par1->barrier);
		// switch to new generation
		tmp = current_generation;
		current_generation = next_generation;
		next_generation = tmp;

		int start7 = thread_id * (double)object_count / nr_threads;
		int end7 = min((thread_id + 1) * (double)object_count / nr_threads, object_count);
		for (int i = start7; i < end7; ++i) {
			current_generation[i].index = i;
		}

		if (k % 5 == 0) {
			if (thread_id == 0){
			print_best_fitness(current_generation);
			}
		}
		pthread_barrier_wait(par1->barrier);
	}
	
	pthread_barrier_wait(par1->barrier);
	int start8 = thread_id * (double)object_count / nr_threads;
	int end8 = min((thread_id + 1) * (double)object_count / nr_threads, object_count);

	compute_fitness_function(objects, current_generation, start8, end8 , sack_capacity);
	
	pthread_barrier_wait(par1->barrier);
	int start9 = thread_id * (double)object_count / nr_threads;
	int end9 = min((thread_id + 1) * (double)object_count / nr_threads, object_count);
	for(int i=start9;i<end9;i++)
	{
				int suma = 0;
				for (int j = 0; j < current_generation[i].chromosome_length; ++j) {
					suma+=current_generation[i].chromosomes[j];
				}
				current_generation[i].sum=suma;
	}
	pthread_barrier_wait(par1->barrier);	
	if(thread_id==0){
		qsort(current_generation, object_count, sizeof(individual), cmpfunc);
		print_best_fitness(current_generation);
	}
	return NULL;
}