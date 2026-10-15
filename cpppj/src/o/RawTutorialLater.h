// Early Missions3~6의 현재 단계 조건을 실제 게이트·장부·raw 객체에 연결한다.
#pragma once
#include "o/RawMissionStart.h"
#include "o/RawTotalMade.h"

namespace netstorm::o {
// 원본의 현재 mutable 타입/사건 전역을 따로 고정하거나 다른 미션의 사본으로 대체하지 않는다.
struct TutorialLaterState {
    std::span<std::uint8_t> battleOptions; // 0052f7e0의 같은 실제 옵션이며 BYTE2만 수정한다.
    const std::uint32_t& localOwner; // 00540c70의 현재 로컬 Player 번호다.
    std::function<std::uint32_t(std::uint32_t)> type; // 인자의 원본 전역 주소에서 현재 TYPE 번호를 읽는다.
    const std::uint32_t& rangeCommand; // 005429c4의 사거리 표시 사건 토큰이다.
    const std::uint32_t& placedCommand; // 005429b4의 실제 배치 완료 사건 토큰이다.
    const std::uint32_t& containedCommand; // 005429b8의 실제 내용물 통지 사건 토큰이다.
    const std::uint32_t& technologyCommand; // 005429c8의 실제 기술 이전 사건 토큰이다.
    const std::uint32_t& captureCommand; // 00542a50의 실제 포획 사건 토큰이다.
    const std::uint32_t& ascendCommand; // 00542a70의 실제 상승 사건 토큰이다.
};
// 게임/생산/UI의 큰 하위는 실제 현재 소비자를 원본 위치에서 호출한다.
struct TutorialLaterHooks {
    std::function<std::uint32_t(std::uint32_t)> findPriest; // 004921f0의 현재 살아 있는 사제 조회다.
    std::function<std::uint32_t(std::uint32_t)> graphFor; // 00462bc0의 현재 SID 연결 Graph 번호다.
    std::function<bool(std::uint32_t)> productionContains; // 0044f970의 같은 현재 로컬 생산 목록이다.
    std::function<void(std::uint32_t)> indicateTechnology; // 004929e0의 실제 기술 알림이다.
    std::function<std::int32_t(float,float)> regionFor; // 0046f530의 실제 지면 지역 번호다.
    std::function<std::uint32_t(float,float,std::uint32_t)> nearby; // 004b2070의 실제 주변 부류 검색이다.
    std::function<bool(std::uint32_t)> priestCaptured; // 00493c00의 실제 사제 포획 여부다.
    std::function<std::uint32_t()> selected; // 004d5430의 현재 선택 SID이며 중복 조회를 합치지 않는다.
    std::function<double()> gameSeconds; // 0055b4d0의 현재 double 게임 시각이다.
    std::function<void()> resetSelection,clearSelection; // 004d5c90(0,0,0) 뒤004d5c60의 서로 다른 실제 효과다.
    std::function<bool(std::string_view)> tell; // 00482de0의 실제 현재 미션 섹션 표시다.
    std::function<void()> closeTransientGumps; // 성공 Tell 뒤00466710의 실제 팝업 정리다.
};
// PATCH004c3f00/004c40f0/004c42b0/004c45e0의 전체 단계 분기다.
class RawTutorialLater {
public:
    // 같은 Pool/TYPE/Player9/제작 장부와 gate/start 상태를 빌린다. 참조 대상은 더 오래 살아야 한다.
    RawTutorialLater(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> players,
        RawTotalMade& made,RawMissionFrameGate& gate,MissionStartState& mission,TutorialLaterState state,TutorialLaterHooks hooks);
    // 실제 입력 DWORD3칸을 사용한다. event의 상위 modifier만 마스킹하고 SID/type 인자는 보존한다.
    std::uint32_t Handle(std::uint32_t tutorial,std::span<const std::uint32_t> event) const;
private:
    // 현재 raw SID의 원본 타입 BYTE/속성 DWORD를 읽는다.
    std::span<const std::uint8_t> Object(std::uint32_t sid) const;
    std::uint32_t Genus(std::uint32_t sid) const;
    // 같은 제작 장부의 signed DWORD 차이를 감김 포함 원본 부호로 판정한다.
    bool GlobalNotAboveLocal(std::uint32_t typeGlobal) const;
    // 두 실제 선택 효과와 성공 Tell의 정지 소유권을 원본 순서로 실행한다.
    void ClearSelection() const;
    void TellOnce(std::string_view name,std::uint32_t& seen) const;
    // 각 미션은 한 switch 분기만 실행하며 같은 호출 중 다음 단계로 넘어가지 않는다.
    void Three(std::span<const std::uint32_t> event) const;
    void Four(std::span<const std::uint32_t> event) const;
    void Five(std::span<const std::uint32_t> event) const;
    void Six(std::span<const std::uint32_t> event) const;
    const SidPool& pool_; // 현재 객체를 읽는 유일한 원본 슬롯 저장소다.
    std::span<const RiftTypeRecord> types_; // 같은 현재 TYPE+ec 부류 표다.
    std::span<const std::uint8_t> players_; // 실제9×180BYTE Player+78의 지역 조회다.
    RawTotalMade& made_; // postPop과 공유하는 실제 전역/첫 로컬 제작 표다.
    RawMissionFrameGate& gate_; // 단계/임시 타이머·정지/Advance의 단일 저장소다.
    MissionStartState& mission_; // Start가 초기화한 실제+c0/+c4 일회성 알림 필드다.
    TutorialLaterState state_; // 실제 타입·현재 owner·명령의 참조다.
    TutorialLaterHooks hooks_; // 생산/선택/지역/Graph/포획/안내의 실제 하위다.
};
}
