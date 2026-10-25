// 전체 Uber와 구분선의 실제 고정 노드를 같은 Client Gump 저장소에 조립한다.
#pragma once
#include "client/UberRootGump.h"
#include "client/HudDividerInput.h"
#include <memory>

namespace netstorm::client { // 같은 공통 UI의 실제 파생 수명과 도킹을 소유한다.
struct RawMenuTree;class RawMenuControls; // 등록표/그리기/문자열은 기존 단일 처리기를 빌린다.
struct RawUberRootContext {
    HudDockGlobals globals; // 실제 현재 화면/지도 전역의 참조다. 이 처리기보다 오래 살아야 한다.
    UberRootHooks root; // UI 밖의 현재 전역·실제 네 자식 공장·메시지 효과를 공급한다.
    HudDockHooks dock; // 실제 화면 무효화/크기 후처리·채팅 표시 설정 장치다.
    std::function<InputEvent(std::uint32_t)> poll;std::function<void(std::uint32_t)> cursor; // 같은 실제 폴링과 커서다.
    std::function<void(std::uint32_t)> replaced; // 공통 등록0 교체 뒤 호스트 루트 별칭만 맞춘다.
};
class RawUberRootControls {
public:
    RawUberRootControls(RawMenuTree tree,RawMenuControls& menus,RawUberRootContext context); // 생성 중 노드를 만들지 않고 실제 분배를 먼저 설치한다.
    ~RawUberRootControls(); // 공통 노드 정리 뒤 자기 출력 대여만 반납한다.
    void Replace(); // 전체 원문 공장과 기존 자식 이동/루트 교체를 실행한다.
    void Reflow(); // 화면 장치 크기 변경 때 기존 실제 구분선/슬레이브를 재부착하고 아래쪽 거리를 보존한다.
    GumpCreateNode* Node(std::uint32_t token) const; // 같은 실제 기본 주소만 반환한다.
    GumpBackgroundNode* Background(std::uint32_t token) const; // 실제 구분선 배경이며 별도 복사본을 만들지 않는다.
    bool Current() const; // 등록0의 실제 파생과 필수 자식들이 아직 살아 있는지 읽는다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode); // 기본 루트/파생 구분선의 실제 부착이다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second); // 실제 dock/드래그 입력이다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second); // 구분선의 실제 프레임 폴링이다.
    void DeleteScalar(std::uint32_t token,std::uint32_t flags); // 두 파생 모두 원문의 기본 scalar를 사용한다.
    void Release(std::uint32_t token); // 실제 scalar 반납 때만 고정 저장소를 지운다.
    std::optional<ScreenPoint> Point(std::string_view label) const; // 검사 입력은 현재 실제 구분선 경계의 중앙만 읽는다. 게임 상태를 쓰지 않는다.
    std::string Report() const; // 실제 루트/자식/지도 전역을 읽기만 한다.
private:
    struct Impl;std::unique_ptr<Impl> impl_; // 콜백이 참조하는 실제 노드/처리기의 주소를 고정한다.
};
}
