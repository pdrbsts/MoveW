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

// Structure to pass data to the EnumWindows callback
struct EnumData {
    std::string targetProcessNameLower;
    std::string actualTitle;
    HWND foundHwnd = NULL;
};

// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumData* pData = reinterpret_cast<EnumData*>(lParam);
    if (pData == nullptr || pData->targetProcessNameLower.empty()) {
        return FALSE; // Stop enumeration if data is invalid or target is empty
    }

    // Skip windows that are not visible or have no title (often background processes)
    if (!IsWindowVisible(hwnd) || GetWindowTextLengthA(hwnd) == 0) {
        return TRUE; // Continue enumeration
    }

    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) {
        return TRUE; // Continue if we couldn't get the process ID
    }

    // Get a handle to the process. PROCESS_QUERY_INFORMATION and PROCESS_VM_READ are needed.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) {
        return TRUE; // Continue if we couldn't open the process (might lack permissions)
    }

    char processName[MAX_PATH] = "<unknown>";
    // Get the process name
    if (GetModuleBaseNameA(hProcess, NULL, processName, MAX_PATH) > 0) {
        std::string currentProcessName = processName;
        std::string currentProcessNameLower = currentProcessName;
        // Convert current process name to lowercase
        std::transform(currentProcessNameLower.begin(), currentProcessNameLower.end(), currentProcessNameLower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        // Compare with target process name (case-insensitive)
        if (currentProcessNameLower == pData->targetProcessNameLower) {
            // Get the window title for reporting purposes
            const int titleBufferSize = 256;
            char windowTitle[titleBufferSize];
            GetWindowTextA(hwnd, windowTitle, titleBufferSize);
            pData->actualTitle = windowTitle; // Store the title of the found window

            pData->foundHwnd = hwnd; // Window found! Store its handle
            CloseHandle(hProcess);
            return FALSE;           // Stop enumeration
        }
    }

    CloseHandle(hProcess);
    return TRUE; // Continue enumeration
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
    // Set console output to UTF-8 to support Portuguese characters
    SetConsoleOutputCP(CP_UTF8);

    // Get console handle and default attributes
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;

    // Print title in bright white
    //SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << "MoveW - Utilitário para Mover Janelas" << std::endl;
    SetConsoleTextAttribute(hConsole, saved_attributes); // Reset color

    // Print copyright in bright yellow
    //SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Copyright (C) 2018 MAPENO.pt" << std::endl << std::endl;
    SetConsoleTextAttribute(hConsole, saved_attributes); // Reset color

    int targetX = 0;
    int targetY = 0;
    std::string processName;
    bool waitForWindow = false;
    bool xSet = false;
    bool ySet = false;
    bool pSet = false; // Changed from fSet

    // --- Argument Parsing ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        try {
            if (arg.rfind("-x", 0) == 0) { // Starts with -x
                targetX = std::stoi(arg.substr(2));
                xSet = true;
            } else if (arg.rfind("-y", 0) == 0) { // Starts with -y
                targetY = std::stoi(arg.substr(2));
                ySet = true;
            } else if (arg.rfind("-p", 0) == 0) { // Starts with -p (changed from -f)
                processName = arg.substr(2);
                if (processName.empty()) {
                   throw std::invalid_argument("Process name cannot be empty.");
                }
                pSet = true; // Changed from fSet
            } else if (arg == "-u") {
                waitForWindow = true;
            } else {
                std::cerr << "Erro: Argumento desconhecido '" << arg << "'\n\n"; // Translated
                printUsage(argv[0]);
                return 1;
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Erro: Valor inválido para o argumento '" << arg << "'. As coordenadas devem ser inteiros. O nome do processo deve ser válido. Detalhes: " << e.what() << "\n\n"; // Translated
            printUsage(argv[0]);
            return 1;
        } catch (const std::out_of_range& e) {
             std::cerr << "Erro: Valor fora do intervalo para o argumento '" << arg << "'. Detalhes: " << e.what() << "\n\n"; // Translated
            printUsage(argv[0]);
            return 1;
        }
    }

    // --- Validate Required Arguments ---
    if (!xSet || !ySet || !pSet) { // Changed from fSet
        std::cerr << "Erro: Faltam argumentos obrigatórios (-x, -y, -p).\n\n"; // Translated
        printUsage(argv[0]);
        return 1;
    }

    // --- Find the Window ---
    EnumData searchData;
    searchData.targetProcessNameLower = processName;
    // Convert target process name to lowercase
    std::transform(searchData.targetProcessNameLower.begin(), searchData.targetProcessNameLower.end(), searchData.targetProcessNameLower.begin(),
                    [](unsigned char c){ return std::tolower(c); });

    if (waitForWindow) {
        std::cout << "A aguardar pela janela pertencente ao processo \"" << processName << "\"..." << std::endl; // Translated
        while (true) {
            EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&searchData));
            if (searchData.foundHwnd != NULL) {
                std::cout << "Janela encontrada \"" << searchData.actualTitle << "\" (HWND: " << searchData.foundHwnd << ") pertencente ao processo \"" << processName << "\"." << std::endl; // Translated
                break; // Found it!
            }
            // Wait for a short period before checking again
            searchData.foundHwnd = NULL; // Reset for the next iteration
            searchData.actualTitle.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } else {
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&searchData));
    }

    // --- Check if Window Found ---
    HWND hwnd = searchData.foundHwnd; // Get the HWND from the struct
    if (hwnd == NULL) {
        std::cerr << "Erro: Janela pertencente ao processo \"" << processName << "\" não encontrada." << std::endl; // Translated
        if (!waitForWindow) {
             std::cerr << "Use a opção -u para esperar que a janela apareça." << std::endl; // Translated
        }
        return 1;
    }

    // --- Move the Window ---
    std::cout << "A tentar mover a janela \"" << searchData.actualTitle << "\" (HWND: " << hwnd << ") pertencente ao processo \"" << processName << "\" para (" << targetX << ", " << targetY << ")..." << std::endl; // Translated

    // Get current window dimensions to preserve them
    RECT windowRect;
    bool movedSuccessfully = false; // Flag to track success
    if (!GetWindowRect(hwnd, &windowRect)) {
         std::cerr << "Aviso: Não foi possível obter as dimensões atuais da janela. A mover com opções padrão." << std::endl; // Translated
         // Use SWP_NOSIZE if getting rect fails
         if (SetWindowPos(hwnd, NULL, targetX, targetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE)) {
             movedSuccessfully = true;
         } else {
            DWORD error = GetLastError();
            std::cerr << "Erro: Falha ao mover a janela. Código de erro SetWindowPos: " << error << std::endl; // Translated
            return 1; // Exit on failure
         }
    } else {
        int width = windowRect.right - windowRect.left;
        int height = windowRect.bottom - windowRect.top;
        if (SetWindowPos(hwnd, NULL, targetX, targetY, width, height, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE)) {
            movedSuccessfully = true;
        } else {
            DWORD error = GetLastError();
            std::cerr << "Erro: Falha ao mover a janela preservando o tamanho. Código de erro SetWindowPos: " << error << std::endl; // Translated
            return 1; // Exit on failure
        }
    }

    if (movedSuccessfully) {
        std::cout << "Janela \"" << searchData.actualTitle << "\" pertencente ao processo \"" << processName << "\" movida com sucesso." << std::endl; // Translated
        return 0; // Success
    } else {
        // Although the code above returns on error, this is for logical completeness
        std::cerr << "A operação de mover a janela reportou falha." << std::endl; // Translated
        return 1; // Failure
    }
}
