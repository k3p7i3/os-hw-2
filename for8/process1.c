#include "common.h"

int main(int argc, char* argv[]) {
    (void)signal(SIGINT, interrupt_handler);

    // инициализация начальных параметров - время выполнения процессами заданий
    if (argc < 4) {
        printf("Initialize all input parameters: write_code_time, review_time, fix_code_time!\n");
        exit(-1);
    }
    write_code_time = atoi(argv[1]);
    review_time = atoi(argv[2]);
    fix_code_time = atoi(argv[3]);

    // инизиализируем разделяемую память и семафоры, общие для всехп процессов
    init_shared_memory();
    init_semaphores();

    process_id = 0;
    do_your_business(process_id); // запускаем "моделирование" процесса

    return 0;
}