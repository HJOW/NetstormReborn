// 원본 HUD divider의 생성·슬레이브 재배치·지도 영역 갱신을 같은 Gump 저장소에 연결한다.
#pragma once
#include "client/GumpBackgroundCreate.h"
#include <utility>

namespace netstorm::client {
// 원본 0x11cBYTE divider의 +b8/+bc~c8/+cc~118 필드다.
struct HudDividerNode : GumpBackgroundNode {
    std::uint32_t axis{}; // +b8: 크기 연동·드래그 제한·커서 모양에 쓰는 원시 플래그다.
    std::int32_t minX{},maxX{},minY{},maxY{}; // +bc/+c0/+c4/+c8의 signed 화면 제한이다.
    std::array<std::pair<std::uint32_t,std::uint32_t>,10> slaves{}; // 현재 serial과 관계 플래그의 고정10개 슬롯이다.
};
// 원본 HUD의 현재 직계 자식 토큰이다. 콜백 뒤 각 필드를 다시 읽는다.
struct HudDockNodes {
    std::uint32_t root{}; // HUD 본체 토큰이며 채팅 크기/표시 부착의 명시 부모다.
    std::uint32_t vertical{},minimapDivider{},chatDivider{}; // +a0/+a4/+a8의 세 divider다.
    std::uint32_t minimap{},production{},chat{},knowledge{}; // +ac/+b0/+b8/+c0의 실제 UI다.
};
// 복제하지 않고 실제 화면/지도 전역에 연결하는 참조 묶음이다.
struct HudDockGlobals {
    const std::int32_t& screenWidth; //005c78f0의 현재 signed 화면 너비다.
    const std::int32_t& screenHeight; //005c78f4의 현재 signed 화면 높이다.
    const std::uint32_t& showHud; //0054db14: mode1의 좌측 덱 표시 관문이다.
    std::int32_t& mapLeft; //005ca99c의 현재 지도 원점X다.
    std::int32_t& mapTop; //005ca9a0의 현재 지도 원점Y다.
    std::int32_t& mapWidth; //005ca9b4의 현재 지도 폭이다.
    std::int32_t& mapHeight; //005ca9b8의 현재 지도 높이다.
    std::array<float,4>& mapFloatRect; //005ca9bc~c8의 현재 지도LTRB다.
    ScreenRect& mapRect; //005ca9cc~d8의 현재 정수 지도LTRB다.
    ScreenRect& leftBottomRect; //005ca9a4~b0의 좌측 하단 제외 영역이다.
    std::uint32_t& fullRefresh; //005949d4의 화면 전체 갱신 카운터다.
    float& savedChatY; //00542674의 숨기기 전 chatdivider 위치다.
};
// 실제 Gump 가상 호출/트리와 현재 전역을 공급하는 명시 경계다.
struct HudDockHooks {
    std::function<GumpCreateNode*(std::uint32_t)> node; // 같은 기본/파생 노드를 읽는다.
    std::function<HudDividerNode*(std::uint32_t)> divider; // 같은 divider의 확장 필드를 읽는다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t,std::uint32_t)> constructBackground; //004655d0의 flags=1인 생성이다.
    std::function<void(std::uint32_t,std::uint32_t)> activate; //004653c0의 활성 등록이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> baseAttach; //00466750의 기본 부착이다.
    std::function<void(std::uint32_t)> push; // 가상+18의 실제 트리 분리다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 가상+20의 파생 부착이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t)> move; // 가상+10의 공통 Move다.
    std::function<std::uint32_t(std::uint32_t)> resolveSerial; //00464020의 현재 serial 조회다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> bubble; //00464e90의 부모 사건 전달이다.
    std::function<void()> closePopups; //00466710의 전역 popup 정리다.
    std::function<void(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::uint32_t)> invalidate; //00499370의 현재 dirty 사각형이다.
    std::function<void()> finishResize; //004973c0의 화면/카메라 후처리다.
    std::function<void(std::uint32_t)> assertion; // RegisterSlave의 원본 line83 진단이다. 정상 복귀 시 계속한다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle; //00464990의 HUD 기본 입력이다.
    std::function<void(bool)> chatVisible; //0042f250의 실제 채팅 표시 설정이다.
    std::function<void(bool)> chatInput; //0042f270의 실제 채팅 입력 표시다.
};
// 상태와 노드는 처리기 및 모든 콜백보다 오래 살아 있어야 한다.
class HudDock {
public:
    // 색은 원본00540452의 signed BYTE다. 토큰 조회 및 일반 재배치 경계는 필수다.
    HudDock(HudDockNodes& nodes,HudDockGlobals globals,const std::int8_t& color,HudDockHooks hooks);
    //004c6c50: 배경 생성 뒤 divider 필드·고정10슬롯을 초기화한다.
    std::uint32_t Construct(std::uint32_t token,std::uint32_t axis,std::int32_t width,std::int32_t height,
        std::uint32_t backgroundFlags,std::uint32_t type,std::uint32_t frame) const;
    //004c4790: 첫 빈 serial슬롯에 등록한다. 중복은 허용하며 가득 차도 child의40000비트를 쓴다.
    void RegisterSlave(std::uint32_t master,std::uint32_t slave,std::uint32_t flags) const;
    //004c47f0: 실제 기본 부착 뒤 현재 관계 플래그의1/2/4/8 순서로 재배치한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    //004c6530: HUDmode1/3/4·채팅 표시 및 divider 이동의 viewport 갱신이다.
    std::uint32_t Handle(std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    //004c7160의 실제 자식 생성 후 등록10회와 세 divider의 제한만 설정한다.
    void RegisterNormalLayout() const;
private:
    // 토큰은 같은 저장소의 유효한 현재 기본 노드를 가리켜야 한다.
    GumpCreateNode& Node(std::uint32_t token) const;
    // 확장 노드와 기본 노드의 주소가 동일한지 검사한다.
    HudDividerNode& Divider(std::uint32_t token) const;
    HudDockNodes& nodes_; // HUD의 현재 자식 포인터를 보유하는 실제 상태다.
    HudDockGlobals globals_; // 실제 전역을 가리키는 참조만 보관한다.
    const std::int8_t& color_; // 런타임 색 패치의 현재 값을 생성에서 읽는다.
    HudDockHooks hooks_; // 실제 공통 Gump와 가상 처리를 연결한다.
};
}
