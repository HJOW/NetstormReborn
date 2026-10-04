// 단위 테스트용 작은 도구 (외부 프레임워크 없음)
//
// 사용법:
//   TEST_CASE(이름) { CHECK(조건); CHECK_NEAR(값, 기대값, 허용오차); }
// TestMain.cpp 가 등록된 테스트를 모두 실행하고, 실패한 검사가 있으면 종료 코드 1 을 돌려준다.
#pragma once

#include <cmath>
#include <cstdio>
#include <vector>

namespace netstorm::test {

// 테스트 함수의 형식
using TestFunction = void (*)();

// 등록된 테스트 하나 (이름과 함수)
struct TestCase {
    const char* name;
    TestFunction function;
};

// 등록된 테스트 목록. 정적 초기화 순서와 무관하게 쓰도록 함수 안의 정적 변수로 둔다.
inline std::vector<TestCase>& Registry() {
    static std::vector<TestCase> cases;
    return cases;
}

// 실패한 검사의 수
inline int& FailureCount() {
    static int failures = 0;
    return failures;
}

// 테스트를 목록에 넣는다. TEST_CASE 매크로가 정적 초기화 때 부른다.
inline bool Register(const char* name, TestFunction function) {
    Registry().push_back(TestCase{name, function});
    return true;
}

// 검사 실패를 기록하고 위치를 표준 오류에 쓴다.
inline void ReportFailure(const char* file, int line, const char* expression) {
    ++FailureCount();
    std::fprintf(stderr, "%s(%d): CHECK failed: %s\n", file, line, expression);
}

}  // namespace netstorm::test

// 테스트 함수를 정의하고 등록한다.
#define TEST_CASE(name)                                                                 \
    static void name();                                                                 \
    static const bool name##_registered = ::netstorm::test::Register(#name, &name);    \
    static void name()

// 조건이 참인지 검사한다. 거짓이어도 테스트는 계속 실행한다.
#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            ::netstorm::test::ReportFailure(__FILE__, __LINE__, #condition); \
        }                                                                   \
    } while (false)

// 실수 값이 기대값에서 허용오차 안에 있는지 검사한다.
#define CHECK_NEAR(actual, expected, tolerance) \
    CHECK(std::fabs((actual) - (expected)) <= (tolerance))
