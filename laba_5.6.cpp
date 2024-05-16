#include <iostream>
#include <thread>
#include <vector>
#include <WinSock2.h>
#include <Ws2tcpip.h>

//////////////////////////////////////////////curl -x http://localhost:8888 http://www.example.com

//Добавление библиотеки ws2_32.lib при компоновке программы
#pragma comment(lib, "ws2_32.lib")
using namespace std;

//Эта функция handleClient отвечает за обработку запросов от клиентов. 
//Она считывает данные из сокета клиента и проверяет наличие ошибок при чтении.
void handleClient(SOCKET clientSocket) {
    // Обработка запроса клиента
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        ::cerr << "Ошибка при приеме данных: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        return;
    }

    // Разбор HTTP-запроса. Здесь полученные данные интерпретируются как HTTP-запрос.
    ::string httpRequest(buffer, bytesReceived);

    // Отправка HTTP-запроса на удаленный сервер (простой пример)
    //Этот участок кода создает сокет для соединения с удаленным сервером, 
    //подключается к нему и отправляет HTTP-запрос.
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    if (InetPton(AF_INET, TEXT("93.184.216.34"), &remoteAddr.sin_addr) != 1) {
        ::cerr << "Неверный адрес\n";
        closesocket(clientSocket);
        return;
    }
    remoteAddr.sin_port = htons(80); // Порт HTTP
    if (connect(remoteSocket, reinterpret_cast<const sockaddr*>(&remoteAddr), sizeof(remoteAddr)) == SOCKET_ERROR) {
        ::cerr << "Не удалось подключиться к удаленному серверу: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        closesocket(remoteSocket);
        return;
    }
    send(remoteSocket, httpRequest.c_str(), httpRequest.length(), 0);

    // Получение ответа от удаленного сервера (простой пример)
    char remoteBuffer[4096];
    int remoteBytesReceived = recv(remoteSocket, remoteBuffer, sizeof(remoteBuffer), 0);
    if (remoteBytesReceived == SOCKET_ERROR) {
        ::cerr << "Ошибка при приеме данных от удаленного сервера: " << WSAGetLastError() << "\n";
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

int main() {
    setlocale(LC_ALL, "RU");
    //Инициализировать Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ::cerr << "Ошибка при инициализации Winsock\n";
        return 1;
    }

    //Создается сокет прокси-сервера для прослушивания входящих соединений.
    SOCKET proxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proxySocket == INVALID_SOCKET) {
        ::cerr << "Ошибка при создании сокета: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    //Привязка сокета к указанному адресу и порту.
    sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = INADDR_ANY;
    proxyAddr.sin_port = htons(8888); // Прокси слушает порт 8888
    if (bind(proxySocket, reinterpret_cast<const sockaddr*>(&proxyAddr), sizeof(proxyAddr)) == SOCKET_ERROR) {
        ::cerr << "Ошибка при привязке сокета: " << WSAGetLastError() << "\n";
        closesocket(proxySocket);
        WSACleanup();
        return 1;
    }

    //Сокет начинает прослушивать входящие соединения.
    if (listen(proxySocket, SOMAXCONN) == SOCKET_ERROR) {
        ::cerr << "Ошибка при ожидании входящих соединений: " << WSAGetLastError() << "\n";
        closesocket(proxySocket);
        WSACleanup();
        return 1;
    }


    ::cout << "Прокси-сервер запущен...\n";

    // Вектор для хранения потоков
    ::vector<::thread> threads;

    //Принимаем соединения и обрабатываем запросы
    //В этом цикле принимаются соединения от клиентов, для каждого создается новый поток handleClient, который обрабатывает запросы. 
    //После завершения работы всех потоков происходит закрытие сокета прокси-сервера и очистка Winsock.
    while (true) {
        SOCKET clientSocket = accept(proxySocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            ::cerr << "Ошибка при принятии соединения: " << WSAGetLastError() << "\n";
            closesocket(proxySocket);
            WSACleanup();
            return 1;
        }

        // Вывод сообщения о создании нового потока для обработки запроса клиента
        ::cout << "Создание нового потока для обработки запроса клиента...\n";

        // Создание нового потока для обработки запроса клиента и добавление его в вектор
        threads.emplace_back(handleClient, clientSocket);
    }

    // Дожидаемся завершения всех потоков
    for (auto& thread : threads) {
        thread.join();
    }

    // Очистка
    closesocket(proxySocket);
    WSACleanup();

    return 0;
}
