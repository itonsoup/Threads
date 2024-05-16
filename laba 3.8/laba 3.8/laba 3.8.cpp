#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <vector>

using namespace std;

condition_variable cv;
bool inputReceived = false;
mutex mtx;
vector<mutex> nodeMutexes;

// Структура для элемента списка
struct Node {
    string data;
    Node* next;
    mutex mtx;

    Node(const string& str) : data(str), next(nullptr) {}
};

// Класс для списка
class LinkedList {
private:
    Node* head;

public:
    LinkedList() : head(nullptr) {}

    // Метод для добавления элемента в начало списка
    void addToFront(const string& str) {
        if (str.length() <= 80) {
            Node* newNode = new Node(str);
            newNode->next = head;
            head = newNode;
        }
        else {
            int numParts = str.length() / 80 + (str.length() % 80 != 0 ? 1 : 0);
            for (int i = numParts - 1; i >= 0; --i) {
                string part = str.substr(i * 80, 80);
                Node* newNode = new Node(part);
                newNode->next = head;
                head = newNode;
            }
        }
    }

    // Метод для печати списка
    void printList() {
        Node* current = head;
        while (current != nullptr) {
            unique_lock<mutex> lock(current->mtx);
            cout << current->data << endl;
            current = current->next;
        }
    }

    // Метод для сортировки списка в лексикографическом порядке
    void bubbleSort() {
        bool swapped = true;
        while (swapped) {
            swapped = false;
            Node* prev = nullptr;
            Node* ptr = head;
            while (ptr != nullptr && ptr->next != nullptr) {
                {
                    lock_guard<mutex> lock1(ptr->mtx);
                    lock_guard<mutex> lock2(ptr->next->mtx);
                    if (ptr->data > ptr->next->data) {
                        swap(ptr->data, ptr->next->data);
                        swapped = true;
                    }
                } // Закрываем блок для освобождения мьютексов
                prev = ptr;
                ptr = ptr->next;
            }
        }
    }

};

// Функция для чтения ввода пользователя и добавления строк в список
void readInput(LinkedList& list) {
    string input;
    while (true) {
        cout << "Введите строку (для вывода списка введите пустую строку): ";
        getline(cin, input);
        if (input.empty()) {
            list.printList();
        }
        else {
            list.addToFront(input);
            unique_lock<mutex> lock(mtx);
            inputReceived = true;
        }

        cv.notify_one();
    }
}

// Функция для сортировки списка каждые 5 секунд
void sortListPeriodically(LinkedList& list) {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return inputReceived; });
        inputReceived = false;
        lock.unlock();
        list.bubbleSort();
    }
}

int main() {
    system("chcp 1251");
    setlocale(LC_ALL, "RU");

    LinkedList list;

    // Создаем потоки для чтения ввода и сортировки списка
    thread inputThread(readInput, ref(list));
    thread sortThread(sortListPeriodically, ref(list));

    // Ожидаем завершение работы потоков
    inputThread.join();
    sortThread.join();

    return 0;
}
