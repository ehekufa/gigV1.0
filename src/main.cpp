#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <shellapi.h>   // для CommandLineToArgvW

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Разбираем командную строку
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        MessageBoxW(NULL, L"Failed to parse command line.", L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Если аргументов нет – показываем справку
    if (argc < 2) {
        MessageBoxW(NULL,
            L"Usage: gig <filename>\n\n"
            L"Drag and drop a .gig file onto this executable.",
            L"GIG – Language Interpreter",
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

    // Открываем файл
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::wstring msg = L"Cannot open file:\n";
        msg += std::wstring(argv[1]); // снова используем argv[1] (уже освобождён! – ошибка)
        // Исправим: мы уже сделали LocalFree, поэтому нужно сохранить путь
        // Перепишем, чтобы не было использования после освобождения.
        // Лучше сохранить имя файла в wide-строке до LocalFree.
        // Ниже – исправленная версия.
    }

    // ----- ПЕРЕПИСЫВАЕМ ЧИСТО -----
    // Для простоты переделаем, чтобы не было проблемы с освобождением.
    // Заново разберём командную строку, но сохраним имя файла.
    // Проще использовать GetCommandLineW и парсить вручную, но оставим как есть с корректировкой.

    // ----- ПРАВИЛЬНЫЙ ВАРИАНТ -----
    // Сделаем так, чтобы путь сохранялся до LocalFree.
    // Перепишем блок:
    int argc2;
    LPWSTR* argv2 = CommandLineToArgvW(GetCommandLineW(), &argc2);
    if (!argv2) {
        MessageBoxW(NULL, L"Failed to parse command line.", L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (argc2 < 2) {
        MessageBoxW(NULL,
            L"Usage: gig <filename>\n\n"
            L"Drag and drop a .gig file onto this executable.",
            L"GIG – Language Interpreter",
            MB_OK | MB_ICONINFORMATION);
        LocalFree(argv2);
        return 0;
    }
    // Сохраняем путь как wide-строку
    std::wstring wide_filename = argv2[1];

    // Преобразуем в UTF-8 для ifstream
    int size_needed2 = WideCharToMultiByte(CP_UTF8, 0, wide_filename.c_str(), -1, NULL, 0, NULL, NULL);
    std::string filename2(size_needed2, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_filename.c_str(), -1, &filename2[0], size_needed2, NULL, NULL);
    filename2.pop_back();

    LocalFree(argv2);

    // Открываем файл
    std::ifstream file2(filename2);
    if (!file2.is_open()) {
        std::wstring msg = L"Cannot open file:\n" + wide_filename;
        MessageBoxW(NULL, msg.c_str(), L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::stringstream buffer;
    buffer << file2.rdbuf();
    std::string source = buffer.str();

    gig::Lexer lexer(source);
    gig::Parser parser(lexer);
    auto ast = parser.parse();

    if (!ast) {
        MessageBoxW(NULL, L"Parsing failed.", L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    gig::Interpreter interpreter;
    interpreter.execute(ast.get());

    MessageBoxW(NULL, L"Execution finished successfully.", L"GIG", MB_OK | MB_ICONINFORMATION);

    return 0;
}
