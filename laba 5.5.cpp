﻿#include <iostream>
#include <WinSock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
////////////////////////////curl -x http://localhost:8888 http://www.example.com

int main() {
    //Инициализация Winsock
    //Этот блок кода инициализирует библиотеку Winsock с помощью функции WSAStartup. 
    //Если инициализация не удалась, выводится сообщение об ошибке и программа завершается.
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка при запуске Winsock\n";
        return 1;
    }

    //Создание сокета
    //Здесь создается сокет для прокси-сервера.Если создание сокета завершается 
    //неудачно, программа выводит сообщение об ошибке и завершается.
    SOCKET proxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proxySocket == INVALID_SOCKET) {
        std::cerr << "Ошибка при создании сокета: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    //Привязка сокета
    //Здесь происходит привязка сокета к адресу и порту. Если привязка 
    //завершается неудачно, программа выводит сообщение об ошибке и завершается.
    sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = INADDR_ANY;
    proxyAddr.sin_port = htons(8888); // Прокси слушает порт 8888
    if (bind(proxySocket, reinterpret_cast<const sockaddr*>(&proxyAddr), sizeof(proxyAddr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка привязки: " << WSAGetLastError() << "\n";
        closesocket(proxySocket);
        WSACleanup();
        return 1;
    }

    //Ожидание входящих соединений
    //Прокси-сервер начинает прослушивать входящие соединения. Если прослушивание 
    //завершается неудачно, программа выводит сообщение об ошибке и завершается.
    if (listen(proxySocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Ошибка при ожидании: " << WSAGetLastError() << "\n";
        closesocket(proxySocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Прокси-сервер запущен...\n";

    //Принятие соединений и обработка запросов
    //Прокси-сервер принимает входящие соединения от клиентов. Если принятие соединения 
    //завершается неудачно, программа выводит сообщение об ошибке и завершается.
    while (true) {
        SOCKET clientSocket = accept(proxySocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Ошибка при принятии: " << WSAGetLastError() << "\n";
            closesocket(proxySocket);
            WSACleanup();
            return 1;
        }

        //Обработка запроса клиента
        //Прокси-сервер принимает запрос от клиента. Если прием данных завершается неудачно, 
        //программа выводит сообщение об ошибке и продолжает работу.
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Ошибка при приеме: " << WSAGetLastError() << "\n";
            closesocket(clientSocket);
            continue;
        }

        //Парсинг HTTP-запроса (простой пример)
        //Прокси-сервер преобразует полученные данные в строку для анализа запроса.
        std::string httpRequest(buffer, bytesReceived);

        //Отправка HTTP-запроса на удаленный сервер (простой пример)
        //Прокси-сервер создает сокет для связи с удаленным сервером, устанавливает соединение и отправляет ему запрос.
        SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in remoteAddr;
        remoteAddr.sin_family = AF_INET;
        if (InetPton(AF_INET, TEXT("93.184.216.34"), &remoteAddr.sin_addr) != 1) {
            std::cerr << "Недопустимый адрес\n";
            closesocket(clientSocket);
            continue;
        }
        remoteAddr.sin_port = htons(80); // HTTP port
        if (connect(remoteSocket, reinterpret_cast<const sockaddr*>(&remoteAddr), sizeof(remoteAddr)) == SOCKET_ERROR) {
            std::cerr << "Ошибка при подключении к удаленному серверу: " << WSAGetLastError() << "\n";
            closesocket(clientSocket);
            closesocket(remoteSocket);
            continue;
        }
        send(remoteSocket, httpRequest.c_str(), httpRequest.length(), 0);

        //Получение ответа от удаленного сервера 
        char remoteBuffer[4096];
        int remoteBytesReceived = recv(remoteSocket, remoteBuffer, sizeof(remoteBuffer), 0);
        if (remoteBytesReceived == SOCKET_ERROR) {
            std::cerr << "Ошибка при приеме от удаленного сервера: " << WSAGetLastError() << "\n";
            closesocket(clientSocket);
            closesocket(remoteSocket);
            continue;
        }

        //Отправка ответа клиенту
        send(clientSocket, remoteBuffer, remoteBytesReceived, 0);

        //Закрытие сокетов
        closesocket(remoteSocket);
        closesocket(clientSocket);
    }

    //Очистка
    closesocket(proxySocket);
    WSACleanup();

    return 0;
}
