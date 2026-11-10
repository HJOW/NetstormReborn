// 7차(오류/예외 처리와 상세 로그) 공통 구조다. 원본에는 대응 코드가 없는 새 장치이며 M7.5의 공통 assertion 파일도 이 구조를 쓴다.
// 사용자 홈의 %USERPROFILE%\Saved Games\Netstorm\logs\yyyyMMddHHmmss.log 에 비정상 종료 직전까지의 상세 정보를 남긴다.
#pragma once
#include <exception>
#include <filesystem>
#include <string>
#include <string_view>

namespace netstorm::platform { // 로그 파일/예외/비정상 종료 핸들러는 OS 경계에 속하므로 platform 계층에 둔다.

// 한 줄 기록의 심각도다. kInfo는 메모리 활동 기록에만 남고, kWarning 이상이 처음 나오면 로그 파일이 만들어진다.
enum class LogLevel {
    kInfo,    // 정상 동작 중의 활동 기록이다(시작 정보·미션 진입 등). 파일을 만들지 않는다.
    kWarning, // 복귀 가능한 진단이다(예: 원문 assertion). 처음 나오면 로그 파일을 만들고 이전 활동 기록을 함께 쓴다.
    kError,   // 처리된 오류 또는 던져진 예외다. 즉시 디스크에 반영한다.
    kFatal    // 프로세스 종료로 이어지는 치명 오류다. 즉시 디스크에 반영한다.
};

// 프로세스 전체가 공유하는 오류 로그다. 모든 함수는 스레드 안전하며, 구성되지 않았으면 메모리 기록만 하고 파일은 만들지 않는다.
// 사용 순서: 프로그램 시작에서 Configure → InstallCrashHandlers → CaptureStandardStreams, 이후 필요한 곳에서 Note/Record*를 호출한다.
class ErrorLog {
public:
    // 로그 폴더(예: UserDataRoot::Logs())와 제품 버전을 지정한다. 파일은 만들지 않으며 폴더도 첫 파일 기록 때 만든다.
    // 이미 구성돼 있으면 열린 파일을 닫고 새 폴더로 다시 구성한다(검사용 재구성 포함).
    static void Configure(const std::filesystem::path& logsDirectory, std::string_view productVersion);
    // 구성 여부다. false이면 Record는 메모리 기록만 한다.
    static bool Configured();
    // 정상 활동 한 줄을 메모리 기록(최근 1024줄)에만 남긴다. 파일을 만들지 않으며 치명 오류 때 앞부분으로 함께 기록된다.
    static void Note(std::string_view text);
    // 지정한 심각도로 한 줄을 남긴다. kWarning 이상이면 파일을 만들고(처음 한 번) 이전 활동 기록을 먼저 쓴다.
    static void Record(LogLevel level, std::string_view category, std::string_view message);
    // 처리된 예외나 던져진 예외(exception_ptr)를 형식 이름·what()·중첩 예외까지 풀어 기록한다. where는 발생/포획 위치 설명이다.
    static void RecordException(std::string_view where, std::exception_ptr error, LogLevel level = LogLevel::kError);
    // 원문 assertion(M7.5)의 표현식·파일·줄을 kWarning으로 기록한다. 환경 변수 NETSTORM_ASSERT_WINDOW=1이면 확인 창도 띄운다.
    static void RecordAssertion(std::string_view expression, std::string_view file, unsigned line);
    // 종료 직전 원인을 kFatal로 기록하고 디스크에 반영한다. 호출 위치의 스택 추적도 함께 남긴다.
    static void RecordFatal(std::string_view where, std::string_view cause);
    // std::terminate·처리되지 않은 SEH 예외·잘못된 CRT 인자·순수 가상 호출·abort 신호를 로그로 남기는 핸들러를 설치한다.
    static void InstallCrashHandlers();
    // std::clog/std::cerr의 모든 줄을 원래 출력과 함께 메모리 활동 기록으로 복제한다. 콘솔 출력은 바뀌지 않는다.
    static void CaptureStandardStreams();
    // 지금까지 만든 로그 파일 경로다. 아직 아무것도 기록하지 않았으면 빈 경로다.
    static std::filesystem::path CurrentFile();
    // 메모리 활동 기록에 남아 있는 줄 수다(검사용).
    static std::size_t BufferedLineCount();
    // 로그 파일 이름 yyyyMMddHHmmss.log를 계산한다(요구 사양의 이름 규칙, 검사용으로 공개).
    static std::string FileNameFor(int year, int month, int day, int hour, int minute, int second);
    // 구성·열린 파일·메모리 기록을 모두 초기화한다(검사용). 설치된 핸들러는 그대로 둔다.
    static void ResetForTesting();
    // 개발 검사용: 지정한 종류로 의도적으로 비정상 종료한다. 종류는 throw/terminate/access/divzero/abort/stack/invalid/purecall이다.
    [[noreturn]] static void CrashForTest(std::string_view kind);
};
}
