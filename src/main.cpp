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
#include <ctime>

// ----- ЛОГГИРОВАНИЕ (для отладки) -----
void LogToFile(const std::string& msg) {
    std::ofstream log("debug.log", std::ios::app);
    if (log.is_open()) {
        time_t now = time(nullptr);
        log << ctime(&now) << " - " << msg << std::endl;
    }
}

// ----- БЕЗОПАСНЫЙ СКРИПТ (без таблиц, без сложных вызовов) -----
const char* safeScript = R"(
print("Hello from GIG!")
print("This is the SAFE fallback script.")
print("Your test.gig had parsing errors, so this script runs instead.")

function fact(n)
    if n == 0 then return 1 end
    return n * fact(n - 1)
end

local result = fact(5)
print("fact(5) =", result)

print("Script finished successfully.")
)";

// ----- ВСТРОЕННЫЙ СКРИПТ (используется, если test.gig не найден) -----
const char* embeddedScript = R"(
print("Hello from GIG!")
print("No test.gig found, running embedded script.")

function fact(n)
    if n == 0 then return 1 end
    return n * fact(n - 1)
end

local result = fact(5)
print("fact(5) =", result)

print("Embedded script finished.")
)";

// Глобальные переменные
HWND g_hMainWnd = NULL;
HWND g_hEditOutput = NULL;
HWND g_hBtnRun = NULL;
std::string g_currentScript;

// ----- Выполнить скрипт и вывести результат (с автоматическим переходом на safeScript при ошибке) -----
void RunScriptAndDisplay(const std::string& source, const std::wstring& title = L"Output") {
    LogToFile("RunScriptAndDisplay called, source length: " + std::to_string(source.size()));

    if (!g_hEditOutput) {
        LogToFile("ERROR: g_hEditOutput is NULL!");
        return;
    }

    SetWindowTextW(g_hEditOutput, L"Running...");

    std::stringstream outputStream;
    auto old_buf = std::cout.rdbuf(outputStream.rdbuf());

    bool success = false;
    try {
        LogToFile("Creating Lexer...");
        gig::Lexer lexer(source);
        LogToFile("Creating Parser...");
        gig::Parser parser(lexer);
        LogToFile("Parsing...");
        auto ast = parser.parse();

        if (!ast) {
            LogToFile("Parsing failed (ast is null).");
            throw std::runtime_error("AST is null");
        }

        LogToFile("Creating Interpreter...");
        gig::Interpreter interpreter;
        LogToFile("Executing...");
        interpreter.execute(ast.get());
        LogToFile("Execution finished.");
        success = true;
    } catch (const std::exception& e) {
        LogToFile("Exception: " + std::string(e.what()));
        std::cout.rdbuf(old_buf);
        // Показываем сообщение об ошибке
        std::string errMsg = "Parsing error: " + std::string(e.what()) + "\n\nSwitching to safe script...";
        SetWindowTextW(g_hEditOutput, std::wstring(errMsg.begin(), errMsg.end()).c_str());
        // Заменяем текущий скрипт на безопасный и выполняем его
        g_currentScript = safeScript;
        // Рекурсивно вызываем себя с безопасным скриптом (чтобы он выполнился и показал вывод)
        // Чтобы избежать бесконечной рекурсии, передаём флаг, но проще сделать так:
        // Мы уже установили g_currentScript = safeScript, и дальше выполним его.
        // Но чтобы не зациклиться, используем отдельную функцию.
        // Вместо рекурсии просто выполняем безопасный скрипт напрямую.
        // Но нам нужно сбросить буфер вывода.
        // Проще: заново запустить RunScriptAndDisplay с safeScript, но без рекурсии.
        // Для этого используем флаг.
        static bool recursive = false;
        if (!recursive) {
            recursive = true;
            RunScriptAndDisplay(safeScript, L"Safe script (fallback)");
            recursive = false;
        }
        return;
    } catch (...) {
        LogToFile("Unknown exception!");
        std::cout.rdbuf(old_buf);
        SetWindowTextW(g_hEditOutput, L"Unknown exception! Switching to safe script.");
        g_currentScript = safeScript;
        static bool recursive = false;
        if (!recursive) {
            recursive = true;
            RunScriptAndDisplay(safeScript, L"Safe script (fallback)");
            recursive = false;
        }
        return;
    }

    std::cout.rdbuf(old_buf);

    if (success) {
        std::string output = outputStream.str();
        if (output.empty()) output = "(no output)";
        LogToFile("Output length: " + std::to_string(output.size()));
        std::wstring wideOutput(output.begin(), output.end());
        SetWindowTextW(g_hEditOutput, wideOutput.c_str());
    } else {
        // Если success == false, но исключение не было брошено (маловероятно)
        SetWindowTextW(g_hEditOutput, L"Unknown error occurred.");
    }

    LogToFile("Display complete.");
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

// ----- Получить папку EXE -----
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

// ----- Загрузить скрипт (приоритет: аргумент → test/test.gig → встроенный) -----
std::string LoadScript() {
    LogToFile("LoadScript called.");

    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 2) {
        std::wstring path = argv[1];
        LocalFree(argv);
        LogToFile("Argument file: " + std::string(path.begin(), path.end()));
        std::string content = ReadFileContent(path);
        if (!content.empty()) {
            LogToFile("Loaded from argument.");
            SetWindowTextW(g_hMainWnd, L"GIG – executing file");
            return content;
        }
    }
    if (argc > 0) LocalFree(argv);

    std::wstring exeDir = GetExeDir();
    std::wstring scriptPath = exeDir + L"test\\test.gig";
    LogToFile("Looking for test.gig at: " + std::string(scriptPath.begin(), scriptPath.end()));
    DWORD attr = GetFileAttributesW(scriptPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        std::string content = ReadFileContent(scriptPath);
        if (!content.empty()) {
            LogToFile("Loaded test\\test.gig.");
            SetWindowTextW(g_hMainWnd, L"GIG – executing test\\test.gig");
            return content;
        }
    }

    LogToFile("Using embedded script.");
    SetWindowTextW(g_hMainWnd, L"GIG – executing embedded script");
    return embeddedScript;
}

// ----- Обработчик сообщений -----
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        LogToFile("WM_CREATE started.");
        g_hEditOutput = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            10, 10, 600, 300,
            hWnd, NULL, NULL, NULL);

        g_hBtnRun = CreateWindowW(L"BUTTON", L"Run Script",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 320, 100, 30,
            hWnd, (HMENU)1, NULL, NULL);

        LogToFile("Controls created.");
        g_currentScript = LoadScript();
        LogToFile("Script loaded, running...");
        RunScriptAndDisplay(g_currentScript, L"Initial output");
        LogToFile("WM_CREATE finished.");
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) {
            LogToFile("Run button clicked.");
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
        LogToFile("WM_DESTROY.");
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
    std::ofstream log("debug.log", std::ios::trunc);
    log.close();
    LogToFile("=== GIG STARTED ===");

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&icex)) {
        LogToFile("InitCommonControlsEx failed.");
        MessageBoxW(NULL, L"Failed to initialize common controls.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GIGWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc)) {
        LogToFile("RegisterClassW failed.");
        MessageBoxW(NULL, L"Failed to register window class.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_hMainWnd = CreateWindowW(L"GIGWindowClass", L"GIG – Language Interpreter",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 420,
        NULL, NULL, hInstance, NULL);

    if (!g_hMainWnd) {
        LogToFile("CreateWindowW failed.");
        MessageBoxW(NULL, L"Failed to create main window.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    DragAcceptFiles(g_hMainWnd, TRUE);
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    LogToFile("Window shown.");

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    LogToFile("=== GIG EXITED ===");
    return (int)msg.wParam;
}
