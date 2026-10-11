// 원본 Gump.cpp의 화면 위치/경계와 트리 표시 수집을 복원한다.
#pragma once
#include "client/DrawSubmission.h"

namespace netstorm::client {
// 원본 Gump의 수집에 필요한 필드다. 링크/객체 포인터는 호스트 식별 토큰으로 대체한다.
struct GumpCollectionNode {
    std::uint32_t flags{},next{},parent{},child{},object{},depth{}; // +4/+0xc/+0x14/+0x18/+0x24/+0x30이다.
    float width{},height{},x{},y{}; // +0x44/+0x48의 배율 적용 크기와 +0x4c/+0x50의 로컬 위치다.
    ScreenPoint cached{}; // +0x6c/+0x70: 일반 위치 조회만 갱신하고 월드 부착 자신은 보존한다.
};
// 루트와 임시 수집 머리 전역이다. overrideHead가 0이면 root.child부터 수집한다.
struct GumpCollectionState {
    std::uint32_t root{},overrideHead{};
    o::OriginalEdition edition{o::OriginalEdition::Patch1078}; // CD는 월드 부착 부모의 정수 위치를 먼저 float으로 저장한다.
};
// node는 현재 링크/필드를 읽고 캐시를 갱신할 저장소를 반환한다. 노드는 호출 동안 살아 있어야 한다.
// project는 object SID의 원본 월드→화면 투영 결과를 공급한다. 투영 자체와 SID 해석은 별도 복원 범위다.
struct GumpCollectionHooks {
    std::function<GumpCollectionNode*(std::uint32_t)> node;
    std::function<ScreenPoint(std::uint32_t)> project;
};
// 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다.
class GumpCollection {
public:
    // 상태/노드 저장소는 수집기보다 오래 살아야 한다. 없는 조회 콜백은 생성 시 거부한다.
    GumpCollection(const GumpCollectionState& state,GumpCollectionHooks hooks);
    // 부모별 float 저장 후 CRT 64비트 절삭의 하위 DWORD를 돌려주고 일반 노드 캐시를 갱신한다.
    ScreenPoint Position(std::uint32_t token) const;
    // 정수 위치에 배율 적용 크기를 더해 절삭한다. 합산 직후 float으로 재반올림하지 않는다.
    ScreenRect Bounds(std::uint32_t token) const;
    // Draw 직전에 같은 저장소의 현재 raw 플래그를 읽는다. visible/dead 필터는 수집기가 담당한다.
    std::uint32_t Flags(std::uint32_t token) const;
    // 원본의 root.child/override 순서로 수집한다. limit은 각 최상위 노드에 같은 값으로 전달한다.
    // 따라서 전체 반환 수는 limit을 넘을 수 있다. 실제 output 크기를 넘기는 쓰기는 예외로 거부한다.
    std::int32_t Collect(ScreenRect view,std::span<o::RenderCollectionEntry> output,std::int32_t limit) const;
    // 지정 노드의 visible/dead 비트를 직접 검사하지 않고 깊이 우선 수집한다. 자식에는 visible만 검사한다.
    std::int32_t CollectNode(std::uint32_t token,ScreenRect view,
        std::span<o::RenderCollectionEntry> output,std::int32_t limit) const;
private:
    // 없는 토큰/저장소는 예외로 거부한다. 원본의 유효 포인터 접근에 대응하는 호스트 계약이다.
    GumpCollectionNode& Node(std::uint32_t token) const;
    // 월드 부착은 반드시 root 직계 자식이다. 투영 이후 로컬 위치를 절삭해 DWORD 덧셈한다.
    ScreenPoint Anchor(const GumpCollectionNode& node) const;
    const GumpCollectionState& state_; // 현재 루트/override를 호출 시 읽는다.
    GumpCollectionHooks hooks_; // 저장소 조회와 월드 투영의 명시 경계다.
};
// 공통 UI 슬롯에 실제 트리 수집을 연결한다. collector는 hooks의 사용 기간 동안 살아 있어야 한다.
void BindGumpCollection(DrawCollectionHooks& hooks,const GumpCollection& collector);
}
