#include <iostream>
#include <curl/curl.h>

//Это макрос, который определяет размер буфера для чтения данных.
#define BUFFER_SIZE 1024
using namespace std;
//////////////////////////////////////////////В проекте указывается ссылка


//Это функция обратного вызова writeCallback, которая вызывается при получении данных от сервера.
//Она записывает полученные данные в строку, указанную в параметре data.
size_t writeCallback(void* ptr, size_t size, size_t nmemb, ::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

int main(int argc, char* argv[]) {
    //Проверка, будет ли передан аргумент командной строки URL, и выводит 
    //сообщение об использовании, если аргумент не был передан
    if (argc != 2) {
        ::cerr << "Использование: " << argv[0] << " <URL>" << ::endl;
        return 1;
    }

    //Этот блок кода сохраняет переданный URL в строковую переменную url
    ::string url = argv[1];

    //Этот блок кода инициализирует библиотеку libcurl и проверяет, была ли инициализация успешной
    CURL* curl = curl_easy_init();
    if (!curl) {
        ::cerr << "Не удалось инициализировать CURL" << ::endl;
        return 1;
    }

    //Этот блок кода устанавливает опцию CURLOPT_URL 
    //для указания URL, на который будет отправлен HTTP-запрос.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    //Этот блок кода устанавливает функцию обратного вызова writeCallback, которая будет вызвана 
    //при получении данных от сервера, и указывает на строку response, в которую будут записаны данные.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    ::string response;

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    //Этот блок кода выполняет HTTP-запрос с помощью библиотеки libcurl и проверяет успешность выполнения запроса.
    //Если запрос завершился с ошибкой, выводится сообщение об ошибке.
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        ::cerr << "Не удалось выполнить HTTP-запрос: " << curl_easy_strerror(res) << ::endl;
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_cleanup(curl);

    // Вывод тела ответа
    ::cout << response << ::endl;

    return 0;
}
