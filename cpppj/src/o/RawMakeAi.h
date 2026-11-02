// 일반 MakeAi의 기본 설정과 owner0 이전/삭제 꼬리를 미션 시작 override와 구별한다.
#pragma once
#include "o/RawMissionAiSettings.h"
#include <functional>
#include <span>

namespace netstorm::o { // 현재 Player/World를 읽는 지연 경계만 빌리고 별도 AI를 소유하지 않는다.
struct MakeAiReads { // 효과 뒤 원문이 다시 읽는 현재 전역이다.
    std::function<std::uint32_t()> battle,missionPost,playerKnown,localOwner; // 594fbc/594fa4/595344/540c70이다.
    std::function<std::uint32_t(std::uint32_t)> brain; // 현재 Player+b0이며 owner0 wrapper가 계산한 local+1을 검사한다.
};
struct MakeAiHooks { // 선택한 실제 분기에서만 필요한 부모다.
    std::function<void(std::span<std::uint8_t>)> initialize; // 40ef80만 실행하며 공통/owner mission override를 적용하지 않는다.
    std::function<float(std::uint32_t)> stormPower; // 40ec50의 현재 분산 SP를 float로 읽는다.
    std::function<std::uint32_t(std::uint32_t,std::span<const std::uint8_t>)> create; // 기존 두 인자417e50 전체 부모다.
    std::function<void(std::uint32_t)> transferObjects; // 4c27c0의 비로컬 섬 객체를 새 owner로 이전한다.
    std::function<void(std::uint32_t)> clearObjects; // 414c80의 flags2 마스크와 가상 삭제0이다.
};
void MakeAi(std::uint32_t owner,std::span<std::uint8_t> settings,const MakeAiReads& reads,const MakeAiHooks& hooks); //600BYTE 미작성 부분은 caller 입력을 유지한다.

struct MakeAiObjectHooks { // 원문 전체 풀 순회의 같은 현재 물리와 가상 효과를 빌린다.
    explicit MakeAiObjectHooks(std::uint32_t& depth):loadingDepth(depth) {} // 같은 실제 깊이의 필수 참조만 생성 때 고정한다.
    std::function<std::uint32_t()> capacity,localOwner,playerType,bridgeType; // 각 원문 위치에서 현재 상한/TYPE/owner를 다시 읽는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> slot; // free도 읽을 수 있는 현재50BYTE 슬롯이다.
    std::function<std::uint32_t(std::uint32_t)> flags; // 4ac200의 현재 TYPE+ec를 조회한다.
    std::function<std::span<std::uint8_t>()> players; // 같은 현재1620BYTE Player9 물리다.
    std::function<std::uint32_t(float,float)> region; // 46f530의 현재 섬 번호다.
    std::function<void(std::uint32_t,std::uint32_t)> setOwner,destroy; // 가상+74(owner)/+10(flags)를 실제 World에 전달한다.
    std::function<void(std::uint32_t)> decrementLocal,decrementBuilt; // 4c25c0/25d0의 같은 두 현재 카운터 표다.
    std::uint32_t& loadingDepth; // 5c89b8의 동일한 중첩 깊이이며 예외 때도 원문 부분 효과를 보존한다.
    std::function<void(std::uint32_t)> loadingDepthChanged; // host의 사제/자산 소비자에 같은 현재 깊이를 전달하는 연결이며 독립 raw 물리에는 불필요하다.
};
void TransferMakeAiObjects(std::uint32_t owner,const MakeAiObjectHooks& hooks); // local/중립·다른섬·두 TYPE 제외와 소유자/계수/첫region을 복원한다.
void ClearMakeAiObjects(std::uint32_t mask,const MakeAiObjectHooks& hooks); // 원문 iterator가 다음 후보를 먼저 고른 뒤 현재flags/가상삭제0을 실행한다.
}
