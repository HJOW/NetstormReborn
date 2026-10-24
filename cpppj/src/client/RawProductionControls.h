// 실제 Client 생산 패널을 같은 원본 Gump 트리/내용물/커서에 연결한다.
#pragma once
#include "client/RawMenuControls.h"
#include "client/RawCampaignSession.h"
#include "client/RawProductionMaintenance.h"
#include "client/ProductionControlContext.h"
namespace netstorm::client {
struct InventoryItemNode; // 우클릭 시 같은 실제 파생 저장소를 정보 메뉴에 빌린다.
struct RawProductionHooks {
    std::function<double()> now; // 원본 벽 시계다.
    std::function<void(std::string_view)> sound,tell; // 실제 음향과 미션 알림이다.
    std::function<std::int32_t(std::string_view,std::int32_t)> configInt; // 같은 현재 Client 설정과 원본 누락 기본값이다.
    std::function<std::string(std::string_view)> translate; // 원본 실패 안내의 현재 번역 바이트다.
    std::function<std::uint32_t(const InventoryItemNode&,ScreenPoint,const std::function<bool(o::Sid)>&)> about; // 같은 내용물과 현재4af010을 실제 정보 메뉴에 연결한다.
    std::function<InputEvent(std::uint32_t)> poll;std::function<void(std::uint32_t)> cursor; // 구분선의 실제 현재 입력과 커서이며 선택된 경우만 요구한다.
    std::function<double()> gameSeconds; // 초기 HUD 관측도 실제 게임 시각을 읽으며 벽 시계와 구별한다.
};
class RawProductionControls {
public:
    RawProductionControls(RawMenuTree tree,RawMenuControls& menus,RawCampaignSession& session,const GameAssets& assets,ProductionMaintenanceGlobals& globals,RawProductionHooks hooks); // 현재 실제 Context를 같은 생성 경계에 빌린다.
    RawProductionControls(RawMenuTree tree,RawMenuControls& menus,ProductionControlContext context,const GameAssets& assets,ProductionMaintenanceGlobals& globals,RawProductionHooks hooks); // Session 전에 실제 물리/전역만으로 전체 생산 부모를 준비한다.
    ~RawProductionControls(); // 공통 노드 삭제가 끝난 뒤 처리기를 반납한다.
    std::uint32_t RootToken() const; // 같은 실제 생산 루트 토큰이며 새/이전 처리기를 구별한다.
    bool HasNodes() const; // 공통 scalar가 반납할 실제 저장소가 남아 있는지 읽는다. Kill 예약과 소멸 완료를 구별한다.
    void Close(); // 월드보다 먼저 모든 연결 Form과 자식을 반납한다.
    void Open(); // 원본43e640/43ddd0 및 초기43ec00 사건이다.
    std::uint32_t Construct(); //0043e640/예산0043e490의 전체 생성만 한다. 추가 도킹/즉시 생산 동기화를 섞지 않는다.
    void Resize(int screenHeight); // 기존 하단80 예약을 유지하며 같은 패널을 새 화면 높이에 재부착한다. 같은 높이는 수명을 바꾸지 않는다.
    GumpCreateNode* Node(std::uint32_t token) const; // 같은 실제 파생 주소다.
    GumpBackgroundNode* Background(std::uint32_t token) const; // 같은 배경 자식이다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode); // 실제 파생 부착이다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second); // 실제 격자/항목/생산 사건이다.
    void Idle(std::uint32_t token); // 원본 예산 및 현재 부모 재연결이다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second); // 실제 구분선 가상+2c 분배다.
    void ParentGone(std::uint32_t token,std::uint32_t parent); // 격자 가상+4를 구별한다.
    void DeleteScalar(std::uint32_t token,std::uint32_t flags); // 실제 텍스트/기본 수명이다.
    void Release(std::uint32_t token); // 공통 scalar가 반납한 동적 저장소와 inline 루트/예산의 토큰을 무효화한다.
    void SetPalette(std::span<const ScreenColor> palette,std::span<const std::uint32_t> colorSortLogical={}); //실제Client는같은고정초기RGB를별도전달하며기존독립검사는명시fallback이다. // 실제 파일 팔레트에서 원본 표를 생성한다.
    std::optional<ScreenPoint> Point(std::uint32_t type) const; // 같은 실제 이미지의 불투명 클릭점이다.
    std::string Report() const; // 실제 SID/경계/그림을 자동 입력에 공급한다.
private:
    struct Impl;std::unique_ptr<Impl> impl_; // 안정된 현재 파생/처리기 주소다.
};
}
