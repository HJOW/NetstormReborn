// NSInfo는 배경 부착 뒤에만 메타맵 관문을 읽는다.
#include "client/HudNsInfo.h"
#include <stdexcept>

namespace netstorm::client {
// 일반 Attach에 실제 도달하는 공통효과만 필수로 요구한다.
HudNsInfo::HudNsInfo(HudNsInfoGlobals globals,HudNsInfoHooks hooks):HudNsInfo(HudNsInfoModeView{[&value=globals.metamap] { return value; },[&value=globals.playerKnown] { return value; }},std::move(hooks)) {}
// 실제 Client 물리의 읽기를 Native DWORD 참조와 같은 소비 위치에 둔다.
HudNsInfo::HudNsInfo(HudNsInfoModeView globals,HudNsInfoHooks hooks):globals_(std::move(globals)),hooks_(std::move(hooks)) {
    if (!globals_.metamap || !globals_.playerKnown || !hooks_.node || !hooks_.base || !hooks_.baseAttach || !hooks_.push || !hooks_.attach) throw std::invalid_argument("NSInfo의 같은 노드/트리 연결 필요");
}
// 호출 중의 고정this 주소를 실제 확장 노드로 조회한다.
HudNsInfoNode& HudNsInfo::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;if (!node) throw std::runtime_error("NSInfo 실제 저장소 없음");return *node;
}
// 배경은 같은 공통 Gump 저장소에서 조회하며 임시복사본을 받지 않는다.
GumpCreateNode& HudNsInfo::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("NSInfo 현재 배경 없음");return *node;
}
// 현재배경포인터는 Push/Attach 콜백에서 바뀔 수 있으므로 축과 마지막 부착마다 재조회한다.
void HudNsInfo::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    auto& node=Node(token);hooks_.baseAttach(token,x,y,parent,mode);hooks_.push(node.background);
    const float width=node.baseWidth;auto& horizontal=Base(node.background);horizontal.baseWidth=width;horizontal.width=static_cast<float>(double(width)*horizontal.scale);
    auto& vertical=Base(node.background);const float height=node.baseHeight;vertical.baseHeight=height;vertical.height=static_cast<float>(double(height)*vertical.scale);
    hooks_.attach(node.background,0,0,token,0);
    if (!globals_.metamap() || !globals_.playerKnown()) return;
    if (!hooks_.metamapLayout) throw std::runtime_error("NSInfo 전체 메타맵 배치 미연결");hooks_.metamapLayout(token);
}
}
