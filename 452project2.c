/* 
Need to do:
1. Fix the ramsied functionality--I do not think the logic is correct, this needs to happen after get some ingredients probably...
4. Make sure fulfills requirements
*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>

// Define constants
#define NUM_MIXERS 2
#define NUM_PANTRIES 1
#define NUM_REFRIGERATORS 2
#define NUM_BOWLS 3
#define NUM_SPOONS 5
#define NUM_OVENS 1
#define NUM_RECIPES 5

// Define semaphore variables
sem_t *mixer_sem, *pantry_sem, *refrigerator_sem, *bowl_sem, *spoon_sem, *oven_sem, *ramsay_sem;

// Define a mutex and condition variable
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Shared resources and their quantities
int ingredients[] = {1, 1, 1, 1, 1, 1, 1, 1, 1}; // Initial quantities
bool ingredient_acquired[] = {false, false, false, false, false, false, false, false, false};

int order_finished = 1;

// Baker structure
typedef struct {
    int id;
    char name[20];
    int recipes_baked;  // Counter variable
    int list_recipes_not_baked[5];
    bool ramsied;       // Flag for Ramsied status
} Baker;

// Ramsied baker ID
int ramsied_baker_id;
int ramsied_recipe_num;

pthread_mutex_t restart_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int restart_recipe = 0;

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

    // Prompt the user to input the number of bakers
    int NUM_BAKERS;
    printf("Enter the number of bakers: ");
    scanf("%d", &NUM_BAKERS);

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

    // Randomly select a baker to be Ramsied
    ramsied_baker_id = rand() % NUM_BAKERS + 1;

    // Randomly select a random recipe of that baker to be Ramsied
    ramsied_recipe_num = rand() % NUM_RECIPES;
    printf("*********Baker %d has been Ramsied on recipe %d!\n", ramsied_baker_id, ramsied_recipe_num);

    // Initialize baker threads
    pthread_t bakers[NUM_BAKERS];
    Baker baker_data[NUM_BAKERS];

    for (int i = 0; i < NUM_BAKERS; i++) {
        baker_data[i].id = i + 1;
        sprintf(baker_data[i].name, "Baker %d", i + 1);
        baker_data[i].recipes_baked = 1;  // Initialize the counter
        baker_data[i].ramsied = false;     // Initialize Ramsied status

        baker_data[i].list_recipes_not_baked[0] = 0; // Initialize the recipes not baked yet
        baker_data[i].list_recipes_not_baked[1] = 0; // Initialize the recipes not baked yet
        baker_data[i].list_recipes_not_baked[2] = 0; // Initialize the recipes not baked yet
        baker_data[i].list_recipes_not_baked[3] = 0; // Initialize the recipes not baked yet
        baker_data[i].list_recipes_not_baked[4] = 0; // Initialize the recipes not baked yet

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


    while (baker->recipes_baked < NUM_RECIPES + 1) {
        pthread_mutex_lock(&restart_mutex);
        if (restart_recipe) {
            printf("\033[1;%dm%s is restarting the current recipe...\033[0m\n", baker->id + 31, baker->name);

            // Reset Ramsied status
            baker->ramsied = true;

            // Clear the restart_recipe variable
            restart_recipe = 0;
        }
        pthread_mutex_unlock(&restart_mutex);



        int recipe_choice = rand() % 5;

        //If the first time through, pick one just randomly 
        //If not, need to only bake the recipes that are left over, not just any random!
        if (baker->ramsied) {
            recipe_choice = ramsied_recipe_num;
        } else {
            recipe_choice = rand() % 5;
            // If not Ramsied, update the list of recipes not baked yet
            while (baker->list_recipes_not_baked[recipe_choice] != 0) {
                recipe_choice = rand() % 5;
            }
            baker->list_recipes_not_baked[recipe_choice] = 1;
        }


        // Check if the baker is Ramsied
        printf("\n");
        //printf("Has this baker been ramsied? This baker is %d and the ramsied baker is %d and recipe is %d and ramsied recipe is %d\n", baker->id, ramsied_baker_id, recipe_choice, ramsied_recipe_num);
        
        if (baker->id == ramsied_baker_id && !baker->ramsied && recipe_choice == ramsied_recipe_num) {
            printf("\033[1;%d;31m%s has been Ramsied on recipe %d! Releasing semaphores and restarting the current recipe...\033[0m\n", baker->id, baker->name, ramsied_recipe_num);
            //printf("\033[1;%dm%s has been Ramsied! Releasing semaphores and restarting the current recipe...\033[0m\n", baker->id + 31, baker->name);

            // Release all semaphores
            sem_post(mixer_sem);
            sem_post(pantry_sem);
            sem_post(refrigerator_sem);
            sem_post(bowl_sem);
            sem_post(spoon_sem);
            sem_post(oven_sem);
            sem_post(ramsay_sem);

            // Set the restart_recipe variable to inform the Ramsied baker to restart
            pthread_mutex_lock(&restart_mutex);
            restart_recipe = 1;
            pthread_mutex_unlock(&restart_mutex);

            // The Ramsied baker will restart the recipe after acquiring semaphores
        }

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

    // Acquire ingredients from refrigerator based on the recipe
    if (strcmp(recipe, "Cookies") == 0 || strcmp(recipe, "Pancakes") == 0) {
        // Acquire refrigerator
        sem_wait(refrigerator_sem);

        printf("\033[1;%dm%s is in the refridgerator\033[0m\n", baker->id + 31, baker->name);
        acquire_ingredient("Egg", baker);
        acquire_ingredient("Milk", baker);
        acquire_ingredient("Butter", baker);
    }

    // Release refrigerator
    if (strcmp(recipe, "Cookies") == 0 || strcmp(recipe, "Pancakes") == 0) {
        sem_post(refrigerator_sem);

        printf("\033[1;%dm%s is out of the refridgerator\033[0m\n", baker->id + 31, baker->name);
    }

    // Acquire pantry
    sem_wait(pantry_sem);
    printf("\033[1;%dm%s is in the pantry\033[0m\n", baker->id + 31, baker->name);

    // Acquire ingredients from pantry based on the recipe
    acquire_ingredient("Flour", baker);
    acquire_ingredient("Sugar", baker);

    if (strcmp(recipe, "Cookies") != 0 && strcmp(recipe, "Pancakes") != 0) {
        acquire_ingredient("Yeast", baker);
        acquire_ingredient("Baking Soda", baker);
        acquire_ingredient("Salt", baker);
        acquire_ingredient("Cinnamon", baker);
    }

    // Release pantry
    sem_post(pantry_sem);
    printf("\033[1;%dm%s is out of the pantry\033[0m\n", baker->id + 31, baker->name);

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

    // Cook in the oven
    acquire_ingredient("Oven", baker);
    printf("\033[1;%dm%s is using the oven!\033[0m\n", baker->id + 31, baker->name);
    use_shared_resource("Oven", baker);

    // Finish cooking
    printf("\033[1;%dm%s has finished baking %s!\033[0m\n", baker->id + 31, baker->name, recipe);
    printf("\t\033[1;%dm%s has baked %d out of 5 recipes!\033[0m\n", baker->id + 31, baker->name, baker->recipes_baked);

    if (baker->recipes_baked == 5) {
        printf("\t\033[1;%dm%s has finished %d!\033[0m\n", baker->id + 31, baker->name, order_finished);
        order_finished++;
    }
    printf("\n");
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