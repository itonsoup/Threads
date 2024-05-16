#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <semaphore.h>

using namespace std;

// Глобальные семафоры-счетчики для деталей A, B и C
sem_t semA, semB, semC;
// Счетчики для подсчета произведенных деталей
int producedA = 0, producedB = 0, producedC = 0;
// Счетчик для подсчета собранных винтиков
int assembledCount = 0;
// Максимальное количество винтиков для сборки
const int MAX_WIDGETS = 10;
// Мьютекс для синхронизации доступа к счетчикам деталей и винтиков
mutex mtx;

// Функция для производства детали A
void produceA() {
    while (producedA < MAX_WIDGETS) {
        this_thread::sleep_for(chrono::seconds(1));
        sem_post(&semA); // Увеличиваем счетчик деталей A
        producedA++;
    }
}

// Функция для производства детали B
void produceB() {
    while (producedB < MAX_WIDGETS) {
        this_thread::sleep_for(chrono::seconds(2));
        sem_post(&semB); // Увеличиваем счетчик деталей B
        producedB++;
    }
}

// Функция для производства детали C
void produceC() {
    while (producedC < MAX_WIDGETS) {
        this_thread::sleep_for(chrono::seconds(3));
        sem_post(&semC); // Увеличиваем счетчик деталей C
        producedC++;
    }
}

// Функция для сборки винтиков
void assembleWidgets() {
    while (assembledCount < MAX_WIDGETS) {
        sem_wait(&semA); // Ожидаем производства детали A
        sem_wait(&semB); // Ожидаем производства детали B
        sem_wait(&semC); // Ожидаем производства детали C

        // Собираем винтик
        {
            lock_guard<mutex> lock(mtx);
            cout << "Винтик собран " << assembledCount + 1 << "!" << endl;
            assembledCount++;
        }
    }
}

int main() {
    setlocale(LC_ALL, "RU");
    // Инициализация семафоров-счетчиков
    sem_init(&semA, 0, 0);
    sem_init(&semB, 0, 0);
    sem_init(&semC, 0, 0);

    // Создание потоков для производства деталей
    thread threadA(produceA);
    thread threadB(produceB);
    thread threadC(produceC);

    // Создание потока для сборки винтиков
    thread threadAssembly(assembleWidgets);

    // Ожидание завершения всех потоков
    threadA.join();
    threadB.join();
    threadC.join();
    threadAssembly.join();

    return 0;
}
