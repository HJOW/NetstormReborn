// 원문 메타 추가Draw의 장치를 현재 실제 Client/메뉴/표면에서 선택한다.
#include "client/ClientMetaSpriteDraw.h"
#include "client/NativeMetaSpriteDraw.h"
#include "client/NativeIslandRig.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/GameAssets.h"
#include "client/RawCampaignSession.h"
#include "client/RawMenuControls.h"
#include "client/GumpDraw.h"
#include "o/RawRiftShapeAccess.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 현재 빌린 물리/화면 참조는 동기 Draw가 끝나면 보관하지 않는다.
namespace { // Image의 원문32비트 포인터 토큰만 호스트의 같은 할당으로 변환한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("메타 Draw Image 필드 물리 범위");std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //unalignedDWORD를 낮은BYTE부터 조립한다.
void Shape(const GumpPixelTarget& target,const ShapeDatabase& database,std::size_t block,std::size_t index,ScreenPoint point,std::int32_t scaleX,std::int32_t scaleY) { const auto& frames=database.Blocks();if (block>=frames.size() || index>=frames[block].frames.size()) throw std::out_of_range("메타 Draw 실제 SHP 프레임 물리 범위");const auto& frame=frames[block].frames[index];if (frame.IsSpecial()) return;target.ScaledImage(database.Decode(block,index),frame,point,scaleX,scaleY); } //plain VFX 변환이며 소유자/추상 색표를 설치하지 않는다.
}
// 숨김/종류 검사는 원문 본체가 수행하며 연결만으로 현재 World/CS를 새로 만들지 않는다.
void DrawClientMetaSprite(Client& client,RawMenuControls& menus,o::Sid object,ScreenPoint point,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
    auto& storage=*client.SurfaceStorage();auto& challenge=*client.ChallengeGlobals();GumpDrawState draw;SetGumpDrawClip(draw,clip);GumpPixelTarget target(pixels,pitch,height,draw);NativeMetaSpriteDrawHooks hooks;
    hooks.rigImage=[&client](auto qa)->std::span<const std::uint8_t> { const auto core=client.GameplayGlobals()->RequireSession().Core();if (qa>65535U || qa>=core.pool.Capacity()) throw std::out_of_range("메타 Draw QA ProcessForm 물리 범위");const auto* rig=dynamic_cast<const NativeIslandRig*>(core.processes.FromForm(o::Sid{static_cast<std::uint16_t>(qa)}));if (!rig) throw std::logic_error("메타 Draw의 현재 QA IslandRig 미연결");return rig->Fields().subspan(0x78,20); };
    hooks.type=[&storage,object] { return std::uint32_t(storage.pool.Slot(object)[10]); }; //TYPE를 요구하는 원문 위치마다 현재 객체를 다시 조회한다.
    hooks.shape=[&](auto type,auto frame,auto at,auto scaleX,auto scaleY) { if (type<o::kFirstAssetTypeNumber || type>=storage.shapes.size()) throw std::out_of_range("메타 Draw 실제 TYPE 물리 범위");if (!storage.shapes[type].loaded) return;Shape(target,client.Assets().Shapes(),type-o::kFirstAssetTypeNumber,frame,at,scaleX,scaleY); };
    hooks.image=[&](auto header,auto at,auto scaleX,auto scaleY) { const auto token=Read(header,4);if (!token) return;const auto bytes=challenge.ArrayBytes(token);ShapeDatabase database(std::vector<std::uint8_t>(bytes.begin(),bytes.end()));Shape(target,database,0,0,at,scaleX,scaleY); }; //같은 실제 Image 토큰을 읽으며 별도 RawImage 소유자를 생성하지 않는다.
    hooks.firstFrame=[&storage](auto type,auto letter) { return storage.frames.at(type).Run(letter).first; };
    hooks.muzzle=[&storage](auto type,auto frame) {
        const auto& selected=storage.types.at(type);const auto& shape=storage.shapes.at(type);o::RiftShapeAccessHooks access;access.typeNumber=[](auto current) { return current.token; };access.frameError=[&storage](auto current,auto index,auto abstract) { storage.RecordDisplayFrameError(current,index,abstract); };access.assertion=[&storage](auto expression,auto file,auto line) { storage.AssertDisplay(expression,file,line); };
        access.shape=[&storage](auto current,auto index) { const auto& value=storage.shapes.at(current);if (!value.loaded || index<0 || static_cast<std::size_t>(index)>=value.frames.size()) throw std::out_of_range("메타 Draw 진단복귀 뒤 포구 SHP 물리 범위");const auto& header=value.frames[static_cast<std::size_t>(index)];return std::array<float,2>{static_cast<float>(header.muzzleX),static_cast<float>(header.muzzleY)}; };return o::ReadRiftShapeFrame({type,selected.flags1,shape.frameCount,shape.loaded,selected.maxHitPoints},frame,0,access);
    };
    hooks.translate=[&client](auto key) { const auto bytes=o::EncodeDisplayText(client.Translate(key));return std::string(bytes.begin(),bytes.end()); };
    hooks.measure=[&menus](auto text,const auto& context) { const std::string bytes(text);std::array<std::int32_t,2> result{};menus.MeasureText({bytes.data(),bytes.size()+1},context,result[0],result[1],0xffffffffU);return result; };
    hooks.text=[&menus](auto text,auto at,const auto& context) { const std::string bytes(text);menus.DrawCurrentText({bytes.data(),bytes.size()+1},at,context); }; //현재 WithDrawing의 실제 GumpTextDraw와 색 상태를 공유한다.
    hooks.assertion=[&storage](auto expression,auto file,auto line) { storage.AssertDisplay(expression,file,line); };
    const auto& palette=client.GetScreen().Colors();DrawNativeMetaSprite(storage.pool.Slot(object),point,{challenge,palette.basic[7],palette.basic[5],palette.basic[0]},hooks);
}
}
