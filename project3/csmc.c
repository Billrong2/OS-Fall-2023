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
const int max_student_number = 2000;
// process overview
int total_session = 0;
int student_finished = 0;
int chair_unused;
int tutoring_on_going = 0;

int total_request = 0;
// defining thread global variables.
void *thread_function_student(void *thread_info);
void *thread_function_tutor(void *thread_info);
void *thread_function_chair(void *arg);

int chair_total = 0;     // one of the input argc
int help_need_total = 0; // one of the input argc
int tutor_total = 0;     // one of the input argc
int student_total = 0;   // one of the input argc

int tutor_aviliable = 0;

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
// structure to represent tutor info
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
void thread_sleep()
{ // half chatgpt
    srand((unsigned int)time(NULL));
    // Generate a random sleep time between 0 and 2000 microseconds (2 milliseconds)
    int sleepTime = rand() % 2001;
    // Simulate thread behavior by sleeping
    usleep(sleepTime);
}
int main(int argc, const char *argv[])
{
    int student_total = atoi(argv[1]);
    int tutor_total = atoi(argv[2]);
    int chair_total = atoi(argv[3]);
    int help_need_total = atoi(argv[4]);
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
    printf("hello world%d %d %d %d %d\n", max_student_number, student_total, tutor_total, chair_total, help_need_total);
    csmcs.students = (struct student *)malloc(student_total * sizeof(struct student));
    csmcs.tutors = (struct tutor *)malloc(tutor_total * sizeof(struct tutor));
    csmcs.queues = (struct chair *)malloc(student_total * sizeof(struct chair));
    
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
        csmcs.students[i].priority = -1;
        csmcs.students[i].helped_time = -1;
        csmcs.students[i].student_queue = -1;
        csmcs.students[i].ID = i;
        assert(pthread_create(&p_student[i], NULL, thread_function_student, (void *)(&csmcs.students[i]))==0);
    }
    for (i = 0; i < tutor_total; i++)
    {

        csmcs.tutors[i].ID = i;
        assert(pthread_create(&p_tutor[i], NULL, thread_function_tutor, (void *)(&csmcs.tutors[i]))==0);
    }
    for (i = 0; i < chair_total; i++)
    {
        csmcs.queues[i].student_queue = -1;
        csmcs.queues[i].arriving_order = -1;
    }
    pthread_join(p_coordinator, NULL);
    for (i = 0; i < student_total; i++)
    {
            printf("input of student_total is %d ;\n", student_total);
        pthread_join(p_student[i], NULL);
    }
    for (i = 0; i < tutor_total; i++)
    {
        pthread_join(p_tutor[i], NULL);
    }
    return 0;
}

void *thread_function_student(void *thread_info)
{
    struct student *students = (struct student *)thread_info;
    printf("im herer %d ;\n", student_total);
    while (1)
    {
        thread_sleep();
        
        pthread_mutex_lock(&student_lock);
        if (chair_unused == 0)
        {

            printf("S: Student %d found no empty chair. Will try again later\n", students->ID);
            pthread_mutex_unlock(&student_lock);
            thread_sleep();
            continue;
        }
        else
        { // if there are chairs aviliable

            total_request++;
            students->arriving_order = total_request;
            chair_unused--;
            printf("S: Student %d takes a seat. Empty chair = %d.\n", students->ID, chair_unused);
            pthread_mutex_unlock(&student_lock);
            sem_post(&student_sem);

            sem_wait(&tutor_sem);
            printf("S: Student %d received help from Tutor %d.\n", students->ID, students->student_queue);
            tutoring_on_going++;
            tutor_aviliable--;
            pthread_mutex_lock(&tutor_lock);
            students->student_queue = -1;
            pthread_mutex_unlock(&tutor_lock);
            pthread_mutex_lock(&student_lock);
            students->student_queue = -1;
            pthread_mutex_unlock(&student_lock);
        }
        if (students->helped_time >= help_need_total)
        {
            pthread_mutex_lock(&student_lock);
            student_finished++;
            pthread_mutex_unlock(&student_lock);
            sem_post(&student_sem); // notify coordinator
            pthread_exit(NULL);     // student thread terminate
        }
    }
}
void *thread_function_tutor(void *thread_info)
{

    while (1)
    {
        if(student_finished >= student_total){
            break;
        }
        int i = 0, serving_student_num = 0, highest_priority = help_need_total, student_index = -1, fcfs = student_total * help_need_total;
        struct tutor *tutors = (struct tutor *)thread_info;

        sem_wait(&coordinator_sem);
        pthread_mutex_lock(&student_lock);
        
        for (i = 0; i < student_total; ++i)
        {
            printf("%d , %d \n", csmcs.students[i].priority, csmcs.students[i].arriving_order);
            if (csmcs.students[i].priority != -1 && csmcs.students[i].priority <= highest_priority && csmcs.students[i].arriving_order <= fcfs)
            {
                highest_priority = csmcs.students[i].priority;
                student_index = i;
                fcfs = csmcs.students[i].arriving_order;
            }
        }
        csmcs.students[i].arriving_order = -1;
        csmcs.students[i].priority = -1;
        chair_unused++;
        tutoring_on_going++;
        pthread_mutex_unlock(&student_lock);
        sem_post(&tutor_sem);
        thread_sleep();
        tutoring_on_going--;
        total_session++;
        printf("T: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n", csmcs.students[serving_student_num].ID, csmcs.tutors->ID, (tutor_total - tutor_aviliable), total_session);
        pthread_mutex_lock(&tutor_lock);
        csmcs.students[student_index].student_queue = tutors->ID;
        pthread_mutex_unlock(&tutor_lock);
    }

    pthread_exit(NULL);
}
void *thread_function_chair(void *arg)
{

    while (student_finished < student_total)
    {

        sem_wait(&student_sem);
        int i = 0; //, serving_student_num = 0, highest_priority = help_need_total, student_index = -1, fcfs = student_total * help_need_total;
        pthread_mutex_lock(&student_lock);
        for (i = 0; i < student_total; i++)
        {
            if (csmcs.queues[i].student_queue > 1)
            {
                csmcs.students[i].arriving_order = csmcs.queues[i].arriving_order;
                csmcs.students[i].priority = csmcs.queues[i].student_queue;
                printf("C: Student %d with priority %d added to the queue. Waiting students now = %d. Total requests = %d\n", csmcs.students[i].ID, csmcs.students[i].priority, chair_total - chair_unused, total_request);
                csmcs.queues[i].student_queue = -1;
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