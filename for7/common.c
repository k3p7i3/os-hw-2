#include "common.h"

const char* sem_names[3] = {
    "sem_for_1_programmer",
    "sem_for_2_programmer",
    "sem_for_3_programmer",
};

const char *shar_mem = "shared-memory";

void init_semaphores() {
    for (int i = 0; i < 3; ++i) {
        if ((sems[i] = sem_open(sem_names[i], O_CREAT, 0666, 0)) == 0) {
            perror("sem_open: can't create one of semaphores\n");
            exit(-1);
        }
    }
}

void init_shared_memory() {
    // open shared memory
    shm_id = shm_open(shar_mem, O_CREAT | O_RDWR, 0666);
    if (shm_id < 0) {
        perror("shm_open: can't open shared memory");
        exit(-1);
    }

    // allocate sharem memory size to struct program[3] if needed
    struct stat shared_info;
    fstat(shm_id, &shared_info);
    if (shared_info.st_size < 3 * sizeof(struct program)) {
        if (ftruncate(shm_id, 3 * sizeof(struct program)) == -1) {
            perror("ftruncate: can't allocate enought shared memory\n");
            exit(-1);
        }
    }

    // связать shared memory с массивом programs
    programs = mmap(0, 3 * sizeof(struct program), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
    for (int i = 0; i < 3; ++i) {
        programs[i].status = IN_PROCESS;
        programs[i].reviewer_id = -1;
    }
}

void close_resources() {
    close(shm_id);
    for (int i = 0; i < 3; ++i) {
        sem_close(sems[i]);
    }
}

void interrupt_handler(int sign) {
    close_resources();

    // родительский процесс все окончательно удаляет
    if (process_id == 0) {
        shm_unlink(shar_mem);
        for (int i = 0; i < 3; ++i) {
            sem_unlink(sem_names[i]);
        }
    }

    exit(0);
}

int get_reviewer_id(int author_id) {
    srand(time(0));
    // выбрать случайного программиста, который будет проверять задачу
    int reviewer_id;
    do {
        reviewer_id = rand() % 3;
    } while (reviewer_id == author_id);

    return reviewer_id;
}

void write_program(int program_id) {
    programs[program_id].status = IN_PROCESS;
    printf("Programmer %d is starting writing his program\n", program_id + 1);

    sleep(write_code_time);

    // назначить ревьюера
    int reviewer_id = get_reviewer_id(program_id);
    programs[program_id].reviewer_id = reviewer_id;
    programs[program_id].status = WAIT_FOR_REVIEW;

    printf("Programmer %d finished writing his program\n", program_id + 1);

    sem_t *sem = sems[reviewer_id];
    sem_post(sem); // известить программиста-ревьюера о том, что ему добавилась задача
}

void fix_program(int program_id) {
    programs[program_id].status = FIX;
    printf("Programmer %d is starting fixing his program after review\n", program_id + 1);
    
    sleep(fix_code_time);
    programs[program_id].status = WAIT_FOR_REVIEW;

    printf("Programmer %d finished fixing his program after review\n", program_id + 1);

    sem_t *sem = sems[programs[program_id].reviewer_id];
    sem_post(sem); // известить программиста-ревьюера о том, что ему добавилась задача
}

void review_program(int process_id, int program_id) {
    programs[program_id].status = REVIEW;
    printf("Programmer %d is starting review program %d\n", process_id + 1, program_id + 1);

    sleep(review_time);

    // шанс успешного вердикта - 60%, провала - 40%
    srand(time(0));
    int result = rand() % 10;
    if (result < 6) {
        programs[program_id].status = SUCCESS;
        printf("Programmer %d finished review program %d, status: SUCCESS \n", process_id + 1, program_id + 1);
    } else {
        programs[program_id].status = FAIL;
        printf("Programmer %d finished review program %d, status: FAIL\n", process_id + 1, program_id + 1);
    }

    // известить программиста-автора о том, что ему добавилась задача (исправить программу или написать новую)
    sem_t *sem = sems[program_id];
    sem_post(sem); 
}

void do_your_business(int process_id) {
    // функция моделирует работу одного процесса-программиста
    sem_t *sem = sems[process_id];
    write_program(process_id); // everybode starts with writing code

    while (1) { // do tasks until program is stopped by user
        // one iteration = one task
        sem_wait(sem); // programmer sleeps (blocked) until he has a task to do

        int task_taken = 0;
        // сначала смотрит, есть ли у него задачи на ревью или на исправления кода
        for (int i = 0; i < 3 && !task_taken; ++i) {
            if (i == process_id) {
                if (programs[i].status == FAIL) {
                    fix_program(i);
                    ++task_taken;
                } 
            } else {
                // i != process_id
                if (programs[i].reviewer_id == process_id && programs[i].status == WAIT_FOR_REVIEW) {
                    review_program(process_id, i);
                    ++task_taken;
                }
            }
        }

        // если нет других задач и программа прошла проверку - пишет новую программу
        if (!task_taken && programs[process_id].status == SUCCESS) {
            write_program(process_id);
        }
    }
}