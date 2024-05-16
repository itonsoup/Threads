#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

mutex mtx_parent;
mutex mtx_child;

void child_thread()
{
    // Захватываем свой мьютекс 
    mtx_child.lock();
    cout << "Дочерний поток захватил свой мьютекс\n";

}

void parent_thread()
{
    // Захватываем свой мьютекс
    mtx_parent.lock();
    cout << "Родительский поток захватил свой мьютекс\n";

    // Пытаемся захватить мьютекс дочернего потока
    cout << "Родительский поток пытается захватить мьютекс дочернего потока\n";
    if (!mtx_child.try_lock()) {
        cout << "Родительский поток не смог захватить мьютекс дочернего потока из-за взаимной блокировки\n";
    }
    else {
        cout << "Родительский поток успешно захватил мьютекс дочернего потока\n";
        mtx_child.unlock();
    }
}



int main()
{
    setlocale(LC_ALL, "RU");
    // Сценарий 3
    cout << "Сценарий 3: Родительский поток сначала захватывает свой мьютекс, затем пытается захватить мьютекс дочернего потока\n";
    thread t1(child_thread);

    thread t2(parent_thread);
    
    t2.join();
    t1.join();
   
    cout << "Сценарий 3: Завершен\n\n";

    return 0;
}
