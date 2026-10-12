// 원본 MenuGump의 크기/확장 프레임과 controlled 메뉴 스택을 같은 Gump 저장소에 연결한다.
#pragma once
#include "client/GumpBackgroundCreate.h"
#include "client/GumpFrame.h"

namespace netstorm::client {
// 원본 메뉴 항목 한도와 controlled 메뉴 표 길이다. 스택 0번은 예약 칸이다.
inline constexpr std::size_t kMenuItemCapacity=40,kControlledMenuCapacity=20;
// 원본 항목 0x3c바이트다. 40바이트 라벨 뒤 +28 자식 등록 ID/+2c 명령/+30 인자/+34 StyleText 토큰/+38 flags다.
struct MenuGumpItem {
    std::array<char,40> label{};
    std::uint32_t childId{},command{},parameter{},styled{},flags{};
};
// 기본 배경 필드를 공유하며 메뉴 크기/프레임/항목 레이아웃이 읽는 파생 필드를 보관한다.
// offsets는 +17c, itemWidth/targetWidth/targetHeight는 +170/+174/+178이다.
struct MenuGumpNode : GumpBackgroundNode {
    std::uint32_t expanding{},menuFlags{},itemCount{},fancy{},title{};
    float itemWidth{},targetWidth{},targetHeight{};
    std::array<float,kMenuItemCapacity> offsets{};
    std::array<float,4> margins{}; // +bc/+c0은 폭 여백, +c4/+c8은 높이 여백이다.
    std::array<float,kMenuItemCapacity> heights{}; // +d0: 현재 측정하거나 가로 메뉴에서 맞춘 항목 높이다.
    std::array<MenuGumpItem,kMenuItemCapacity> items{}; // +228: 40개 원본 항목의 저장소다.
    // +cc 현재 선택/+21c 선택 소멸 통지 토큰이다. 실제 호스트 함수 호출은 생성/수명 처리기가 분배한다.
    std::int32_t selection{-1};
    std::uint32_t selectionCallback{};
};
// 원본 메뉴 표와 signed index의 DWORD 비트다. 생성/항목 작성은 별도 단계에서 이 표를 채운다.
struct MenuGumpState {
    std::array<std::uint32_t,kControlledMenuCapacity> entries{};
    std::uint32_t index{};
    // 10.78 00593024/CD 0051fc7c: 정상 메뉴 생성/소멸에서 DWORD로 증가/감소하는 현재 개수다.
    std::uint32_t instances{};
};
// 가상 Push/Pop/Attach/Redraw와 기본 Gump 재그리기 요청의 명시 경계다.
// 콜백은 현재 메뉴 필드/표/루트를 바꿀 수 있다. node/baseNode는 같은 안정된 저장소를 가리켜야 한다.
struct MenuGumpHooks {
    std::function<MenuGumpNode*(std::uint32_t)> node;
    std::function<GumpCreateNode*(std::uint32_t)> baseNode;
    std::function<void(std::uint32_t)> push,pop;
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    std::function<void(std::uint32_t,std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attachObject;
    std::function<void(std::uint32_t,bool)> redraw,baseRedraw;
};
// 원본 메뉴의 정상 프레임/스택 몸체다. 항목 레이아웃은 MenuGumpLayout이 담당하며 생성·입력 처리는 후속이다.
class MenuGump {
public:
    // 같은 생성/수명/메뉴 상태와 안정된 노드 저장소가 처리기보다 오래 살아야 한다.
    MenuGump(GumpCreateState& create,MenuGumpState& state,const GumpLifetime& lifetime,MenuGumpHooks hooks);
    // 00475f20/00475fb0: 부모 크기를 쓰고 fancy이면 현재 장식 Push→크기 재계산→Attach 순서다.
    void SetWidth(std::uint32_t token,float width) const;
    void SetHeight(std::uint32_t token,float height) const;
    // 00476050: 현재 fancy 자식에 먼저 요청하고 기본 Gump의 재그리기 요청을 호출한다.
    void Redraw(std::uint32_t token,bool erase) const;
    // 00476630: 가로 메뉴는 index×항목 폭/현재 높이, 세로 메뉴는 현재 폭/저장 offset을 반환한다.
    std::array<float,2> ItemPosition(std::uint32_t token,std::int32_t index) const;
    // 004767d0: 방향에 따라 한 축을 0으로 시작하고 실제 idle 표에 등록한 뒤 expanding을 세운다.
    void StartExpand(std::uint32_t token,std::uint32_t flags) const;
    // 00476700: 현재 목표보다 작으면 가상 Push→높이→폭→Pop, 다음 완료 호출에서 expanding만 지운다.
    void Idle(std::uint32_t token) const;
    // 00475c40/00475c50: 현재 표 토큰 및 현재 메뉴의 DWORD 항목 수를 읽는다.
    std::uint32_t Current() const;
    std::uint32_t ItemCount() const;
    // 00475c00: 현재 토큰을 꺼내고 칸을 지우며 판본별 index 쓰기 순서를 유지한다.
    // 예약 칸 꺼내기/표 범위 밖 접근은 원본 assert/무효 포인터 계약 밖이므로 변경 전에 거부한다.
    std::uint32_t TakeCurrent() const;
    // 00475ca0: 현재 메뉴의 가상 Attach 뒤 다시 읽은 현재 칸을 지우고 index를 내린다.
    void AttachCurrent(std::int32_t x,std::int32_t y,std::uint32_t parent) const;
    // 00475cf0: 객체 부착 가상 +3c에 현재 등록표 루트를 전달한 뒤 현재 메뉴 칸을 제거한다.
    void AttachObjectCurrent(std::uint32_t object,std::int32_t x,std::int32_t y) const;
private:
    // 도달한 노드/기본 노드/스택 칸만 검사한다. 예외 전에 실행된 원본 쓰기는 보존한다.
    MenuGumpNode& Node(std::uint32_t token) const;
    GumpCreateNode& BaseNode(std::uint32_t token) const;
    std::size_t Index() const;
    void DropCurrent() const;
    // 장식의 현재 제목 높이를 CRT 64비트 절삭 후 하위 signed DWORD로 변환해 재부착한다.
    void AttachFancy(std::uint32_t token) const;
    GumpCreateState& create_;
    MenuGumpState& state_;
    const GumpLifetime& lifetime_;
    MenuGumpHooks hooks_;
};
// 메뉴 가상 +8/+30만 분배하고 다른 타입의 기존 프레임 처리기를 보존한다.
void BindMenuGumpFrame(GumpFrameHooks& hooks,std::function<const MenuGump*(std::uint32_t)> resolve);
}
