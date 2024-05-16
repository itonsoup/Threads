#include <iostream>
#include <thread>
#include <semaphore.h>

using namespace std;

sem_t parent_sem;
sem_t child_sem;
int counter = 0; // переменная счетчика должна быть доступна для обоих потоков

void parent_thread()
{
    while (true) {
        // Ждем сигнала от дочернего потока о готовности вывода строки
        sem_wait(&parent_sem);
        if (counter >= 10)
            break;
        cout << "Line " << counter + 1 << " of the first thread\n";
        counter++;
        // Оповещаем дочерний поток о готовности к выводу строки
        sem_post(&child_sem);
    }
}

void child_thread()
{
    while (true) {
        // Ждем сигнала от родительского потока о готовности вывода строки
        sem_wait(&child_sem);
        if (counter >= 10)
            break;
        cout << "Line " << counter + 1 << " of the second thread\n";
        counter++;
        // Оповещаем родительский поток о готовности к выводу строки
        sem_post(&parent_sem);
    }
}

int main()
{
    // Инициализируем семафоры
    sem_init(&parent_sem, 0, 1);
    sem_init(&child_sem, 0, 0);

    thread t1(parent_thread);
    thread t2(child_thread);
    t1.join();
    t2.join();

    // Уничтожаем семафоры
    sem_destroy(&parent_sem);
    sem_destroy(&child_sem);

    return 0;
}
