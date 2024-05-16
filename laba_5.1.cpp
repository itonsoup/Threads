#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>
#include <string>

//////////////////////////////////////////////GET-запрос через Postman
using namespace std;

#define MAX_CLIENTS 510
#define BUFFER_SIZE 1024

//Создаем функцию для HTTP-ответа, который будет отправлен клиенту
string generateHttpResponse() {
    string responseBody = "Server received your request";
    // Определение длины тела ответа
    string contentLength = to_string(responseBody.length());

    // Формирование HTTP-ответа с правильной длиной содержимого
    return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + contentLength + "\r\n\r\n" + responseBody;
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "RU");

    //Функция main должна принять 4 аргумента:имя исполняемого файла, порт. адрес сервера и порт сервера
    if (argc != 4) {
        ::cerr << "Ошибка: Неверное количество аргументов командной строки." << ::endl;
        ::cerr << "Использование: " << argv[0] << " <port> <server_address> <server_port>" << ::endl;
        return 1;
    }

    //Преобразуем аргументы командной строки в числовые значения и выводим их на экран
    int port = atoi(argv[1]);
    const char* server_address = argv[2];
    int server_port = atoi(argv[3]);

    ::cout << "Порт: " << port << ::endl;
    ::cout << "Адрес сервера: " << server_address << ::endl;
    ::cout << "Порт сервера: " << server_port << ::endl;

    // Инициализация Winsock (Windows Sockets) для использования сетевых функций Windows
    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        ::cerr << "Ошибка: WSAStartup не удался" << ::endl;
        return 1;
    }

    // Создание сокета для прослушивания входящих соединений
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        ::cerr << "Ошибка: не удалось создать сокет" << ::endl;
        WSACleanup();
        return 1;
    }

    // Привязка созданного сокета к созданному адресу и порту
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        ::cerr << "Ошибка: не удалось привязать сокет к адресу" << ::endl;
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    // Начало прослушивания порта ждя входящих соединений
    if (listen(listener, MAX_CLIENTS) == SOCKET_ERROR) {
        ::cerr << "Ошибка: не удалось прослушать порт" << ::endl;
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    // Инициализация набора сокетов для мониторинга ввода-вывода
    fd_set master_fds;
    FD_ZERO(&master_fds);
    FD_SET(listener, &master_fds);
    int max_fd = listener;

    //бесконечный цикл, который мониторит события ввода-вывода на сокетах
    while (true) {
        fd_set read_fds = master_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == SOCKET_ERROR) {
            ::cerr << "Ошибка: select не удался" << ::endl;
            break;
        }

        // Обработка событий на сокетах. 
        // Если событие происходит на слушающем сокете, программа принимает входящее соединение и добавляет его в набор сокетов. 
        // Если событие происходит на клиентском сокете, программа принимает данные от клиента, формирует HTTP-ответ и отправляет его клиенту
        for (int fd = 0; fd <= max_fd; ++fd) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == listener) {
                    // Принятие входящего соединения
                    SOCKET client_socket = accept(listener, NULL, NULL);
                    if (client_socket == INVALID_SOCKET) {
                        ::cerr << "Ошибка: не удалось принять соединение" << ::endl;
                        continue;
                    }
                    FD_SET(client_socket, &master_fds);
                    if (client_socket > max_fd) {
                        max_fd = client_socket;
                    }
                }
                else {
                    // Обработка данных от клиента
                    char buffer[BUFFER_SIZE] = { 0 };
                    int bytes_recv = recv(fd, buffer, BUFFER_SIZE, 0);
                    if (bytes_recv <= 0) {
                        // Обработка завершения соединения
                        if (bytes_recv == 0) {
                            ::cout << "Соединение закрыто клиентом" << ::endl;
                        }
                        else {
                            ::cerr << "Ошибка: recv не удался" << ::endl;
                        }
                        closesocket(fd);
                        FD_CLR(fd, &master_fds);
                    }
                    else {
                        // Просто выводим информацию о запросе
                        ::cout << "Получен запрос от клиента" << ::endl;
                        ::cout << "Данные запроса: " << buffer << ::endl;

                        // Генерируем HTTP-ответ и отправляем клиенту
                        ::string response = generateHttpResponse();
                        
                        ::cout << "Отправка ответа клиенту: " << response << ::endl;
                        
                        send(fd, response.c_str(), response.length(), 0);

                        // Закрываем соединение с клиентом
                        closesocket(fd);
                        FD_CLR(fd, &master_fds);
                    }
                }
            }
        }
    }

    // Закрытие сокета прослушивания
    closesocket(listener);

    // Очистка Winsock
    WSACleanup();

    return 0;
}
