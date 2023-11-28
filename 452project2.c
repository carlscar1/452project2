// this is what I have worked through with chatgpt and troubleshooting. 
// semaphores are weird on macOS so those functions had to be changed to work on mine
// I am not sure if that will entirely mess up how it works for you...

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

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

// Shared resources
int flour = 1, sugar = 1, yeast = 1, baking_soda = 1, salt = 1, cinnamon = 1;
int eggs = 1, milk = 1, butter = 1;

// Baker structure
typedef struct {
    int id;
    char name[20];
} Baker;

// Function prototypes
void *baker_thread(void *arg);
void acquire_ingredient(const char *ingredient, sem_t *semaphore, Baker *baker);
void use_shared_resource(int *resource, const char *resource_name, sem_t *semaphore, Baker *baker);
void cook_recipe(const char *recipe, Baker *baker);

// Function prototype for the signal handler
void signal_handler(int signo);

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

        // Destroy semaphores
        sem_t *semaphores[] = {mixer_sem, pantry_sem, refrigerator_sem, bowl_sem, spoon_sem, oven_sem, ramsay_sem};

        for (int i = 0; i < sizeof(semaphores) / sizeof(semaphores[0]); i++) {
            sem_destroy(semaphores[i]);
        }

        // Optionally, you may choose to exit the program gracefully
        exit(EXIT_SUCCESS);
    }
}

void *baker_thread(void *arg) {
    Baker *baker = (Baker *)arg;

    while (1) {
        // Attempt to acquire Ramsay semaphore
        if (rand() % 100 < 5) {
            sem_wait(ramsay_sem);
            printf("\033[1;%dm%s has been Ramsied!\033[0m\n", baker->id + 31, baker->name);

            // Release all semaphores
            sem_post(mixer_sem);
            sem_post(pantry_sem);
            sem_post(refrigerator_sem);
            sem_post(bowl_sem);
            sem_post(spoon_sem);
            sem_post(oven_sem);

            // Release Ramsay semaphore
            sem_post(ramsay_sem);
        }

        // Choose a random recipe to cook
        int recipe_choice = rand() % 5;
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
    }
}

void acquire_ingredient(const char *ingredient, sem_t *semaphore, Baker *baker) {
    printf("\033[1;%dm%s is acquiring %s...\033[0m\n", baker->id + 31, baker->name, ingredient);
    sem_wait(semaphore);
    printf("\033[1;%dm%s has acquired %s!\033[0m\n", baker->id + 31, baker->name, ingredient);
}

void use_shared_resource(int *resource, const char *resource_name, sem_t *semaphore, Baker *baker) {
    printf("\033[1;%dm%s is using %s...\033[0m\n", baker->id + 31, baker->name, resource_name);
    // Simulate some processing time
    usleep(rand() % 1000000);
    printf("\033[1;%dm%s has finished using %s!\033[0m\n", baker->id + 31, baker->name, resource_name);
    sem_post(semaphore);
}

void cook_recipe(const char *recipe, Baker *baker) {
    printf("\033[1;%dm%s is cooking %s...\033[0m\n", baker->id + 31, baker->name, recipe);

    // Acquire ingredients
    acquire_ingredient("Flour", pantry_sem, baker);
    acquire_ingredient("Sugar", pantry_sem, baker);

    if (strcmp(recipe, "Cookies") == 0 || strcmp(recipe, "Pancakes") == 0) {
        acquire_ingredient("Milk", refrigerator_sem, baker);
        acquire_ingredient("Butter", refrigerator_sem, baker);
    }

    if (strcmp(recipe, "Pancakes") == 0) {
        acquire_ingredient("Baking Soda", pantry_sem, baker);
        acquire_ingredient("Salt", pantry_sem, baker);
        acquire_ingredient("Egg", refrigerator_sem, baker);
    }

    if (strcmp(recipe, "Homemade Pizza Dough") == 0) {
        acquire_ingredient("Yeast", pantry_sem, baker);
        acquire_ingredient("Sugar", pantry_sem, baker);
        acquire_ingredient("Salt", pantry_sem, baker);
    }

    if (strcmp(recipe, "Soft Pretzels") == 0) {
        acquire_ingredient("Yeast", pantry_sem, baker);
        acquire_ingredient("Sugar", pantry_sem, baker);
        acquire_ingredient("Salt", pantry_sem, baker);
        acquire_ingredient("Baking Soda", pantry_sem, baker);
        acquire_ingredient("Egg", refrigerator_sem, baker);
    }

    if (strcmp(recipe, "Cinnamon Rolls") == 0) {
        acquire_ingredient("Yeast", pantry_sem, baker);
        acquire_ingredient("Sugar", pantry_sem, baker);
        acquire_ingredient("Salt", pantry_sem, baker);
        acquire_ingredient("Butter", pantry_sem, baker);
        acquire_ingredient("Egg", refrigerator_sem, baker);
        acquire_ingredient("Cinnamon", pantry_sem, baker);
    }

    // Acquire shared resources
    acquire_ingredient("Bowl", bowl_sem, baker);
    acquire_ingredient("Spoon", spoon_sem, baker);
    acquire_ingredient("Mixer", mixer_sem, baker);

    // Use ingredients and resources
    use_shared_resource(&flour, "Flour", pantry_sem, baker);
    use_shared_resource(&sugar, "Sugar", pantry_sem, baker);
    if (strcmp(recipe, "Cookies") == 0 || strcmp(recipe, "Pancakes") == 0) {
        use_shared_resource(&milk, "Milk", refrigerator_sem, baker);
        use_shared_resource(&butter, "Butter", refrigerator_sem, baker);
    }
    if (strcmp(recipe, "Pancakes") == 0) {
        use_shared_resource(&baking_soda, "Baking Soda", pantry_sem, baker);
        use_shared_resource(&salt, "Salt", pantry_sem, baker);
        use_shared_resource(&eggs, "Egg", refrigerator_sem, baker);
    }
    if (strcmp(recipe, "Homemade Pizza Dough") == 0) {
        use_shared_resource(&yeast, "Yeast", pantry_sem, baker);
        use_shared_resource(&sugar, "Sugar", pantry_sem, baker);
        use_shared_resource(&salt, "Salt", pantry_sem, baker);
    }
    if (strcmp(recipe, "Soft Pretzels") == 0) {
        use_shared_resource(&yeast, "Yeast", pantry_sem, baker);
        use_shared_resource(&sugar, "Sugar", pantry_sem, baker);
        use_shared_resource(&salt, "Salt", pantry_sem, baker);
        use_shared_resource(&baking_soda, "Baking Soda", pantry_sem, baker);
        use_shared_resource(&eggs, "Egg", refrigerator_sem, baker);
    }
    if (strcmp(recipe, "Cinnamon Rolls") == 0) {
        use_shared_resource(&yeast, "Yeast", pantry_sem, baker);
        use_shared_resource(&sugar, "Sugar", pantry_sem, baker);
        use_shared_resource(&salt, "Salt", pantry_sem, baker);
        use_shared_resource(&butter, "Butter", pantry_sem, baker);
        use_shared_resource(&eggs, "Egg", refrigerator_sem, baker);
        use_shared_resource(&cinnamon, "Cinnamon", pantry_sem, baker);
    }

    use_shared_resource(NULL, "Bowl", bowl_sem, baker);
    use_shared_resource(NULL, "Spoon", spoon_sem, baker);
    use_shared_resource(NULL, "Mixer", mixer_sem, baker);

    // Cook in the oven
    acquire_ingredient("Oven", oven_sem, baker);
    use_shared_resource(NULL, "Oven", oven_sem, baker);

    // Finish cooking
    printf("\033[1;%dm%s has finished cooking %s!\033[0m\n", baker->id + 31, baker->name, recipe);
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