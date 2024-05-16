#include <iostream>
#include <Windows.h>

using namespace std;

void childProcessFunction(DWORD childProcessId) {
    cout << "Дочерний процесс: " << childProcessId << endl;
}
int main() {
    setlocale(LC_ALL, "RU");
    //system("chcp 1251");

    // Создаем именованные семафоры
   HANDLE parentSemaphore = CreateSemaphore(NULL, 1, 1, L"ParentSemaphore");
   HANDLE childSemaphore = CreateSemaphore(NULL, 0, 1, L"ChildSemaphore");

    // Создаем процесс-ребенка
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    WCHAR szCmdline[] = L"ChildProcess.exe"; // Имя исполняемого файла дочернего процесса
    if (!CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        cerr << "Ошибка при создании процесса-ребенка: " << GetLastError() << endl;
        return 1;
    }

    // Цикл вывода сообщений
    for (int i = 0; i < 10; ++i) {
        // Если четный шаг, то родительский процесс выводит сообщение
        if (i % 2 == 0) {
            WaitForSingleObject(parentSemaphore, INFINITE);
            cout << "Родительский процесс: " << i + 1 << endl;
            ReleaseSemaphore(childSemaphore, 1, NULL);
        }
        // Иначе дочерний процесс
        else {
            WaitForSingleObject(childSemaphore, INFINITE);
            childProcessFunction(pi.dwProcessId);
            ReleaseSemaphore(parentSemaphore, 1, NULL);
        }
    }

    // Ожидание завершения дочернего процесса
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Закрываем именованные семафоры
    CloseHandle(parentSemaphore);
    CloseHandle(childSemaphore);

    return 0;
}
