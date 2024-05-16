#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

//Макросы, определяющие количество философов, задержку при еде и количество еды на столе
#define PHILO 5
#define DELAY 1500
#define FOOD 15

//HANDLE: Тип данных, представляющий дескриптор объекта операционной системы Windows. 
// Обычно используется для представления дескрипторов файлов, процессов, потоков, событий и других ресурсов.
HANDLE forks[PHILO];
HANDLE phils[PHILO];
//Критическая секция в Windows для реализации механизма синхронизации доступа к общим ресурсам в многопоточных приложениях
CRITICAL_SECTION foodlock;
//CONDITION_VARIABLE cv;

//Задержка между действиями философов
int sleep_seconds = 1;

//DWORD: 32-битное беззнаковое целое число (unsigned long), используемое в Windows API для представления целых чисел.
//WINAPI: Это макрос, который определяет атрибут функции, указывающий на использование стандартных соглашений вызова функций Windows API.
//LPVOID - это тип указателя на void, используется для указания на область памяти, но не указывает на конкретный тип данных.
DWORD WINAPI philosopher(LPVOID param);
//Объявление функций для дальнейшего использования
int food_on_table();
void get_fork(int, int, char*);
void down_forks(int, int);

//Это функция, которая представляет поведение философа.
//В начале она получает свой идентификатор (id), который будет использоваться для определения, какие вилки ему нужны.
//Затем каждому философу присваивается правая и левая вилки, при этом последний философ получает правую вилку с номером PHILO - 1 и левую с номером 0, чтобы вилки образовывали кольцо.
DWORD WINAPI philosopher(LPVOID param) {
    int id = (int)param;
    printf("Philosopher %d sitting down to dinner.\n", id);
    int right_fork = id;
    int left_fork = (id + 1) % PHILO;

    //Этот цикл представляет действия философов во время ужина.
    //Он проверяет наличие еды на столе, и если еды больше нет, философ завершает ужин.
    //Если философ с идентификатором 1 (индексация начинается с 0) просыпается первым, он спит sleep_seconds секунд.
    while (1) {
        int f = food_on_table();
        if (f == 0)
            break; // Если еды на столе нет, выходим из цикла
        if (id == 1)
            Sleep(sleep_seconds);

        //Философы получают вилки.
        //Они сначала берут правую вилку, а затем левую. 
        //Это предотвращает возникновение взаимоблокировок (deadlock).
        printf("Philosopher %d: get dish %d.\n", id, f);
        get_fork(id, right_fork, (char*)"right");
        get_fork(id, left_fork, (char*)"left ");

        //После получения вилок философ ест, засыпает на DELAY миллисекунд), а затем кладет вилки на стол.
        printf("Philosopher %d: eating.\n", id);
        Sleep(DELAY);
        down_forks(left_fork, right_fork);
    }
    printf("Philosopher %d is done eating.\n", id);
    return 0;
}

//Функция обновляет количество еды на столе и возвращает его текущее значение.
//Использует критическую секцию foodlock для безопасного доступа к общей переменной food.
int food_on_table() {
    static int food = FOOD;
    int myfood;
    EnterCriticalSection(&foodlock);
    if (food > 0)
        food--;
    myfood = food;
    LeaveCriticalSection(&foodlock);
    return myfood;
}

//Функция для взятия вилок из общего набора вилок на столе
void get_fork(int phil, int fork, char* hand) {
    WaitForSingleObject(forks[fork], INFINITE);
    printf("Philosopher %d: got %s fork %d\n", phil, hand, fork);

    int other_fork = (fork + 1) % PHILO;
    DWORD dwWaitResult = WaitForSingleObject(forks[other_fork], 0);
    if (dwWaitResult == WAIT_TIMEOUT) { // Если другая вилка занята, освобождаем текущую и засыпаем
        ReleaseMutex(forks[fork]);
        printf("Philosopher %d: couldn't get other fork, releasing %s fork %d\n", phil, hand, fork);
        Sleep(1000); // Засыпаем на 1 секунду, чтобы предотвратить бесконечное ожидание
    }
}

//Функция для опускания вилок на стол
void down_forks(int f1, int f2) {
    //Разблокировка массива мьютексов.
    ReleaseMutex(forks[f1]);
    ReleaseMutex(forks[f2]);
}

int main(int argn, char** argv) {
    int i;

    if (argn == 2)
        sleep_seconds = atoi(argv[1]);

    //Инициализируем критическую секцию foodlock.
    InitializeCriticalSection(&foodlock);
    //Перебор по всем философам и всем вилкам в цикле
    for (i = 0; i < PHILO; i++)
        forks[i] = CreateMutex(NULL, FALSE, NULL);
    for (i = 0; i < PHILO; i++)
        //Создаем поток для каждого философа
        phils[i] = CreateThread(NULL, 0, philosopher, (LPVOID)i, 0, NULL);
    //Ожидание завершения всех созданных потоков.
    WaitForMultipleObjects(PHILO, phils, TRUE, INFINITE);
    return 0;
}
