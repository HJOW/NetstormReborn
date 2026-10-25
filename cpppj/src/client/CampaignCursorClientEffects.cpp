#include "client/ClientGameplayGlobals.h"
// 커서의 외부 경계를 실제 장치/원본 트리 처리기에 연결하며 성공 값을 합성하지 않는다.
#include "client/CampaignCursorClientEffects.h"
#include "client/RawMenuControls.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 보호 PE의542874/542870 DWORD이며 수신 허용5427f4와 구별한다.
constexpr std::uint32_t kCursorGuiFlags=0x0200005f,kCursorGuiReceive=0x0200005e;
// 번역·측정·출력이 같은 원본 Windows-1252 바이트를 사용하게 한다.
std::string CursorBytes(Client& client,std::string_view key) {
    const auto bytes=o::EncodeOriginalText(client.Translate(key));return {bytes.begin(),bytes.end()};
}
}
// PickTopLevel과 재귀 Dispatch의 반환 토큰을 직접 가상 처리기의 DWORD 반환과 구별한다.
CursorPlacementHooks MakeClientCursorPrepareHooks(Client& client,std::function<Renderer&()> renderer) {
    if (!renderer) renderer=[&client]() -> Renderer& { return client.GetRenderer(); };
    CursorPlacementHooks hooks;hooks.project=[&client](float x,float y) { const auto point=ProjectCameraView(client.GameplayGlobals()->Camera().Snapshot(),x,y);return ScreenPoint{point[0],point[1]}; };
    hooks.invalidate=[renderer=std::move(renderer)](ScreenRect rect,std::uint32_t flags) { renderer().Invalidate(rect,flags); };return hooks;
}
// 실제 Gump 수신은 Native 카메라와 별도로 현재 트리를 빌린다.
void BindCampaignCursorGumps(CampaignCursorEffects& effects,RawButtonControls& controls) {
    effects.placement.gumpAt=[&controls](std::int32_t x,std::int32_t y) { return controls.PickGump({x,y}); };
    effects.placement.accept=[&controls](std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,std::uint32_t hidden) {
        return controls.DispatchGump(token,event,first,second,hidden!=0);
    };
    effects.placement.parent=effects.drop.guiParent=[&controls](std::uint32_t token) { return controls.GumpParentSid(token); };
    effects.drop.guiFlags=[&controls](std::uint32_t token) { return controls.HandleGump(token,kCursorGuiFlags,0,0); };
    effects.drop.guiReceive=[&controls](std::uint32_t token,o::Sid form) {
        static_cast<void>(controls.HandleGump(token,kCursorGuiReceive,form.value,0));
    };
    // 원본466710은 sticky 팝업의 Kill 예약이며 현재 호출 안에서 Cleanup하지 않는다.
    if (!effects.lifecycle.closePopups) effects.lifecycle.closePopups=[&controls] { controls.RequestClosePopups(); };
}
// 장치는 사용 시 조회하여 해상도/Renderer 교체 뒤에도 이전 포인터를 보관하지 않는다.
CampaignCursorEffects MakeCampaignCursorClientEffects(Client& client,RawButtonControls& controls,CampaignCursorEffects effects) {
    BindCampaignCursorGumps(effects,controls);
    effects.drop.showSp=[&controls](auto cost,auto x,auto y,auto reference) {
        auto* gumps=controls.Menus();if (!gumps) throw std::logic_error("커서 비용 표시의 실제 Gump 트리 미연결");gumps->ShowSp(cost,x,y,reference);
    };
    effects.lifecycle.selectCursor=[&client](std::uint32_t number) { client.GetCursor().Set(std::bit_cast<std::int32_t>(number)); };
    effects.lifecycle.translate=[&client](std::string_view key) { return CursorBytes(client,key); };
    effects.lifecycle.measure=[&client](std::string_view bytes,const std::array<std::uint32_t,8>& words) {
        const std::string terminated(bytes);std::int32_t width{},height{}; // 원문 바이트 뒤 NUL 한 개를 측정 범위에 포함한다.
        GumpText(client.Fonts()).Measure(std::span(terminated.c_str(),terminated.size()+1),DecodeGumpTextContext(words),width,height,-1);
        return std::array{width,height};
    };
    effects.lifecycle.mouse=[&client] { const auto mouse=client.Poll(InputCode::kLeftButton);return ScreenPoint{mouse.x,mouse.y}; };
    effects.placement.invalidate=MakeClientCursorPrepareHooks(client).invalidate;
    return effects;
}
}
