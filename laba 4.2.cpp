﻿#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
                            //Задачи 25-26
using namespace std;
mutex mtx; // Мьютекс для синхронизации доступа к очереди

// Структура для очереди сообщений
class MessageQueue {
private:
    queue<string> messages; // Очередь сообщений
    
    condition_variable notFull; // Условная переменная для оповещения производителя
    condition_variable notEmpty; // Условная переменная для оповещения потребителя
    const size_t maxSize; // Максимальный размер очереди
    int messageCount; // Счетчик сообщений
    bool stopRequested; // Флаг запроса остановки
    int count = 0; //Счетчик для ограничения вывода сообщений 
    bool finished = false; //Флаг завершения

public:
    // Конструктор
    MessageQueue(size_t max_size) : maxSize(max_size), messageCount(0), stopRequested(false) {}

    // Помещение сообщения в очередь
    int put(const string& msg) {
        unique_lock<mutex> lock(mtx);
        notFull.wait(lock, [this]() { return messages.size() < maxSize; }); // Ожидание, пока очередь не станет не полной
        string trimmedMsg = msg.substr(0, 80); // Обрезаем до 80 символов
        messages.push(trimmedMsg); // Добавляем в очередь обрезанное сообщение
        notEmpty.notify_one(); // Уведомляем потребителя о наличии нового сообщения
        return trimmedMsg.length(); // Возвращаем количество переданных символов
    }

    // Извлечение сообщения из очереди
    int get(string& buf, size_t bufsize) {
        unique_lock<mutex> lock(mtx);
        notEmpty.wait(lock, [this]() { return !messages.empty() || stopRequested; }); // Ожидание, пока в очереди не появится сообщение или не будет запроса на остановку
        if (stopRequested && messages.empty()) return 0; // Если был запрос на остановку и очередь пуста, возвращаем 0
        string message = messages.front(); // Получаем первое сообщение из очереди
        messages.pop(); // Удаляем его из очереди
        notFull.notify_one(); // Уведомляем производителя о наличии места в очереди
        size_t length = min(bufsize, message.length()); // Определяем длину для копирования
        buf = message.substr(0, length); // Копируем сообщение в буфер
        ++messageCount; // Увеличиваем счетчик сообщений
        return message.length();
    }

    // Остановка всех ожидающих операций и обнуление семафоров
    void stop() {
        stopRequested = true; // Устанавливаем флаг запроса остановки
        notEmpty.notify_all(); // Уведомляем все потоки о запросе на остановку
    }

    // Проверка, нужно ли останавливаться
    bool shouldStop() const {
        return stopRequested && messageCount >= 10; // Останавливаемся, если был запрос на остановку и получено 10 сообщений
    }
};

// Функция для производителя
void producer(MessageQueue& mq, const string& msg, int id) {
    while (true) {
        if (mq.shouldStop()) break; // Проверяем, нужно ли останавливаться
        mq.put("Producer " + to_string(id) + ": " + msg);
        this_thread::sleep_for(chrono::milliseconds(500)); // Имитация работы производителя
        if (msg == "exit") break; // Выход из цикла при получении сообщения "exit"
    }
}

// Функция для потребителя
void consumer(MessageQueue& mq, int id, int& count) {
    while (true) {
        if (mq.shouldStop()) break; // Проверяем, нужно ли останавливаться
        string msg;
        mq.get(msg, 100); // Буфер размером 100 символов
        {
            lock_guard<mutex> lock(mtx); // Блокировка мьютекса для вывода на консоль
            cout << "Consumer " << id << " received: " << msg << endl;
        }
        if (++count >= 10) break;
        this_thread::sleep_for(chrono::milliseconds(1000)); // Имитация обработки сообщения
    }
}

int main() {
    MessageQueue mq(10); // Очередь сообщений с максимальным размером 10

    // Создание потоков производителей и потребителей
    thread producer1(producer, ref(mq), "Message from producer 1", 1);
    thread producer2(producer, ref(mq), "Message from producer 2", 2);
    int count1 = 0, count2 = 0;
    thread consumer1(consumer, ref(mq), 1, ref(count1));
    thread consumer2(consumer, ref(mq), 2, ref(count2));

    // Ожидание завершения потоков
    producer1.join();
    producer2.join();
    consumer1.join();
    consumer2.join();

    return 0;
}
