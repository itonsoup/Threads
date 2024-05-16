#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <algorithm>

using namespace std;
condition_variable cv;
bool inputReceived = false;
mutex mtx;

// Структура для элемента списка
struct Node {
    string data;
    Node* next;

    Node(const string& str) : data(str), next(nullptr) {}
};

// Класс для списка
class LinkedList {
private:
    Node* head;
    


public:
    LinkedList() : head(nullptr) {}

    // Метод для добавления элемента в начало списка
    //void addToFront(const string& str) {
    //    unique_lock<mutex> lock(mtx);
    //    Node* newNode = new Node(str);
    //    newNode->next = head;
    //    head = newNode;
    //}

    void addToFront(const string& str) {
        unique_lock<mutex> lock(mtx);
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
        unique_lock<mutex> lock(mtx);
        Node* current = head;
        while (current != nullptr) {
            cout << current->data << endl;
            current = current->next;
        }
    }

    // Метод для сортировки списка в лексикографическом порядке
    void bubbleSort() {
        unique_lock<mutex> lock(mtx);
        bool swapped = true;
        Node* ptr;
        while (swapped) {
            swapped = false;
            ptr = head;
            while (ptr->next != nullptr) {
                if (ptr->data > ptr->next->data) {
                    swap(ptr->data, ptr->next->data);
                    swapped = true;
                }
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
            lock_guard<mutex> lock(mtx);
            inputReceived = true;
        }

        cv.notify_one();
    }
}

// Функция для сортировки списка каждые 5 секунд
void sortListPeriodically(LinkedList& list) {
    while (true) {
        // Ожидание сигнала о получении ввода
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return inputReceived; });

        // Сброс флага ввода
        inputReceived = false;
        lock.unlock();

        // Сортировка списка
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
