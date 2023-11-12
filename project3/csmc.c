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


int tutoring_finished=0;
int chair_unused = 0;
int tutoring_on_going =0;
int chair_num = 0;
int tutor_aviliable = 0;
int visited_num = 0;