// 7차 공통 오류 로그의 구현이다. 로그 줄 형식, 지연 파일 생성, 메모리 활동 기록, 비정상 종료 핸들러를 담는다.
#include "platform/ErrorLog.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <deque>
#include <exception>
#include <iostream>
#include <mutex>
#include <new>
#include <stdexcept>
#include <stdlib.h> // _set_invalid_parameter_handler/_set_purecall_handler 선언이다.
#include <streambuf>
#include <string.h> // strcpy_s 선언이다.
#include <typeinfo>

namespace netstorm::platform {
namespace { // 이 번역 단위 안에서만 쓰는 전역 상태와 보조 함수다.
// 메모리 활동 기록의 최대 줄 수다. 치명 오류 직전 상황을 충분히 남기되 메모리를 제한한다.
constexpr std::size_t kRingLimit = 1024;
// 한 줄의 최대 바이트 수다. 너무 긴 진단은 잘라 로그 파일이 한 줄로 폭주하지 않게 한다.
constexpr std::size_t kLineLimit = 4096;
// 프로세스 종료 시점까지 파괴하지 않는 전역 상태다(종료 순서 문제와 비정상 종료 핸들러의 접근을 피한다).
struct State {
    // mutex: 모든 기록 함수가 공유하는 잠금이다.
    std::mutex mutex;
    // configured: Configure가 호출됐는지 여부다.
    bool configured{};
    // directory: 로그 파일을 만들 폴더(UTF-8 변환 전의 경로)다.
    std::filesystem::path directory;
    // directoryWide: 비정상 종료 핸들러가 힙 할당 없이 쓰도록 미리 복사한 UTF-16 폴더 경로다.
    wchar_t directoryWide[1024]{};
    // version: 제품 버전 문자열(예: 11.01)이다.
    std::string version;
    // file: 열린 로그 파일 핸들이다. 아직 만들지 않았으면 INVALID_HANDLE_VALUE다.
    HANDLE file{INVALID_HANDLE_VALUE};
    // filePath: 만든 로그 파일 경로다.
    std::filesystem::path filePath;
    // ring: 최근 활동 기록(UTF-8 한 줄씩)이다.
    std::deque<std::string> ring;
    // crashHandlersInstalled: 핸들러를 중복 설치하지 않기 위한 표식이다.
    bool crashHandlersInstalled{};
    // streamsCaptured: 표준 스트림 복제를 중복 설치하지 않기 위한 표식이다.
    bool streamsCaptured{};
    // previousTerminate: 설치 전의 terminate 핸들러다.
    std::terminate_handler previousTerminate{};
    // fatalWritten: 치명 기록의 재귀(핸들러 안의 추가 오류)를 막는 표식이다.
    volatile LONG fatalDepth{};
};
// 전역 상태를 한 번만 만든다.
State& S() { static State* state = new State; return *state; }

// 현재 지역 시각으로 "yyyy-MM-dd HH:mm:ss.fff" 문자열을 만든다.
std::string Timestamp() {
    SYSTEMTIME t{}; ::GetLocalTime(&t);
    char buffer[40]{};
    std::snprintf(buffer, sizeof buffer, "%04u-%02u-%02u %02u:%02u:%02u.%03u", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    return buffer;
}
// 심각도 이름이다.
const char* LevelName(LogLevel level) {
    switch (level) { case LogLevel::kInfo: return "INFO "; case LogLevel::kWarning: return "WARN "; case LogLevel::kError: return "ERROR"; default: return "FATAL"; }
}
// 한 줄을 만든다. 개행은 공백 두 칸으로 접어 한 기록이 한 줄에 머물게 하지 않고 이어지는 줄은 들여쓴다.
std::string FormatLine(LogLevel level, std::string_view category, std::string_view message) {
    std::string text(message.substr(0, kLineLimit * 8)); // 다중 줄 메시지의 전체 상한이다.
    std::string line = "[" + Timestamp() + "] [" + LevelName(level) + "] [" + std::string(category) + "] ";
    // 이어지는 줄의 앞에 들여쓰기를 붙이는 반복이다.
    for (std::size_t i = 0; i < text.size(); ++i) { line += text[i]; if (text[i] == '\n' && i + 1 < text.size()) line += "    "; }
    return line;
}
// UTF-16 경로를 UTF-8 문자열로 바꾼다.
std::string Utf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    const int needed = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(needed > 0 ? needed : 0), '\0');
    if (needed > 0) ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), needed, nullptr, nullptr);
    return out;
}
// 핸들에 바이트를 모두 쓴다. 비정상 종료 핸들러에서도 쓰므로 힙을 쓰지 않는다.
void WriteAll(HANDLE file, const char* data, std::size_t size) {
    // 부분 쓰기를 이어서 처리하는 반복이다.
    while (size > 0) {
        DWORD written = 0;
        if (!::WriteFile(file, data, static_cast<DWORD>(size > 0x10000 ? 0x10000 : size), &written, nullptr) || written == 0) return;
        data += written; size -= written;
    }
}
// 파일을 연다(없으면 폴더부터 만든다). 이미 열려 있으면 아무 것도 하지 않는다. 잠금은 호출자가 잡는다.
// 힙 할당 없이 미리 복사한 UTF-16 폴더 경로만 쓰므로 비정상 종료 핸들러에서도 호출할 수 있다.
bool EnsureOpenLocked(State& s) {
    if (s.file != INVALID_HANDLE_VALUE) return true;
    if (!s.configured || s.directoryWide[0] == L'\0') return false;
    // 폴더를 만든다: 경로 구분자마다 순서대로 CreateDirectoryW를 호출하는 반복이다(이미 있으면 실패해도 무시).
    wchar_t partial[1024]{};
    for (std::size_t i = 0; s.directoryWide[i] && i < 1000; ++i) {
        partial[i] = s.directoryWide[i];
        if ((s.directoryWide[i] == L'\\' || s.directoryWide[i] == L'/') && i > 2) { partial[i + 1] = L'\0'; ::CreateDirectoryW(partial, nullptr); }
    }
    ::CreateDirectoryW(s.directoryWide, nullptr);
    SYSTEMTIME t{}; ::GetLocalTime(&t);
    wchar_t path[1100]{};
    std::swprintf(path, 1100, L"%ls\\%04u%02u%02u%02u%02u%02u.log", s.directoryWide, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    const bool existed = ::GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
    // FILE_APPEND_DATA만 요청해 같은 초에 만든 같은 이름이 있어도 덮어쓰지 않고 이어 쓴다.
    s.file = ::CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (s.file == INVALID_HANDLE_VALUE) return false;
    if (!existed) { static const char bom[3] = {'\xEF', '\xBB', '\xBF'}; WriteAll(s.file, bom, 3); } // 메모장 등이 UTF-8로 인식하도록 BOM을 붙인다.
    return true;
}
// 파일 경로를 UTF-8 문자열이 아닌 path로 보관한다(힙을 쓰므로 일반 경로에서만 호출한다).
void RememberPathLocked(State& s) {
    if (!s.filePath.empty() || s.file == INVALID_HANDLE_VALUE) return;
    wchar_t path[1100]{};
    const DWORD length = ::GetFinalPathNameByHandleW(s.file, path, 1100, FILE_NAME_NORMALIZED);
    if (length > 0 && length < 1100) {
        std::wstring text(path, length);
        if (text.rfind(L"\\\\?\\", 0) == 0) text.erase(0, 4); // 확장 경로 접두를 제거한다.
        s.filePath = std::filesystem::path(text);
    }
}
// 파일 머리말(처음 한 번)이다. 제품·프로세스·OS 정보를 남긴다.
std::string HeaderText(const State& s) {
    char os[160]{};
    using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);
    OSVERSIONINFOW info{}; info.dwOSVersionInfoSize = sizeof info;
    if (const auto module = ::GetModuleHandleW(L"ntdll.dll"))
        if (const auto fn = reinterpret_cast<RtlGetVersionFn>(::GetProcAddress(module, "RtlGetVersion"))) fn(&info);
    SYSTEM_INFO sys{}; ::GetNativeSystemInfo(&sys);
    std::snprintf(os, sizeof os, "Windows %lu.%lu build %lu, processorArchitecture=%u", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber, static_cast<unsigned>(sys.wProcessorArchitecture));
    wchar_t exe[1024]{}; ::GetModuleFileNameW(nullptr, exe, 1024);
    wchar_t cwd[1024]{}; ::GetCurrentDirectoryW(1024, cwd);
    std::string text = "==== NetStorm 오류 로그 ====\n";
    text += "제품 버전: " + s.version + "\n";
    text += "프로세스 ID: " + std::to_string(::GetCurrentProcessId()) + "\n";
    text += "실행 파일: " + Utf8(exe) + "\n";
    text += "작업 폴더: " + Utf8(cwd) + "\n";
    text += "명령줄: " + Utf8(::GetCommandLineW()) + "\n";
    text += std::string("운영체제: ") + os + "\n";
    text += "로그 시작: " + Timestamp() + "\n";
    return text;
}
// 파일이 처음 만들어질 때 머리말과 이전 활동 기록을 쓴다. 잠금은 호출자가 잡는다.
void WriteHeaderAndRingLocked(State& s) {
    const std::string header = HeaderText(s);
    WriteAll(s.file, header.data(), header.size());
    const std::string title = "---- 이전 활동 기록(최근 " + std::to_string(s.ring.size()) + "줄) ----\n";
    WriteAll(s.file, title.data(), title.size());
    // 메모리 활동 기록을 시간 순서대로 파일에 옮기는 반복이다.
    for (const auto& line : s.ring) { WriteAll(s.file, line.data(), line.size()); WriteAll(s.file, "\n", 1); }
    const std::string footer = "---- 이후 기록 ----\n";
    WriteAll(s.file, footer.data(), footer.size());
}
// 메모리 활동 기록에 한 줄을 넣고 오래된 줄을 버린다. 잠금은 호출자가 잡는다.
void PushRingLocked(State& s, const std::string& line) {
    s.ring.push_back(line.size() > kLineLimit ? line.substr(0, kLineLimit) + "...(잘림)" : line);
    // 상한을 넘으면 가장 오래된 줄부터 버리는 반복이다.
    while (s.ring.size() > kRingLimit) s.ring.pop_front();
}
// 한 줄을 기록한다. level이 kWarning 이상이면 파일에 쓰고 kError 이상이면 디스크에 반영한다.
void RecordLine(LogLevel level, std::string_view category, std::string_view message) {
    State& s = S();
    const std::string line = FormatLine(level, category, message);
    std::lock_guard<std::mutex> lock(s.mutex);
    // 아직 파일이 없고 경고 이상이면 파일을 만들고 머리말과 이전 활동 기록을 먼저 쓴다.
    if (s.file == INVALID_HANDLE_VALUE && level >= LogLevel::kWarning && EnsureOpenLocked(s)) {
        RememberPathLocked(s);
        WriteHeaderAndRingLocked(s);
    }
    if (s.file != INVALID_HANDLE_VALUE) { // 파일이 열려 있으면 낮은 심각도도 이어서 남긴다.
        WriteAll(s.file, line.data(), line.size()); WriteAll(s.file, "\n", 1);
        if (level >= LogLevel::kError) ::FlushFileBuffers(s.file);
    } else PushRingLocked(s, line); // 파일이 없으면(경고 미만이거나 구성되지 않음) 메모리 활동 기록에만 둔다.
}
// 예외 한 개와 중첩 예외를 문자열로 푼다. depth는 무한 중첩을 막는 한계다.
std::string DescribeCurrentException(int depth = 0) {
    if (depth > 8) return "(중첩 예외가 너무 깊어 생략)";
    try { throw; }
    catch (const std::exception& error) {
        std::string text = std::string(typeid(error).name()) + ": " + error.what();
        try { std::rethrow_if_nested(error); }
        catch (...) { text += "\n  원인: " + DescribeCurrentException(depth + 1); }
        return text;
    }
    catch (const std::string& error) { return "std::string: " + error; }
    catch (const char* error) { return std::string("const char*: ") + (error ? error : "(null)"); }
    catch (int code) { return "int: " + std::to_string(code); }
    catch (...) { return "알 수 없는 형식의 예외"; }
}
std::string DescribeException(std::exception_ptr error) {
    if (!error) return "(예외 없음)";
    try { std::rethrow_exception(error); }
    catch (...) { return DescribeCurrentException(); }
}
// 주소가 속한 모듈 이름과 RVA를 "이름+0x오프셋" 형태로 stack 버퍼에 쓴다. 힙을 쓰지 않는다.
void FormatAddress(char* out, std::size_t size, const void* address) {
    HMODULE module = nullptr;
    wchar_t name[260]{};
    if (::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<LPCWSTR>(address), &module) && module) {
        ::GetModuleFileNameW(module, name, 260);
        const wchar_t* base = name; // 경로 끝의 파일 이름만 쓴다.
        for (const wchar_t* p = name; *p; ++p) if (*p == L'\\' || *p == L'/') base = p + 1;
        char narrow[260]{};
        ::WideCharToMultiByte(CP_UTF8, 0, base, -1, narrow, sizeof narrow - 1, nullptr, nullptr);
        std::snprintf(out, size, "%s+0x%llx (0x%p)", narrow, static_cast<unsigned long long>(reinterpret_cast<const char*>(address) - reinterpret_cast<const char*>(module)), address);
    } else std::snprintf(out, size, "0x%p", address);
}
// 파일(필요하면 만들고)에 raw 텍스트를 쓴다. 비정상 종료 핸들러용이며 힙을 쓰지 않고 가능하면 잠금 없이 접근한다.
void CrashWrite(const char* text) {
    State& s = S();
    const bool locked = s.mutex.try_lock(); // 이미 잠겨 있으면(충돌한 스레드가 잡았을 수 있음) 잠금 없이 계속한다.
    const bool existed = s.file != INVALID_HANDLE_VALUE;
    if (EnsureOpenLocked(s)) {
        if (!existed) WriteHeaderAndRingLocked(s); // 힙의 deque를 읽는 최선 노력이다.
        WriteAll(s.file, text, std::strlen(text));
        ::FlushFileBuffers(s.file);
    }
    if (locked) s.mutex.unlock();
}
// 현재 스택의 호출 경로를 "#번호 모듈+오프셋" 줄로 쓴다.
void CrashWriteBacktrace(const CONTEXT* context) {
    void* frames[48]{};
    USHORT count = 0;
#if defined(_M_X64)
    // 예외 문맥에서 시작해 각 프레임을 풀어 올라가는 x64 표준 방법이다.
    if (context) {
        CONTEXT local = *context;
        // 최대 48 프레임까지 되감는 반복이다. 현재 명령 주소를 기록한 뒤 한 프레임씩 올라간다.
        while (count < 48) {
            frames[count++] = reinterpret_cast<void*>(local.Rip);
            DWORD64 imageBase = 0;
            const auto entry = ::RtlLookupFunctionEntry(local.Rip, &imageBase, nullptr);
            if (!entry) { // 잎 함수: 반환 주소가 스택 맨 위다.
                if (!local.Rsp) break;
                local.Rip = *reinterpret_cast<DWORD64*>(local.Rsp); local.Rsp += 8;
            } else {
                void* handlerData = nullptr; DWORD64 frame = 0;
                ::RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, local.Rip, entry, &local, &handlerData, &frame, nullptr);
            }
            if (!local.Rip) break;
        }
    } else count = ::RtlCaptureStackBackTrace(0, 48, frames, nullptr);
#else
    count = ::RtlCaptureStackBackTrace(0, 48, frames, nullptr);
    static_cast<void>(context);
#endif
    CrashWrite("스택 추적:\n");
    // 모은 프레임을 한 줄씩 쓰는 반복이다.
    for (USHORT i = 0; i < count; ++i) {
        char address[400]{}; FormatAddress(address, sizeof address, frames[i]);
        char line[480]{}; std::snprintf(line, sizeof line, "  #%02u %s\n", static_cast<unsigned>(i), address);
        CrashWrite(line);
    }
}
// 예외 코드의 이름이다.
const char* ExceptionName(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION: return "ACCESS_VIOLATION";
    case EXCEPTION_STACK_OVERFLOW: return "STACK_OVERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_PRIV_INSTRUCTION: return "PRIV_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR: return "IN_PAGE_ERROR";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_BREAKPOINT: return "BREAKPOINT";
    case 0xC0000409: return "STACK_BUFFER_OVERRUN(fail-fast)";
    case 0xE06D7363: return "C++ 예외(처리되지 않음)";
    default: return "알 수 없는 SEH 코드";
    }
}
// 처리되지 않은 SEH 예외 필터다. 로그를 남기고 기본 처리(종료)로 넘긴다.
LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS* pointers) {
    State& s = S();
    if (::InterlockedIncrement(&s.fatalDepth) > 1) return EXCEPTION_CONTINUE_SEARCH; // 핸들러 안에서 다시 충돌하면 재귀하지 않는다.
    char line[512]{}; char address[400]{};
    const EXCEPTION_RECORD* record = pointers ? pointers->ExceptionRecord : nullptr;
    CrashWrite("\n");
    std::snprintf(line, sizeof line, "[%s] [FATAL] [crash] 처리되지 않은 예외로 비정상 종료합니다.\n", Timestamp().c_str());
    CrashWrite(line);
    if (record) {
        FormatAddress(address, sizeof address, record->ExceptionAddress);
        std::snprintf(line, sizeof line, "예외 코드: 0x%08lx (%s)\n예외 주소: %s\n스레드 ID: %lu\n", record->ExceptionCode, ExceptionName(record->ExceptionCode), address, ::GetCurrentThreadId());
        CrashWrite(line);
        if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
            if (record->NumberParameters >= 2) {
                const char* kind = record->ExceptionInformation[0] == 0 ? "읽기" : record->ExceptionInformation[0] == 1 ? "쓰기" : "실행";
                std::snprintf(line, sizeof line, "접근 종류: %s, 대상 주소: 0x%llx\n", kind, static_cast<unsigned long long>(record->ExceptionInformation[1]));
                CrashWrite(line);
            }
        }
    }
#if defined(_M_X64)
    if (pointers && pointers->ContextRecord) {
        const CONTEXT& c = *pointers->ContextRecord;
        std::snprintf(line, sizeof line, "레지스터: RIP=%016llx RSP=%016llx RBP=%016llx EFLAGS=%08lx\n  RAX=%016llx RBX=%016llx RCX=%016llx RDX=%016llx\n  RSI=%016llx RDI=%016llx R8=%016llx R9=%016llx\n",
            c.Rip, c.Rsp, c.Rbp, c.EFlags, c.Rax, c.Rbx, c.Rcx, c.Rdx, c.Rsi, c.Rdi, c.R8, c.R9);
        CrashWrite(line);
    }
#endif
    CrashWriteBacktrace(pointers ? pointers->ContextRecord : nullptr);
    CrashWrite("---- 비정상 종료 기록 끝 ----\n");
    return EXCEPTION_CONTINUE_SEARCH;
}
// std::terminate 핸들러다. 처리되지 않은 C++ 예외 또는 직접 terminate 호출의 원인을 남긴다.
void TerminateHandler() {
    State& s = S();
    if (::InterlockedIncrement(&s.fatalDepth) == 1) {
        CrashWrite("\n");
        char line[200]{}; std::snprintf(line, sizeof line, "[%s] [FATAL] [terminate] std::terminate가 호출되었습니다.\n", Timestamp().c_str());
        CrashWrite(line);
        if (std::current_exception()) { // 처리되지 않은 예외가 원인이면 형식과 메시지를 남긴다.
            const std::string what = "처리되지 않은 C++ 예외: " + DescribeException(std::current_exception()) + "\n";
            CrashWrite(what.c_str());
        } else CrashWrite("활성 예외 없음(직접 terminate/noexcept 위반 등)\n");
        CrashWriteBacktrace(nullptr);
        CrashWrite("---- 비정상 종료 기록 끝 ----\n");
    }
    if (s.previousTerminate && s.previousTerminate != TerminateHandler) s.previousTerminate();
    std::abort();
}
// 잘못된 CRT 인자 핸들러다.
void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned line, std::uintptr_t) {
    State& s = S();
    if (::InterlockedIncrement(&s.fatalDepth) == 1) {
        CrashWrite("\n");
        const std::string text = "[" + Timestamp() + "] [FATAL] [invalid-parameter] 잘못된 CRT 인자: 식=" + Utf8(expression ? expression : L"") + " 함수=" + Utf8(function ? function : L"") + " 파일=" + Utf8(file ? file : L"") + ":" + std::to_string(line) + "\n";
        CrashWrite(text.c_str());
        CrashWriteBacktrace(nullptr);
        CrashWrite("---- 비정상 종료 기록 끝 ----\n");
    }
    std::abort();
}
// 순수 가상 함수 호출 핸들러다.
void PurecallHandler() {
    State& s = S();
    if (::InterlockedIncrement(&s.fatalDepth) == 1) {
        CrashWrite("\n");
        char line[200]{}; std::snprintf(line, sizeof line, "[%s] [FATAL] [purecall] 순수 가상 함수가 호출되었습니다.\n", Timestamp().c_str());
        CrashWrite(line);
        CrashWriteBacktrace(nullptr);
        CrashWrite("---- 비정상 종료 기록 끝 ----\n");
    }
    std::abort();
}
// abort() 신호 핸들러다. 로그를 남기고 기본 동작으로 되돌려 다시 발생시킨다.
void AbortSignalHandler(int) {
    State& s = S();
    if (::InterlockedIncrement(&s.fatalDepth) == 1) {
        CrashWrite("\n");
        char line[200]{}; std::snprintf(line, sizeof line, "[%s] [FATAL] [abort] abort()가 호출되었습니다.\n", Timestamp().c_str());
        CrashWrite(line);
        CrashWriteBacktrace(nullptr);
        CrashWrite("---- 비정상 종료 기록 끝 ----\n");
    }
    std::signal(SIGABRT, SIG_DFL);
    std::raise(SIGABRT);
}
// 표준 스트림 한 줄을 원래 출력으로 전달하면서 메모리 활동 기록에도 복제하는 streambuf다.
class TeeBuf final : public std::streambuf {
public:
    // sink: 원래 스트림 버퍼다. tag: 기록에 붙일 스트림 이름이다.
    TeeBuf(std::streambuf* sink, const char* tag) : sink_(sink), tag_(tag) {}
protected:
    // 한 글자를 원래 버퍼로 보내고 줄을 모은다.
    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof())) return sink_ ? sink_->pubsync() : 0;
        Collect(static_cast<char>(ch));
        return sink_ ? sink_->sputc(static_cast<char>(ch)) : ch;
    }
    // 여러 글자를 원래 버퍼로 보내고 줄을 모은다.
    std::streamsize xsputn(const char_type* text, std::streamsize count) override {
        // 받은 글자를 줄 단위로 모으는 반복이다.
        for (std::streamsize i = 0; i < count; ++i) Collect(text[i]);
        return sink_ ? sink_->sputn(text, count) : count;
    }
    // 동기화 요청을 원래 버퍼로 넘긴다.
    int sync() override { return sink_ ? sink_->pubsync() : 0; }
private:
    // 개행을 만나면 모은 줄을 활동 기록으로 보낸다.
    void Collect(char ch) {
        if (ch == '\n') { Flush(); return; }
        if (line_.size() < kLineLimit) line_ += ch;
    }
    // 모은 줄을 활동 기록으로 보낸다.
    void Flush() { if (!line_.empty()) { ErrorLog::Note(std::string("[") + tag_ + "] " + line_); line_.clear(); } }
    // sink_: 원래 스트림 버퍼다.
    std::streambuf* sink_;
    // tag_: 스트림 이름(clog/cerr)이다.
    const char* tag_;
    // line_: 아직 개행을 받지 못한 현재 줄이다.
    std::string line_;
};
}

// 로그 폴더와 제품 버전을 저장한다. 파일과 폴더는 만들지 않는다.
void ErrorLog::Configure(const std::filesystem::path& logsDirectory, std::string_view productVersion) {
    State& s = S();
    std::lock_guard<std::mutex> lock(s.mutex);
    if (s.file != INVALID_HANDLE_VALUE) { ::CloseHandle(s.file); s.file = INVALID_HANDLE_VALUE; }
    s.filePath.clear();
    s.directory = logsDirectory;
    s.version = std::string(productVersion);
    std::wstring wide = std::filesystem::absolute(logsDirectory).wstring();
    while (!wide.empty() && (wide.back() == L'\\' || wide.back() == L'/')) wide.pop_back(); // 끝 구분자를 제거해 이름 연결을 단순하게 한다.
    s.directoryWide[0] = L'\0';
    if (!wide.empty() && wide.size() < 1000) { std::wmemcpy(s.directoryWide, wide.c_str(), wide.size() + 1); s.configured = true; }
    else s.configured = false;
    s.fatalDepth = 0;
}
// 구성 여부다.
bool ErrorLog::Configured() { State& s = S(); std::lock_guard<std::mutex> lock(s.mutex); return s.configured; }
// 정상 활동 한 줄을 메모리 기록에 남긴다.
void ErrorLog::Note(std::string_view text) { RecordLine(LogLevel::kInfo, "info", text); }
// 지정한 심각도로 기록한다.
void ErrorLog::Record(LogLevel level, std::string_view category, std::string_view message) { RecordLine(level, category, message); }
// 예외를 풀어 기록한다.
void ErrorLog::RecordException(std::string_view where, std::exception_ptr error, LogLevel level) {
    RecordLine(level, "exception", std::string(where) + ": " + DescribeException(error));
}
// 원문 assertion을 기록하고 선택적으로 창을 띄운다.
void ErrorLog::RecordAssertion(std::string_view expression, std::string_view file, unsigned line) {
    const std::string text = std::string(file) + ":" + std::to_string(line) + " " + std::string(expression);
    RecordLine(LogLevel::kWarning, "assertion", text);
    wchar_t flag[8]{};
    if (::GetEnvironmentVariableW(L"NETSTORM_ASSERT_WINDOW", flag, 8) > 0 && flag[0] == L'1') { // 무인 실행을 막지 않도록 기본은 창을 띄우지 않는다.
        const int wideSize = ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring message(static_cast<std::size_t>(wideSize > 0 ? wideSize : 1), L'\0');
        if (wideSize > 0) ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, message.data(), wideSize);
        if (::MessageBoxW(nullptr, message.c_str(), L"NetStorm assertion", MB_ICONERROR | MB_OKCANCEL | MB_TOPMOST) == IDCANCEL) {
            RecordFatal("assertion 창", "사용자가 취소를 선택해 종료합니다: " + text);
            std::_Exit(3);
        }
    }
}
// 종료 직전 원인을 기록하고 디스크에 반영한다.
void ErrorLog::RecordFatal(std::string_view where, std::string_view cause) {
    RecordLine(LogLevel::kFatal, "fatal", std::string(where) + ": " + std::string(cause));
    State& s = S();
    std::lock_guard<std::mutex> lock(s.mutex);
    if (s.file != INVALID_HANDLE_VALUE) ::FlushFileBuffers(s.file);
}
// 핸들러를 한 번만 설치한다.
void ErrorLog::InstallCrashHandlers() {
    State& s = S();
    std::lock_guard<std::mutex> lock(s.mutex);
    if (s.crashHandlersInstalled) return;
    s.crashHandlersInstalled = true;
    s.previousTerminate = std::set_terminate(TerminateHandler);
    ::SetUnhandledExceptionFilter(UnhandledFilter);
    _set_invalid_parameter_handler(InvalidParameterHandler);
    _set_purecall_handler(PurecallHandler);
    std::signal(SIGABRT, AbortSignalHandler);
    ULONG guarantee = 128 * 1024; // 스택 오버플로 뒤에도 필터가 쓸 여유 스택을 확보한다.
    ::SetThreadStackGuarantee(&guarantee);
}
// 표준 스트림 복제를 한 번만 설치한다.
void ErrorLog::CaptureStandardStreams() {
    State& s = S();
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        if (s.streamsCaptured) return;
        s.streamsCaptured = true;
    }
    static TeeBuf* clogTee = new TeeBuf(std::clog.rdbuf(), "clog"); // 프로세스 끝까지 유지하는 의도적 누수다.
    std::clog.rdbuf(clogTee);
    static TeeBuf* cerrTee = new TeeBuf(std::cerr.rdbuf(), "cerr");
    std::cerr.rdbuf(cerrTee);
}
// 만든 파일 경로다.
std::filesystem::path ErrorLog::CurrentFile() { State& s = S(); std::lock_guard<std::mutex> lock(s.mutex); return s.filePath; }
// 메모리 활동 기록 줄 수다.
std::size_t ErrorLog::BufferedLineCount() { State& s = S(); std::lock_guard<std::mutex> lock(s.mutex); return s.ring.size(); }
// 요구 사양의 파일 이름이다.
std::string ErrorLog::FileNameFor(int year, int month, int day, int hour, int minute, int second) {
    char buffer[32]{}; std::snprintf(buffer, sizeof buffer, "%04d%02d%02d%02d%02d%02d.log", year, month, day, hour, minute, second); return buffer;
}
// 검사용 초기화다.
void ErrorLog::ResetForTesting() {
    State& s = S();
    std::lock_guard<std::mutex> lock(s.mutex);
    if (s.file != INVALID_HANDLE_VALUE) { ::CloseHandle(s.file); s.file = INVALID_HANDLE_VALUE; }
    s.filePath.clear(); s.directory.clear(); s.directoryWide[0] = L'\0'; s.version.clear(); s.ring.clear(); s.configured = false; s.fatalDepth = 0;
}
#pragma warning(push)
#pragma warning(disable : 4717) // 스택 오버플로 시험 함수의 의도적 무한 재귀 경고다.
// 개발 검사용 의도적 비정상 종료다. 오류 보고 대화상자와 abort 메시지 상자를 꺼서 무인 검사가 멈추지 않게 한다.
[[noreturn]] void ErrorLog::CrashForTest(std::string_view kind) {
    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (kind == "throw") throw std::runtime_error("로그 검사용 의도적 예외");
    if (kind == "terminate") std::terminate();
    if (kind == "access") { volatile int* null = nullptr; *null = 1; }
    if (kind == "divzero") { volatile int zero = 0; volatile int value = 1 / zero; static_cast<void>(value); }
    if (kind == "abort") std::abort();
    if (kind == "stack") { struct R { static int Go(int n) { volatile char pad[512]; pad[0] = static_cast<char>(n); return Go(n + 1) + pad[0]; } }; static_cast<void>(R::Go(0)); }
    if (kind == "invalid") { _set_invalid_parameter_handler(InvalidParameterHandler); strcpy_s(static_cast<char*>(nullptr), 0, "x"); }
    if (kind == "purecall") PurecallHandler();
    throw std::invalid_argument("알 수 없는 로그 검사 종류");
}
#pragma warning(pop)
}
