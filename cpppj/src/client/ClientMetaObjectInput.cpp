// 메타 객체의 메뉴/항해 입력에 현재 실제 Client·CS·화면·설정만 공급한다.
#include "client/ClientMetaObjectInput.h"
#include "client/NativeMetaObjectInput.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientLocalStateGlobals.h" //영역 메뉴도 같은 현재51×144BYTE 기록을 읽는다.
#include "client/RawObjectMenuSlots.h" //현재 실제 VT의+ac로 두 메타 입력을 구별한다.
#include "client/RawCampaignSession.h"
#include "client/RawMenuControls.h"
#include "client/RawCameraView.h"
#include "o/RawRiftShapeAccess.h"
#include "o/OriginalText.h"
#include <bit>
#include <optional>
#include <stdexcept>

namespace netstorm::client { //장치는 호출 뒤 보관하지 않고 실제 입력 큐만 예약을 소유한다.
bool InputClientMetaObject(Client& client,RawMenuControls& menus,std::uint32_t sid,InputEvent event,const std::function<void(std::string_view)>& query) {
    auto& storage=*client.SurfaceStorage();if (sid>65535U || sid>=storage.pool.Capacity()) throw std::out_of_range("메타 객체 입력의 현재 SID 물리 범위");const o::Sid object{static_cast<std::uint16_t>(sid)};
    NativeMetaObjectInputHooks hooks;menus.BindMetaObjectInput(hooks);
    hooks.typeName=[&client,object] { return client.GameplayGlobals()->RequireSession().ObjectBaseName(object); };
    hooks.helpWord=[&client]()->std::optional<std::string> { const auto word=client.Configuration().Configuration().Get("helpWord");if (!word) return {};const auto bytes=o::EncodeDisplayText(*word);return std::string(bytes.begin(),bytes.end()); }; //실제 Config의 존재 조회로 ReadString의 변경 여부0과 빈 문자열을 NULL로 혼동하지 않는다.
    hooks.command=[](auto address) { const auto value=MetaObjectInputCommandValue(address);if (!value) throw std::logic_error("메타 객체 입력의 실제 명령 주소 미등록");return *value; };
    hooks.descriptionParameter=[&client](auto battle) { return client.ChallengeGlobals()->Current()+battle*0x124U+0x101U; }; //CS 논리 주소를 유지하며 호스트 포인터를32비트로 자르지 않는다. f3 소비자는 후속이다.
    hooks.translate=[&client](auto key) { const auto bytes=o::EncodeDisplayText(client.Translate(key));return std::string(bytes.begin(),bytes.end()); };
    hooks.project=[&client](float x,float y) { const auto point=ProjectCameraView(client.GameplayGlobals()->Camera().Snapshot(),x,y);return ScreenPoint{point[0],point[1]}; };
    hooks.frameZero=[&storage](auto type) {
        const auto& selected=storage.types.at(type);const auto& shape=storage.shapes.at(type);std::optional<o::SquidDisplayFrame> header;o::RiftShapeAccessHooks access;
        access.typeNumber=[](auto current) { return current.token; };access.frameError=[&storage](auto current,auto index,auto abstract) { storage.RecordDisplayFrameError(current,index,abstract); };
        access.assertion=[&storage](auto expression,auto,auto) { storage.AssertDisplay(expression,"c:\\ns\\o\\rifttype.h",0xe1); };
        access.shape=[&storage,&header](auto current,auto index) { const auto& value=storage.shapes.at(current);if (!value.loaded || index<0 || static_cast<std::size_t>(index)>=value.frames.size()) throw std::out_of_range("메타 입력 진단복귀 뒤 실제 SHP 물리 범위");header=value.frames[static_cast<std::size_t>(index)];return std::array<float,2>{static_cast<float>(header->width),static_cast<float>(header->height)}; };
        static_cast<void>(o::ReadRiftShapeFrame({type,selected.flags1,shape.frameCount,shape.loaded,selected.maxHitPoints},0,0,access));return std::array<std::int32_t,4>{header->width,header->height,header->hotspotX,header->hotspotY};
    }; //현재 TYPE의 frame0 검사/복귀형 진단 뒤 signed WORD 네 필드를 읽는다.
    hooks.query=query;hooks.queue=[&client](auto command,auto first,auto second) { client.Input().PushCommand(command,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };
    hooks.assertion=[&storage](auto expression,auto file,auto line) { storage.AssertDisplay(expression,file,line); };
    const auto raw=storage.pool.Slot(object);const auto vtable=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);const auto open=ObjectMenuSlots(vtable).open;
    if (open==0x479390U) return InputNativeMetaZoneObject(sid,raw,event,client.LocalStateGlobals()->MetaZoneRecords(),hooks);
    if (open!=0x4789b0U) throw std::logic_error("실제 메타 객체+ac의 원문 입력 미등록");return InputNativeMetaObject(sid,raw,event,{*client.ChallengeGlobals(),client.GameplayGlobals()->player.battleOwner},hooks);
} //선택/kill/활성 관문은 추가하지 않고4789b0의 실제 배율 관문과 분기만 적용한다.
}
