#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h> 
#include <semaphore.h> 
#include <unistd.h> 
#include <time.h> 
 
#define MAX_ROW_CAPACITY 6 
#define MAX_GROUPS_TO_CHECK 3 
#define MAX_QUEUE_SIZE 100 
 
sem_t mutex; 
int group_queue[MAX_QUEUE_SIZE]; 
int front = 0, rear = 0; 
int single_rider_count = 0; 
 
void enqueue_group(int size) { 
    if ((rear + 1) % MAX_QUEUE_SIZE != front) { 
        group_queue[rear] = size; 
        rear = (rear + 1) % MAX_QUEUE_SIZE; 
    } 
} 
 
int dequeue_group() { 
    if (front != rear) { 
        int size = group_queue[front]; 
        front = (front + 1) % MAX_QUEUE_SIZE; 
        return size; 
    } 
    return -1; 
} 
 
int peek_group(int index) { 
    if ((rear + MAX_QUEUE_SIZE - front) % MAX_QUEUE_SIZE > index) { 
        int pos = (front + index) % MAX_QUEUE_SIZE; 
        return group_queue[pos]; 
    } 
    return -1; 
} 
 
void remove_groups(int n) { 
    front = (front + n) % MAX_QUEUE_SIZE; 
} 
 
int queue_size() { 
    return (rear + MAX_QUEUE_SIZE - front) % MAX_QUEUE_SIZE; 
} 
 
void* group_generator(void* arg) { 
    while (1) { 
        int group_size = (rand() % 2 == 0) ? 2 : 3; 
        sem_wait(&mutex); 
        enqueue_group(group_size); 
        printf("Group of %d added to normal queue\n", group_size); 
        sem_post(&mutex); 
        sleep(1); 
    } 
    return NULL; 
} 
 
void* single_rider_generator(void* arg) { 
    while (1) { 
        sem_wait(&mutex); 
        single_rider_count++; 
        printf("Single rider added to single rider queue\n"); 
        sem_post(&mutex); 
        sleep(2); 
    } 
    return NULL; 
} 
 
void* cart_dispatcher(void* arg) { 
    while (1) { 
        sleep(5); 
 
        sem_wait(&mutex); 
        int seats_left = MAX_ROW_CAPACITY; 
        int row[6] = {0}; 
        int filled = 0; 
 
        int combos[MAX_GROUPS_TO_CHECK] = {0}; 
        int group_count = queue_size(); 
        int used_groups = 0; 
 
        if (group_count > 0) { 
            int g[MAX_GROUPS_TO_CHECK]; 
            for (int i = 0; i < MAX_GROUPS_TO_CHECK && i < group_count; i++) { 
                g[i] = peek_group(i); 
            } 
 
            //3+3 
            if (g[0] == 3 && g[1] == 3) { 
                row[filled++] = 3; row[filled++] = 3; 
                seats_left -= 6; 
                used_groups = 2; 
            } 
            //2+2+2 
            else if (g[0] == 2 && g[1] == 2 && g[2] == 2) { 
                row[filled++] = 2; row[filled++] = 2; row[filled++] = 2; 
                seats_left -= 6; 
                used_groups = 3; 
            } 
            //2+3 or 3+2 
            else if ((g[0] == 2 && g[1] == 3) || (g[0] == 3 && g[1] == 2)) { 
                row[filled++] = g[0]; 
                row[filled++] = g[1]; 
                seats_left -= (g[0] + g[1]); 
                used_groups = 2; 
            } 
            //one 
            else if (g[0] <= MAX_ROW_CAPACITY) { 
                row[filled++] = g[0]; 
                seats_left -= g[0]; 
                used_groups = 1; 
            } 
        } 
 
        remove_groups(used_groups); 
 
        while (seats_left > 0 && single_rider_count > 0) { 
            row[filled++] = 1; 
            seats_left--; 
            single_rider_count--; 
        } 
 
        if (used_groups == 0 && single_rider_count == 0 && queue_size() == 0) { 
            sem_post(&mutex); 
            continue; 
        } 
 
        printf("\nCart departed with row: "); 
  
