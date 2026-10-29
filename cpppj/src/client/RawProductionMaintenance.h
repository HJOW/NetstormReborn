// 원본43ef10의 다리 공급·품질/수량 타이머와 비용 전환을 같은 내용물에 적용한다.
#pragma once
#include "o/RawContainedFinder.h"
#include <functional>
#include <vector>
#include <optional>
#include <string_view>
namespace netstorm::client {
// 원본 전역의 수명은 개별 CombatGump와 미션보다 길다.
struct ProductionMaintenanceGlobals {
    std::vector<double> stockUntil; // 557d80: TYPE별 벽시각 종료값이며 최초 한 번0으로 초기화한다.
    std::uint32_t sequence{},stockEnabled{1},quickBridges{},forcedPattern{}; // 557dac/5318f4/54db78/54db7c이다.
    std::uint32_t bridgeType{82}; // 현재5411a0이며 실제 Session의 다리 타입과 공유한다.
    std::uint32_t rootSerial{},budgetSerial{},policyFirst{},policyByte{}; //557d7c/557d74/557d70/557d68의 실제 현재 생산 식별·정책 전역이다.
    std::uint32_t backgroundType{},backgroundFrame{},panelFrame{2}; // 같은 생성 부모가 사용하는5ca920/924/928이다.
    std::uint32_t chatAlertSerial{}; // 바로 다음005ca92c의 BSS0이며 수신 알림이 같은 등록 ID를 읽는다.
    std::int32_t stockLimit{1},bridgeDelay{1}; // 초기화 후54db74=1,531b2c=1이다.
    std::vector<std::uint32_t> testBridgePatterns; // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 비어 있으면 원본 조각 결정을 사용한다.
    std::uint64_t testBridgeDraws{}; // 테스트 전용 지정 순서의 실제 공급 횟수다. 패널 재그리기로 되감지 않는다.
    void ConfigureTestBridgePatterns(std::string_view text); // 테스트 전용: 0~25의 쉼표 목록을 반복한다. 빈 값/off는 해제하며 원본 분석 결과가 아니다.
    std::optional<std::uint32_t> NextTestBridgePattern(); // 테스트 전용: 실제 새 조각 생성 때만 다음 번호를 반환한다. 원본 분석 결과가 아니다.
};
// 패널 필드는 다시 그리기와 구별하며 실제 월드를 떠날 때 새로 생성한다.
struct ProductionMaintenancePanel {
    bool initialized{}; // 호스트의 재그리기와 실제 새 CombatGump 생성을 구별하는 수명 표시다.
    double bridgeUntil{}; // CombatGump+c8의 벽시각 공급 타이머다.
    std::int32_t oldMoney{},slots{}; // +d8의 절삭한 마지막SP와+e0의 다리 슬롯 한도다.
    std::uint32_t temple{}; // +dc의 이전 templateOwner 신전SID다.
};
// 콜백 시점의 같은 현재 전역/Player 저장소만 조회한다.
struct ProductionMaintenanceReads {
    std::function<bool()> active; // fort/battle이며 challenge가 아니어야 한다.
    std::function<std::uint32_t()> technology,templateTechnology,temple,editor,generatorTimer,stockRate; // 현재 기술/템플릿 기술/신전/편집/설정7DWORD/옵션BYTE2다.
    std::function<bool(o::Sid)> artificial; // 부모의 owner BYTE와 같은 Player+b0 AI 포인터다.
};
// 실제 Session과 같은 생성·삭제·조회 및 같은 UI의 재부착을 연결한다.
struct ProductionMaintenanceHooks {
    std::function<double()> wallSeconds,gameSeconds; // 460d70과 프레임55b4d0의 서로 다른 실제 시계다.
    std::function<float()> money; // PlayerKnown이면 로컬 분산SP, 아니면 원본 float0이다.
    std::function<void()> touchDeck; // 44f760은 공급 판정 전에 같은 현재Deck을 조회한다.
    std::function<std::uint32_t()> pattern; // 같은 게임 난수10000→4257c0의 실제 패턴이다.
    std::function<void(o::Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> create; // 4afa70의 실제 생성/부착 사슬이다.
    std::function<void(o::Sid)> destroy; // 같은 내용물의 가상+10(0) 삭제다.
    std::function<bool(std::uint32_t)> allowed; // 4c2400의 현재 종류 허용 여부다.
    std::function<std::int32_t(std::uint32_t)> cost; // 같은 원본 비용표의 해독한 signed 정수다.
    std::function<void(std::uint32_t)> refresh; // bit0=텔레포트,bit1=다리의 실제 Push→가상부착 순서다.
};
// 현재 풀과 기존 검색기를 직접 쓰며 별도 생산/재고 목록을 만들지 않는다.
class RawProductionMaintenance {
public:
    RawProductionMaintenance(o::SidPool& pool,std::size_t typeCount,const o::ContainedFinderState& finder,ProductionMaintenanceGlobals& globals,ProductionMaintenancePanel& panel,ProductionMaintenanceReads reads,ProductionMaintenanceHooks hooks); // 모든 참조는 이 처리기보다 오래 살아야 한다.
    void Run() const; // 공통 dirty/격자 부모 갱신 뒤 원본 공급·내용물/비용 갱신 단락을 한 번 실행한다.
private:
    o::Sid Parent(std::uint32_t value) const; // DWORD가 실제 풀 안에 있을 때만 WORD SID로 좁힌다.
    o::SidPool& pool_;std::size_t typeCount_;const o::ContainedFinderState& finder_; // 같은 현재 원문과 TYPE 수/검색 전역이다.
    ProductionMaintenanceGlobals& globals_;ProductionMaintenancePanel& panel_; // 각각 앱 전역과 현재 패널의 단일 상태다.
    ProductionMaintenanceReads reads_;ProductionMaintenanceHooks hooks_; // 실제 현재 조회와 선택된 효과다.
};
}
