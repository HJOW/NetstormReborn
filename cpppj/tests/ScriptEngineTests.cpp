// 14차 QuickJS 엔진 탑재: 엔진 생성·평가·해제, 자원 한도, 외부 장치 부재와 시작 자체 검사를 게임 실행 없이 확인한다.
// (2026-11-11 AGENTS 14차 QuickJS 엔진 탑재로 신규)
#include "TestSupport.h"
#include "platform/ScriptEngine.h"
#include <chrono>
#include <stdexcept>
#include <string>

using netstorm::platform::ScriptEngine;
using netstorm::platform::ScriptEngineLimits;

namespace { // 이 파일의 검사 도우미다.
// text가 prefix로 시작하는지 확인한다.
bool StartsWith(const std::string& text, const std::string& prefix) { return text.compare(0, prefix.size(), prefix) == 0; }
// text 안에 part가 있는지 확인한다.
bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }
}

// 짧은 식·undefined·JSON 결과를 문자열로 돌려준다.
TEST_CASE(ScriptEngine_EvaluatesExpressions) {
    ScriptEngine engine;
    const auto sum = engine.Evaluate("1 + 2");
    CHECK(sum.succeeded && sum.text == "3");
    const auto nothing = engine.Evaluate("void 0");
    CHECK(nothing.succeeded && nothing.text == "undefined");
    const auto json = engine.Evaluate("JSON.stringify({ a: [1, 'b'], c: null })");
    CHECK(json.succeeded && json.text == "{\"a\":[1,\"b\"],\"c\":null}");
}

// 같은 엔진의 전역 상태는 평가 사이에 유지되고, 다른 엔진과는 섞이지 않는다.
TEST_CASE(ScriptEngine_GlobalStateIsPerEngine) {
    ScriptEngine first;
    ScriptEngine second;
    CHECK(first.Evaluate("globalThis.counter = 41").succeeded);
    const auto next = first.Evaluate("++globalThis.counter");
    CHECK(next.succeeded && next.text == "42");
    const auto other = second.Evaluate("typeof globalThis.counter");
    CHECK(other.succeeded && other.text == "undefined");
}

// UTF-8 원문과 결과가 한국어·일본어·보조 평면 문자(서로게이트 쌍)까지 그대로 왕복한다.
TEST_CASE(ScriptEngine_Utf8RoundTrip) {
    ScriptEngine engine;
    const auto korean = engine.Evaluate("'훨리베이스'.length + ':' + '훨리베이스'");
    CHECK(korean.succeeded && korean.text == "5:훨리베이스");
    const auto japanese = engine.Evaluate("['ワークショップ'].join()");
    CHECK(japanese.succeeded && japanese.text == "ワークショップ");
    const auto astral = engine.Evaluate("'🌩'.length + ':' + '🌩'");
    CHECK(astral.succeeded && astral.text == "2:🌩");
}

// 엔진 예외는 C++ 예외가 아니라 실패 결과가 되며, 오류 이름·메시지·파일 이름이 남고 엔진은 계속 쓸 수 있다.
TEST_CASE(ScriptEngine_ExceptionsBecomeFailedResults) {
    ScriptEngine engine;
    const auto thrown = engine.Evaluate("function boom() { throw new TypeError('boom'); }\nboom();", "<test-script>");
    CHECK(!thrown.succeeded && StartsWith(thrown.text, "TypeError: boom"));
    CHECK(Contains(thrown.text, "<test-script>")); // 스택 줄에 넘긴 파일 이름이 들어간다.
    const auto syntax = engine.Evaluate("let = ;");
    CHECK(!syntax.succeeded && StartsWith(syntax.text, "SyntaxError"));
    const auto value = engine.Evaluate("throw 42");
    CHECK(!value.succeeded && value.text == "42"); // Error가 아닌 값도 문자열로 남는다.
    const auto after = engine.Evaluate("1 + 1");
    CHECK(after.succeeded && after.text == "2");
}

// 평가는 엄격 모드라 선언하지 않은 변수에 대입하면 ReferenceError다.
TEST_CASE(ScriptEngine_StrictMode) {
    ScriptEngine engine;
    const auto result = engine.Evaluate("undeclaredVariable = 1");
    CHECK(!result.succeeded && StartsWith(result.text, "ReferenceError"));
}

// 시간 한도: 무한 반복이 한도 근처에서 "interrupted"로 끊기고 엔진은 다시 쓸 수 있다.
TEST_CASE(ScriptEngine_TimeLimitInterruptsInfiniteLoop) {
    ScriptEngineLimits limits;
    limits.timeLimitMilliseconds = 50;
    ScriptEngine engine(limits);
    const auto started = std::chrono::steady_clock::now();
    const auto result = engine.Evaluate("for (;;) {}");
    const auto elapsed = std::chrono::steady_clock::now() - started;
    CHECK(!result.succeeded && StartsWith(result.text, "InternalError: interrupted"));
    CHECK(elapsed < std::chrono::seconds(5)); // 50ms 한도가 실제로 실행을 멈췄다(넉넉한 상한).
    const auto after = engine.Evaluate("'alive'");
    CHECK(after.succeeded && after.text == "alive");
}

// 메모리 한도: 끝없는 할당이 out of memory 실패로 멈추고(엔진이 null을 던져도 같은 문구), 함수 범위가 끝나 해제된 뒤 엔진은 다시 쓸 수 있다.
TEST_CASE(ScriptEngine_MemoryLimitStopsRunawayAllocation) {
    ScriptEngineLimits limits;
    limits.memoryBytes = 4u * 1024u * 1024u;
    limits.timeLimitMilliseconds = 10000; // 메모리 한도가 먼저 걸리도록 시간 한도는 넉넉히 둔다.
    ScriptEngine engine(limits);
    const auto baseline = engine.MemoryUsedBytes();
    CHECK(baseline > 0 && baseline < limits.memoryBytes);
    const auto result = engine.Evaluate("(() => { const all = []; for (;;) all.push(new Array(1024).fill(7)); })()");
    CHECK(!result.succeeded && result.memoryLimitReached);
    CHECK(StartsWith(result.text, "InternalError: out of memory (script engine memory limit of 4194304 bytes)"));
    CHECK(engine.MemoryUsedBytes() <= limits.memoryBytes);
    const auto after = engine.Evaluate("[1, 2].length");
    CHECK(after.succeeded && after.text == "2" && !after.memoryLimitReached);
    const auto thrownNull = engine.Evaluate("throw null"); // 스크립트가 직접 던진 null은 메모리 부족으로 바꾸지 않는다.
    CHECK(!thrownNull.succeeded && thrownNull.text == "null" && !thrownNull.memoryLimitReached);
}

// 컨텍스트조차 만들 수 없는 작은 한도면 생성자가 std::runtime_error를 던지고 자원을 남기지 않는다.
TEST_CASE(ScriptEngine_TinyMemoryLimitFailsConstruction) {
    ScriptEngineLimits limits;
    limits.memoryBytes = 16u * 1024u;
    bool threw = false;
    try { ScriptEngine engine(limits); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
}

// 스택 한도: 끝없는 재귀가 게임 스레드 스택을 넘기 전에 RangeError로 멈춘다.
TEST_CASE(ScriptEngine_StackLimitStopsDeepRecursion) {
    ScriptEngine engine;
    const auto result = engine.Evaluate("function down(n) { return down(n + 1) + 1; }\ndown(0)");
    CHECK(!result.succeeded && StartsWith(result.text, "RangeError: Maximum call stack size exceeded"));
    const auto after = engine.Evaluate("'alive'");
    CHECK(after.succeeded && after.text == "alive");
}

// Promise 작업은 Evaluate가 돌아오기 전에 모두 실행되고, 작업 안의 처리되지 않은 예외는 실패 결과가 된다.
TEST_CASE(ScriptEngine_RunsPendingJobsBeforeReturning) {
    ScriptEngine engine;
    const auto scheduled = engine.Evaluate("globalThis.done = 0; Promise.resolve(5).then((v) => { globalThis.done = v; }); 'scheduled'");
    CHECK(scheduled.succeeded && scheduled.text == "scheduled");
    const auto done = engine.Evaluate("globalThis.done");
    CHECK(done.succeeded && done.text == "5");
    const auto failed = engine.Evaluate("queueMicrotask(() => { throw new Error('in job'); }); 'queued'");
    CHECK(!failed.succeeded && StartsWith(failed.text, "Error: in job"));
}

// 외부 장치 부재: 파일/OS/네트워크 객체가 없고, 모듈 적재기가 없어 import()로 파일을 읽을 수 없다.
TEST_CASE(ScriptEngine_ExposesNoHostAccess) {
    ScriptEngine engine;
    const auto globals = engine.Evaluate(
        "[typeof std, typeof os, typeof require, typeof process, typeof fetch, typeof XMLHttpRequest, typeof WebSocket, typeof scriptArgs, typeof print].join()");
    CHECK(globals.succeeded && globals.text == "undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined");
    CHECK(engine.Evaluate("globalThis.loaded = 'pending'; import('fs').then(() => { globalThis.loaded = 'loaded'; }, () => { globalThis.loaded = 'rejected'; }); 0").succeeded);
    const auto loaded = engine.Evaluate("globalThis.loaded");
    CHECK(loaded.succeeded && loaded.text == "rejected");
}

// 엔진을 여러 번 만들고 해제해도 문제가 없고, 사용 중인 힙 크기를 보고한다.
TEST_CASE(ScriptEngine_RepeatedCreateAndDestroy) {
    // 생성·평가·해제를 반복해 해제 순서와 자원 반납을 확인한다.
    for (int round = 0; round < 50; ++round) {
        ScriptEngine engine;
        CHECK(engine.MemoryUsedBytes() > 0);
        CHECK(engine.Evaluate("[...'abc'].reverse().join('')").text == "cba");
    }
}

// 버전 문자열은 탑재한 고정 릴리스(v0.18.0)다.
TEST_CASE(ScriptEngine_EngineVersion) {
    CHECK(ScriptEngine::EngineVersion() == "QuickJS-ng 0.18.0");
}

// 시작 자체 검사: 고정 항목 6개가 모두 통과하고 보고서 요약 줄이 PASS다.
TEST_CASE(ScriptEngine_SelfCheckPasses) {
    const auto report = netstorm::platform::RunScriptEngineSelfCheck();
    CHECK(report.passed);
    CHECK(report.failure.empty());
    CHECK(report.items.size() == 6);
    CHECK(report.engineVersion == "QuickJS-ng 0.18.0");
    const auto text = netstorm::platform::FormatScriptSelfCheckReport(report);
    CHECK(Contains(text, "QuickJS self-check PASS (6/6,"));
    CHECK(!Contains(text, "[FAIL]"));
}
