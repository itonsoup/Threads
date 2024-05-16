#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <algorithm>
#include <chrono>
#include <mutex>

using namespace std;

mutex mtx;

// Структура для узла связного списка
struct ListNode {
    string data;
    ListNode* next;

    ListNode(const string& str) : data(str), next(nullptr) {}
};

// Функция для сортировки строк по длине
bool compareLength(const string& a, const string& b) {
    return a.length() < b.length();
}

// Функция для вставки нового узла в связный список
void insertNode(ListNode*& head, const string& str) {
    ListNode* newNode = new ListNode(str);
    if (!head) {
        head = newNode;
    }
    else {
        ListNode* current = head;
        while (current->next) {
            current = current->next;
        }
        current->next = newNode;
    }
}

// Функция для вывода связного списка в стандартный поток вывода
void printList(ListNode* head) {
    ListNode* current = head;
    while (current) {
        cout << current->data << endl;
        current = current->next;
    }
}

// Функция для сортировки строки по длине с помощью sleepsort
void sleepSort(ListNode*& head, const string& str) {
    // Вычисляем время ожидания пропорционально длине строки
    this_thread::sleep_for(chrono::milliseconds(str.length() * 100)); // Предполагаем, что 1 символ == 100 миллисекундам

    // Блокировка mutex перед изменением списка, 
    // чтобы обеспечить синхронизацию доступа к списку из разных потоков
    mtx.lock();
    insertNode(head, str);
    mtx.unlock();
}

int main() {
    setlocale(LC_ALL, "RU");

    // Имя файла для чтения
    string filename = "text.txt";

    // Открываем файл для чтения
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл: " << filename << endl;
        return 1;
    }

    // Вектор для хранения считанных строк
    vector<string> strings;

    // Считываем строки из файла
    string line;
    while (getline(file, line)) {
        strings.push_back(line);
    }

    // Закрываем файл
    file.close();

    // Сортируем строки по длине
    sort(strings.begin(), strings.end(), compareLength);

    // Создаем голову связного списка (начало)
    ListNode* head = nullptr;

    // Создаем поток для каждой строки
    vector<thread> threads;
    for (const string& str : strings) {
        threads.emplace_back(sleepSort, ref(head), str);
    }

    // Ждем завершения всех потоков
    for (thread& t : threads) {
        t.join();
    }

    // Выводим все эелементы связного списка после создания всех потоков
    printList(head);

    // Освобождаем память, выделенную для связного списка
    ListNode* current = head;
    while (current) {
        ListNode* temp = current;
        current = current->next;
        delete temp;
    }

    return 0;
}
