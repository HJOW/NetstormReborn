// 생산 내용물 항목의 실제 SHP 자식 배치·수량·클릭·장식·비용 판정을 복원한다.
#pragma once
#include "client/InventoryGump.h"
#include "client/PriestPlacementAssets.h"
#include "o/RawCanonPixelShape.h"
#include "o/CanonTypeDecoder.h"

namespace netstorm::client {
// TYPE+11c는 gumpFrame이다. +118 defaultFrame/+128 baseFrame과 구별한다.
struct InventoryItemType {
    std::int32_t gumpFrame{-1}; // 비-1이면 양/패턴 인자 대신 명시 프레임으로 쓴다.
    const o::RiftTypeFrames* frames{}; // 현재 실제4바이트 프레임 코드와 FindFlags다.
};
// 실제 decoder가 반환한 한 유효 칸이다.
struct InventoryItemCell { std::int32_t frame{};float x{},y{}; }; // 현재 프레임/단정도 좌표다.
// 실제 원본 전역을 같은 안정 저장소에서 읽고 lastType만 갱신한다.
struct InventoryItemState {
    const std::uint32_t& localOwner;const bool& editor; // SP 조회 owner와 에디터 원문이다.
    const std::uint32_t& stockEnabled;const std::uint32_t& inputDisabled; // 5318f4 수량과55b4b0 입력 관문이다.
    std::uint32_t& lastType;const std::uint32_t& bridgeType; // 557d6c 마지막 선택과5411a0 실제 다리다.
    const float& pixelsX;const float& pixelsY; // 59a92c/30 현재16/11 픽셀 단위다.
    const std::uint32_t& backgroundType;const std::uint32_t& backgroundFrame; // 장식의 실제 현재 질감이다.
};
// 같은 실제 격자/그림/TextGump/수명/현재 상태에 연결하는 명시 외부 경계다.
struct InventoryItemHooks {
    std::function<InventoryItemNode*(std::uint32_t)> node; // 기존 항목의 같은 파생 주소다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 그림·텍스트·자식의 실제 공통 주소다.
    std::function<InventoryItemType(std::uint32_t)> type; // 현재 gumpFrame/프레임 코드다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> baseAttach; // 같은 기본 GumpTree 부착이다.
    std::function<void(std::uint32_t)> clearChildren;std::function<void()> cleanup; // 원본 자식 Kill/지연 삭제 큐다.
    std::function<bool(std::uint32_t,bool)> robust; // 실제 SID 검사의 allowDead=1이다.
    std::function<void(std::string_view)> assertion; // 원본 진단 뒤에도 정상 몸체를 계속 실행한다.
    std::function<o::PriestPlacementPixelShape(std::uint32_t,std::uint32_t,bool)> measure; // 같은43cbd0 전체 픽셀 경계다.
    std::function<void(o::CanonTypeQuery,const std::function<void(InventoryItemCell)>&)> walk; // 같은425c20/425860 순회다.
    std::function<std::uint32_t(std::size_t)> allocate; // 같은 등록표의 실제0xb4그림/0xa4텍스트 저장소다.
    std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::uint32_t)> image; // 실제419fc0 생성자다.
    std::function<void(std::uint32_t)> text; // 실제4bb170(0,0,0,0)이다.
    std::function<void(std::uint32_t,std::string_view)> styledText; // 실제4bc800의 같은 StyleText다.
    std::function<void(std::uint32_t)> fitText; // 실제TextGump::Fit(0,0)이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 실제 자식 가상+20이다.
    std::function<std::int32_t(std::uint32_t)> costUnits; // 기존SquidReward::TypeCostUnits의 실제 비용표 복호 값이다.
    std::function<float(std::uint32_t)> money; // 실제40ec50의 현재 SP다.
    std::function<ScreenPoint(std::uint32_t)> position; // 같은 GumpCollection의 현재 화면점이다.
    std::function<void(ScreenRect,std::uint32_t,std::uint32_t,std::uint32_t)> background; // 실제465690 질감 장식이다.
    std::function<std::array<float,2>(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> local; // 실제 사건→로컬 좌표다.
    std::function<std::uint32_t()> knowledgeOpen; // 실제445330이며 우클릭 전달 뒤 다시 읽는다.
    std::function<std::uint32_t(std::uint32_t,float,float)> imageHit; // 실제ImageGump::Hit이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> denied; // 실제4c2790(1,0x41) 정책 질의다.
    std::function<void(std::string_view)> tell; // 실제NoBridgeYet 알림이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> forward; // 현재 부모 체인의464e90이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> about; // 실제43d330 메뉴다. 빈 성공으로 대체하지 않는다.
};
// 기존 항목/내용물/자산을 사용한다. 별도 Deck 목록이나 버튼을 만들지 않는다.
class InventoryItemGump {
public:
    // 모든 현재 상태/노드·소비자는 각 호출과 처리기보다 오래 살아야 한다.
    InventoryItemGump(const o::SidPool& pool,InventoryItemState state,InventoryItemHooks hooks);
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 실제43ce40 전체다.
    void Draw(std::uint32_t token) const; // 실제43cdc0 장식만 그리며 아이콘은 자식이 그린다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; // 실제43d4c0 전체다.
    std::uint32_t Affordable(o::Sid content) const; // 실제4af010의 비용float→현재SP→에디터 판정이다.
private:
    InventoryItemNode& Node(std::uint32_t token) const; // 같은 파생 저장소만 공급한다.
    GumpCreateNode& Base(std::uint32_t token) const; // 같은 실제 그림/TextGump 저장소다.
    o::Sid Content(std::uint32_t sid) const; // 원문DWORD를 확인한 뒤에만 물리SID로 좁힌다.
    const o::SidPool& pool_;InventoryItemState state_;InventoryItemHooks hooks_; // 단일 내용물 저장소와 현재 전역·효과다.
};
// 기존 전체 픽셀 측정기/실제 자산의 decoder와 TYPE+11c를 연결한다. 객체/SP/GUI 효과는 보존한다.
void BindInventoryItemAssets(InventoryItemHooks& hooks,const GameAssets& assets,const PriestPlacementAssets& geometry,const o::RawCanonPixelShape& measure);
}
