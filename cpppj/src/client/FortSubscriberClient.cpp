// 등록 ID 변경을 같은 현재 header/handle/등록 설정/일반 파일 저장에 전달한다.
#include "client/FortSubscriberClient.h"
#include "client/CampaignClientServices.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientSpGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawFortSubscriberChange.h"
#include "o/RawFortSubscriberLoad.h"
#include "o/RawFortSubscriberWrite.h"
#include "o/RawSubscriberIdGenerate.h"
#include "o/RawPlayerTemplateState.h"
#include <optional>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 현재 Fort의 등록 동기화는 World/Session 수명과 독립이다.
namespace { // 저장소의 실제 DWORD와 선택된 장치를 원문 접근 위치에서만 조회한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("등록 Fort 저장 실제 장치 미연결");return function; } // 비선택 파일 경계를 선행 요구하지 않는다.
std::uint32_t Header(CampaignTemplateStorage& storage,std::uint32_t slot,std::size_t offset=0) { // 같은 현재20BYTE 헤더의 한 DWORD만 읽는다.
    const auto headers=storage.Headers();if (slot>=9 || offset>16 || headers.size()!=180) throw std::out_of_range("등록 Fort Template 헤더 범위");
    std::uint32_t value{}; // 매 조회 시점의 실제 DWORD를 낮은 BYTE부터 조립한다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(headers[slot*20U+offset+byte])<<(byte*8U); // 이웃 헤더와 현재 저장소를 변경하지 않는다.
    return value;
}
void ReportChangedAssertion(Client& client,o::PlayerTemplateStateAssertion assertion) { //004bd360의 원문 표현식/파일/줄을 실제 Client 진단으로 전달한다.
    switch (assertion) {
    case o::PlayerTemplateStateAssertion::DotFileName:client.SurfaceStorage()->AssertDisplay("tm->fileName[0] != '.'","\\Ns\\O\\Template.cpp",0x1c1);break;
    case o::PlayerTemplateStateAssertion::MineTransientFlag:client.SurfaceStorage()->AssertDisplay("0","\\Ns\\O\\Template.cpp",0x1c3);break;
    case o::PlayerTemplateStateAssertion::InMissionSave:client.SurfaceStorage()->AssertDisplay("!inMission","\\Ns\\O\\Template.cpp",0x1c6);break;
    case o::PlayerTemplateStateAssertion::MissingNotifyTemplate:client.SurfaceStorage()->AssertDisplay("tm","\\Ns\\O\\Template.cpp",0x1c7);break;
    default:throw std::logic_error("등록 Fort Notify 밖의 State getter 진단");
    }
}
}
void SynchronizeClientRegisteredSubscriber(Client& client,CampaignTemplateStorage& storage,std::uint32_t id,std::span<const std::uint8_t> hiddenPlayers,const std::function<TemplateFileWriteHooks()>& files) { //004b32f0의 manager/fort 관문은 기존 raw 부모가 직접 소유한다.
    auto& gameplay=*client.GameplayGlobals();auto& subscribers=*client.SubscriberGlobals(); // 같은 Client 전역이며 호출용 사본을 구성하지 않는다.
    o::FortSubscriberChangeHooks change;
    change.owner=[&gameplay] { return gameplay.player.templateOwner; };change.manager=[&storage](auto slot) { return Header(storage,slot); };
    change.fortMode=[&client] { return client.SurfaceStorage()->ownerMode.fort ? 1U : 0U; };change.handle=[&subscribers] { return std::span(subscribers.Handle()); };
    change.subscriber=[&](auto slot,auto capturedId,auto handle) {
        o::FortSubscriberWriteHooks write;std::optional<TemplateWriteStream> stream; // 같은 Create 반환 수명을 ID/문자열 쓰기가 끝날 때까지 고정한다.
        write.manager=[&storage](auto selected) { return Header(storage,selected); };write.assertion=[&client](auto line) { client.SurfaceStorage()->AssertDisplay("tm","\\Ns\\O\\Template.cpp",line); };
        write.registerSubscriber=[&client,&subscribers] {
            const auto services=MakeCampaignSubscriberClientServices(client);o::FortSubscriberLoadHooks registration;registration.readRegistered=services.readRegistered;registration.configure=services.configure;
            registration.generate=[&client,clock=services.clock] {
                const auto owner=client.RandomGlobals();o::SubscriberIdGenerateHooks generate;generate.time32=ClientCurrentUnixSeconds;generate.clock=clock; // 같은 최초시드 DWORD/실제 CRT를 빌린다.
                generate.expired=[&client](auto level,auto text) { client.WriteDiagnosticLog(level,text); };
                return o::GenerateSubscriberId({owner->SubscriberIdInitialized()},o::BindSubscriberIdCrtRandom(owner->Crt(),std::move(generate)));
            };
            o::FinishFortSubscriberRegistration({subscribers.MySubscriber()},registration);
        };
        write.create=[&](auto manager,auto section) { stream=storage.CreateManagerSection(manager,section);return 1U; }; // 실제 선택된 manager의 Subscriber만 교체한다.
        write.write=[&](auto token,auto bytes) { if (token!=1 || !stream) throw std::logic_error("등록 Fort 고정 Subscriber stream 누락");storage.WriteSection(*stream,bytes); };
        o::SaveFortSubscriber(slot,capturedId,handle,write);
    };
    change.notify=[&](auto slot) {
        o::PlayerTemplateChangedHooks notify;notify.header=[&storage](auto selected,auto offset) { return Header(storage,selected,offset); };notify.name=[&storage](auto selected)->const std::string& { return storage.Name(selected); };
        notify.mission=[&client] { return client.SurfaceStorage()->links.mission; };notify.assertion=[&client](auto assertion) { ReportChangedAssertion(client,assertion); };
        notify.save=[&](auto selected) {
            o::FortImageHooks image;image.typeCount=[&client] { return static_cast<std::uint32_t>(client.SurfaceStorage()->types.size()); };image.assertion=[](auto) { throw std::logic_error("등록 Fort 실제 이미지 진단"); }; // 기존 Session 저장과 같은 이미지 진단 경계다.
            TemplateFileWriteHooks writer;writer.permissions=[&](auto name,auto mode) { const auto current=Need(files)();Need(current.permissions)(name,mode); };writer.write=[&](auto name,auto bytes) { const auto current=Need(files)();Need(current.write)(name,bytes); }; // 이미지 생성/현재 저장·숨김 관문 뒤에만 파일 namespace를 요구한다.
            storage.SaveFile(selected,{client.ReadOnlyForts(),gameplay.player.battleOwner,hiddenPlayers},std::move(image),writer);
        };
        o::NotifyPlayerTemplateChanged(slot,notify);
    };
    o::ChangeFortSubscriber(id,std::nullopt,subscribers.MySubscriber(),change); // 이름 NULL이므로 설정/제목/handle 복사를 추가하지 않는다.
}
}
