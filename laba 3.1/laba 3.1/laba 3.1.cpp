#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

mutex mtx;
int counter = 0; // переменная счетчика должна быть доступна для обоих потоков

void hello()
{
    while (true) {
        //создание объекта unique_lock для мьютекса, чтобы заблокировать его и обеспечить одиночный доступ
        unique_lock<mutex> lock(mtx);
        if (counter >= 10)
            return;
        if (counter % 2 == 1) {
            cout << "Line " << counter + 1 << " of the second thread\n";
            counter++;
        }
        lock.unlock();
    }
}

int main()
{
    thread t(hello);
    while (true) {
        unique_lock<mutex> lock(mtx);
        if (counter >= 10)
            break;
        if (counter % 2 == 0) {
            cout << "Line " << counter + 1 << " of the first thread\n";
            counter++;
        }
        lock.unlock();
    }
    t.join();
    return 0;
}
