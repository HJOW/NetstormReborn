// Secret Workshop의 실제 A~H 조건을 공유 단계/타이머와 같은 제작 장부에 연결한다.
#pragma once
#include "o/RawMissionFrameGate.h"
#include "o/RawTotalMade.h"
#include <string_view>

namespace netstorm::o {
// 단계/타이머는 gate만 소유한다. 나머지는 현재 게임의 원본 전역을 참조한다.
struct TutorialTwoState {
    std::span<std::uint8_t> battleOptions; // 0052f7e0의 현재 BYTE 옵션 표다. A는2/4번 각각1바이트만 쓴다.
    std::uint32_t& denySalvage; // 00595078의 현재 회수 금지 DWORD다. H마다0으로 쓴다.
    std::uint32_t& workshopType; // 005411ac의 현재 썬 워크샾 타입 번호다.
    std::uint32_t& archerType; // 00541174의 현재 썬 디스크 발사기 타입 번호다.
    std::uint32_t& salvageCommand; // 0054286c의 실제 회수 완료 사건 토큰이다.
};
// 실제 옵션/입력/생산/Gump의 하위 경계다. 누락을 빈 성공으로 대체하지 않는다.
struct TutorialTwoHooks {
    std::function<double()> gameSeconds; // 0055b4d0의 현재 double 게임 시각이다.
    std::function<void()> refreshEnergyRange; // 004b4860의 현재 옵션 공급 반지름/제곱 갱신이다.
    std::function<std::uint32_t()> selected; // 004d5430의 현재 선택SID다. 원본의 중복 호출을 유지한다.
    std::function<bool(std::uint32_t)> productionContains; // 0044f970의 현재 로컬 생산 목록에 타입이 있는지 조회한다.
    std::function<void()> resetSelection; // 004d5c90(0,0,0)의 실제 선택 초기화다.
    std::function<void()> clearSelection; // 004d5c60의 실제 후속 선택 해제다. resetSelection 뒤 별도로 호출한다.
    std::function<bool(std::string_view)> tell; // 00482de0의 실제 섹션 Tell이며 창이 열렸으면 참이다.
    std::function<void()> closeTransientGumps; // 00466710의 현재 팝업 정리다. Tell 성공 뒤 시계 정지보다 먼저 호출한다.
};
// PATCH004c3bb0 전체를 복원한다. 원본 허용/제작 집계와 미션 정지는 기존 실제 모듈을 사용한다.
class RawTutorialTwo {
public:
    // 같은 Pool/TYPE/TotalMade/gate의 저장소를 사용한다. 현재 옵션은 최소5바이트가 필요하다.
    RawTutorialTwo(const SidPool& pool,std::span<const RiftTypeRecord> types,RawTotalMade& made,
        RawMissionFrameGate& gate,TutorialTwoState state,TutorialTwoHooks hooks);
    // A~H 한 분기만 실행하며 원본처럼 반환값은 항상0이다. I는 성공 섹션 자체가 처리한다.
    std::uint32_t Handle(std::uint32_t event) const;
private:
    // 두 번째 현재 선택 조회의 SID로 실제 TYPE+ec를 읽는다. 첫 조회가0이면 두 번째 조회를 생략한다.
    bool TempleSelected() const;
    // 원본 두 선택 정리 함수를 같은 순서로 호출한다. Advance 실패여도 이 효과는 유지한다.
    void ClearSelection() const;
    const SidPool& pool_; // 현재 선택SID와 실제 타입 BYTE를 읽는 유일한 raw 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 genus(flags2) 원문 표이며 별도 선택 분류를 캐시하지 않는다.
    RawTotalMade& made_; // postPop이 갱신하는 같은 허용/두 번째 로컬 누적 제작 집계다.
    RawMissionFrameGate& gate_; // 같은 stage/timer와 실제 정지/잠금/예약 처리기다.
    TutorialTwoState state_; // 현재 옵션/금지/타입/회수명령의 참조다.
    TutorialTwoHooks hooks_; // 실제 UI/생산/옵션의 명시 외부 하위다.
};
}
