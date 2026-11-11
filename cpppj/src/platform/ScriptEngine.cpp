// QuickJS-ng 엔진 하나의 생성·평가·해제와 시작 자체 검사를 구현한다. 설명과 사용법은 ScriptEngine.h에 있다.
// 엔진은 quickjs-libc 없이 언어 내장 객체만 가진 컨텍스트(JS_NewContext)로 만든다. 그래서 std/os 모듈·파일·소켓·환경 변수에 닿는 함수가 스크립트에 없다.
// 엔진 핵심은 CRT rand/srand·FPU 제어 워드·작업 폴더를 바꾸지 않으므로(소스 검사 결과) 게임의 난수·부동소수 결정성에 영향을 주지 않는다.
// (2026-11-11 AGENTS 14차 QuickJS 엔진 탑재로 신규)
#include "platform/ScriptEngine.h"

#include <malloc.h> // _msize: MSVC CRT 블록의 실제 크기다(엔진 상류도 Windows에서 같은 함수를 쓴다).
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#if defined(_MSC_VER)
#pragma warning(push, 0) // 상류 헤더의 /W4 경고를 이 파일의 경고와 섞지 않는다(상류 파일은 바꾸지 않는다).
#endif
#include "quickjs.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace netstorm::platform {
namespace { // 이 파일 안에서만 쓰는 엔진 도우미와 자체 검사 정의다.

// 실행 시간 한도를 재는 단조 시계다. 시스템 시각 변경의 영향을 받지 않는다.
using Clock = std::chrono::steady_clock;

// 평가 한 번의 마감 시각이다. 엔진 인터럽트 콜백이 opaque 인자로 받아 읽는다.
// (2026-11-11 14차 신규)
struct EvaluationDeadline {
    // 평가 중이고 시간 한도가 있을 때만 true다. 평가 밖에서 엔진이 내부 작업(GC 등)을 해도 끊지 않게 한다.
    bool active = false;
    // 이 시각이 지나면 실행을 끊는다.
    Clock::time_point at{};
};

// 엔진이 주기적으로 부르는 인터럽트 콜백이다. 마감이 지났으면 1을 돌려 실행을 "interrupted" 예외로 끝낸다.
// (2026-11-11 14차 신규: 무한 반복 스크립트가 게임 스레드를 붙잡지 못하게 한다)
int InterruptPastDeadline(JSContext*, void* opaque) {
    const auto* deadline = static_cast<const EvaluationDeadline*>(opaque);
    return deadline->active && Clock::now() >= deadline->at ? 1 : 0;
}

// 평가가 끝나면(예외 경로 포함) 마감을 끄는 범위 객체다.
// (2026-11-11 14차 신규)
class DeadlineScope {
public:
    // 한도가 0이 아니면 지금부터 한도 밀리초 뒤를 마감으로 켠다.
    DeadlineScope(EvaluationDeadline& deadline, std::uint32_t limitMilliseconds) : deadline_(deadline) {
        deadline_.at = Clock::now() + std::chrono::milliseconds(limitMilliseconds);
        deadline_.active = limitMilliseconds != 0;
    }
    // 범위를 벗어나면 마감을 끈다.
    ~DeadlineScope() { deadline_.active = false; }
    // 마감 상태는 한 평가에만 속하므로 복사를 막는다.
    DeadlineScope(const DeadlineScope&) = delete;
    // 마감 상태는 한 평가에만 속하므로 복사 대입을 막는다.
    DeadlineScope& operator=(const DeadlineScope&) = delete;
    // 마감이 켜져 있고 이미 지났는지 확인한다(Promise 작업 사이 검사용).
    bool Expired() const { return deadline_.active && Clock::now() >= deadline_.at; }

private:
    // 엔진 상태가 소유한 마감 값의 참조다.
    EvaluationDeadline& deadline_;
};

// 엔진 할당자 콜백이 공유하는 메모리 예산이다. 상류 JS_SetMemoryLimit 대신 여기서 한도를 지킨다.
// 상류 한도는 할당자 앞단에서 거절하므로 거절 사실을 알 수 없고, 한도가 완전히 차면 오류 객체도 못 만들어 예외 값이 null이 된다.
// 할당자에서 직접 거절하면 그 사실(refused)로 null 예외를 "out of memory"로 보고할 수 있다.
// (2026-11-11 14차 신규)
struct AllocatorBudget {
    // 한도(바이트)다. 0이면 무제한이다.
    std::size_t limitBytes = 0;
    // 엔진이 지금 CRT 힙에서 잡고 있는 바이트(_msize 합계)다.
    std::size_t usedBytes = 0;
    // 한도 때문에 할당을 거절했으면 true다. Evaluate 시작 때 지운다.
    bool refused = false;
};

// releasing 바이트를 돌려주고 requested 바이트를 새로 잡아도 한도 안인지 확인한다. 넘으면 refused를 켜고 false다.
// (2026-11-11 14차 신규)
bool AdmitAllocation(AllocatorBudget& budget, std::size_t releasing, std::size_t requested) {
    if (budget.limitBytes == 0) return true;
    const std::size_t base = budget.usedBytes - releasing;
    if (requested > budget.limitBytes || base > budget.limitBytes - requested) {
        budget.refused = true;
        return false;
    }
    return true;
}

// 엔진 calloc 콜백이다. 곱셈 넘침과 한도를 확인한 뒤 CRT calloc을 부른다.
// (2026-11-11 14차 신규)
void* BudgetCalloc(void* opaque, size_t count, size_t size) {
    auto& budget = *static_cast<AllocatorBudget*>(opaque);
    if (size != 0 && count > SIZE_MAX / size) return nullptr;
    if (!AdmitAllocation(budget, 0, count * size)) return nullptr;
    void* block = std::calloc(count, size);
    if (block) budget.usedBytes += _msize(block);
    return block;
}

// 엔진 malloc 콜백이다. 한도를 확인한 뒤 CRT malloc을 부른다.
// (2026-11-11 14차 신규)
void* BudgetMalloc(void* opaque, size_t size) {
    auto& budget = *static_cast<AllocatorBudget*>(opaque);
    if (!AdmitAllocation(budget, 0, size)) return nullptr;
    void* block = std::malloc(size);
    if (block) budget.usedBytes += _msize(block);
    return block;
}

// 엔진 free 콜백이다. 잡을 때 더한 _msize만큼 사용량에서 뺀다.
// (2026-11-11 14차 신규)
void BudgetFree(void* opaque, void* block) {
    if (!block) return;
    auto& budget = *static_cast<AllocatorBudget*>(opaque);
    budget.usedBytes -= _msize(block);
    std::free(block);
}

// 엔진 realloc 콜백이다. 기존 블록 크기를 돌려준다고 보고 한도를 확인한다. 크기 0은 해제로 처리한다.
// (2026-11-11 14차 신규)
void* BudgetRealloc(void* opaque, void* block, size_t size) {
    if (!block) return BudgetMalloc(opaque, size);
    if (size == 0) {
        BudgetFree(opaque, block);
        return nullptr;
    }
    auto& budget = *static_cast<AllocatorBudget*>(opaque);
    const std::size_t previous = _msize(block);
    if (!AdmitAllocation(budget, previous, size)) return nullptr; // 거절해도 기존 블록은 그대로 남는다(엔진이 계속 소유).
    void* moved = std::realloc(block, size);
    if (moved) budget.usedBytes = budget.usedBytes - previous + _msize(moved);
    return moved;
}

// 엔진 usable-size 콜백이다. 상류 Windows 기본값과 같은 _msize를 쓴다.
// (2026-11-11 14차 신규)
size_t BudgetUsableSize(const void* block) {
    return block ? _msize(const_cast<void*>(block)) : 0;
}

// 예산 할당자 콜백 묶음이다. JS_NewRuntime2에 넘긴다.
// (2026-11-11 14차 신규)
constexpr JSMallocFunctions kBudgetAllocator = {BudgetCalloc, BudgetMalloc, BudgetFree, BudgetRealloc, BudgetUsableSize};

// 대기 중인 엔진 예외를 꺼내 버린다. 예외 문자열 변환 중 또 예외가 날 때 엔진 상태를 깨끗이 되돌리는 데 쓴다.
// (2026-11-11 14차 신규)
void DiscardPendingException(JSContext* context) {
    JS_FreeValue(context, JS_GetException(context));
}

// 값을 UTF-8 문자열로 바꾼다. 변환 자체가 JS 예외를 내면 false를 돌려주고 그 예외는 엔진에 남겨 둔다.
// (2026-11-11 14차 신규)
bool TryToUtf8(JSContext* context, JSValueConst value, std::string& text) {
    size_t length = 0;
    const char* characters = JS_ToCStringLen(context, &length, value);
    if (!characters) return false;
    text.assign(characters, length);
    JS_FreeCString(context, characters);
    return true;
}

// 예외 값을 "이름: 메시지" 문자열로 바꾸고, Error 객체에 stack 문자열이 있으면 줄을 바꿔 이어 붙인다. exception의 소유권을 가져가 해제한다.
// (2026-11-11 14차 신규)
std::string DescribeException(JSContext* context, JSValue exception) {
    std::string text;
    if (!TryToUtf8(context, exception, text)) { // toString 자체가 실패하는 예외 값이면 대신할 표시를 쓴다.
        DiscardPendingException(context);
        text = "<exception value could not be converted to text>";
    }
    if (JS_IsError(exception)) { // Error 계열이면 JS 스택(파일 이름·줄 번호)을 덧붙여 원인 추적을 돕는다.
        JSValue stack = JS_GetPropertyStr(context, exception, "stack");
        std::string stackText;
        if (JS_IsException(stack)) DiscardPendingException(context);
        else if (JS_IsString(stack) && TryToUtf8(context, stack, stackText) && !stackText.empty()) {
            if (stackText.back() == '\n') stackText.pop_back(); // 엔진이 붙이는 마지막 줄바꿈은 결과 문자열에서 뺀다.
            text += '\n';
            text += stackText;
        }
        JS_FreeValue(context, stack);
    }
    JS_FreeValue(context, exception);
    return text;
}

// 시작 자체 검사의 고정 항목 정의다. 외부 입력 없이 이 표의 식만 평가한다.
// (2026-11-11 14차 신규)
struct SelfCheckCase {
    // 보고서에 쓰는 항목 이름이다.
    const char* name;
    // 평가할 JavaScript 원문(UTF-8)이다.
    const char* source;
    // 평가가 성공해야 하면 true, 예외로 끝나야 하면 false다.
    bool expectSuccess;
    // 성공 항목은 결과 전체, 예외 항목은 예외 문자열 첫 줄의 기대값이다.
    const char* expected;
};

// 시작 자체 검사 항목 표다. 식 평가·JSON·UTF-8 한글 왕복·BigInt·예외 전달, 그리고 파일/네트워크 장치가 없음을 확인한다.
// (2026-11-11 14차 신규)
constexpr SelfCheckCase kSelfCheckCases[] = {
    {"expression", "[1, 2, 3].map((x) => x * 2).join(',')", true, "2,4,6"},
    {"json", "JSON.stringify({ unit: 'Priest', cost: 25 })", true, "{\"unit\":\"Priest\",\"cost\":25}"},
    {"utf8-korean", "'사제·신전'.length + ':' + '사제·신전'", true, "5:사제·신전"},
    {"bigint", "(2n ** 64n).toString()", true, "18446744073709551616"},
    {"exception", "throw new RangeError('selfcheck')", false, "RangeError: selfcheck"},
    {"no-host-access", "[typeof std, typeof os, typeof require, typeof fetch, typeof XMLHttpRequest, typeof WebSocket].join(',')", true,
        "undefined,undefined,undefined,undefined,undefined,undefined"},
};

} // namespace

// ScriptEngine의 내부 상태다. 런타임·컨텍스트 포인터와 평가 시간 한도를 가진다.
// (2026-11-11 14차 신규)
struct ScriptEngine::State {
    // 엔진 런타임(힙·GC·작업 큐의 소유자)이다.
    JSRuntime* runtime = nullptr;
    // 전역 객체와 내장 객체를 가진 실행 컨텍스트다.
    JSContext* context = nullptr;
    // Evaluate 한 번의 시간 한도(밀리초)다. 0이면 한도 없음이다.
    std::uint32_t timeLimitMilliseconds = 0;
    // 인터럽트 콜백이 읽는 현재 평가의 마감이다. 런타임보다 오래 살아야 하므로 이 상태 안에 둔다.
    EvaluationDeadline deadline;
    // 할당자 콜백이 읽고 쓰는 메모리 예산이다. 런타임 해제(JS_FreeRuntime)의 free 콜백까지 살아 있어야 하므로 이 상태 안에 둔다.
    AllocatorBudget budget;
};

// 예산 할당자로 런타임 → 한도(스택·인터럽트) → 컨텍스트 순서로 만든다. 실패하면 만든 것을 해제하고 std::runtime_error를 던진다.
// (2026-11-11 14차 신규)
ScriptEngine::ScriptEngine(const ScriptEngineLimits& limits) : state_(std::make_unique<State>()) {
    state_->timeLimitMilliseconds = limits.timeLimitMilliseconds;
    state_->budget.limitBytes = limits.memoryBytes;
    state_->runtime = JS_NewRuntime2(&kBudgetAllocator, &state_->budget); // 메모리 한도는 할당자 콜백이 지킨다(JS_SetMemoryLimit은 쓰지 않는다).
    if (!state_->runtime) throw std::runtime_error("QuickJS 런타임을 만들지 못했습니다(메모리 부족 또는 한도 초과)");
    JS_SetMaxStackSize(state_->runtime, limits.stackBytes); // 0이면 엔진이 스택 검사를 하지 않는다.
    JS_SetInterruptHandler(state_->runtime, InterruptPastDeadline, &state_->deadline);
    state_->context = JS_NewContext(state_->runtime);
    if (!state_->context) { // 컨텍스트 실패 시 소멸자가 불리지 않으므로 런타임을 여기서 해제한다.
        JS_FreeRuntime(state_->runtime);
        state_->runtime = nullptr;
        throw std::runtime_error("QuickJS 컨텍스트를 만들지 못했습니다(메모리 부족 또는 한도 초과)");
    }
}

// 컨텍스트를 먼저, 런타임을 나중에 해제한다(엔진이 요구하는 순서). 남은 Promise 작업도 런타임 해제가 함께 정리한다.
// (2026-11-11 14차 신규)
ScriptEngine::~ScriptEngine() {
    if (!state_) return;
    if (state_->context) JS_FreeContext(state_->context);
    if (state_->runtime) JS_FreeRuntime(state_->runtime);
    assert(state_->budget.usedBytes == 0 && "QuickJS 런타임 해제 뒤 할당자 사용량이 0이 아니다(계산 불일치 또는 누수)");
}

// 엄격 모드 전역 스크립트로 평가하고 결과를 문자열로 바꾼 뒤 대기 중인 Promise 작업을 비운다. 엔진 예외는 결과의 text로 돌려준다.
// (2026-11-11 14차 신규)
ScriptEvaluation ScriptEngine::Evaluate(std::string_view source, std::string_view fileName) {
    const std::string code(source);   // JS_Eval은 input[length]가 '\0'인 버퍼를 요구하므로 복사본을 만든다.
    const std::string name(fileName); // 오류 스택에 쓰는 이름도 '\0'으로 끝나야 한다.
    JSRuntime* runtime = state_->runtime;
    JSContext* context = state_->context;
    JS_UpdateStackTop(runtime); // 스택 한도를 지금 호출 위치 기준으로 다시 잡는다(생성 위치보다 깊은 곳에서 호출될 수 있다).
    DeadlineScope deadline(state_->deadline, state_->timeLimitMilliseconds);
    state_->budget.refused = false; // 이번 평가에서 한도 거절이 있었는지만 본다.
    ScriptEvaluation result;
    // 실패 결과를 마무리한다. 한도 거절이 있었으면 "out of memory"를 첫 줄에 두고 엔진이 남긴 예외 문자열(null 제외)을 뒤에 붙인다.
    const auto fail = [&](std::string text) {
        result.memoryLimitReached = state_->budget.refused;
        if (result.memoryLimitReached) {
            std::string label = "InternalError: out of memory (script engine memory limit of " + std::to_string(state_->budget.limitBytes) + " bytes)";
            if (text != "null" && !text.empty()) label += "\n" + text;
            text = std::move(label);
        }
        result.text = std::move(text);
        return result;
    };
    JSValue value = JS_Eval(context, code.c_str(), code.size(), name.c_str(), JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
    if (JS_IsException(value)) return fail(DescribeException(context, JS_GetException(context)));
    std::string text;
    const bool converted = TryToUtf8(context, value, text);
    JS_FreeValue(context, value);
    if (!converted) return fail(DescribeException(context, JS_GetException(context))); // 결과 값의 toString이 예외를 낸 경우다.
    // 평가가 예약한 Promise 작업(then/queueMicrotask)을 시간 한도 안에서 모두 실행한다.
    // 시간 한도로 멈추면 남은 작업은 다음 Evaluate가 이어서 실행한다(엔진에 작업 큐를 비우는 공개 API가 없다).
    while (JS_IsJobPending(runtime)) {
        if (deadline.Expired()) return fail("InternalError: interrupted (pending jobs exceeded the time limit)");
        JSContext* jobContext = nullptr;
        if (JS_ExecutePendingJob(runtime, &jobContext) < 0) { // 작업 안의 처리되지 않은 예외다.
            JSContext* failed = jobContext ? jobContext : context;
            return fail(DescribeException(failed, JS_GetException(failed)));
        }
    }
    result.succeeded = true;
    result.text = std::move(text);
    result.memoryLimitReached = state_->budget.refused;
    return result;
}

// 할당자 콜백이 센 현재 사용량(AllocatorBudget::usedBytes)을 돌려준다.
// (2026-11-11 14차 신규)
std::size_t ScriptEngine::MemoryUsedBytes() const {
    return state_->budget.usedBytes;
}

// 엔진이 보고하는 버전 문자열(JS_GetVersion) 앞에 엔진 이름을 붙인다.
// (2026-11-11 14차 신규)
std::string ScriptEngine::EngineVersion() {
    return std::string("QuickJS-ng ") + JS_GetVersion();
}

// 기본 한도의 엔진 하나로 kSelfCheckCases를 차례로 평가하고 해제한다. 엔진 생성 실패는 보고서의 failure로 돌려준다.
// (2026-11-11 14차 신규)
ScriptSelfCheckReport RunScriptEngineSelfCheck() {
    ScriptSelfCheckReport report;
    report.engineVersion = ScriptEngine::EngineVersion();
    const auto started = Clock::now();
    try {
        ScriptEngine engine; // 기본 한도(16MiB·256KiB 스택·1초)로 만들고 이 범위를 벗어나면 해제한다.
        for (const auto& check : kSelfCheckCases) { // 고정 항목을 차례로 평가하고 기대값과 비교한다.
            const auto evaluation = engine.Evaluate(check.source, "<netstorm-selfcheck>");
            ScriptSelfCheckItem item;
            item.name = check.name;
            item.expectSuccess = check.expectSuccess;
            item.expected = check.expected;
            item.actual = check.expectSuccess ? evaluation.text : evaluation.text.substr(0, evaluation.text.find('\n'));
            item.passed = evaluation.succeeded == check.expectSuccess && item.actual == item.expected;
            report.items.push_back(std::move(item));
        }
    } catch (const std::exception& error) {
        report.failure = error.what();
    }
    report.elapsedMilliseconds = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
    report.passed = report.failure.empty() && !report.items.empty()
        && std::all_of(report.items.begin(), report.items.end(), [](const ScriptSelfCheckItem& item) { return item.passed; });
    return report;
}

// 엔진 버전 줄, 항목별 [ OK ]/[FAIL] 줄, 항목 밖 실패 줄, PASS/FAIL 요약 줄 순서로 쓴다.
// (2026-11-11 14차 신규)
std::string FormatScriptSelfCheckReport(const ScriptSelfCheckReport& report) {
    std::string text = "QuickJS self-check: " + report.engineVersion + "\n";
    std::size_t passedCount = 0;
    for (const auto& item : report.items) { // 항목마다 한 줄을 쓰고 실패하면 기대값도 함께 쓴다.
        if (item.passed) ++passedCount;
        text += item.passed ? "  [ OK ] " : "  [FAIL] ";
        text += item.name + ": " + item.actual;
        if (!item.passed) text += "  (expected " + std::string(item.expectSuccess ? "" : "exception ") + item.expected + ")";
        text += '\n';
    }
    if (!report.failure.empty()) text += "  failure: " + report.failure + "\n";
    char summary[128];
    std::snprintf(summary, sizeof summary, "QuickJS self-check %s (%zu/%zu, %.2f ms)\n", report.passed ? "PASS" : "FAIL", passedCount,
        report.items.size(), report.elapsedMilliseconds);
    return text + summary;
}

} // namespace netstorm::platform
