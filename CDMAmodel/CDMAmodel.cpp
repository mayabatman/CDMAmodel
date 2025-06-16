#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <bitset> 
#include <thread>

#define BUFSIZESENDER 8
#define BUFSIZERECEIVER 32

bool check_input_signals(char* str) {
    if (sizeof(str) != 8) return false;
    for (int i = 0; i < 8; i++) {
        if (str[i] != '+' && str[i] != '-')
            return false;
    }
    return true;
}

bool check_bit(char *bit) {
    if((sizeof(bit) / sizeof(char*))!=1)
        return false;
    if (bit[0] != '0' && bit[0] != '1')
        return false;
    return true;
}

int _tmain(VOID)
{
    setlocale(LC_CTYPE, "rus");

    //---- делаем пайпы

    BOOL   fConnected = FALSE;
    DWORD  dwThreadId = 0;
    HANDLE hPipeSender[4], hPipeReceiver[4];
    LPCTSTR NameSender = TEXT("\\\\.\\pipe\\sender");
    LPCTSTR NameReceiver = TEXT("\\\\.\\pipe\\receiver");

    //четыре экземпляра сендера
    for (int i = 0; i < 4; i++) {
        hPipeSender[i] = CreateNamedPipe(
            NameSender,               // имя пайпа
            PIPE_ACCESS_DUPLEX,       // в обе стороны работаетр
            PIPE_TYPE_BYTE |          // принимаем байтики
            PIPE_READMODE_BYTE |      // принимаем байтики
            PIPE_WAIT,                // функции write read блокируются пока не получится выполнить
            PIPE_UNLIMITED_INSTANCES, // максимальное количество экземпляров пайпа
            BUFSIZESENDER,                  // размер буфера выхода
            BUFSIZESENDER,                  // размер буфера входа
            0,                        // таймайт клиента будет 50 мс
            NULL);                    // аттрибуты безопасности будут по умолчанию

        if (hPipeSender[i] == INVALID_HANDLE_VALUE)
        {
            printf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError());
            return -1;
        }
    }
    //четыре экземпляра ресивера
    for (int i = 0; i < 4; i++) {
        hPipeReceiver[i] = CreateNamedPipe(
            NameReceiver,             // имя пайпа
            PIPE_ACCESS_DUPLEX,       // в обе стороны работает
            PIPE_TYPE_BYTE |          // принимаем байтики
            PIPE_READMODE_BYTE |      // принимаем байтики
            PIPE_WAIT,                // функции write read блокируются пока не получится выполнить
            PIPE_UNLIMITED_INSTANCES, // максимальное количество экземпляров пайпа
            BUFSIZERECEIVER,                  // размер буфера выхода
            BUFSIZERECEIVER,                  // размер буфера входа
            0,                        // таймайт клиента будет 50 мс
            NULL);                    // аттрибуты безопасности будут по умолчанию

        if (hPipeReceiver[i] == INVALID_HANDLE_VALUE)
        {
            printf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError());
            return -1;
        }
    }
    //получаем полный путь к файлу
    TCHAR buffer[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    std::string cur_path = std::string(buffer).substr(0, pos);
    std::string path_sender = cur_path + "\\CDMAsender.exe";
    std::string path_receiver = cur_path + "\\CDMAreceiver.exe";
    //буфер сигналов
    char * signals = new char[8];
    //не делем их индиувидуальными, потому что нам они вообще не нужны
    STARTUPINFO si[8]; 
    PROCESS_INFORMATION pi[8];
    for (int i = 0; i < 8; i++) {
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        ZeroMemory(&pi[i], sizeof(pi[i]));
    }
    //---- получаем данные для работы
    //---- делаем 4 процесса-сендера
    //---- делаем 4 процесса-ресивера

    for (int i = 0; i < 4; i++) {
        do {
            std::cout << "Введите элементарные сигналы для " << i << " пары в формате \"+-+-+-+-\": ";
            std::cin >> signals;
        } while (!check_input_signals(signals));

        std::string path = path_sender + " " + signals;

       if (!CreateProcess(NULL,
           (LPTSTR)const_cast<char*>(path.c_str()),        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si[i],            // Pointer to STARTUPINFO structure
            &pi[i])            // Pointer to PROCESS_INFORMATION structure
            )
        {
            printf("CreateProcess failed (%d).\n", GetLastError());
            return -1;
        }
        path = path_receiver + " \"" + signals + "\"";

        if (!CreateProcess(NULL,
            (LPTSTR)const_cast<char*>(path.c_str()),        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si[i+4],            // Pointer to STARTUPINFO structure
            &pi[i+4])            // Pointer to PROCESS_INFORMATION structure
            )
        {
            printf("CreateProcess failed (%d).\n", GetLastError());
            return -1;
        }

    }

    //---- ждем подключения четыре экземпляра сендера


    for (int i = 0; i < 4; i++) {
        fConnected = ConnectNamedPipe(hPipeSender[i], NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected){}
        else {
            printf("%d Client connection fail %d", i + 1, GetLastError());
            //если у сендера не получилось подключиться нахер сворачиваемся
            CloseHandle(hPipeSender[i]);
            return -1;
        }
    }

    //---- ждем подключения четырех экземпляра ресиверов
    for (int i = 0; i < 4; i++) {
        fConnected = ConnectNamedPipe(hPipeReceiver[i], NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected){}
        else {
            printf("%d Client connection fail %d", i + 1, GetLastError());
            //если у сендера не получилось подключиться нахер сворачиваемся
            CloseHandle(hPipeReceiver[i]);
            return -1;
        }
    }

    HANDLE hHeap = GetProcessHeap();
    BOOL fSuccess = FALSE;
    TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZESENDER * sizeof(TCHAR));
    DWORD cbBytesRead = 0;
    int8_t message_receiver[8];
    LPTSTR lpvMessage;
    DWORD cbWritten;

    while (true) {

        //---- отправляем sender-ам, какие у них биты

        char* bit = new char[1];

        for (int i = 0; i < 4; i++) {
            do {
                std::cout << "Введите бит для " << i << " пары: ";
                std::cin >> bit;
            } while (!check_bit(bit));

            lpvMessage = (LPTSTR)(bit);

            //отправляем сообщение, в котором зашифрован 1 бит
            fSuccess = WriteFile(
                hPipeSender[i],        // пайп
                lpvMessage,             // сообщение 
                1,                // длина сообщения (всегда 1 байт отправляем) 
                &cbWritten,             // сюда запишется сколько байт послалось 
                NULL);                  // не overlapped 

            if (!fSuccess)
            {
                printf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
                return -1;
            }
        }


        //---- получаем по отправленному сообщению от каждого сендера
        memset(pchRequest, 0, sizeof(pchRequest));

        for (int c = 0; c < sizeof(message_receiver); c++) {
            message_receiver[c] = 0;
        }

        for (int i = 0; i < 4; i++) {
            //читаем (ждем) что там ответил sender
            fSuccess = ReadFile(
                hPipeSender[i],          //экземпляр пайпа
                pchRequest,              //куда запишем пришедший байтик
                BUFSIZESENDER,           //размер пришедшего байтика
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
                FlushFileBuffers(hPipeSender[i]);
                DisconnectNamedPipe(hPipeSender[i]);
                CloseHandle(hPipeSender[i]);
                hPipeSender[i] = INVALID_HANDLE_VALUE;
                continue;
            }

            //---- преобразуем сообщения от сендеров

            char* tmp_mes = (char*)pchRequest;
            for (int c = 0; c < sizeof(tmp_mes); c++) {
                if (tmp_mes[c] == '+')
                    message_receiver[c] += 1;
                else
                    message_receiver[c] -= 1;
            }
        }

        //---- преобразуем полученный массив в сообщение
        std::string message_string;
        for (int c = 0; c < sizeof(message_receiver); c++) {
            char* simp = new char[8];
            _itoa_s(message_receiver[c], simp, 8, 10);
            message_string.append(simp);
            message_string.append(";");
        }

        //---- рассылаем одно большое сообщение ресиверам, которое будут расшифровывать уже они 

        std::cout << "Обработанная последовательность битов: ";

        lpvMessage = (LPTSTR)(message_string.c_str());

        for (int i = 0; i < 4; i++) {
            //отправляем сообщение, в котором зашифрован 1 бит
            fSuccess = WriteFile(
                hPipeReceiver[i],       // пайп
                lpvMessage,             // сообщение 
                BUFSIZERECEIVER,                // длина сообщения (всегда 8 байт отправляем) 
                &cbWritten,             // сюда запишется сколько байт послалось 
                NULL);                  // не overlapped 

            if (!fSuccess)
            {
                printf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
                return -1;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << std::endl;
    }

    for (int i = 0; i < 8; i++) {
        // Wait until child process exits.
        WaitForSingleObject(pi[i].hProcess, INFINITE);

        // Close process and thread handles. 
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }
        
    

    //освобождаем буфер
    HeapFree(hHeap, 0, pchRequest);
    //выход
    printf("exit\n");

    return 0;
}