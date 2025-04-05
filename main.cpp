#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <psapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "psapi.lib")

struct EnumData {
    std::string targetProcessNameLower;
    std::string actualTitle;
    HWND foundHwnd = NULL;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumData* pData = reinterpret_cast<EnumData*>(lParam);
    if (pData == nullptr || pData->targetProcessNameLower.empty()) {
        return FALSE;
    }
    if (!IsWindowVisible(hwnd) || GetWindowTextLengthA(hwnd) == 0) {
        return TRUE;
    }
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) {
        return TRUE;
    }
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) {
        return TRUE;
    }
    char processName[MAX_PATH] = "<unknown>";

    if (GetModuleBaseNameA(hProcess, NULL, processName, MAX_PATH) > 0) {
        std::string currentProcessName = processName;
        std::string currentProcessNameLower = currentProcessName;
        std::transform(currentProcessNameLower.begin(), currentProcessNameLower.end(), currentProcessNameLower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (currentProcessNameLower == pData->targetProcessNameLower) {
            const int titleBufferSize = 256;
            char windowTitle[titleBufferSize];
            GetWindowTextA(hwnd, windowTitle, titleBufferSize);
            pData->actualTitle = windowTitle;
            pData->foundHwnd = hwnd;
            CloseHandle(hProcess);
            return FALSE;
        }
    }
    CloseHandle(hProcess);
    return TRUE;
}

void printUsage(const char* progName) {
    std::cerr << "Uso: " << progName << " -x<coord> -y<coord> -p<nome_processo> [-u]\n"
              << "Move a primeira janela encontrada pertencente ao processo <nome_processo> para (x, y).\n\n"
              << "Opções:\n"
              << "  -x<coord>         Coordenada X de destino.\n"
              << "  -y<coord>         Coordenada Y de destino.\n"
              << "  -p<nome_processo> Nome do executável (ex: notepad.exe).\n"
              << "  -u                Esperar indefinidamente que a janela apareça.\n\n"
              << "Exemplo 1: movew -x1024 -y0 -pnotepad.exe\n"
              << "Exemplo 2: movew -x0 -y0 -pnotepad.exe -u\n";
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;

	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << "MoveW - Utilitário para Mover Janelas" << std::endl;
    SetConsoleTextAttribute(hConsole, saved_attributes);

	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Copyright (C) 2018 MAPENO.pt" << std::endl << std::endl;
    SetConsoleTextAttribute(hConsole, saved_attributes);

    int targetX = 0;
    int targetY = 0;
    std::string processName;
    bool waitForWindow = false;
    bool xSet = false;
    bool ySet = false;
    bool pSet = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        try {
            if (arg.rfind("-x", 0) == 0) {
                targetX = std::stoi(arg.substr(2));
                xSet = true;
            } else if (arg.rfind("-y", 0) == 0) {
                targetY = std::stoi(arg.substr(2));
                ySet = true;
            } else if (arg.rfind("-p", 0) == 0) {
                processName = arg.substr(2);
                if (processName.empty()) {
                   throw std::invalid_argument("Process name cannot be empty.");
                }
                pSet = true;
            } else if (arg == "-u") {
                waitForWindow = true;
            } else {
                std::cerr << "Erro: Argumento desconhecido '" << arg << "'\n\n";
                printUsage(argv[0]);
                return 1;
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Erro: Valor inválido para o argumento '" << arg << "'. As coordenadas devem ser inteiros. O nome do processo deve ser válido. Detalhes: " << e.what() << "\n\n";
            printUsage(argv[0]);
            return 1;
        } catch (const std::out_of_range& e) {
             std::cerr << "Erro: Valor fora do intervalo para o argumento '" << arg << "'. Detalhes: " << e.what() << "\n\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (!xSet || !ySet || !pSet) {
        std::cerr << "Erro: Faltam argumentos obrigatórios (-x, -y, -p).\n\n";
        printUsage(argv[0]);
        return 1;
    }

    EnumData searchData;
    searchData.targetProcessNameLower = processName;

    std::transform(searchData.targetProcessNameLower.begin(), searchData.targetProcessNameLower.end(), searchData.targetProcessNameLower.begin(),
                    [](unsigned char c){ return std::tolower(c); });
    if (waitForWindow) {
        std::cout << "A aguardar pela janela pertencente ao processo \"" << processName << "\"..." << std::endl;
        while (true) {
            EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&searchData));
            if (searchData.foundHwnd != NULL) {
                std::cout << "Janela encontrada \"" << searchData.actualTitle << "\" (HWND: " << searchData.foundHwnd << ") pertencente ao processo \"" << processName << "\"." << std::endl;
                break;
            }
            searchData.foundHwnd = NULL;
            searchData.actualTitle.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } else {
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&searchData));
    }

    HWND hwnd = searchData.foundHwnd;
    if (hwnd == NULL) {
        std::cerr << "Erro: Janela pertencente ao processo \"" << processName << "\" não encontrada." << std::endl;
        if (!waitForWindow) {
             std::cerr << "Use a opção -u para esperar que a janela apareça." << std::endl;
        }
        return 1;
    }

    std::cout << "A tentar mover a janela \"" << searchData.actualTitle << "\" (HWND: " << hwnd << ") pertencente ao processo \"" << processName << "\" para (" << targetX << ", " << targetY << ")..." << std::endl;

    RECT windowRect;
    bool movedSuccessfully = false;
    if (!GetWindowRect(hwnd, &windowRect)) {
         std::cerr << "Aviso: Não foi possível obter as dimensões atuais da janela. A mover com opções padrão." << std::endl;

         if (SetWindowPos(hwnd, NULL, targetX, targetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE)) {
             movedSuccessfully = true;
         } else {
            DWORD error = GetLastError();
            std::cerr << "Erro: Falha ao mover a janela. Código de erro SetWindowPos: " << error << std::endl;
            return 1;
         }
    } else {
        int width = windowRect.right - windowRect.left;
        int height = windowRect.bottom - windowRect.top;
        if (SetWindowPos(hwnd, NULL, targetX, targetY, width, height, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE)) {
            movedSuccessfully = true;
        } else {
            DWORD error = GetLastError();
            std::cerr << "Erro: Falha ao mover a janela preservando o tamanho. Código de erro SetWindowPos: " << error << std::endl;
            return 1;
        }
    }
    if (movedSuccessfully) {
        std::cout << "Janela \"" << searchData.actualTitle << "\" pertencente ao processo \"" << processName << "\" movida com sucesso." << std::endl;
        return 0;
    } else {
        std::cerr << "A operação de mover a janela reportou falha." << std::endl;
        return 1;
    }
}
