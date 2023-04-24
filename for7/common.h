#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

extern const char* sem_names[3];
sem_t *sems[3];

extern const char *shar_mem;
int shm_id;

int process_id;
int write_code_time;
int review_time;
int fix_code_time;

enum program_status
{
    IN_PROCESS,      // программист пишет программу
    WAIT_FOR_REVIEW, // программа передана на проверку
    REVIEW,          // программа проверяется
    FAIL,            // программа написана неправильно
    SUCCESS,         // программа написана правильно
    FIX,             // программист исправляет свою работу
};

struct program {
    int reviewer_id;
    enum program_status status;
};

struct program *programs;

void init_semaphores();
void init_shared_memory();
void close_resources();
void interrupt_handler(int sign);
int get_reviewer_id(int author_id);
void write_program(int program_id);
void fix_program(int program_id);
void review_program(int process_id, int program_id);
void do_your_business(int process_id);