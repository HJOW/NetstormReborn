// 0044dab0의 가상 진단·모든 Form 재귀와 현재 들여쓰기 전역을 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 객체 trace는 월드의 같은 물리 풀을 읽고 출력 장치만 빌린다.
// 원문 DWORD 검사와 제품의 정상0/1 bool 소유자를 같은 현재 비영 관문으로 빌린다.
class ObjectTraceFlag {
public:
    ObjectTraceFlag(const std::uint32_t& value):word_(&value) {} // 원문 비정규 DWORD도 매번 비영으로 판정한다.
    ObjectTraceFlag(const bool& value):boolean_(&value) {} // 실제 Client의 bool 주소를 빌리며 DWORD 지역 복사본을 만들지 않는다.
    operator bool() const { return word_ ? *word_!=0 : *boolean_; } // 재귀/외부 출력 뒤 바뀐 현재값을 다시 읽는다.
private:
    const std::uint32_t* word_{};const bool* boolean_{}; // 두 원래 소유자 중 생성자가 선택한 하나의 주소만 사용한다.
};
// 원문 소비 시점에 같은 현재 값을 읽는 진단/권한/물리 슬롯 전역의 뷰다.
struct ObjectTraceGlobals {
    std::uint32_t& depth; // 0055a4a0의 현재 재귀 깊이이며 DWORD 덧셈/뺄셈을 보존한다.
    const std::uint32_t& count; // 005c847c의 현재 논리 끝 번호이며 슬롯 자체는 물리 풀로 검사한다.
    ObjectTraceFlag checkingDead; // 005e4794: 같은 현재 진단 관문이며 지역 정수 사본을 쓰지 않는다.
    ObjectTraceFlag metamap; // 00594fc8: 실제 현재 메타맵 관문이다.
    const bool& boss; // 00540bc4의 같은 실제 권한 값이며 false일 때 dead 진단을 요청한다.
};
// 가상 +0x70과 로그 setter/출력/assert 경계를 실제 호출 위치에 제공한다.
struct ObjectTraceHooks {
    std::function<void(Sid)> trace; // 객체의 현재 가상 +0x70이며 반환 뒤 현재 head를 다시 읽는다.
    std::function<void(std::uint32_t)> indent; // 004c2600이 받는 현재 깊이를 같은 로그 들여쓰기에 쓴다.
    std::function<void(std::int32_t,std::string_view)> log; // 004c2630의 level29/원래 서식 바이트다.
    std::function<void()> deadParent; // 004b22d0의 원본 dead 부모 assert를 보고하고 돌아오는 경계다.
};
// 0이면 아무 장치도 부르지 않는다. free 루트는 한 줄만 기록하고 나머지는 현재 Form을 재귀 순회한다.
// TYPE<=69 후보의 free/dead/void 상태를 추가 필터링하지 않으며 외부 효과 전 next를 보존한다.
void TraceRawObject(const SidPool& pool,const ObjectTraceGlobals& globals,const ObjectTraceHooks& hooks,std::uint32_t sid);
}
