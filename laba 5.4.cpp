#include <iostream>
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#define BUFFER_SIZE 1024
using namespace std;
/////////////////////////////////////UML адрес в проекте

//Здесь объявляются глобальные переменные для синхронизации потоков: мьютекс mtx для обеспечения 
//многопоточного доступа к данным, условная переменная cv для сигнализации о готовности данных, 
//и флаг ready, указывающий, готовы ли данные для использования.
::mutex mtx;
::condition_variable cv;
bool ready = false;

//Это функция обратного вызова writeCallback, которая вызывается библиотекой libcurl при получении
//данных от сервера. Она записывает полученные данные в строку, указанную в параметре data.
size_t writeCallback(void* ptr, size_t size, size_t nmemb, ::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

//Это функция networkThread, которая выполняет HTTP-запрос асинхронно в отдельном потоке. 
//Она использует библиотеку libcurl для выполнения запроса. После получения ответа от сервера, 
//она устанавливает флаг ready в true и уведомляет ожидающий поток с помощью условной переменной.
void networkThread(const ::string& url, ::string& response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        ::cerr << "Не удалось инициализировать CURL" << ::endl;
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        ::cerr << "Не удалось выполнить HTTP-запрос: " << curl_easy_strerror(res) << ::endl;
    }

    curl_easy_cleanup(curl);

    {
        ::lock_guard<::mutex> lock(mtx);
        ready = true;
    }
    cv.notify_one();
}

//Это функция userInteractionThread, которая ожидает, пока не будут готовы данные для вывода. 
//Она блокирует выполнение до тех пор, пока флаг ready не станет true, с помощью условной переменной.
void userInteractionThread(::string& response) {
    {
        ::unique_lock<::mutex> lock(mtx);
        cv.wait(lock, [] { return ready; });
    }

    // Вывод тела ответа
    ::cout << response << ::endl;
}

int main(int argc, char* argv[]) {
    //Функция main
    //Она проверяет, был ли передан аргумент командной строки (URL), и выводит сообщение об 
    //использовании, если аргумент не был передан. 
    //Затем она создает два потока: один для выполнения HTTP-запроса (networkThread), а 
    //другой для вывода ответа пользователю (userInteractionThread). 
    //После завершения работы обоих потоков, программа завершает свою работу.
    if (argc != 2) {
        ::cerr << "Usage: " << argv[0] << " <URL>" << ::endl;
        return 1;
    }

    ::string url = argv[1];
    ::string response;

    ::thread network(networkThread, url, ::ref(response));
    ::thread interaction(userInteractionThread, ::ref(response));

    network.join();
    interaction.join();

    return 0;
}
