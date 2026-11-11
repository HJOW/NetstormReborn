// 게임에 넣은 JavaScript 엔진(QuickJS-ng)의 런타임·컨텍스트 하나를 소유하고 짧은 코드를 평가하는 장치와 시작 자체 검사다.
// 원본 게임에는 대응 코드가 없는 새 장치다. 엔진 헤더(quickjs.h)는 ScriptEngine.cpp 안에만 두어 이 헤더를 포함하는 쪽이 엔진 구현에 묶이지 않게 한다.
// 현재 범위는 "엔진 탑재"뿐이다. 게임 상태·파일·네트워크·OS 기능은 스크립트에 노출하지 않으며, 엔진도 quickjs-libc(std/os 모듈) 없이 언어 내장 객체만 가진다.
// (2026-11-11 AGENTS 14차 QuickJS 엔진 탑재로 신규. 연동 범위 확대는 LEFT_JOBS.md 결정 D7)
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::platform { // 스크립트 엔진은 원본에 없는 외부 라이브러리 경계라 OS 장치와 같은 platform 계층에 둔다.

// 엔진 하나에 거는 자원 한도다. 기본값은 시작 자체 검사와 짧은 식 평가에 충분하고 게임 프로세스를 위협하지 않는 크기다.
// 사용법: ScriptEngine 생성자에 넘긴다. 한도를 넘으면 해당 Evaluate가 실패 결과(예외 문자열)를 돌려주고 엔진은 계속 쓸 수 있다.
// (2026-11-11 14차 신규)
struct ScriptEngineLimits {
    // 엔진이 CRT 힙에서 잡을 수 있는 총 바이트 한도다. 엔진 할당자 콜백이 직접 세고 넘는 할당을 거절하므로 JS 쪽 "out of memory"가 된다.
    // 0이면 한도 없음이다.
    std::size_t memoryBytes = 16u * 1024u * 1024u;
    // JS 실행이 쓸 수 있는 네이티브 스택 깊이(바이트)다. 게임 주 스레드의 MSVC 기본 스택이 1MiB이므로 그보다 충분히 작게 둔다.
    // Evaluate가 호출 시점의 스택 위치를 기준으로 다시 잡으므로 호출하는 스레드에는 이 값보다 넉넉한 여유 스택이 있어야 한다.
    std::size_t stackBytes = 256u * 1024u;
    // Evaluate 한 번의 실행 시간 한도(밀리초)다. 넘으면 엔진 인터럽트로 "interrupted" 예외가 된다. 0이면 한도 없음이다.
    std::uint32_t timeLimitMilliseconds = 1000;
};

// Evaluate 한 번의 결과다. 성공이면 결과 값을 문자열로 바꾼 UTF-8, 실패면 예외 문자열(+가능하면 JS 스택)이다.
// (2026-11-11 14차 신규)
struct ScriptEvaluation {
    // 예외 없이 평가와 대기 중인 Promise 작업까지 끝났으면 true다.
    bool succeeded = false;
    // 성공: 결과 값의 문자열 표현(undefined는 "undefined"). 실패: "RangeError: ..." 같은 예외 표현과 이어지는 스택 줄이다.
    // 메모리 한도로 실패하면 "InternalError: out of memory (...)"로 시작한다(엔진이 오류 객체조차 만들지 못해 null을 던진 경우 포함).
    std::string text;
    // 이 평가 중 메모리 한도 때문에 엔진 할당을 거절한 적이 있으면 true다. 스크립트가 그 예외를 잡고 성공한 경우에도 true일 수 있다.
    bool memoryLimitReached = false;
};

// QuickJS-ng 런타임(JSRuntime)과 컨텍스트(JSContext) 한 쌍을 소유하는 RAII 객체다. 생성 때 만들고 소멸 때 해제한다.
// 사용법: ScriptEngine engine; auto result = engine.Evaluate("1 + 2"); → result.succeeded == true, result.text == "3".
// 한 객체는 한 스레드에서만 써야 한다(엔진 런타임이 스레드 안전하지 않다). 서로 다른 객체는 다른 스레드에서 따로 써도 된다.
// (2026-11-11 14차 QuickJS 엔진 탑재로 신규)
class ScriptEngine {
public:
    // 런타임·컨텍스트를 만들고 한도를 건다. 엔진이 메모리를 얻지 못하면 std::runtime_error를 던진다.
    explicit ScriptEngine(const ScriptEngineLimits& limits = {});
    // 컨텍스트를 먼저, 런타임을 나중에 해제한다.
    ~ScriptEngine();
    // 엔진 핸들은 하나의 소유자만 가지므로 복사를 막는다.
    ScriptEngine(const ScriptEngine&) = delete;
    // 엔진 핸들은 하나의 소유자만 가지므로 복사 대입을 막는다.
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    // 전역 스크립트(엄격 모드) 하나를 평가하고, 이어서 대기 중인 Promise 작업을 시간 한도 안에서 모두 실행한다.
    // source는 UTF-8 JavaScript 원문, fileName은 오류 스택에 표시할 이름이다. 엔진 예외는 C++ 예외로 던지지 않고 결과로 돌려준다.
    ScriptEvaluation Evaluate(std::string_view source, std::string_view fileName = "<netstorm>");
    // 엔진이 지금 CRT 힙에서 잡고 있는 바이트 수(할당자 콜백이 센 값)다. 진단/검사용이며 객체를 훑지 않아 비용이 작다.
    std::size_t MemoryUsedBytes() const;
    // 탑재한 엔진 이름과 버전이다(예: "QuickJS-ng 0.18.0"). 엔진을 만들지 않고도 호출할 수 있다.
    static std::string EngineVersion();

private:
    // quickjs.h 형식을 헤더에 드러내지 않기 위한 내부 상태(런타임·컨텍스트 포인터와 시간 한도)다. 정의는 ScriptEngine.cpp에 있다.
    struct State;
    // 내부 상태의 단독 소유 포인터다. 생성에 성공한 객체에서는 항상 유효하다.
    std::unique_ptr<State> state_;
};

// 시작 자체 검사의 항목 하나다. 고정된 짧은 식을 평가해 기대 문자열과 비교한 결과를 담는다.
// (2026-11-11 14차 신규)
struct ScriptSelfCheckItem {
    // 사람이 읽는 항목 이름이다(영문 식별자, 로그·CLI 출력용).
    std::string name;
    // 평가가 성공해야 하는지(true) 예외로 끝나야 하는지(false)다.
    bool expectSuccess = true;
    // 기대 결과 문자열이다. 예외 항목은 예외 문자열의 앞부분(접두)과 비교한다.
    std::string expected;
    // 실제 결과 문자열이다(성공 결과 또는 예외 문자열의 첫 줄).
    std::string actual;
    // 기대와 일치했으면 true다.
    bool passed = false;
};

// 시작 자체 검사 전체의 결과다.
// (2026-11-11 14차 신규)
struct ScriptSelfCheckReport {
    // 검사한 엔진 이름과 버전이다(ScriptEngine::EngineVersion()).
    std::string engineVersion;
    // 실행한 항목 목록이다. 엔진 생성 자체가 실패하면 비어 있고 failure에 이유가 들어간다.
    std::vector<ScriptSelfCheckItem> items;
    // 엔진 생성 실패 등 항목 밖의 실패 이유다. 비어 있으면 항목 밖 실패가 없었다는 뜻이다.
    std::string failure;
    // 엔진 생성부터 해제까지 걸린 시간(밀리초)이다.
    double elapsedMilliseconds = 0.0;
    // failure가 비어 있고 모든 항목이 통과했으면 true다.
    bool passed = false;
};

// 엔진을 하나 만들어 고정 항목(식 평가·JSON·UTF-8 한글·BigInt·예외 전달·외부 장치 부재)을 평가한 뒤 해제한다.
// 게임 시작 때와 Netstorm.exe --quickjs-selftest가 호출한다. 엔진 오류는 던지지 않고 보고서의 failure로 돌려준다.
// (2026-11-11 14차 신규: 탑재 범위의 "생성·해제와 최소 자체 검사")
ScriptSelfCheckReport RunScriptEngineSelfCheck();

// 자체 검사 보고서를 사람이 읽는 여러 줄 UTF-8 텍스트로 만든다(CLI 출력과 경고 로그 본문용). 마지막 줄은 PASS/FAIL 요약이다.
// (2026-11-11 14차 신규)
std::string FormatScriptSelfCheckReport(const ScriptSelfCheckReport& report);

} // namespace netstorm::platform
