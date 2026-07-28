#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <shellapi.h>

// Получить папку, где находится exe
std::wstring GetExePath() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring exePath(buffer);
    size_t pos = exePath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exePath = exePath.substr(0, pos + 1);
    }
    return exePath;
}

// Выполнить скрипт и показать вывод в окне
int RunScript(const std::wstring& scriptPath) {
    // Преобразуем путь в UTF-8
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, scriptPath.c_str(), -1, NULL, 0, NULL, NULL);
    std::string filename(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, scriptPath.c_str(), -1, &filename[0], size_needed, NULL, NULL);
    filename.pop_back();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::wstring msg = L"Cannot open file:\n" + scriptPath;
        MessageBoxW(NULL, msg.c_str(), L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Перенаправляем cout в строку, чтобы перехватить вывод print
    std::stringstream outputStream;
    auto old_buf = std::cout.rdbuf(outputStream.rdbuf());

    gig::Lexer lexer(source);
    gig::Parser parser(lexer);
    auto ast = parser.parse();

    if (!ast) {
        std::cout.rdbuf(old_buf);
        MessageBoxW(NULL, L"Parsing failed.", L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    gig::Interpreter interpreter;
    interpreter.execute(ast.get());

    // Восстанавливаем cout
    std::cout.rdbuf(old_buf);

    // Получаем вывод
    std::string output = outputStream.str();
    if (output.empty()) {
        output = "Execution finished successfully (no output).";
    }

    // Показываем вывод в окне
    std::wstring wideOutput;
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, NULL, 0);
    wideOutput.resize(wideSize);
    MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, &wideOutput[0], wideSize);
    wideOutput.pop_back();

    MessageBoxW(NULL, wideOutput.c_str(), L"GIG Output", MB_OK | MB_ICONINFORMATION);

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        MessageBoxW(NULL, L"Failed to parse command line.", L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Если передан файл – выполняем его
    if (argc >= 2) {
        std::wstring scriptPath = argv[1];
        LocalFree(argv);
        return RunScript(scriptPath);
    }

    LocalFree(argv);

    // Нет аргументов – ищем test\test.gig рядом с exe
    std::wstring exeDir = GetExePath();
    std::wstring defaultScript = exeDir + L"test\\test.gig";

    DWORD attr = GetFileAttributesW(defaultScript.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        // Файла нет – показываем справку
        MessageBoxW(NULL,
            L"Usage: gig <filename>\n\n"
            L"Drag and drop a .gig file onto this executable.\n"
            L"Or place 'test\\test.gig' in the executable directory.",
            L"GIG – Language Interpreter",
            MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    return RunScript(defaultScript);
}
