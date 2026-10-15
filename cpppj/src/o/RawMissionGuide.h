// PATCH Guide의 현재 미션 포인터·pending/실제 창 관문·단계 반복을 원본 순서로 복원한다.
#pragma once
#include "o/RawMissionStart.h"

namespace netstorm::o {
// Guide가 소유한 현재 미션과 실제 상태/UI/가상 단계의 외부 경계다. 현재 객체를 복제하지 않는다.
struct MissionGuideHooks {
    std::function<MissionStartState*()> current; // Guide+800의 현재 미션을 각 원본 위치에서 읽는다. NULL은 미션 없음이다.
    std::function<std::uint32_t()> pending; // 004b32e0의 현재 pending State이며0일 때만 창 관문을 읽는다.
    std::function<bool()> dialogsClear; // 004c8e90의 현재 실제 다이얼로그 부재 판정이다.
    std::function<void(MissionStartState&,std::uint32_t)> stage; // 현재 객체의 가상+4..1c를0..6 순서 번호로 호출한다.
    std::function<void(MissionStartState&)> postpone; // 차단된 현재 객체의 가상+20 재지연이다.
    std::function<void()> release; // 00463ab0의 실제 현재 미션 소멸/Guide 이름·포인터 정리다.
    std::function<void()> invalidStage; // 범위 밖 단계의 실제 원본 진단이며 진단 후 현재 상태를 다시 읽는다.
};
// Guide Run은 상태 변화가 있으면 같은 화면 호출 안에서 다음 단계를 다시 방문한다.
class RawMissionGuide {
public:
    // PATCH 원문과 모든 실제 경계가 필요하다. 콜백 대상은 처리기보다 오래 살아야 한다.
    RawMissionGuide(OriginalEdition edition,MissionGuideHooks hooks);
    // 00463bb0: 취소→pending/창→advance→가상 단계/재지연→현재 포인터/단계 재조회 순서다.
    void Run() const;
    // 00482c80: 실제 같은 미션의 DWORD 단계 증가와 flags bit1 해제를 수행한다.
    static void Advance(MissionStartState& state);
private:
    // 원본이 현재 미션을 역참조하는 위치에서만 NULL의 손상 범위를 명시적으로 거부한다.
    MissionStartState& RequireCurrent() const;
    MissionGuideHooks hooks_; // 실제 현재 포인터/상태/UI·가상 몸체를 읽고 호출한다.
};
}
