#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <bitset>
#include <string>
#include <sstream>
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <filesystem>
#include <thread>
#include <vector>
#define BUFSIZE 8

int _tmain(int argc, TCHAR* argv[])
{
    if (argc <= 1)
    {
        return -1;
    }

    setlocale(LC_CTYPE, "rus");

    std::ifstream  f1;
    char signals_vector[8];
    char bit;

    // получаем строку типа +-+-+-+-.
    // считаем, что - это -1, а + это +1, но с этим будет разбираться общий процесс
    for (int i = 0; i < (sizeof(argv[1])) && i < sizeof(signals_vector); i++)
    {
        signals_vector[i] = (char)argv[1][i];
    }


    HANDLE hPipe;
    LPTSTR lpvMessage;
    TCHAR  chBuf[BUFSIZE];
    BOOL   fSuccess = FALSE;
    DWORD  cbRead, cbToWrite, cbWritten, dwMode;
    LPTSTR lpszPipename = (LPTSTR)TEXT("\\\\.\\pipe\\sender");
    HANDLE hHeap = GetProcessHeap();
    TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
    DWORD cbBytesRead = 0;

    while (1)
    {
        //открываем пайп
        hPipe = CreateFile(
            lpszPipename,   // пайп
            GENERIC_WRITE | 
            GENERIC_READ,   // и писать и читать будем
            0,              // без настроек sharing
            NULL,           // аттрибуты безопасности по умолчанию потому что он все равно игнорится когда это пайп
            OPEN_EXISTING,  // открываем существующий пайп
            0,              // аттрибуты по умолчанию
            NULL);          // без файла атрибутов потому что он все равно игнорится когда это пайп

        //выходим если получили нормальный хэндл на пайп
        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        //если ошибка не в том, что пайп занят, то до свидания
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            printf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
            return -1;
        }

        //ждем освобождения максимум 20 сек
        if (!WaitNamedPipe(lpszPipename, 20000))
        {
            printf("Could not open pipe: 20 second wait timed out.");
            return -1;
        }
    }

    while (true) {
        memset(pchRequest, 0, sizeof(pchRequest));
        //читаем бит, который будем отправлять
        fSuccess = ReadFile(
            hPipe,                //экземпляр пайпа
            pchRequest,              //куда запишем пришедшее сообщение
            1, //размер пришедшего байтика
            &cbBytesRead,            // сколько байтиков пришло
            NULL);                   // не используем overlapped I/O

        if (!fSuccess || cbBytesRead == 0)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {
                printf(TEXT("client disconnected.\n"));
            }
            else
            {
                printf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
            }
            //если была ошибка чтения, то сворачиваем данный экземляр и уходим
            CloseHandle(hPipe);
            return -1;
        }

        bit = (char)(pchRequest[0]);


        char tmp_vector[8];

        for (int i = 0; i < sizeof(signals_vector); i++) {
            if (bit == '0')
                if (signals_vector[i] == '-')
                    tmp_vector[i] = '+';
                else
                    tmp_vector[i] = '-';
            else
                if (signals_vector[i] == '-')
                    tmp_vector[i] = '-';
                else
                    tmp_vector[i] = '+';
        }

        lpvMessage = (LPTSTR)(tmp_vector);

        //отправляем сообщение, в котором зашифрован 1 бит
        fSuccess = WriteFile(
            hPipe,                  // пайп
            lpvMessage,             // сообщение 
            BUFSIZE,                // длина сообщения (всегда 8 байт отправляем) 
            &cbWritten,             // сюда запишется сколько байт послалось 
            NULL);                  // не overlapped 

        if (!fSuccess)
        {
            printf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
            return -1;
        }

    }
    CloseHandle(hPipe);
    return -1;

    return 0;
}
