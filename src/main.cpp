#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <shellapi.h>

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

// ----- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ -----
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

std::string ReadFile(const std::wstring& path) {
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

int RunScript(const std::string& source, const std::wstring& title = L"GIG Output") {
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

    std::cout.rdbuf(old_buf);

    std::string output = outputStream.str();
    if (output.empty()) output = "(no output)";

    std::wstring wideOutput;
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, NULL, 0);
    wideOutput.resize(wideSize);
    MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, &wideOutput[0], wideSize);
    wideOutput.pop_back();

    MessageBoxW(NULL, wideOutput.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // --- ОТЛАДОЧНОЕ ОКНО: всегда появляется при запуске ---
    MessageBoxW(NULL, L"GIG started!", L"Debug", MB_OK);

    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        MessageBoxW(NULL, L"Failed to parse command line.", L"GIG Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Если передан файл – выполняем его
    if (argc >= 2) {
        std::wstring path = argv[1];
        LocalFree(argv);
        std::string source = ReadFile(path);
        if (source.empty()) {
            MessageBoxW(NULL, L"File not found or empty.", L"GIG Error", MB_OK | MB_ICONERROR);
            return 1;
        }
        return RunScript(source, L"GIG Output (file)");
    }

    LocalFree(argv);

    // Нет аргументов – ищем test\test.gig рядом с exe
    std::wstring exeDir = GetExeDir();
    std::wstring scriptPath = exeDir + L"test\\test.gig";

    DWORD attr = GetFileAttributesW(scriptPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        std::string source = ReadFile(scriptPath);
        if (!source.empty()) {
            return RunScript(source, L"GIG Output (test\\test.gig)");
        }
    }

    // Если файла нет – используем встроенный скрипт
    return RunScript(embeddedScript, L"GIG Output (embedded)");
}
