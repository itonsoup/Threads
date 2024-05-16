﻿#include <iostream>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#pragma comment(lib, "ws2_32.lib")
//////////////////////////curl -x http://localhost:8888 http://www.example.com

//Эти строки определяют переменные и структуры для управления потоками. 
//Mutex используется для синхронизации доступа к общим ресурсам, condition используется для ожидания событий, 
//threads хранит потоки, stop атомарная переменная для сигнала остановки работы потоков.
constexpr int MAX_THREADS = 10;

std::mutex mutex;
std::condition_variable condition;
std::vector<std::thread> threads;
std::atomic<bool> stop(false);

// Эта функция handleClient отвечает за обработку запросов от клиентов. 
//Она считывает данные из сокета клиента и проверяет наличие ошибок при чтении.
void handleClient(SOCKET clientSocket) {
    //Обработка запроса клиента
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Ошибка при приеме данных: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        return;
    }

    //Разбор HTTP-запроса 
    std::string httpRequest(buffer, bytesReceived);

    //Отправка HTTP-запроса на удаленный сервер 
    //Этот участок кода создает сокет для соединения с удаленным сервером, подключается к нему и отправляет HTTP-запрос.
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    if (InetPton(AF_INET, TEXT("93.184.216.34"), &remoteAddr.sin_addr) != 1) {
        std::cerr << "Неверный адрес\n";
        closesocket(clientSocket);
        return;
    }
    remoteAddr.sin_port = htons(80); // Порт HTTP
    if (connect(remoteSocket, reinterpret_cast<const sockaddr*>(&remoteAddr), sizeof(remoteAddr)) == SOCKET_ERROR) {
        std::cerr << "Не удалось подключиться к удаленному серверу: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        closesocket(remoteSocket);
        return;
    }
    send(remoteSocket, httpRequest.c_str(), httpRequest.length(), 0);

    //Получение ответа от удаленного сервера 
    //После получения ответа от удаленного сервера, ответ отправляется клиенту, а сокеты закрываются.
    char remoteBuffer[4096];
    int remoteBytesReceived = recv(remoteSocket, remoteBuffer, sizeof(remoteBuffer), 0);
    if (remoteBytesReceived == SOCKET_ERROR) {
        std::cerr << "Ошибка при приеме данных от удаленного сервера: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        closesocket(remoteSocket);
        return;
    }

    //Отправка ответа клиенту
    send(clientSocket, remoteBuffer, remoteBytesReceived, 0);

    //Закрытие сокетов
    closesocket(remoteSocket);
    closesocket(clientSocket);
}
//Функция worker представляет потоковую функцию. 
//Она ожидает подключения к сокету proxySocket и обрабатывает запросы клиентов, вызывая функцию handleClient.
void worker(SOCKET proxySocket) {
    fd_set readSet;
    int maxFd = proxySocket + 1;

    while (!stop) {
        FD_ZERO(&readSet);
        FD_SET(proxySocket, &readSet);

        if (select(maxFd, &readSet, nullptr, nullptr, nullptr) == SOCKET_ERROR) {
            std::cerr << "Ошибка выбора\n";
            continue;
        }

        if (FD_ISSET(proxySocket, &readSet)) {
            SOCKET clientSocket = accept(proxySocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Ошибка при приеме: " << WSAGetLastError() << "\n";
                continue;
            }

            handleClient(clientSocket); // Обработка запроса клиента
        }
    }
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "RU");
    //Проверяется количество аргументов командной строки и устанавливается размер пула потоков.
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <thread_pool_size>\n";
        return 1;
    }

    //Получить размер пула потоков из аргумента командной строки
    int poolSize = std::stoi(argv[1]);
    if (poolSize <= 0 || poolSize > MAX_THREADS) {
        std::cerr << "Неверный размер пула потоков\n";
        return 1;
    }

    //Инициализировать Winsock
    //Winsock инициализируется для работы с сетью.
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка запуска WSA\n";
        return 1;
    }

    //Создание сокета для прокси-сервера
    SOCKET proxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proxySocket == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета:  " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    //Привязать сокет к указанному адресу и порту
    sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = INADDR_ANY;
    proxyAddr.sin_port = htons(8888); // Прокси слушает на порту 8888
    if (bind(proxySocket, reinterpret_cast<const sockaddr*>(&proxyAddr), sizeof(proxyAddr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка привязки: " << WSAGetLastError() << "\n";
        closesocket(proxySocket);
        WSACleanup();
        return 1;
    }

    //Сокет начинает прослушивать входящие соединения
    if (listen(proxySocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Ошибка прослушивания: " << WSAGetLastError() << "\n";
        closesocket(proxySocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Прокси-сервер запущен...\n";

    //Здесь запускаются рабочие потоки, ожидается нажатие клавиши для завершения работы, после чего 
    //потоки останавливаются, сокеты закрываются, и происходит очистка ресурсов.
    for (int i = 0; i < poolSize; ++i) {
        threads.emplace_back(worker, proxySocket);
    }

    //Ожидание сигнала завершения
    std::cin.get(); // Ожидание нажатия любой клавиши для завершения

    //Остановка рабочих потоков
    stop = true;
    for (auto& thread : threads) {
        thread.join();
    }

    //Очистка
    closesocket(proxySocket);
    WSACleanup();

    return 0;
}
