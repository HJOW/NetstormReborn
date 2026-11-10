// 7차 공통 오류 로그(ErrorLog)의 파일 이름/지연 생성/이전 활동 기록/예외 풀이/스레드 안전을 게임 없이 확인한다.
// 비정상 종료 핸들러(SEH/terminate/abort)는 실제 프로세스 종료가 필요하므로 별도 실행 검사(--log-crash-test)에서 확인한다.
#include "TestSupport.h"
#include "platform/ErrorLog.h"
#include "platform/UserDataRoot.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace netstorm::platform;
namespace {
// 검사마다 새 임시 폴더를 만들고 소멸 때 지운다. 로그는 이 폴더 아래 logs에만 쓰인다.
struct TempLogs {
    // base: 검사 전용 임시 루트다.
    std::filesystem::path base;
    // logs: ErrorLog가 파일을 만들 폴더(아직 만들지 않는다).
    std::filesystem::path logs;
    // 서로 다른 검사가 같은 폴더를 쓰지 않도록 증가하는 번호다.
    static int Next() { static int counter = 0; return ++counter; }
    // 임시 루트를 정하고 ErrorLog를 새로 구성한다. 폴더는 만들지 않는다.
    TempLogs() {
        base = std::filesystem::temp_directory_path() / ("netstorm-errorlog-test-" + std::to_string(Next()) + "-" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::filesystem::remove_all(base);
        logs = base / "logs";
        ErrorLog::ResetForTesting();
        ErrorLog::Configure(logs, "11.01");
    }
    // 구성을 해제하고 임시 폴더를 지운다.
    ~TempLogs() { ErrorLog::ResetForTesting(); std::error_code ignored; std::filesystem::remove_all(base, ignored); }
    // 로그 폴더의 모든 파일 경로다.
    std::vector<std::filesystem::path> Files() const {
        std::vector<std::filesystem::path> result;
        if (!std::filesystem::exists(logs)) return result;
        // 폴더의 항목을 모아 반환하는 반복이다.
        for (const auto& entry : std::filesystem::directory_iterator(logs)) result.push_back(entry.path());
        return result;
    }
    // 단 하나 만들어진 로그 파일의 전체 바이트를 읽는다.
    std::string Read() const {
        const auto files = Files();
        CHECK(files.size() == 1);
        if (files.empty()) return {};
        std::ifstream input(files.front(), std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    }
};
// 문자열에 부분 문자열이 있는지 확인한다.
bool Has(const std::string& text, std::string_view part) { return text.find(part) != std::string::npos; }
}

// 요구 사양의 로그 파일 이름(yyyyMMddHHmmss.log)을 정확히 만든다.
TEST_CASE(ErrorLog_FileNameFollowsRequiredPattern) {
    CHECK(ErrorLog::FileNameFor(2026, 11, 10, 14, 15, 2) == "20261110141502.log");
    CHECK(ErrorLog::FileNameFor(2027, 1, 2, 3, 4, 5) == "20270102030405.log");
}
// 사용자 데이터 루트의 로그 폴더는 소문자 logs이다.
TEST_CASE(ErrorLog_UserDataRootLogsFolderIsLowercaseLogs) {
    const UserDataRoot root(std::filesystem::path("C:/Users/Tester/Saved Games/Netstorm"));
    CHECK(root.Logs().filename() == "logs" && root.Logs().parent_path() == root.Root());
}
// 정상 활동만 있으면 파일도 폴더도 만들지 않는다.
TEST_CASE(ErrorLog_NormalNotesDoNotCreateFileOrFolder) {
    TempLogs temp;
    ErrorLog::Note("정상 활동 1"); ErrorLog::Note("정상 활동 2"); ErrorLog::Record(LogLevel::kInfo, "x", "정보");
    CHECK(!std::filesystem::exists(temp.logs) && ErrorLog::CurrentFile().empty() && ErrorLog::BufferedLineCount() == 3);
}
// 첫 경고가 파일을 만들고 머리말·이전 활동 기록·경고 줄을 순서대로 쓴다.
TEST_CASE(ErrorLog_FirstWarningCreatesNamedFileWithPriorActivity) {
    TempLogs temp;
    ErrorLog::Note("앞선 활동 A"); ErrorLog::Note("앞선 활동 B");
    ErrorLog::Record(LogLevel::kWarning, "검사", "첫 경고");
    const auto files = temp.Files();
    CHECK(files.size() == 1);
    if (files.empty()) return;
    CHECK(std::regex_match(files.front().filename().string(), std::regex("^[0-9]{14}\\.log$")));
    CHECK(ErrorLog::CurrentFile().filename() == files.front().filename());
    const auto text = temp.Read();
    CHECK(text.size() > 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0); // UTF-8 BOM이다.
    CHECK(Has(text, "제품 버전: 11.01") && Has(text, "프로세스 ID:") && Has(text, "작업 폴더:") && Has(text, "운영체제: Windows"));
    const auto a = text.find("앞선 활동 A"), b = text.find("앞선 활동 B"), w = text.find("첫 경고"), marker = text.find("---- 이후 기록 ----");
    CHECK(a != std::string::npos && b != std::string::npos && w != std::string::npos && a < b && b < marker && marker < w);
}
// 파일이 열린 뒤에는 낮은 심각도의 줄도 같은 파일에 이어 쓴다.
TEST_CASE(ErrorLog_AfterFileOpensLaterNotesAppend) {
    TempLogs temp;
    ErrorLog::Record(LogLevel::kError, "검사", "오류 하나");
    ErrorLog::Note("오류 뒤 활동");
    const auto text = temp.Read();
    CHECK(Has(text, "[ERROR] [검사] 오류 하나") && Has(text, "오류 뒤 활동") && text.find("오류 하나") < text.find("오류 뒤 활동"));
}
// 예외는 형식 이름과 메시지를 남기고 중첩 원인까지 풀어 쓴다.
TEST_CASE(ErrorLog_ExceptionRecordsTypeMessageAndNestedCause) {
    TempLogs temp;
    try {
        try { throw std::out_of_range("안쪽 원인"); }
        catch (...) { std::throw_with_nested(std::runtime_error("바깥 오류")); }
    } catch (...) { ErrorLog::RecordException("검사 위치", std::current_exception(), LogLevel::kFatal); }
    const auto text = temp.Read();
    CHECK(Has(text, "[FATAL]") && Has(text, "검사 위치") && Has(text, "runtime_error") && Has(text, "바깥 오류") && Has(text, "원인:") && Has(text, "out_of_range") && Has(text, "안쪽 원인"));
}
// 문자열/정수 등 std::exception이 아닌 예외도 형식을 구별해 남긴다.
TEST_CASE(ErrorLog_NonStandardExceptionsAreDescribed) {
    TempLogs temp;
    try { throw 42; } catch (...) { ErrorLog::RecordException("정수 예외", std::current_exception()); }
    struct Opaque {};
    try { throw Opaque{}; } catch (...) { ErrorLog::RecordException("알 수 없는 예외", std::current_exception()); }
    const auto text = temp.Read();
    CHECK(Has(text, "int: 42") && Has(text, "알 수 없는 형식의 예외"));
}
// assertion은 표현식·파일·줄을 경고로 남긴다.
TEST_CASE(ErrorLog_AssertionRecordsExpressionFileAndLine) {
    TempLogs temp;
    ErrorLog::RecordAssertion("keyValue", "\\Ns\\O\\Winutil.cpp", 731);
    const auto text = temp.Read();
    CHECK(Has(text, "[WARN ] [assertion]") && Has(text, "\\Ns\\O\\Winutil.cpp:731 keyValue"));
}
// 한국어 메시지는 UTF-8 바이트 그대로 보존된다.
TEST_CASE(ErrorLog_KoreanTextRoundTripsAsUtf8) {
    TempLogs temp;
    ErrorLog::Record(LogLevel::kError, "한글", "연결되지 않은 작업: 훨리베이스 후처리");
    CHECK(Has(temp.Read(), "[한글] 연결되지 않은 작업: 훨리베이스 후처리"));
}
// 메모리 활동 기록은 최근 1024줄만 보관하고 첫 경고 때 그 최근 줄만 쓴다.
TEST_CASE(ErrorLog_RingKeepsOnlyNewest1024Lines) {
    TempLogs temp;
    for (int i = 0; i < 1100; ++i) ErrorLog::Note("활동 번호 " + std::to_string(i) + " 끝"); // 오래된 줄을 밀어내는 반복이다.
    CHECK(ErrorLog::BufferedLineCount() == 1024);
    ErrorLog::Record(LogLevel::kWarning, "검사", "경고");
    const auto text = temp.Read();
    CHECK(Has(text, "활동 번호 1099 끝") && Has(text, "활동 번호 76 끝") && !Has(text, "활동 번호 75 끝") && !Has(text, "활동 번호 0 끝"));
}
// 구성하지 않은 상태에서는 치명 기록도 파일을 만들거나 예외를 던지지 않는다.
TEST_CASE(ErrorLog_UnconfiguredNeverCreatesFiles) {
    ErrorLog::ResetForTesting();
    ErrorLog::Record(LogLevel::kFatal, "검사", "구성 없음");
    CHECK(!ErrorLog::Configured() && ErrorLog::CurrentFile().empty());
}
// 여러 스레드의 기록이 줄 단위로 섞이지 않고 모두 남는다.
TEST_CASE(ErrorLog_ConcurrentRecordsStayWholeLines) {
    TempLogs temp;
    std::vector<std::thread> threads;
    // 네 스레드가 각각 100줄을 쓰는 반복이다.
    for (int t = 0; t < 4; ++t) threads.emplace_back([t] { for (int j = 0; j < 100; ++j) ErrorLog::Record(LogLevel::kError, "스레드", "T" + std::to_string(t) + "-" + std::to_string(j) + " 본문끝"); });
    for (auto& thread : threads) thread.join();
    const auto text = temp.Read();
    int found = 0;
    // 모든 기대 줄이 한 번씩, 온전한 한 줄로 있는지 확인하는 반복이다.
    for (int t = 0; t < 4; ++t) for (int j = 0; j < 100; ++j) { const auto line = "T" + std::to_string(t) + "-" + std::to_string(j) + " 본문끝\n"; if (text.find(line) != std::string::npos) ++found; }
    CHECK(found == 400);
}
// 같은 초에 다시 구성해도 이전 파일을 덮어쓰지 않고 이어 쓴다.
TEST_CASE(ErrorLog_SameSecondReopenAppendsInsteadOfOverwriting) {
    TempLogs temp;
    ErrorLog::Record(LogLevel::kError, "검사", "첫 구성의 기록");
    ErrorLog::Configure(temp.logs, "11.01");
    ErrorLog::Record(LogLevel::kError, "검사", "둘째 구성의 기록");
    const auto files = temp.Files();
    if (files.size() == 1) { const auto text = temp.Read(); CHECK(Has(text, "첫 구성의 기록") && Has(text, "둘째 구성의 기록")); }
    else CHECK(files.size() == 2); // 초가 바뀐 드문 경우에는 두 파일이 서로 덮어쓰지 않는다.
}
// 로그는 지정한 logs 폴더 밖에 아무 파일도 만들지 않는다.
TEST_CASE(ErrorLog_WritesOnlyInsideConfiguredLogsFolder) {
    TempLogs temp;
    ErrorLog::Record(LogLevel::kError, "검사", "폴더 확인");
    std::size_t outside = 0;
    // 임시 루트 아래의 모든 항목 중 logs 밖의 것을 세는 반복이다.
    for (const auto& entry : std::filesystem::recursive_directory_iterator(temp.base)) if (entry.path().parent_path() != temp.logs && entry.path() != temp.logs) ++outside;
    CHECK(outside == 0 && temp.Files().size() == 1);
}
