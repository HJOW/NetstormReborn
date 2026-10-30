// 10.78의 기본 Gump 갱신 요청과 지도 좌표별 제한을 같은 현재 UI/Renderer에 전달한다.
#pragma once
#include "client/GumpCollection.h"

namespace netstorm::client { // 실제 UI 위치와 현재 지도 경계를 읽는 표시 요청 계층이다.
// 00445290: 사각형의 각 좌표를 해당 지도 축의 두 경계 안으로 제한한다.
ScreenRect ClampOriginalScreenRect(ScreenRect rectangle,ScreenRect bounds);
// 노드/지도/dirty 장치는 호출 동안 살아 있어야 하며 실제 호출 시 현재 값을 읽는다.
struct GumpRedrawHooks {
    std::function<GumpCollectionNode*(std::uint32_t)> node; // 동일 트리의 현재 flags/parent/크기와 위치 캐시를 조회한다.
    std::function<ScreenRect()> map; //499310이 Position 뒤에 읽는 현재 지도LTRB다.
    std::function<void(ScreenRect,std::uint32_t)> invalidate; //497580에 전달할 실제 사각형/flags이며 요청을 먼저 합치지 않는다.
};
// 기본 두 함수만 복원한다. 메뉴 장식 등 파생 Redraw는 이 기본 요청을 호출할 때 별도로 분배한다.
class GumpRedraw {
public:
    GumpRedraw(const GumpCollection& collection,GumpRedrawHooks hooks); //같은 위치 본체와 현재 장치를 빌리며 게임 객체를 복제하지 않는다.
    void Whole(std::uint32_t token,std::uint32_t argument=0) const; //464a50→499370: 가시 조상 뒤 Position/폭·높이 절삭과XYWH를 요청한다. 원문 인수는 미사용이다.
    void Map(std::uint32_t token,std::uint32_t argument=0) const; //464ab0→499310: Position+실수 크기의 끝점을 절삭하고 현재 지도 안의 양의 영역을 요청한다.
private:
    GumpCollectionNode& Node(std::uint32_t token) const; //없거나 해제된 토큰을 읽기 전에 호스트 계약 오류로 거부한다.
    bool Visible(std::uint32_t token) const; //자기/모든 조상의 bit1만 읽으며 dead/void를 별도 가시성 필터로 쓰지 않는다.
    const GumpCollection& collection_; //같은 실제 Position/Bounds와 원문 캐시·월드 투영을 빌린다.
    GumpRedrawHooks hooks_; //호출 시 현재 저장소/지도/Renderer 요청 장치다.
};
}
