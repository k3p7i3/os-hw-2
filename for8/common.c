#include "common.h"

const char *mem_path = "shared-memory";
const char *sem_path = "semaphores";

void init_semaphores() {
    key_t key = ftok(sem_path, 0);
    sem_id = semget(key, 3, 0666 | IPC_CREAT);
    if (sem_id < 0) {
        perror("semget: can't init semaphores");
        exit(-1);
    }
}

void init_shared_memory() {
    // open shared memory
    key_t key = ftok(mem_path, 0);
    shm_id = shmget(key, 3 * sizeof(struct program), 0666 | IPC_CREAT);
    if (shm_id < 0) {
        perror("shmget: can't open shared memory");
        exit(-1);
    }

    programs = (struct program *)shmat(shm_id, 0, 0);
    if (programs == NULL) {
        perror("cant attach shared memory to programs");
        exit(-1);
    }
}

void close_resources() {
    shmdt(programs);
}

void interrupt_handler(int sign) {
    close_resources();

    // родительский процесс все окончательно удаляет
    if (process_id == 0) {
        shmctl(shm_id, IPC_RMID, NULL);
        semctl(sem_id, 0, IPC_RMID, 0);
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

    struct sembuf buf;
    buf.sem_num = reviewer_id;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    semop(sem_id, &buf, 1); // известить программиста-ревьюера о том, что ему добавилась задача
}

void fix_program(int program_id) {
    programs[program_id].status = FIX;
    printf("Programmer %d is starting fixing his program after review\n", program_id + 1);
    
    sleep(fix_code_time);
    programs[program_id].status = WAIT_FOR_REVIEW;

    printf("Programmer %d finished fixing his program after review\n", program_id + 1);

    struct sembuf buf;
    buf.sem_num = programs[program_id].reviewer_id;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    semop(sem_id, &buf, 1); // известить программиста-ревьюера о том, что ему добавилась задача
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
    struct sembuf buf;
    buf.sem_num = program_id;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    semop(sem_id, &buf, 1);
}

void do_your_business(int process_id) {
    // функция моделирует работу одного процесса-программиста

    write_program(process_id); // everybode starts with writing code

    while (1) { // do tasks until program is stopped by user
        // one iteration = one task
        struct sembuf buf;
        buf.sem_num = process_id;
        buf.sem_op = -1;
        buf.sem_flg = 0;
        semop(sem_id, &buf, 1); // programmer sleeps (blocked) until he has a task to do

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
