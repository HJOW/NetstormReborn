// Tutorial 소멸의 직접 전역 쓰기와 실제 AI/기본 Mission 반납 순서를 공유한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::o { // 10.78 원문 Tutorial 소멸의 장치 독립적인 꼬리를 제공한다.
struct TutorialMissionReleaseState { // 각 필드는 사본이 아닌 현재 원본 대응 DWORD를 빌리며 전체 해제 호출 동안 유효해야 한다.
    std::int32_t& denySalvage; // 00595078의 현재 회수 금지 값이며 허용 표 갱신 뒤 0으로 쓴다.
    std::int32_t& denyAscend; // 0059507c의 현재 승급 금지 값이며 회수 금지 다음에 0으로 쓴다.
    std::uint32_t& missionNumber; // 005ca8e0의 현재 미션 번호를 그대로 빌린다.
    std::uint32_t& tutorialNumber; // 005ca8e4의 현재 Early 분배 번호이며 미션 번호 다음에 지운다.
    std::int32_t& stage; // 005ca8e8의 현재 signed 단계 DWORD다. 낮은 BYTE만 지우지 않는다.
    std::uint32_t& stageDirty; // 005ca8ec의 현재 단계 변경 표식이며 마지막 직접 0쓰기다.
};
struct TutorialMissionReleaseHooks { // 원문이 호출하는 세 큰 하위 효과만 실제 소유자에 연결한다.
    std::function<void(std::uint32_t)> setAllowAll; // 004c23c0의 실제 허용 표 갱신이며 고정 인자 1을 받는다.
    std::function<void()> removeAllBrains; // 00413b20의 현재 owner1..8 AI destructor/free/Player+b0=0이다.
    std::function<void()> releaseBaseMission; // 00483210의 실제 기본 Mission 해제다. 본문 예외 정리에서도 한 번 수행한다.
};
// 허용 표→여섯 DWORD 0→전체 AI→기본 Mission 순서다. 본문 실패의 부분 쓰기를 보존하고 기본 Mission만 정리한다.
// 원문 004c3210 및 unwind 004fb940 대응. 시계·UI·World·음악의 수명은 이 소멸에 추가하지 않는다.
// 근거와 범위: docs/exe/cpp-guide-release-20261101.md. 원본 32비트 SEH 객체 ABI를 이식한 함수는 아니다.
void ReleaseTutorialMission(TutorialMissionReleaseState state,const TutorialMissionReleaseHooks& hooks);
}
