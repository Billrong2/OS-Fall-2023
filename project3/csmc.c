// libraries
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define NDEBUG ;

// define max number of students
const int max_student_number = 2000;

// defining thread global variables.
int chair_total = 0; // one of the input argc
int help_need_total = 0; // one of the input argc
int tutor_total = 0; // one of the input argc
int student_total = 0; // one of the input argc
// process overview
int tutoring_finished=0;
int student_finished = 0;
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
struct csmc_info{
    struct student *students;
    struct tutor *tutors;
};
struct student{
    int ID;
    int helped_time;
    int priority;
    int arriving_order;
    int student_queue;
};
// structure to represent tutor info
struct tutor{
    int ID;
    int next_target;
    int tutor_queue;
};
void thread_sleep(){ // half chatgpt
    srand((unsigned int)time(NULL));
    // Generate a random sleep time between 0 and 2000 microseconds (2 milliseconds)
    int sleepTime = rand() % 2001;
    // Simulate thread behavior by sleeping
    usleep(sleepTime);
}
void init(){
    // init all sem
    sem_init(&student_sem,0,0);
    sem_init(&tutor_sem,0,0);
    sem_init(&seat_sem,0,0);
    sem_init(&coordinator_sem,0,0);

    pthread_mutex_init(&chair_lock,NULL);
    pthread_mutex_init(&tutor_lock,NULL);
    pthread_mutex_init(&student_lock,NULL);
    pthread_mutex_init(&queue_lock,NULL);
    // pthread_t p_student[student_total];
    // pthread_t p_tutor[tutor_total];
    // pthread_t p_chair[chair_total];
}
void *thread_function_student(void *thread_info){
    // struct student *students = (struct student *) thread_info_student;
    struct csmc_info *csmcs = (struct csmc_info *) thread_info;
    while(student_finished < student_total){
        pthread_mutex_lock(&student_lock);
        if (chair_unused == 0){
            printf("S: Student %d found no empty chair. Will try again later", csmcs->students->ID);
            pthread_mutex_unlock(&student_lock);
            thread_sleep();
        }
        else{// if there are chairs aviliable
            chair_unused --;
            tutoring_on_going ++;
            tutor_aviliable --;
            csmcs->students->student_queue --; 
            printf("S: Student %d takes a seat. Empty chair = %d .", csmcs->students->ID, chair_unused);
            pthread_mutex_unlock(&student_lock);
            sem_post(&student_sem);
            if(tutor_aviliable > 0){
                printf("S: Student %d received help from Tutor %d.", csmcs->students->ID, csmcs->tutors->ID);
            }
        }
        
        





        if (csmcs->students->helped_time >= help_need_total){
            pthread_mutex_lock(&student_lock);
            student_finished ++;
            pthread_mutex_unlock(&student_lock);
            sem_post(&student_sem);
            pthread_exit(NULL);
        }
    }
    
    // sem_wait(&semaphore);
    // sem_post(&semaphore);
    // pthread_exit(NULL);
}
void *thread_function_tutor(void *threadID){
    long tid = (long)threadID;
    // sem_wait(&semaphore);
    // sem_post(&semaphore);
    // pthread_exit(NULL);
}
void *thread_function_chair(void *threadID){
    long tid = (long)threadID;
    // sem_wait(&semaphore);
    // sem_post(&semaphore);
    // pthread_exit(NULL);
}

void t_init(struct csmc_info *arg)
{
    pthread_t p_student[student_total];
    pthread_t p_tutor[tutor_total];
    int i = 0, id_count = 1;
    pthread_t p_coordinator;
    struct csmc_info *csmcs = (struct csmc_info *) arg;
    pthread_create(&p_coordinator, NULL,thread_function_chair,NULL);
    pthread_join(p_coordinator, NULL);
    for (i = 0; i<student_total;i++){
        csmcs->students[i].ID = id_count;
        id_count++;
        pthread_create(&p_student[i], NULL,thread_function_student,(void*)(&csmcs));
        pthread_join(p_student[i],NULL);
    }
    id_count=1;
    for (i = 0; i<tutor_total;i++){
        csmcs->tutors[i].ID = id_count;
        id_count++;
        pthread_create(&p_tutor[i], NULL,thread_function_tutor,(void*)(&csmcs));
        pthread_join(p_tutor[i],NULL);
    }
}
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
    struct csmc_info *csmcs = (struct csmc_info *)malloc(sizeof(struct csmc_info));
    csmcs->students = (struct student *)malloc(student_total * sizeof(struct student));
    csmcs->tutors = (struct tutor *)malloc(tutor_total * sizeof(struct tutor));
    //struct csmc_info *csmcs = {*students, *tutors};
    for (i = 0; i < student_total; i++){
        csmcs->students[i].arriving_order = -1;
        csmcs->students[i].priority = -1;
        csmcs->students[i].helped_time = -1;
        csmcs->students[i].student_queue = -1;
    }
    for (i =0; i< tutor_total; i++){
        csmcs->tutors[i].tutor_queue = -1;
        csmcs->tutors[i].next_target = -1;
    }
    init();
    t_init(csmcs);
    return 0;
}