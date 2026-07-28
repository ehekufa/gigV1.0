#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

// ----- ВСТРОЕННЫЙ СКРИПТ (резервный) -----
const char* embeddedScript = R"(
print("Hello from GIG!")
print("This is the embedded fallback script.")
print("To use your own script, place test/test.gig next to this executable.")

function fact(n)
    if n == 0 then return 1 end
    return n * fact(n - 1)
end

local result = fact(5)
print("fact(5) =", result)

print("Fallback script finished.")
)";

// Глобальные переменные для GUI
HWND g_hMainWnd = NULL;
HWND g_hEditOutput = NULL;
HWND g_hBtnRun = NULL;
std::string g_currentScript; // текущий скрипт для выполнения

// ----- Выполнить скрипт и вывести результат в поле -----
void RunScriptAndDisplay(const std::string& source, const std::wstring& title = L"Output") {
    // Перенаправляем stdout в строку
    std::stringstream outputStream;
    auto old_buf = std::cout.rdbuf(outputStream.rdbuf());

    gig::Lexer lexer(source);
    gig::Parser parser(lexer);
    auto ast = parser.parse();

    if (!ast) {
        std::cout.rdbuf(old_buf);
        std::string error = "Parsing failed!";
        SetWindowTextW(g_hEditOutput, std::wstring(error.begin(), error.end()).c_str());
        return;
    }

    gig::Interpreter interpreter;
    interpreter.execute(ast.get());

    std::cout.rdbuf(old_buf);

    std::string output = outputStream.str();
    if (output.empty()) output = "(no output)";

    // Преобразуем в wide-строку для SetWindowTextW
    std::wstring wideOutput(output.begin(), output.end());
    SetWindowTextW(g_hEditOutput, wideOutput.c_str());
}

// ----- Загрузить скрипт из файла -----
std::string ReadFileContent(const std::wstring& path) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, NULL, 0, NULL, NULL);
    std::string filename(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, &filename[0], size_needed, NULL, NULL);
    filename.pop_back();

    std::ifstream file(filename);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ----- Получить папку, где находится EXE -----
std::wstring GetExeDir() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring exePath(buffer);
    size_t pos = exePath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exePath = exePath.substr(0, pos + 1);
    }
    return exePath;
}

// ----- Загрузить скрипт для выполнения (приоритет: аргумент, test/test.gig, встроенный) -----
std::string LoadScript() {
    // Сначала проверяем, был ли передан файл через командную строку
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 2) {
        std::wstring path = argv[1];
        LocalFree(argv);
        std::string content = ReadFileContent(path);
        if (!content.empty()) {
            SetWindowTextW(g_hMainWnd, L"GIG – executing file");
            return content;
        }
    }
    LocalFree(argv);

    // Если нет аргумента, ищем test\test.gig рядом с EXE
    std::wstring exeDir = GetExeDir();
    std::wstring scriptPath = exeDir + L"test\\test.gig";
    DWORD attr = GetFileAttributesW(scriptPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        std::string content = ReadFileContent(scriptPath);
        if (!content.empty()) {
            SetWindowTextW(g_hMainWnd, L"GIG – executing test\\test.gig");
            return content;
        }
    }

    // Иначе используем встроенный
    SetWindowTextW(g_hMainWnd, L"GIG – executing embedded script");
    return embeddedScript;
}

// ----- Обработчик сообщений окна -----
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Создаём многострочное текстовое поле для вывода
        g_hEditOutput = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            10, 10, 600, 300,
            hWnd, NULL, NULL, NULL);

        // Кнопка "Run"
        g_hBtnRun = CreateWindowW(L"BUTTON", L"Run Script",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 320, 100, 30,
            hWnd, (HMENU)1, NULL, NULL);

        // Загружаем и выполняем скрипт
        g_currentScript = LoadScript();
        RunScriptAndDisplay(g_currentScript, L"Initial output");
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) { // кнопка Run
            // Перезагружаем скрипт (по тому же приоритету) или используем текущий
            g_currentScript = LoadScript();
            RunScriptAndDisplay(g_currentScript, L"Run pressed");
        }
        break;
    }
    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        wchar_t filePath[MAX_PATH];
        DragQueryFileW(hDrop, 0, filePath, MAX_PATH);
        DragFinish(hDrop);

        // Проверяем расширение .gig
        std::wstring ext = filePath;
        if (ext.size() >= 4 && ext.substr(ext.size() - 4) == L".gig") {
            std::string content = ReadFileContent(filePath);
            if (!content.empty()) {
                g_currentScript = content;
                std::wstring title = L"GIG – executing " + std::wstring(filePath);
                SetWindowTextW(g_hMainWnd, title.c_str());
                RunScriptAndDisplay(g_currentScript, title);
            }
        } else {
            MessageBoxW(hWnd, L"Please drop a .gig file.", L"Warning", MB_OK | MB_ICONWARNING);
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ----- ТОЧКА ВХОДА -----
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Инициализация общих элементов управления (для кнопок и т.д.)
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // Регистрируем класс окна
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GIGWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Failed to register window class.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Создаём окно
    g_hMainWnd = CreateWindowW(L"GIGWindowClass", L"GIG – Language Interpreter",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 420,
        NULL, NULL, hInstance, NULL);

    if (!g_hMainWnd) {
        MessageBoxW(NULL, L"Failed to create main window.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Разрешаем Drag&Drop
    DragAcceptFiles(g_hMainWnd, TRUE);

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    // Цикл сообщений
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
