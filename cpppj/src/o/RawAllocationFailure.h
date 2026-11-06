// 원문 PATCH00435f50의 메모리 실패 안내/현재 게임 정리/종료 순서를 플랫폼 효과와 분리한다.
#pragma once
#include <cstdint>

namespace netstorm::o { // 원본 포인터를 호스트 주소로 해석하지 않고 현재 전역과 명시 장치만 빌린다.

//004b3490은 메시지에서 돌아온 뒤 현재 초기화/세 모드/재진입 방지 DWORD를 직접 읽는다.
struct AllocationFailureState {
    const std::uint32_t& initialized; //0054dbb8의 현재 초기화 완료 DWORD다.
    const bool& fort; //00594fb8의 현재 편집 모드다.
    const bool& battle; //00594fbc의 현재 전투 모드다.
    const bool& metamap; //00594fc8의 현재 메타맵 모드다.
    std::uint32_t& exiting; //005c890c이며 전체 정리 호출 전에1을 써 중복 진입을 막는다.
};

// 실패 중 함수 포장/문자열을 새로 할당하지 않는 고정 함수 포인터 경계다.
struct AllocationFailureHooks {
    void* context{}; // 모든 동기 장치가 공유하는 호출자이며 공유본문보다 오래 살아야 한다.
    void (*message)(void*,const char*,const char*,std::uint32_t){}; // NULL owner의 고정 메시지/제목/0x10을 실제 OS 또는 명시 관찰기로 전달한다.
    void (*cleanup)(void*,std::uint32_t){}; // 기존004b2df0 전체 부모를 인자0으로 호출하며 새 정리 알고리즘을 만들지 않는다.
    std::uintptr_t (*currentEvent)(void*){}; // 정리 뒤 현재0054dbcc를 읽는다. 실제 구 HTTP 서버가 없는 호스트는0이다.
    void (*signal)(void*,std::uintptr_t){}; // 선택된 비NULL 이벤트에만0046d140/SetEvent를 적용한다.
    void (*stopMusic)(void*){}; //004aad70의 현재 준비된 기본 음악 채널 정지만 실행한다.
    void (*terminate)(void*,std::int32_t){}; // 마지막 원문_exit(0)이며 검사는 동기 종료 요청을 관찰한다.
};

bool CleanupRawGameForExit(AllocationFailureState state,const AllocationFailureHooks& hooks); //004b3490의 현재 관문/guard1/기존 전체 정리 호출을 공유한다. 정리가 선택되면true다.
void HandleRawAllocationFailure(AllocationFailureState state,const AllocationFailureHooks& hooks); //435f50의 메시지→정리→현재event/신호→빈435060→음악정지→종료0 순서를 실행한다.
}
