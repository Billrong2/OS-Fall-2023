// libraries
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
// define max number of students
#define max_student_number 2000
// process overview
int total_session;
int student_finished;
int chair_unused;
int tutoring_on_going;

int total_request = 0;
int chair_total;     // one of the input argc
int help_need_total; // one of the input argc
int tutor_total;     // one of the input argc
int student_total;   // one of the input argc
int tutor_aviliable;
int *global_tutor_ID;

// lock and thread variable

// structure to represent student info
struct csmc_info
{
    struct student *students;
    struct tutor *tutors;
    struct chair *queues;
};
struct student
{
    int ID;
    int helped_time;
    int priority;
    int arriving_order;
    int student_queue;
};
struct chair
{
    int student_queue;
    int arriving_order;
};
struct tutor
{
    int ID;
};
// make our csmcs global struct
struct csmc_info csmcs;

sem_t student_sem;
sem_t tutor_sem;
sem_t seat_sem;
sem_t coordinator_sem;
pthread_mutex_t chair_lock;
pthread_mutex_t tutor_lock;
pthread_mutex_t student_lock;
pthread_mutex_t queue_lock;
// defining thread global variables.

void thread_sleep()
{ // half chatgpt
    srand((unsigned int)time(NULL));
    // Generate a random sleep time between 0 and 2000 microseconds (2 milliseconds)
    float sleepTime = rand() % 2001;
    // Simulate thread behavior by sleeping
    usleep(sleepTime);
}
void *thread_function_student(void *thread_info)
{
    struct student *students = (struct student *)thread_info;
    int student_ID = students->ID; // student ID keep changing in struct, dont know why
    while (1)
    {
        if(students->helped_time >= help_need_total){
            printf("student %d term\n", students->ID  );
            break;
        }
        thread_sleep();
        pthread_mutex_lock(&student_lock);
        if (chair_unused == 0)
        {
            printf("S: Student %d found no empty chair. Will try again later\n", students->ID);
            pthread_mutex_unlock(&student_lock);
            
            continue;
        }
        else if(chair_unused >0)
        { // if there are chairs aviliable
            
            //students->arriving_order = total_request;
            chair_unused--;
            total_request++;
            csmcs.queues[student_ID].student_queue = total_request;
            printf("S: Student %d takes a seat. Empty chair = %d.\n", students->ID, chair_unused);
            
            pthread_mutex_unlock(&student_lock);
            sem_post(&student_sem);// student seated, notify coordinator
            sem_wait(&tutor_sem); // as student remain seated, wait for tutor come to pick up that student
            printf("S: Student %d received help from Tutor %d.\n", student_ID, global_tutor_ID[student_ID]);
            pthread_mutex_lock(&tutor_lock);
            students->student_queue = -1;
            students->helped_time +=1;
            students->priority -=1;
            chair_unused++;
            pthread_mutex_unlock(&tutor_lock);
        }
    }
    pthread_mutex_lock(&student_lock);
    student_finished++;
    csmcs.queues[students->ID].student_queue = -1;
    students->student_queue = -1;
    pthread_mutex_unlock(&student_lock);
    sem_post(&student_sem); // notify coordinator
    pthread_exit(NULL);     // student thread terminate
}
void *thread_function_tutor(void *thread_info)
{
    struct tutor *tutors = (struct tutor *)thread_info;
    int tutor_ID = tutors->ID;
    while (student_finished < student_total)
    {
        int i = 0, highest_priority = 0, student_index = -1;
        sem_wait(&coordinator_sem);
        pthread_mutex_lock(&student_lock);
        
        for (i = 0; i < student_total; ++i)
        {
            if (csmcs.students[i].arriving_order != -1 && csmcs.students[i].priority >= highest_priority)
            {
                highest_priority = csmcs.students[i].priority;
                student_index = i;

            }
        }
        if (chair_unused == chair_total)
        {
            pthread_mutex_unlock(&student_lock);
            continue;
        }
        csmcs.students[i].arriving_order = -1;
        //csmcs.students[i].priority --;
        
        tutoring_on_going++;
        tutor_aviliable --;
        csmcs.students[student_index].student_queue = tutor_ID;
        pthread_mutex_unlock(&student_lock);

        thread_sleep();
        pthread_mutex_lock(&student_lock);
        tutoring_on_going--;
        total_session++;
        sem_post(&tutor_sem);
        global_tutor_ID[student_index] = tutor_ID;
        printf("T: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n", csmcs.students[student_index].ID, tutor_ID, (tutor_total - tutor_aviliable), total_session);
        
        pthread_mutex_unlock(&student_lock);

        pthread_mutex_lock(&tutor_lock);
        csmcs.students[student_index].student_queue = -1;
        pthread_mutex_unlock(&tutor_lock);
    }
    printf("Tutor %d term\n", tutor_ID);
    pthread_exit(NULL);
}
void *thread_function_chair(void *arg)
{
    while (1)
    {
        if (student_finished >= student_total)
        {
            break;
        }
        int i = 0;
        sem_wait(&student_sem);
         //, serving_student_num = 0, highest_priority = help_need_total, student_index = -1, fcfs = student_total * help_need_total;
        pthread_mutex_lock(&student_lock);
        for (i = 0; i < student_total; i++)
        {
            //printf("csmcs queue %d\n",csmcs.queues[i].student_queue);
            if (csmcs.queues[i].student_queue >=1) // student seatted detected
            {
                csmcs.students[i].arriving_order = csmcs.queues[i].student_queue;
                printf("C: Student %d with priority %d added to the queue. Waiting students now = %d. Total requests = %d\n", csmcs.students[i].ID, csmcs.students[i].priority, chair_total - chair_unused, total_request);
                csmcs.queues[i].student_queue = -1; // student seatted recorded, clear the seat
                sem_post(&coordinator_sem);
            }
        }
        pthread_mutex_unlock(&student_lock);
    }
    int i;
    // post info let all tutor terminate
    for (i = 0; i < tutor_total; i++)
    {
        sem_post(&coordinator_sem);
    }
    pthread_exit(NULL);
}

int main(int argc, const char *argv[])
{
    student_total = atoi(argv[1]);
    tutor_total = atoi(argv[2]);
    chair_total = atoi(argv[3]);
    help_need_total = atoi(argv[4]);
    int i = 0;
    tutor_aviliable = tutor_total;
    chair_unused = chair_total;

    for (i = 0; i < argc; i++)
    {
        if (atoi(argv[i]) >= max_student_number)
        {
            printf("Invalid Input: Index Out of Bound!\n exiting program");
            return 1;
        }
    }
    if (argc != 5)
    {
        printf("Not enough input \n");
        return 1;
    }
    csmcs.students = (struct student *)malloc(student_total * sizeof(struct student));
    csmcs.tutors = (struct tutor *)malloc(tutor_total * sizeof(struct tutor));
    csmcs.queues = (struct chair *)malloc(student_total * sizeof(struct chair));
    global_tutor_ID = (int *)malloc(student_total * sizeof(int));

    sem_init(&student_sem, 0, 0);
    sem_init(&tutor_sem, 0, 0);
    sem_init(&coordinator_sem, 0, 0);
    pthread_mutex_init(&chair_lock, NULL);
    pthread_mutex_init(&tutor_lock, NULL);
    pthread_mutex_init(&student_lock, NULL);
    pthread_t p_student[student_total];
    pthread_t p_tutor[tutor_total];
    pthread_t p_coordinator;
    pthread_create(&p_coordinator, NULL, thread_function_chair, NULL);

    for (i = 0; i < student_total; i++)
    {
        csmcs.students[i].arriving_order = -1;
        csmcs.students[i].priority = help_need_total;
        csmcs.students[i].helped_time = 0;
        csmcs.students[i].student_queue = -1;
        csmcs.students[i].ID = i;
        assert(pthread_create(&p_student[i], NULL, thread_function_student, (void *)(&csmcs.students[i])) == 0);
    }
    for (i = 0; i < tutor_total; i++)
    {

        csmcs.tutors[i].ID = i;
        assert(pthread_create(&p_tutor[i], NULL, thread_function_tutor, (void *)(&csmcs.tutors[i])) == 0);
    }
    for (i = 0; i < chair_total; i++)
    {
        csmcs.queues[i].student_queue = -1;
        csmcs.queues[i].arriving_order = -1;
    }
    pthread_join(p_coordinator, NULL);
    for (i = 0; i < student_total; i++)
    {
        //printf("input of student_total is %d ;\n", student_total);
        pthread_join(p_student[i], NULL);
    }
    for (i = 0; i < tutor_total; i++)
    {
        pthread_join(p_tutor[i], NULL);
    }
    return 0;
}
