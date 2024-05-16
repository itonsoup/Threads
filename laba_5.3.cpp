﻿#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>

#define BUFFER_SIZE 1024

//Это определение структуры AsyncData, которая используется 
//для передачи данных и информации об операции ввода-вывода.
struct AsyncData {
    OVERLAPPED overlapped;
    HANDLE fileHandle;
    std::vector<char> buffer;
};
//Это функция обратного вызова readCompletionRoutine, которая вызывается 
//при завершении асинхронной операции чтения данных из файла. 
//Она выводит информацию о прочитанных данных или сообщение об ошибке.
void CALLBACK readCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
    AsyncData* asyncData = reinterpret_cast<AsyncData*>(lpOverlapped);
    if (dwErrorCode == ERROR_SUCCESS) {
        std::cout << "Read " << dwNumberOfBytesTransfered << " bytes: " << std::string(asyncData->buffer.begin(), asyncData->buffer.begin() + dwNumberOfBytesTransfered) << std::endl;
    }
    else {
        std::cerr << "ReadFileEx failed with error code " << dwErrorCode << std::endl;
    }
    delete asyncData;
}

//Это функция обратного вызова writeCompletionRoutine, которая вызывается 
//при завершении асинхронной операции записи данных в файл. 
//Она выводит информацию о записанных данных или сообщение об ошибке.
void CALLBACK writeCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
    AsyncData* asyncData = reinterpret_cast<AsyncData*>(lpOverlapped);
    if (dwErrorCode == ERROR_SUCCESS) {
        std::cout << "Write " << dwNumberOfBytesTransfered << " bytes" << std::endl;
    }
    else {
        std::cerr << "WriteFileEx failed with error code " << dwErrorCode << std::endl;
    }
    delete asyncData;
}

int main(int argc, char* argv[]) {
    system("chcp 1251");

    //Она проверяет, был ли передан аргумент командной строки (путь к файлу), и 
    //выводит сообщение об использовании, если аргумент не был передан.
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
        return 1;
    }

    //Этот блок кода преобразует путь к файлу из многобайтовой строки в широкую строку,
    //чтобы его можно было использовать с функциями Windows API.
    const char* filePath = argv[1];

    // Преобразуем многобайтовую строку в широкую строку
    std::wstring wideFilePath(filePath, filePath + strlen(filePath));

    //Этот блок кода открывает файл для чтения и записи с помощью 
    //функции CreateFile и проверяет успешность операции.
    HANDLE fileHandle = CreateFile(wideFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Не удалось открыть файл" << std::endl;
        return 1;
    }

    // Очистка содержимого файла
    if (!SetEndOfFile(fileHandle)) {
        std::cerr << "Не удалось очистить содержимое файла" << std::endl;
        CloseHandle(fileHandle);
        return 1;
    }

    //Здесь создается буфер для чтения данных из файла, а также структура AsyncData, заполняются
    //ее поля, и вызывается асинхронная операция чтения файла с помощью функции ReadFileEx.
    std::vector<char> readBuffer(BUFFER_SIZE, 0);
    AsyncData* readData = new AsyncData();
    readData->overlapped = { 0 };
    readData->fileHandle = fileHandle;
    readData->buffer = readBuffer;

    if (!ReadFileEx(fileHandle, readBuffer.data(), BUFFER_SIZE, reinterpret_cast<LPOVERLAPPED>(readData), readCompletionRoutine)) {
        std::cerr << "Не удалось прочитать файл" << std::endl;
        delete readData;
        CloseHandle(fileHandle);
        return 1;
    }

    std::string inputData;
    std::cout << "Введите данные для записи в файл: ";
    std::getline(std::cin, inputData);

    //После ввода данных пользователем и создания буфера для записи, вызывается асинхронная операция 
    //записи в файл с помощью функции WriteFileEx. После завершения операций файл закрывается.
    std::vector<char> writeBuffer(inputData.begin(), inputData.end());
    AsyncData* writeData = new AsyncData();
    writeData->overlapped = { 0 };
    writeData->fileHandle = fileHandle;
    writeData->buffer = writeBuffer;

    if (!WriteFileEx(fileHandle, writeBuffer.data(), writeBuffer.size(), reinterpret_cast<LPOVERLAPPED>(writeData), writeCompletionRoutine)) {
        std::cerr << "Не удалось записать в файл" << std::endl;
        delete writeData;
        CloseHandle(fileHandle);
        return 1;
    }

    // Дождитесь завершения асинхронных операций
    std::cout << "Файл обновлен" << std::endl;
    

    CloseHandle(fileHandle);
    return 0;
}
