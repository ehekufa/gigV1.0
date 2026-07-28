#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <windows.h>   // для MessageBox

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Разбираем командную строку
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        MessageBox(NULL, L"Failed to parse command line.", L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (argc < 2) {
        MessageBox(NULL, 
            L"Usage: gig <filename>\n\n"
            L"Drag and drop a .gig file onto this executable.",
            L"GIG - Language Interpreter",
            MB_OK | MB_ICONINFORMATION);
        LocalFree(argv);
        return 0;
    }

    // Преобразуем wide-строку в UTF-8
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, NULL, 0, NULL, NULL);
    std::string filename(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, &filename[0], size_needed, NULL, NULL);
    filename.pop_back(); // убираем завершающий ноль

    LocalFree(argv);

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::string msg = "Cannot open file:\n" + filename;
        MessageBoxA(NULL, msg.c_str(), "GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    gig::Lexer lexer(source);
    gig::Parser parser(lexer);
    auto ast = parser.parse();

    if (!ast) {
        MessageBoxA(NULL, "Parsing failed.", "GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    gig::Interpreter interpreter;
    interpreter.execute(ast.get());

    // Если выполнение завершилось без ошибок, покажем сообщение об успехе
    MessageBoxA(NULL, "Execution finished successfully.", "GIG", MB_OK | MB_ICONINFORMATION);

    return 0;
}
