#define _CRT_SECURE_NO_WARNINGS
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
#include <vector>
#include <map>

// ----- ПРОТОТИПЫ ВСЕХ ФУНКЦИЙ (чтобы избежать ошибок C3861) -----
void LogToFile(const std::string& msg);
void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void draw_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void draw_circle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b);
void draw_text(int x, int y, const std::string& text);
void clear_screen();
void flip_screen();
bool is_key_pressed(const std::string& keyName);
std::string ReadFileContent(const std::wstring& path);
std::wstring GetExeDir();
std::string LoadScript();
void RunScriptAndDisplay(const std::string& source, const std::wstring& title);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CanvasWndProc(HWND, UINT, WPARAM, LPARAM);

// ----- ГРАФИЧЕСКОЕ СОСТОЯНИЕ -----
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 400;
std::vector<uint32_t> screenBuffer(SCREEN_WIDTH * SCREEN_HEIGHT, 0);
bool screenDirty = true;
HWND g_hCanvas = NULL;
std::map<int, bool> keyStates;

// Глобальные переменные для GUI
HWND g_hMainWnd = NULL;
HWND g_hEditOutput = NULL;
HWND g_hBtnRun = NULL;
std::string g_currentScript;

// ----- ГРАФИЧЕСКИЕ ФУНКЦИИ (С++) -----
void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;
    screenBuffer[y * SCREEN_WIDTH + x] = color;
    screenDirty = true;
}
void draw_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    for (int dy = 0; dy < h; ++dy)
        for (int dx = 0; dx < w; ++dx)
            draw_pixel(x + dx, y + dy, r, g, b);
}
void draw_circle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b) {
    for (int y = -radius; y <= radius; ++y)
        for (int x = -radius; x <= radius; ++x)
            if (x*x + y*y <= radius*radius)
                draw_pixel(cx + x, cy + y, r, g, b);
}
void draw_text(int x, int y, const std::string& text) {
    HDC hdc = GetDC(g_hCanvas);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255,255,255));
    TextOutA(hdc, x, y, text.c_str(), (int)text.size());
    ReleaseDC(g_hCanvas, hdc);
}
void clear_screen() {
    std::fill(screenBuffer.begin(), screenBuffer.end(), 0xFF000000);
    screenDirty = true;
}
void flip_screen() {
    screenDirty = true;
    InvalidateRect(g_hCanvas, NULL, FALSE);
    UpdateWindow(g_hCanvas);
}
bool is_key_pressed(const std::string& keyName) {
    int vk = 0;
    if (keyName == "up") vk = VK_UP;
    else if (keyName == "down") vk = VK_DOWN;
    else if (keyName == "left") vk = VK_LEFT;
    else if (keyName == "right") vk = VK_RIGHT;
    else if (keyName == "space") vk = VK_SPACE;
    else if (keyName == "enter") vk = VK_RETURN;
    else if (keyName == "esc") vk = VK_ESCAPE;
    else if (keyName.size()==1 && keyName[0]>='a' && keyName[0]<='z') vk = 'A' + (keyName[0]-'a');
    else if (keyName.size()==1 && keyName[0]>='0' && keyName[0]<='9') vk = keyName[0];
    else return false;
    auto it = keyStates.find(vk);
    return it != keyStates.end() && it->second;
}

// ----- ВСТРОЕННЫЕ ФУНКЦИИ GIG (обёртки) -----
namespace gig {
    Value draw_pixel_func(Environment&, const std::vector<Value>& args) {
        if (args.size()<5) return Value(); int x=(int)args[0].number; int y=(int)args[1].number; int r=(int)args[2].number; int g=(int)args[3].number; int b=(int)args[4].number; draw_pixel(x,y,(uint8_t)r,(uint8_t)g,(uint8_t)b); return Value();
    }
    Value draw_rect_func(Environment&, const std::vector<Value>& args) {
        if (args.size()<7) return Value(); int x=(int)args[0].number; int y=(int)args[1].number; int w=(int)args[2].number; int h=(int)args[3].number; int r=(int)args[4].number; int g=(int)args[5].number; int b=(int)args[6].number; draw_rect(x,y,w,h,(uint8_t)r,(uint8_t)g,(uint8_t)b); return Value();
    }
    Value draw_circle_func(Environment&, const std::vector<Value>& args) {
        if (args.size()<6) return Value(); int cx=(int)args[0].number; int cy=(int)args[1].number; int radius=(int)args[2].number; int r=(int)args[3].number; int g=(int)args[4].number; int b=(int)args[5].number; draw_circle(cx,cy,radius,(uint8_t)r,(uint8_t)g,(uint8_t)b); return Value();
    }
    Value draw_text_func(Environment&, const std::vector<Value>& args) {
        if (args.size()<3) return Value(); int x=(int)args[0].number; int y=(int)args[1].number; auto s=args[2].as_string(); if(!s) return Value(); draw_text(x,y,s->data); return Value();
    }
    Value clear_func(Environment&, const std::vector<Value>&) { clear_screen(); return Value(); }
    Value update_func(Environment&, const std::vector<Value>&) { flip_screen(); return Value(); }
    Value key_pressed_func(Environment&, const std::vector<Value>& args) {
        if(args.empty()) return Value(false);
        auto s=args[0].as_string(); if(!s) return Value(false);
        return Value(is_key_pressed(s->data));
    }
}

// ----- ЛОГГИРОВАНИЕ -----
void LogToFile(const std::string& msg) {
    std::ofstream log("debug.log", std::ios::app);
    if (log.is_open()) {
        time_t now = time(nullptr);
        log << ctime(&now) << " - " << msg << std::endl;
    }
}

// ----- БЕЗОПАСНЫЙ СКРИПТ (с графикой) -----
const char* safeScript = R"(
print("GIG Game Demo")
print("Drawing a red rectangle, blue circle and text.")
clear()
draw_rect(100, 100, 200, 150, 255, 0, 0)
draw_circle(400, 200, 80, 0, 0, 255)
draw_text(50, 50, "Press SPACE to exit")
update()
)";

// ----- ВСТРОЕННЫЙ СКРИПТ -----
const char* embeddedScript = R"(
print("No test.gig found, running embedded script.")
clear()
draw_rect(10, 10, 100, 100, 255, 255, 0)
draw_circle(300, 200, 100, 0, 255, 0)
draw_text(200, 350, "Hello from GIG!")
update()
)";

// ----- ФУНКЦИИ ЗАГРУЗКИ И ВЫПОЛНЕНИЯ -----
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

std::wstring GetExeDir() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring exePath(buffer);
    size_t pos = exePath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) exePath = exePath.substr(0, pos+1);
    return exePath;
}

std::string LoadScript() {
    int argc; LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 2) {
        std::wstring path = argv[1]; LocalFree(argv);
        std::string content = ReadFileContent(path);
        if (!content.empty()) { SetWindowTextW(g_hMainWnd, L"GIG – executing file"); return content; }
    }
    if (argc > 0) LocalFree(argv);
    std::wstring exeDir = GetExeDir();
    std::wstring scriptPath = exeDir + L"test\\test.gig";
    DWORD attr = GetFileAttributesW(scriptPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        std::string content = ReadFileContent(scriptPath);
        if (!content.empty()) { SetWindowTextW(g_hMainWnd, L"GIG – executing test\\test.gig"); return content; }
    }
    SetWindowTextW(g_hMainWnd, L"GIG – executing embedded script");
    return embeddedScript;
}

void RunScriptAndDisplay(const std::string& source, const std::wstring& title = L"Output") {
    LogToFile("RunScriptAndDisplay called");
    if (!g_hEditOutput) return;
    SetWindowTextW(g_hEditOutput, L"Running...");
    std::stringstream outputStream;
    auto old_buf = std::cout.rdbuf(outputStream.rdbuf());

    bool success = false;
    try {
        gig::Lexer lexer(source);
        gig::Parser parser(lexer);
        auto ast = parser.parse();
        if (!ast) throw std::runtime_error("AST is null");
        gig::Interpreter interpreter;
        interpreter.execute(ast.get());
        success = true;
    } catch (const std::exception& e) {
        std::cout.rdbuf(old_buf);
        std::string errMsg = "Parsing error: " + std::string(e.what()) + "\n\nSwitching to safe script...";
        SetWindowTextW(g_hEditOutput, std::wstring(errMsg.begin(), errMsg.end()).c_str());
        static bool recursive = false;
        if (!recursive) {
            recursive = true;
            g_currentScript = safeScript;
            RunScriptAndDisplay(safeScript, L"Safe script");
            recursive = false;
        }
        return;
    } catch (...) {
        std::cout.rdbuf(old_buf);
        SetWindowTextW(g_hEditOutput, L"Unknown exception!");
        static bool recursive = false;
        if (!recursive) {
            recursive = true;
            g_currentScript = safeScript;
            RunScriptAndDisplay(safeScript, L"Safe script");
            recursive = false;
        }
        return;
    }
    std::cout.rdbuf(old_buf);
    if (success) {
        std::string output = outputStream.str();
        if (output.empty()) output = "(no output)";
        std::wstring wideOutput(output.begin(), output.end());
        SetWindowTextW(g_hEditOutput, wideOutput.c_str());
    }
}

// ----- ОБРАБОТЧИК ОКНА -----
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        LogToFile("WM_CREATE started.");
        g_hEditOutput = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            10, 10, 400, 180, hWnd, NULL, NULL, NULL);
        g_hBtnRun = CreateWindowW(L"BUTTON", L"Run Script", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 200, 100, 30, hWnd, (HMENU)1, NULL, NULL);
        g_hCanvas = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_OWNERDRAW,
            420, 10, SCREEN_WIDTH, SCREEN_HEIGHT, hWnd, NULL, NULL, NULL);
        SetWindowLongPtrW(g_hCanvas, GWLP_WNDPROC, (LONG_PTR)CanvasWndProc);
        LogToFile("Controls created.");
        g_currentScript = LoadScript();
        RunScriptAndDisplay(g_currentScript, L"Initial output");
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            g_currentScript = LoadScript();
            RunScriptAndDisplay(g_currentScript, L"Run pressed");
        }
        break;
    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        wchar_t filePath[MAX_PATH];
        DragQueryFileW(hDrop, 0, filePath, MAX_PATH);
        DragFinish(hDrop);
        std::wstring ext = filePath;
        if (ext.size() >= 4 && ext.substr(ext.size()-4) == L".gig") {
            std::string content = ReadFileContent(filePath);
            if (!content.empty()) {
                g_currentScript = content;
                SetWindowTextW(g_hMainWnd, L"GIG – executing dropped file");
                RunScriptAndDisplay(g_currentScript, L"Dropped file");
            }
        } else {
            MessageBoxW(hWnd, L"Please drop a .gig file.", L"Warning", MB_OK);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ----- ОБРАБОТЧИК ХОЛСТА (клавиши) -----
LRESULT CALLBACK CanvasWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
        keyStates[(int)wParam] = true;
        return 0;
    case WM_KEYUP:
        keyStates[(int)wParam] = false;
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = SCREEN_WIDTH;
        bmi.bmiHeader.biHeight = -SCREEN_HEIGHT;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        SetDIBitsToDevice(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                          0, 0, 0, SCREEN_HEIGHT,
                          screenBuffer.data(), &bmi, DIB_RGB_COLORS);
        EndPaint(hWnd, &ps);
        return 0;
    }
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// ----- ТОЧКА ВХОДА -----
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::ofstream log("debug.log", std::ios::trunc); log.close();
    LogToFile("=== GIG STARTED ===");

    INITCOMMONCONTROLSEX icex = {sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GIGWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    g_hMainWnd = CreateWindowW(L"GIGWindowClass", L"GIG – Game Interpreter",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 500,
        NULL, NULL, hInstance, NULL);
    if (!g_hMainWnd) return 1;

    DragAcceptFiles(g_hMainWnd, TRUE);
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    LogToFile("=== GIG EXITED ===");
    return (int)msg.wParam;
}
