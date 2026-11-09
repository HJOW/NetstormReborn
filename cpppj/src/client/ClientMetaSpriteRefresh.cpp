// 원문498ff0/499310·Form finder/현재 Gump를 같은 Client의 실제 메타 갱신에 공급한다.
#include "client/ClientMetaSpriteRefresh.h"
#include "client/NativeMetaSpriteRefresh.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/Renderer.h"
#include "client/GumpRedraw.h" //현재 viewport의 원문 좌표별 clamp를 그대로 사용한다.
#include "client/RawCameraView.h"
#include "o/RawRiftShapeAccess.h"
#include <bit>
#include <optional>
#include <stdexcept>

namespace netstorm::client { //동일한실제본문을장기Client장치와선택된현재World의짧은override가공유한다.
namespace { // 원문 좌표/필드 비트만 호스트에서 정의된 계산으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) { if (at>raw.size() || width>raw.size()-at) throw std::out_of_range("메타 dirty 장치의 현재 필드 물리 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(raw[at+byte])<<(8*byte);return value; } //선택된 폭만 낮은BYTE부터 조립한다.
std::int32_t Add(std::int32_t left,std::int32_t right) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(left)+std::bit_cast<std::uint32_t>(right)); } //LEA 좌표 감김이다.
std::int32_t Sub(std::int32_t left,std::int32_t right) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(left)-std::bit_cast<std::uint32_t>(right)); } //SUB 좌표 감김이다.
std::int32_t Scale(std::int16_t value,std::uint32_t zoom) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int32_t>(value))*zoom)>>16; } //signed WORD×현재 배율의 lowDWORD/SAR16이다.
ScreenRect Bounds(Client& client,std::span<const std::uint8_t> raw) {
    const auto type=Read(raw,10,1),frame=Read(raw,0x24);const auto worldX=std::bit_cast<float>(Read(raw,0xe)),worldY=std::bit_cast<float>(Read(raw,0x12));const auto& view=client.GameplayGlobals()->Camera().Snapshot();
    const auto x=Sub(CameraFtolLow(static_cast<double>(worldX)*16.0+0.5),view.pixels[0]),y=Sub(CameraFtolLow(static_cast<double>(worldY)*11.0+0.5),view.pixels[1]);auto& storage=*client.SurfaceStorage();const auto& selected=storage.types.at(type);const auto& shape=storage.shapes.at(type);if (!shape.loaded) return {};
    std::optional<o::SquidDisplayFrame> header;o::RiftShapeAccessHooks access;access.typeNumber=[](auto current) { return current.token; };access.frameError=[&storage](auto current,auto index,auto abstract) { storage.RecordDisplayFrameError(current,index,abstract); };access.assertion=[&storage](auto expression,auto file,auto line) { storage.AssertDisplay(expression,file,line); };
    access.shape=[&storage,&header](auto current,auto index) { const auto& currentShape=storage.shapes.at(current);if (!currentShape.loaded || index<0 || static_cast<std::size_t>(index)>=currentShape.frames.size()) throw std::out_of_range("메타 dirty 진단복귀 뒤 SHP 물리 범위");header=currentShape.frames[static_cast<std::size_t>(index)];return std::array<float,2>{static_cast<float>(header->width),static_cast<float>(header->height)}; };static_cast<void>(o::ReadRiftShapeFrame({type,selected.flags1,shape.frameCount,shape.loaded,selected.maxHitPoints},std::bit_cast<std::int32_t>(frame),0,access));
    const auto zoom=client.GameplayGlobals()->Camera().Snapshot().zoom;const auto left=Sub(x,Scale(header->hotspotX,zoom)),top=Sub(y,Scale(header->hotspotY,zoom));return {left,top,Add(Add(left,Scale(header->width,zoom)),1),Add(Add(top,Scale(header->height,zoom)),1)};
} //현재498ff0의 두 FTOL→카메라 뺄셈→SHP 존재/진단→현재 배율·네 WORD 순서다.
}
//같은 실제 Client/controls가 현재 메타4787c0과 일반 Sprite4791b0을 선택하며 새 Session/World를 만들지 않는다.
void RefreshClientMetaSprite(Client& client,RawButtonControls& controls,std::span<const std::uint8_t> raw) {
        auto& storage=*client.SurfaceStorage();auto& game=*client.GameplayGlobals();NativeMetaSpriteRefreshHooks hooks;hooks.bounds=[&client](auto object) { return Bounds(client,object); };
        hooks.dirty=[&client,&controls](auto rect) { rect=ClampOriginalScreenRect(rect,controls.MapViewRect());if (rect.right!=rect.left && Sub(rect.right,rect.left)>=0 && rect.bottom!=rect.top && Sub(rect.bottom,rect.top)>=0) client.GetRenderer().Invalidate(rect,0); }; //00499310은 일반 표시 억제 관문을 추가하지 않는다.
        hooks.raw=[&storage](auto sid)->std::span<const std::uint8_t> { if (sid>65535U || sid>=storage.pool.Capacity()) throw std::out_of_range("메타 dirty Form SID 물리 범위");return storage.pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)}); };
        hooks.redrawGump=[&controls](auto id) { if (!controls.Menus()) throw std::logic_error("메타 dirty 현재 Gump 장치 미연결");controls.Menus()->RedrawRegisteredGump(id); };
        hooks.assertion=[&storage](auto expression,auto file,auto line) { storage.AssertDisplay(expression,file,line); };
        NativeMetaSpriteRefreshGlobals globals{*client.ChallengeGlobals(),game.GameAux()->DependFormType(),game.GameAux()->ProcessTypeEnd(),storage.Checking(),storage.ownerMode.challenge,game.Boss()};
        if (Read(raw,0)==0x50cc08U) {
            const auto owned=storage.pool.Bytes();const auto first=reinterpret_cast<std::uintptr_t>(owned.data()),address=reinterpret_cast<std::uintptr_t>(raw.data());
            if (raw.size()!=50 || owned.size()<50 || address<first || address-first>owned.size()-50 || (address-first)%50) throw std::invalid_argument("Sprite 표시 장치의 같은 실제 Pool 슬롯 계약 오류");
            RefreshNativeSprite({const_cast<std::uint8_t*>(raw.data()),raw.size()},globals,hooks); //실제 소유 저장소의 같은 가변50BYTE임을 확인한 뒤 원문BYTE35 쓰기만 허용한다.
        } else RefreshNativeMetaSprite(raw,globals,hooks);
}
//현재World의명시대여도동일실제본문을공유하며짧은controls는선택된Runtimepin으로보존한다.
void BindClientMetaSpriteRefresh(Client& client,RawButtonControls& controls,RawSurfaceWorld& world) { world.BindMetaRefresh([&client,&controls](auto raw) { RefreshClientMetaSprite(client,controls,raw); }); }
}
