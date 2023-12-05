#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

// Define constants
#define NUM_BAKERS 3
#define NUM_MIXERS 2
#define NUM_PANTRIES 1
#define NUM_REFRIGERATORS 2
#define NUM_BOWLS 3
#define NUM_SPOONS 5
#define NUM_OVENS 1

// Define semaphore variables
sem_t *mixer_sem, *pantry_sem, *refrigerator_sem, *bowl_sem, *spoon_sem, *oven_sem, *ramsay_sem;

// Define a mutex and condition variable
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Shared resources and their quantities
int ingredients[] = {1, 1, 1, 1, 1, 1, 1, 1, 1}; // Initial quantities
bool ingredient_acquired[] = {false, false, false, false, false, false, false, false, false};

// Baker structure
typedef struct {
    int id;
    char name[20];
    int recipes_baked;  // New counter variable
} Baker;

// Function prototypes
void *baker_thread(void *arg);
void acquire_ingredient(const char *ingredient, Baker *baker);
void use_shared_resource(const char *resource_name, Baker *baker);
void cook_recipe(const char *recipe, Baker *baker);
void signal_handler(int signo);
int ingredient_index(const char *ingredient);

pthread_mutex_t ingredients_mutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
    srand(time(NULL));

    // Set up the signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    // Initialize semaphores
    mixer_sem = sem_open("/mixer_sem", O_CREAT, 0644, NUM_MIXERS);
    pantry_sem = sem_open("/pantry_sem", O_CREAT, 0644, NUM_PANTRIES);
    refrigerator_sem = sem_open("/refrigerator_sem", O_CREAT, 0644, NUM_REFRIGERATORS);
    bowl_sem = sem_open("/bowl_sem", O_CREAT, 0644, NUM_BOWLS);
    spoon_sem = sem_open("/spoon_sem", O_CREAT, 0644, NUM_SPOONS);
    oven_sem = sem_open("/oven_sem", O_CREAT, 0644, NUM_OVENS);
    ramsay_sem = sem_open("/ramsay_sem", O_CREAT, 0644, 1);

    // Initialize baker threads
    pthread_t bakers[NUM_BAKERS];
    Baker baker_data[NUM_BAKERS];

    for (int i = 0; i < NUM_BAKERS; i++) {
        baker_data[i].id = i + 1;
        sprintf(baker_data[i].name, "Baker %d", i + 1);
        baker_data[i].recipes_baked = 0;  // Initialize the counter
        pthread_create(&bakers[i], NULL, baker_thread, &baker_data[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_BAKERS; i++) {
        pthread_join(bakers[i], NULL);
    }

    // Destroy semaphores
    sem_t *semaphores[] = {mixer_sem, pantry_sem, refrigerator_sem, bowl_sem, spoon_sem, oven_sem, ramsay_sem};

    for (int i = 0; i < sizeof(semaphores) / sizeof(semaphores[0]); i++) {
        sem_destroy(semaphores[i]);
    }

    return 0;
}

// Signal handler function
void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("\nReceived Ctrl+C signal. Cleaning up...\n");

        // Optionally, you may choose to unlink named semaphores
        sem_unlink("/mixer_sem");
        sem_unlink("/pantry_sem");
        sem_unlink("/refrigerator_sem");
        sem_unlink("/bowl_sem");
        sem_unlink("/spoon_sem");
        sem_unlink("/oven_sem");
        sem_unlink("/ramsay_sem");

        // Exit the program gracefully
        exit(EXIT_SUCCESS);
    }
}

void *baker_thread(void *arg) {
    Baker *baker = (Baker *)arg;

    while (baker->recipes_baked < 5) {  // Change 5 to the total number of recipes
        int recipe_choice = rand() % 5;  // Declare recipe_choice here

        // Existing code...
        switch (recipe_choice) {
            case 0:
                cook_recipe("Cookies", baker);
                break;
            case 1:
                cook_recipe("Pancakes", baker);
                break;
            case 2:
                cook_recipe("Homemade Pizza Dough", baker);
                break;
            case 3:
                cook_recipe("Soft Pretzels", baker);
                break;
            case 4:
                cook_recipe("Cinnamon Rolls", baker);
                break;
        }

        baker->recipes_baked++;  // Increment the counter
    }

    pthread_exit(NULL);
}

void acquire_ingredient(const char *ingredient, Baker *baker) {
    int index = ingredient_index(ingredient);
    if (index >= 0) {
        pthread_mutex_lock(&ingredients_mutex);
        if (ingredients[index] > 0) {
            printf("\033[1;%dm%s is acquiring %s...\033[0m\n", baker->id + 31, baker->name, ingredient);
            while (ingredients[index] <= 0) {
                usleep(1000);
            }
            ingredients[index]--;
            printf("\033[1;%dm%s has acquired %s!\033[0m\n", baker->id + 31, baker->name, ingredient);
        }
        pthread_mutex_unlock(&ingredients_mutex);
    }
}

void use_shared_resource(const char *resource_name, Baker *baker) {
    if (resource_name != NULL) {
        int index = ingredient_index(resource_name);
        if (index >= 0) {
            pthread_mutex_lock(&ingredients_mutex);
            ingredients[index]++;
            pthread_mutex_unlock(&ingredients_mutex);
        }
    }
}


void cook_recipe(const char *recipe, Baker *baker) {
    printf("\033[1;%dm%s is cooking %s...\033[0m\n", baker->id + 31, baker->name, recipe);

    // Acquire ingredients
    acquire_ingredient("Flour", baker);
    acquire_ingredient("Sugar", baker);

    if (strcmp(recipe, "Cookies") == 0 || strcmp(recipe, "Pancakes") == 0) {
        acquire_ingredient("Milk", baker);
        acquire_ingredient("Butter", baker);
    }

    if (strcmp(recipe, "Pancakes") == 0) {
        acquire_ingredient("Baking Soda", baker);
        acquire_ingredient("Salt", baker);
        acquire_ingredient("Egg", baker);
    }

    if (strcmp(recipe, "Homemade Pizza Dough") == 0) {
        acquire_ingredient("Yeast", baker);
        acquire_ingredient("Sugar", baker);
        acquire_ingredient("Salt", baker);
    }

    if (strcmp(recipe, "Soft Pretzels") == 0) {
        acquire_ingredient("Yeast", baker);
        acquire_ingredient("Sugar", baker);
        acquire_ingredient("Salt", baker);
        acquire_ingredient("Baking Soda", baker);
        acquire_ingredient("Egg", baker);
    }

    if (strcmp(recipe, "Cinnamon Rolls") == 0) {
        acquire_ingredient("Yeast", baker);
        acquire_ingredient("Sugar", baker);
        acquire_ingredient("Salt", baker);
        acquire_ingredient("Butter", baker);
        acquire_ingredient("Egg", baker);
        acquire_ingredient("Cinnamon", baker);
    }

    // Acquire shared resources
    acquire_ingredient("Bowl", baker);
    acquire_ingredient("Spoon", baker);
    acquire_ingredient("Mixer", baker);

    // Use ingredients and resources
    use_shared_resource("Flour", baker);
    use_shared_resource("Sugar", baker);
    if (strcmp(recipe, "Cookies") == 0 || strcmp(recipe, "Pancakes") == 0) {
        use_shared_resource("Milk", baker);
        use_shared_resource("Butter", baker);
    }
    if (strcmp(recipe, "Pancakes") == 0) {
        use_shared_resource("Baking Soda", baker);
        use_shared_resource("Salt", baker);
        use_shared_resource("Egg", baker);
    }
    if (strcmp(recipe, "Homemade Pizza Dough") == 0) {
        use_shared_resource("Yeast", baker);
        use_shared_resource("Sugar", baker);
        use_shared_resource("Salt", baker);
    }
    if (strcmp(recipe, "Soft Pretzels") == 0) {
        use_shared_resource("Yeast", baker);
        use_shared_resource("Sugar", baker);
        use_shared_resource("Salt", baker);
        use_shared_resource("Baking Soda", baker);
        use_shared_resource("Egg", baker);
    }
    if (strcmp(recipe, "Cinnamon Rolls") == 0) {
        use_shared_resource("Yeast", baker);
        use_shared_resource("Sugar", baker);
        use_shared_resource("Salt", baker);
        use_shared_resource("Butter", baker);
        use_shared_resource("Egg", baker);
        use_shared_resource("Cinnamon", baker);
    }

    use_shared_resource("Bowl", baker);
    use_shared_resource("Spoon", baker);
    use_shared_resource("Mixer", baker);

    // Cook in the oven
    acquire_ingredient("Oven", baker);
    use_shared_resource("Oven", baker);

    // Finish cooking
    printf("\033[1;%dm%s has finished cooking %s!\033[0m\n", baker->id + 31, baker->name, recipe);
}

int ingredient_index(const char *ingredient) {
    if (strcmp(ingredient, "Flour") == 0) return 0;
    if (strcmp(ingredient, "Sugar") == 0) return 1;
    if (strcmp(ingredient, "Yeast") == 0) return 2;
    if (strcmp(ingredient, "Baking Soda") == 0) return 3;
    if (strcmp(ingredient, "Salt") == 0) return 4;
    if (strcmp(ingredient, "Cinnamon") == 0) return 5;
    if (strcmp(ingredient, "Egg") == 0) return 6;
    if (strcmp(ingredient, "Milk") == 0) return 7;
    if (strcmp(ingredient, "Butter") == 0) return 8;
    return -1; // Invalid ingredient
}



/*
Please note that this code uses ANSI escape codes for colored terminal output. The \033[1;%dm and \033[0m are used 
to set the text color for each baker. The rand() function is used to simulate some processing time and randomness 
in acquiring ingredients.

Additionally, this code includes a chance for a baker to be "Ramsied," where all semaphores are released, 
and the baker starts their current recipe from the beginning. This is triggered with a 5% probability.

You may need to adjust this code based on your specific requirements and add error handling as needed. 
Also, be sure to compile it with the appropriate flags, such as -pthread.
*/




//imports

// struct for bakers to include current recipe and status

//prompt user for how many bakers

//create n bakers which are threads 

// probably make arrays for the recepie and ingredients gatherd for it
//compare the two to check if all ingredients are obtained to start baking process

//first task is to get these ingredients (each has a semiphore)
//each baker chooses a recipe randomly from their list?

//could do if want to get an ingredient and see being used, grab next ingredient, if none

//once a full recipe list is obtained, then go to get in line for bowl, spoon, mixer

//once mixed, get in line for oven

//announce finished that recipe
//take that recipe off that baker's TO DO list

//if recipe list empty, announce DONE
//if not empty, then move on to getting next ingredients for next recipe 



//Ramsied Functionality:
//Free all the current semiphores of a SPECIFIC baker (he said could be the same baker every time)
//make sure their arrays and lists are updated appropriately 


//Total ingredients list: Flour, Sugar, Milk, Butter, Baking Soda, Salt, Egg, Yeast, Cinnamon





/*
For this project you will be using semaphores, threads and shared memory.
Each baker will run as its own thread. Access to each resource will require a counting/binary
semaphore as appropriate.
The program should prompt the user for a number of bakers. Each baker will be competing for
the resources to create each of the recipes.

Each baker is in a shared kitchen with the following resources:
Mixer - 2
Pantry – 1
Refrigerator - 2
Bowl – 3
Spoon – 5
Oven – 1

Only one baker may ‘access’ an ingredient at a time.
Ingredients available in the pantry:
1. Flour
2. Sugar
3. Yeast
4. Baking Soda
5. Salt
6. Cinnamon

Ingredients available in the refrigerator:
1. Egg(s)
2. Milk
3. Butter

Only one baker may be in the pantry at a time.
Two bakers may be in the refrigerator at a time.

To ‘bake’ a recipe, the baker must acquire each of the ingredients listed below:
Cookies: Flour, Sugar, Milk, Butter
Pancakes: Flour, Sugar, Baking soda, Salt, Egg, Milk, Butter
Homemade pizza dough: Yeast, Sugar, Salt
Soft Pretzels: Flour, Sugar, Salt, Yeast, Baking Soda, Egg
Cinnamon rolls: Flour, Sugar, Salt, Butter, Eggs, Cinnamon


Once all the ingredients have been acquired: a bowl, a spoon and a mixer must be acquired to
mix the ingredients together. After they have been mixed together, it must be cooked in the
oven.

Each baker must complete (and cook) each of the recipes once and then announce they have
finished.

Each baker must attempt to complete each item as soon as possible (don’t put them to sleep).
Output to the terminal what each of the bakers are doing in real time. The output from each
baker should be a different color.

You may use System V semaphores or POSIX semaphores.

Assign one of the bakers to have a chance to be ‘Ramsied’, this means they must release all
semaphores and start their current recipe from the beginning. This must be programmed to
occur every time the program is run.
*/