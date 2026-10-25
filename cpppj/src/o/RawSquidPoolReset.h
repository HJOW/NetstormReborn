// 월드 Pool 정리 부모004af170/CD004aaa70을 기존 실제 SID 저장소와 실패점에 연결한다.
#pragma once
#include <cstdint>

namespace netstorm::o { //하위 SID Reset과 부모의 추가 기록·좌표 효과를 구별한다.
class SidPool;struct RawPathPoint; //두 대상은 호출 전체 동안 같은 실제 저장소여야 한다.
//이미 하위 Reset이 끝난 시작 생성자가 재실행 없이 같은 부모 꼬리만 잇는다. 기록80DWORD→실패Y→X다.
void FinishSquidPoolReset(SidPool& pool,RawPathPoint& invalid);
//실제 하위 Reset(capacity) 정상 반환 뒤에만 같은 기록·실패점을 정리한다. 선행 UI/Kernel/Runtime 반납은 호출자 책임이다.
void ResetSquidWorldPool(SidPool& pool,RawPathPoint& invalid,std::int32_t capacity=0);
}
