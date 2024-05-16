#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

mutex mtx;
condition_variable cv;
int counter = 0; // переменная счетчика должна быть доступна для обоих потоков

void parent_thread()
{
    while (true) {
        unique_lock<mutex> lock(mtx);
        // Ждем сигнала от дочернего потока о том, что пора выводить строку
        cv.wait(lock, [] { return counter % 2 == 0; });
        if (counter >= 10)
            break;
        cout << "Line " << counter + 1 << " of the first thread\n";
        counter++;
        // Сигнализируем дочернему потоку о том, что пора его пробудить
        cv.notify_one();
    }
}

void child_thread()
{
    while (true) {
        unique_lock<mutex> lock(mtx);
        // Ждем сигнала от родительского потока о том, что пора выводить строку
        cv.wait(lock, [] { return counter % 2 == 1; });
        if (counter >= 10)
            break;
        cout << "Line " << counter + 1 << " of the second thread\n";
        counter++;
        // Сигнализируем родительскому потоку о том, что пора его пробудить
        cv.notify_one();
    }
}

int main()
{
    thread t1(parent_thread);
    thread t2(child_thread);
    t1.join();
    t2.join();

    return 0;
}
