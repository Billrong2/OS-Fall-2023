// libraries
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
// define max number of students
const int max_student_number = 2000;

// defining thread global variables.
int chair_total = 0; // one of the input argc
int help_need_total = 0; // one of the input argc
int tutor_total = 0; // one of the input argc
int student_total = 0; // one of the input argc
// process overview
int tutoring_finished=0;
int chair_unused = 0;
int tutoring_on_going =0;
int tutor_aviliable = 0;
int visited_num = 0; 
// lock and thread variable
sem_t student_sem;
sem_t tutor_sem;
sem_t seat_sem;
sem_t coordinator_sem;
pthread_mutex_t chair_lock;
pthread_mutex_t tutor_lock;
pthread_mutex_t student_lock;
pthread_mutex_t queue_lock;
// structure to represent student info
struct student{
    int ID;
    int helped_time;
    int priority;
    int arriving_order;
};
// structure to represent tutor info
struct tutor{
    int ID;
    int next_target;
};
struct queue{
    int tutor_queue;
    int student_queue;
    int chair_queue;
};

// entering main method
int main(int argc, const char *argv[]){

    int student_total = atoi(argv[1]);
    int tutor_total = atoi(argv[2]);
    int chair_total = atoi(argv[3]);
    int help_need_total = atoi(argv[4]);
    int i = 0;
    for(i =0; i< argc; i++){
        if(atoi(argv[i]) >= max_student_number){
            printf("Invalid Input: Index Out of Bound!\n exiting program");
            return 1;
        } 
    }
    if (argc!= 5){
        printf("Not enough input \n");
        return 1;
    }
    printf("hello world%d %d %d %d %d", max_student_number, student_total, tutor_total, chair_total, help_need_total);
    struct student *students = (struct student *)malloc(student_total * sizeof(struct student));
    struct tutor *tutors = (struct tutor *)malloc(tutor_total * sizeof(struct tutor));
    for (i = 0; i < student_total; i++){
        students[i].arriving_order = -1;
        students[i].priority = -1;
        students[i].helped_time = -1;
    }
    return 0;
}