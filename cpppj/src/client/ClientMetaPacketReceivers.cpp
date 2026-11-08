// 원문의 선택 순서·현재 전역·진입 CS를 유지하고 실제 하위만 도달 위치에서 요청한다.
#include "client/ClientMetaPacketReceivers.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/RawCampaignSession.h" //선택된 image/지면 효과만 현재 실제 Runtime을 빌린다.
#include "client/CurrentAllocationBytes.h"
#include "client/NativeMetaParticipantCreate.h"
#include "client/NativeMetaParticipantMove.h"
#include "client/NativeMetaBattleOptions.h"
#include "client/NativeCameraRig.h"
#include "client/ClientChatRuntime.h"
#include "client/ClientAudio.h"
#include "o/RawRiftShapeAccess.h"
#include "o/RawStateBattleOptions.h"
#include "o/RawMissionBattleOptions.h"
#include <algorithm>
#include <bit>
#include "client/ClientChallengeGlobals.h"
#include "client/ClientSubscriberGlobals.h" //현재 자기 subscriber의 실제 DWORD를 조회한다.
#include "client/RawSurfaceWorld.h"
#include "client/NativeMetaParticipantRemove.h"
#include "client/NativeMetaParticipantName.h"
#include "client/NativeMetaBattleCreate.h"
#include "client/NativeMetaBattleRemove.h"
#include "client/NativeMetaSailError.h"
#include "client/ClientMetaBattleObjects.h"
#include "client/NativeIslandRig.h"
#include "client/RawChatChannels.h"
#include "client/UberGump.h"
#include "o/RawPlayerNotification.h" //42b210의 같은 기술 합계 본체다.
#include "o/OriginalText.h"
#include <string>
#include <stdexcept>

namespace netstorm::client { //같은 Client는 등록표/실제 Source보다 오래 살아야 한다.
namespace { //선택되지 않은 UI/World 장치를 앞에서 요구하지 않는다.
//현재 비정렬 필드의 선택 폭만 읽으며 인접 BYTE를 앞에서 검사하지 않는다.
std::uint32_t ReadClientMetaPacket(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Client Meta packet 실제 읽기 범위");std::uint32_t result{};for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result; } //낮은 BYTE부터 지정 폭을 조립한다.
//원문 지정 폭의 부분 쓰기를 유지하고 나머지 scratch를 지우지 않는다.
void WriteClientMetaPacket(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Client Meta packet 실제 쓰기 범위");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //한 BYTE씩 실제 목적에 쓴다.
//df590의 protocol/type/초기 선언 길이만 작성한다.
void BeginClientMetaPacket(std::span<std::uint8_t> packet,std::uint8_t type) { WriteClientMetaPacket(packet,0,1,1);WriteClientMetaPacket(packet,1,type,1);WriteClientMetaPacket(packet,2,0,2); }
//실제 전역11/Player/NULL 관문 뒤에만 같은 현재 Binding의 가상 송신을 선택한다.
bool SendClientMetaMutable(Client& client,std::span<std::uint8_t> packet) { const auto storage=client.SurfaceStorage();return client.ConnectionGlobals()->SendMutableNative(11,packet,client.PlayerRecords()->Records(),[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }); }
//실제 원문47cab0이 선택된 지점에서만 현재 같은 UI의 NSInfo를 갱신한다.
void RefreshClientMetaPacketInfo(Client& client) { if (!client.Menu()) throw std::logic_error("메타 수신의 현재 NSInfo UI 없음");client.Menu()->RefreshNativePacketInfo(); }
//현재 번역 문자열을 기존 표시 인코딩으로 한 번만 변환한다.
std::string TranslateClientMetaPacket(Client& client,std::string_view text) { const auto bytes=o::EncodeDisplayText(client.Translate(text));return {bytes.begin(),bytes.end()}; }
//모든 하위 효과는 Client의 현재 장기 데이터만 잡고 지난 Meta/Session을 캡처하지 않는다.
void ReceiveClientMetaParticipantRemove(Client& client,std::span<const std::uint8_t> packet) {
    if (!client.SurfaceStorage()->ownerMode.challenge) return;if (!client.ChallengeGlobals()->Current()) client.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xd04);
    NativeMetaParticipantRemoveHooks hooks;hooks.checking=client.SurfaceStorage()->Checking();hooks.assertion=[&client](auto expression,auto file,auto line) { client.SurfaceStorage()->AssertDisplay(expression,file,line); };hooks.log=[&client](auto text) { client.WriteDiagnosticLog(9,text); };
    hooks.battle=[&client](auto id) { client.UpdateNativeMetaBattle(id); };hooks.chat=[&client](auto subscriber) { client.ChatChannels().Forget(subscriber); };hooks.leave=[&client](auto object,auto downward) { auto& rig=client.AcquireNativeIslandRig(object,0,1);LeaveNativeIslandRig(rig,downward); };hooks.refreshInfo=[&client] { RefreshClientMetaPacketInfo(client); };
    ReceiveNativeMetaParticipantRemove(*client.ChallengeGlobals(),packet,hooks);
}
//메타 모드·CS 진단 뒤에만 실제 같은 Factory/QA/Pop과 선택 Rig를 연결한다.
void ReceiveClientMetaBattleCreate(Client& client,std::span<const std::uint8_t> packet) {
    if (!client.SurfaceStorage()->ownerMode.challenge) return;if (!client.ChallengeGlobals()->Current()) client.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xce6);
    NativeMetaBattleCreateHooks hooks;BindClientMetaBattleObjects(hooks,client);hooks.animate=[&client](auto object,auto delay) { auto& rig=client.AcquireNativeIslandRig(object,12.0F,0);EnterNativeBattleRig(rig,delay); };ReceiveNativeMetaBattleAdd(*client.ChallengeGlobals(),packet,hooks);
}
//삭제 부모의 진입 CS·실제 이미지 반납·40참가자 검사를 같은 순서로 유지한다.
void ReceiveClientMetaBattleRemove(Client& client,std::span<const std::uint8_t> packet) {
    if (!client.SurfaceStorage()->ownerMode.challenge) return;if (!client.ChallengeGlobals()->Current()) client.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xcdd);
    NativeMetaBattleRemoveHooks hooks;hooks.log=[&client](auto text) { client.WriteDiagnosticLog(9,text); };hooks.assertion=[&client](auto expression,auto file,auto line) { client.SurfaceStorage()->AssertDisplay(expression,file,line); };hooks.leave=[&client](auto object) { auto& rig=client.AcquireNativeIslandRig(object,0.0F,1);LeaveNativeIslandRig(rig,false); };ReceiveNativeMetaBattleRemove(*client.ChallengeGlobals(),packet,hooks);
}
//88h에는 모드/CS0/known/phase/송신자 선행 관문을 넣지 않는다.
void ReceiveClientMetaParticipantName(Client& client,std::span<const std::uint8_t> packet) {
    NativeMetaParticipantNameHooks hooks;hooks.translate=[&client](auto text) { return TranslateClientMetaPacket(client,text); };hooks.weight=[&client](auto core) { return ClientMetaCoreWeight(client,core); };hooks.update=[&client](auto participant,auto mask) { client.UpdateNativeMetaParticipant(participant,mask); };hooks.refreshInfo=[&client] { RefreshClientMetaPacketInfo(client); };ReceiveNativeMetaParticipantName(*client.ChallengeGlobals(),packet,hooks);
}
}
//매 호출 현재 TYPE와 Core 비트를 읽어 같은 unsigned BYTE 합계를 반환한다.
std::uint8_t ClientMetaCoreWeight(Client& client,std::span<const std::uint8_t> core) {
    o::CoreTechnologyWeightHooks hooks;hooks.count=[&client] { return static_cast<std::int32_t>(client.SurfaceStorage()->types.size()); };hooks.type=[&client](auto index) { const auto& type=client.SurfaceStorage()->types.at(static_cast<std::size_t>(index));return o::CoreTechnologyWeightType{type.techBitIndex,type.level}; };return o::CoreTechnologyWeight(core,hooks);
}
//원문 모드 조기 반환/CS 진단 뒤에만 같은 안내/기록 후행을 선택한다.
void ReceiveClientMetaSailError(Client& client,std::span<const std::uint8_t> packet) {
    if (!client.SurfaceStorage()->ownerMode.challenge) return;if (!client.ChallengeGlobals()->Current()) client.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xd33);
    NativeMetaSailErrorHooks hooks;hooks.translate=[&client](auto text) { return TranslateClientMetaPacket(client,text); };hooks.note=[&client](auto object,auto text) { client.ShowNativeObjectNote(object,text); };hooks.tell=[&client](auto section) { if (!client.Menu()) throw std::logic_error("항해 오류의 현재 Tell UI 없음");client.Menu()->ShowNativePacketTell(section); };hooks.update=[&client](auto participant,auto mask) { client.UpdateNativeMetaParticipant(participant,mask); };ReceiveNativeMetaSailError(*client.ChallengeGlobals(),packet,client.GameplayGlobals()->Subscribers()->MySubscriber(),hooks);
}
//기존 RefreshParticipantCore의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
void RefreshClientMetaParticipantCore(Client& client) {
    auto& game=*client.GameplayGlobals();if (game.world.missionPost || !game.player.playerKnown) return;auto& challenge=*client.ChallengeGlobals();const auto own=challenge.Participant(game.player.battleOwner);const auto battle=ReadClientMetaPacket(own,0xa4);if (!battle) return;
    for (std::uint32_t index=0;index<40;++index) { //원문의8묶음×5기록을 같은 인덱스 순서로 읽는다.
        const auto peer=challenge.Participant(index);if (!(peer[0x64]&1U) || ReadClientMetaPacket(peer,0xa4)!=battle) continue;std::array<std::uint8_t,10> core;std::copy_n(peer.begin()+0x54,10,core.begin());client.CoreDataGlobals()->StoreMetadata(ReadClientMetaPacket(peer,0xa8)+1U,ReadClientMetaPacket(peer,4),core); //595370+slot*16은595360의slot+1행이며0번 장부를 덮지 않는다.
    }
}

//기존 ConstructParticipant의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
void ConstructClientMetaParticipant(Client& client,std::span<std::uint8_t> record,std::span<const std::uint8_t> packet) {
    NativeMetaParticipantCreateHooks h;client.BindNativeMetaParticipantDevices(h);h.participantType=[&client] { return client.ChallengeGlobals()->ParticipantType(); };
    h.guildChanged=[&client](auto subscriber) { std::array<std::uint8_t,80> scratch;CurrentAllocationBytes(scratch);const auto key=std::string("M")+std::to_string(std::bit_cast<std::int32_t>(subscriber));return client.Configuration().ReadStringBytes(key,scratch); };
    h.guild=[&client](auto subscriber,auto destination) { std::array<std::uint8_t,80> scratch;CurrentAllocationBytes(scratch);const auto key=std::string("M")+std::to_string(std::bit_cast<std::int32_t>(subscriber));if (client.Configuration().ReadStringBytes(key,scratch)) static_cast<void>(client.Configuration().ReadStringBytes(key,destination)); }; //42ca60은 두 번째 scratch 확인이 바뀐 경우에만 실제 목적을 읽는다.
    h.refreshCore=[&client] { RefreshClientMetaParticipantCore(client); };h.status=[&client](auto kind,auto participant,auto enabled) { client.SetNativeMetaParticipantStatus(kind,std::bit_cast<std::int32_t>(participant),enabled); };
    h.image=[&client](auto header,auto identity) { auto& session=client.GameplayGlobals()->RequireSession();const auto runtimePin=client.GameplayGlobals()->PinCurrentRuntime(session);auto bitmap=session.BuildMetaParticipantImage(std::span<const std::uint8_t,6>(identity));client.ChallengeGlobals()->ConvertImage(header,std::move(bitmap),true,[&client](auto expression,auto line) { client.SurfaceStorage()->AssertDisplay(expression,".\\Image.cpp",line); }); };
    h.name.translate=[&client](auto text) { const auto bytes=o::EncodeDisplayText(client.Translate(text));return std::string(bytes.begin(),bytes.end()); };h.name.weight=[&client](auto core) { return ClientMetaCoreWeight(client,core); };
    h.enter=[&client](auto object,auto self) { auto& rig=client.AcquireNativeIslandRig(object,-12.0F,0);EnterNativeParticipantRig(rig,self,0); };h.intro=[&client] { StartClientMetaIntroCamera(client); };h.noIntro=[&client] { return client.Configuration().GetInt("noShowChalIntro"); };h.tell=[&client](auto section) { if (!client.Menu()) throw std::logic_error("참가자 생성의 현재 Tell UI 없음");client.Menu()->ShowNativePacketTell(section); };h.monitor=[&client](auto name) { client.MonitorMetaParticipantName(name); };
    ConstructNativeMetaParticipant(record,packet,client.GameplayGlobals()->GameAux()->SubscriberMarker(),client.GameplayGlobals()->Subscribers()->MySubscriber(),h);
}

//기존 ReceiveParticipantAdd의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
void ReceiveClientMetaParticipantAdd(Client& client,std::span<const std::uint8_t> packet) {
    if (!client.SurfaceStorage()->ownerMode.challenge) return;auto& challenge=*client.ChallengeGlobals();if (!challenge.Current()) client.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xd0d);
    if (ReadClientMetaPacket(packet,5)==client.CurrentSubscriber()) { if (packet.size()<5) throw std::out_of_range("20h 자기 참가자 BYTE 물리 부족");auto& game=*client.GameplayGlobals();const std::uint32_t metamap=client.SurfaceStorage()->ownerMode.challenge;client.CoreDataGlobals()->Select(game.player,metamap,packet[4],1);++client.LocalStateGlobals()->MetaPending();++client.LocalStateGlobals()->MetaRefresh(); }
    NativeMetaParticipantAddHooks h;h.assertion=[&client](auto expression,auto file,auto line) { client.SurfaceStorage()->AssertDisplay(expression,file,line); };h.construct=[&client](auto record,auto source) { ConstructClientMetaParticipant(client,record,source); };h.battle=[&client](auto id) { client.UpdateNativeMetaBattle(id); };h.seat=MakeClientMetaSailDevices(client).seat;
    h.chat=[&client](auto subscriber,auto name,auto flags) { if (subscriber==client.CurrentSubscriber()) { static_cast<void>(client.ChatChannels().Add(subscriber,std::nullopt,flags));return; }const auto end=std::find(name.begin(),name.end(),0);if (end==name.end()) throw std::out_of_range("20h Chat packet 이름의 실제 NUL 물리 부족");static_cast<void>(client.ChatChannels().Add(subscriber,std::string_view(reinterpret_cast<const char*>(name.data()),static_cast<std::size_t>(end-name.begin())),flags)); }; //자기 채널의 원문 조기 반환에서는 이름을 먼저 읽지 않는다.
    h.update=[&client](auto participant,auto mask) { client.UpdateNativeMetaParticipant(participant,mask); };h.refreshInfo=[&client] { RefreshClientMetaPacketInfo(client); };ReceiveNativeMetaParticipantAdd(challenge,packet,h);
}

//기존 ReceiveParticipantMove의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
void ReceiveClientMetaParticipantMove(Client& client,std::span<const std::uint8_t> packet) {
    if (!client.SurfaceStorage()->ownerMode.challenge) return;if (!client.ChallengeGlobals()->Current()) client.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xd3c);NativeMetaParticipantMoveHooks h;h.owner=[&client] { return client.GameplayGlobals()->player.battleOwner; };h.assertion=[&client](auto expression,auto file,auto line) { client.SurfaceStorage()->AssertDisplay(expression,file,line); };
    h.kick=[&client](auto battle,auto slot) { ExplodeClientMetaBattleSlot(client,battle,slot); };h.battle=[&client](auto id) { client.UpdateNativeMetaBattle(id); };h.kickedSound=[&client] { client.WorldAudio().Sounds().PlayByName("kickedoff.wav",0,0,0,1,0); };h.dropSound=[&client] { client.WorldAudio().Sounds().PlayByName("dropPiece.wav",0,0,0,0,0); };h.thunder=[&client](float x,float y) { client.WorldAudio().Sounds().PlayNameAt(x,y,"thundercrack.wav",0); };h.music=[&client] { client.WorldAudio().RequestMusic("anticipation.mus"); };const auto sail=MakeClientMetaSailDevices(client);h.seat=sail.seat;h.animate=sail.animate;
    h.resetOptions=[](auto options) { o::ResetMissionBattleOptions(o::OriginalEdition::Patch1078,options); };h.sendOptions=[&client](auto battle,auto options) { SendClientMetaData(client,battle,options,87); };h.chatMode=[&client](auto mode) { client.RefreshNativeMetaChatMode(mode); };h.chatMembers=[&client] { RefreshClientBattleChatMembers(client); };h.refreshInfo=[&client] { RefreshClientMetaPacketInfo(client); };h.update=[&client](auto participant,auto mask) { client.UpdateNativeMetaParticipant(participant,mask); };h.refreshCore=[&client] { RefreshClientMetaParticipantCore(client); };
    h.rebuildName=[&client](auto participant) { NativeMetaParticipantNameHooks name;name.translate=[&client](auto text) { const auto bytes=o::EncodeDisplayText(client.Translate(text));return std::string(bytes.begin(),bytes.end()); };name.weight=[&client](auto core) { return ClientMetaCoreWeight(client,core); };RebuildNativeMetaParticipantLabel(client.ChallengeGlobals()->Participant(participant),name); };auto& game=*client.GameplayGlobals();ReceiveNativeMetaParticipantMove(*client.ChallengeGlobals(),{const_cast<std::uint8_t*>(packet.data()),packet.size()},game.Subscribers()->MySubscriber(),game.player.playerKnown,h); //원본packet5 bit8을 같은 실제 입력에만 쓰며 사본/readonly 임시 입력을 만들지 않는다.
}

//기존 ExplodeBattleSlot의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
void ExplodeClientMetaBattleSlot(Client& client,std::uint32_t battle,std::uint32_t slot) {
    NativeMetaBattleSlotHooks h;h.assertion=[&client](auto expression,auto file,auto line) { client.SurfaceStorage()->AssertDisplay(expression,file,line); };h.shape=MakeClientMetaSeatShapeDevices(client);h.raw=[&client](auto object)->std::span<const std::uint8_t> { const auto storage=client.SurfaceStorage();if (object>65535U || object>=storage->pool.Capacity()) throw std::out_of_range("메타 슬롯 효과의 실제 DWORD SID 물리 범위");return storage->pool.Slot(o::Sid{static_cast<std::uint16_t>(object)}); };
    h.ground=[&client](float x,float y) { auto& session=client.GameplayGlobals()->RequireSession();const auto runtimePin=client.GameplayGlobals()->PinCurrentRuntime(session);session.GroundImpact(x,y); };h.sound=[&client](float x,float y) { client.WorldAudio().Sounds().PlayNameAt(x,y,"explodeSlot.wav",0); };ExplodeNativeMetaBattleSlot(*client.ChallengeGlobals(),std::bit_cast<std::int32_t>(battle),std::bit_cast<std::int32_t>(slot),client.GameplayGlobals()->Camera().Snapshot(),h);
}

//기존 ReceiveBattleOptions의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
void ReceiveClientMetaBattleOptions(Client& client,std::uint32_t sender,std::span<const std::uint8_t> packet) {
    auto& game=*client.GameplayGlobals();auto& local=*client.LocalStateGlobals();auto& storage=*client.SurfaceStorage();
    o::ReceiveStateBattleOptions(sender,packet,{*client.GameplayGlobals()->BattleOptionsOwner(),client.ChallengeGlobals()->Current(),game.GameAux()->RefundPercent(),storage.ownerMode.battle,game.world.missionPost,local.SavedGame()},{
        [&client](auto expression) { client.SurfaceStorage()->AssertDisplay(expression,".\\State.cpp",0xd51); },
        [&client](auto,auto bytes) { NativeMetaBattleOptionsHooks receive;receive.owner=[&client] { return client.GameplayGlobals()->player.battleOwner; };receive.log=[&client](auto text) { client.WriteDiagnosticLog(9,text); };receive.guySound=[&client] { client.WorldAudio().Sounds().PlayByName("guy.wav",0,0,0,0,0); };receive.refreshInfo=[&client] { RefreshClientMetaPacketInfo(client); };ReceiveNativeMetaBattleOptions(*client.ChallengeGlobals(),bytes,receive); },
        [&client] { auto& current=*client.GameplayGlobals();o::UpdateMissionOptionSupplyRange(*client.GameplayGlobals()->BattleOptionsOwner(),current.ManaRanges(),current.ManaRadius(),current.energy.radiusSquared); },
        [&client] { return o::MissionOptionTypeCost(*client.GameplayGlobals()->BattleOptionsOwner()); },
        [&client](float cost) { auto& current=*client.SurfaceStorage();const auto type=client.GameplayGlobals()->GameAux()->GeyserType();if (type>=current.types.size()) throw std::out_of_range("메타67h 현재 비용 TYPE 물리 범위");current.types[type].cost=cost; },
        [&client] { return o::MissionOptionRefundPercent(*client.GameplayGlobals()->BattleOptionsOwner()); },[&client] { if (!client.Menu()) throw std::logic_error("67h 현재 HUD 재생성 UI 없음");client.Menu()->RecreateNativePacketHud(); }}); //임시 함수 장치의 수명은 이 전체 동기 호출 끝까지 유지된다.
}

//기존 SeatShapeDevices의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
NativeMetaSeatHooks MakeClientMetaSeatShapeDevices(Client& client) {
        auto& storage=*client.SurfaceStorage();NativeMetaSeatHooks shape;shape.firstFrame=[&storage](auto type) { return storage.frames.at(type).Run('C').first; };
        shape.muzzle=[&storage](auto type,auto frame) { const auto& selected=storage.types.at(type);const auto& current=storage.shapes.at(type);o::RiftShapeAccessHooks access;
            access.typeNumber=[](auto value) { return value.token; };access.frameError=[&storage](auto number,auto index,auto abstract) { storage.RecordDisplayFrameError(number,index,abstract); };access.assertion=[&storage](auto expression,auto file,auto line) { storage.AssertDisplay(expression,file,line); };
            access.shape=[&storage](auto number,auto index) { const auto& value=storage.shapes.at(number);if (!value.loaded || index<0 || static_cast<std::size_t>(index)>=value.frames.size()) throw std::out_of_range("메타 좌석 진단복귀 뒤 SHP 포구 물리 범위");const auto& header=value.frames[static_cast<std::size_t>(index)];return std::array<float,2>{static_cast<float>(header.muzzleX),static_cast<float>(header.muzzleY)}; };
            return o::ReadRiftShapeFrame({type,selected.flags1,current.frameCount,current.loaded,selected.maxHitPoints},frame,0,access);
        };return shape;
}

//기존 SailDevices의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
NativeMetaSailHooks MakeClientMetaSailDevices(Client& client) {
    NativeMetaSailHooks hooks;hooks.subscriber=[&client] { return client.CurrentSubscriber(); };hooks.send=[&client](auto packet) { static_cast<void>(SendClientMetaMutable(client,packet)); };
    hooks.animate=[&client](auto record) { auto& rig=client.AcquireNativeIslandRig(ReadClientMetaPacket(record,0xb8),0,0);MoveNativeParticipantRig(rig,record); };
    hooks.seat=[&client](auto participant,auto battle,auto seat) { const auto shape=MakeClientMetaSeatShapeDevices(client);return NativeMetaSeatPosition(*client.ChallengeGlobals(),participant,battle,seat,client.GameplayGlobals()->Camera().RendererView(),shape); };return hooks;
}

//기존 SendMetaData의 실제 현재 전역/하위 선택 순서를 장기 Client 본문으로 공유한다.
void SendClientMetaData(Client& client,std::uint32_t key,std::span<const std::uint8_t> data,std::uint32_t count) {
    if (std::bit_cast<std::int32_t>(count)>=160) return;if (count>159U) throw std::out_of_range("메타자료의 음수 원문 할당/복사 물리 범위");std::array<std::uint8_t,166> scratch;const auto size=count+7U;auto packet=std::span(scratch).first(size);std::fill(packet.begin(),packet.end(),std::uint8_t{0xff});BeginClientMetaPacket(packet,0x67);WriteClientMetaPacket(packet,2,4,2);WriteClientMetaPacket(packet,4,key,1);WriteClientMetaPacket(packet,5,count,2);
    // rep movsd는각실제DWORD를읽고쓴뒤마지막BYTE들을진행한다.미정다른scratch는초기화하지않는다.
    std::uint32_t offset{};for (;offset+4U<=count;offset+=4U) { if (offset>data.size() || 4U>data.size()-offset) throw std::out_of_range("메타자료의 실제 source DWORD 부족");const std::array<std::uint8_t,4> word{data[offset],data[offset+1],data[offset+2],data[offset+3]};std::copy(word.begin(),word.end(),packet.begin()+7+offset); }
    for (;offset<count;++offset) { if (offset>=data.size()) throw std::out_of_range("메타자료의 실제 source BYTE 부족");packet[7+offset]=data[offset]; } //원문나머지0..3BYTE만추가로쓴다.
    static_cast<void>(SendClientMetaMutable(client,packet)); //같은실제sender관문을통과할때만0040ae80이WORD길이를확정한다.
}

//Native 고정 F와 nonNULL 빈 observer를 Meta/일반/Editor의 장기 기본표에 설치한다.
void InstallClientMetaPacketReceivers(Client& client,o::RawPacketDispatch& dispatch) {
    dispatch.ObservePacket(0x20,[client=&client](auto packet,auto sending) { ObserveNativeMetaParticipantAdd(packet,sending,[client](auto expression,auto line) { client->SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x20,1,0);dispatch.RequireLength(0x20,0xffff);
    dispatch.RegisterCanonicalDirect(0x20,0x004b4710U,[client=&client](auto,auto*,auto packet) { ReceiveClientMetaParticipantAdd(*client,packet); }); //모드 자연 반환 앞에는 Source/Session/packet을 요구하지 않는다.
    dispatch.Observe(0x22,[](std::uint32_t) {});dispatch.Version(0x22,1,0);dispatch.RequireLength(0x22,17);
    dispatch.RegisterCanonicalDirect(0x22,0x004b4820U,[client=&client](auto,auto*,auto packet) { ReceiveClientMetaParticipantMove(*client,packet); }); //같은 mutable frame을 실제42c2e0에 전달한다.
    dispatch.Version(0x67,1,0);dispatch.RequireLength(0x67,0xffff); //40ae80은 기존 공용 실제 송신 관찰기를 유지한다.
    dispatch.RegisterCanonicalDirect(0x67,0x004b4900U,[client=&client](auto sender,auto*,auto packet) { ReceiveClientMetaBattleOptions(*client,sender,packet); });

    dispatch.ObservePacket(0x21,[client=&client](auto packet,auto sending) { ObserveNativeMetaParticipantRemove(packet,sending,[client](auto expression,auto line) { client->SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x21,1,0);dispatch.RequireLength(0x21,0xffff);
    dispatch.RegisterCanonicalDirect(0x21,0x004b46d0U,[client=&client](auto,auto*,auto packet) { ReceiveClientMetaParticipantRemove(*client,packet); }); //저장된 F는 packet type이 바뀌어도 이 본문을 유지한다.
    dispatch.Observe(0x23,[](std::uint32_t) {});dispatch.Version(0x23,1,0);dispatch.RequireLength(0x23,20);
    dispatch.RegisterCanonicalDirect(0x23,0x004b4640U,[client=&client](auto,auto*,auto packet) { ReceiveClientMetaBattleCreate(*client,packet); }); //원문40b380은 비NULL RET이며 성공 ACK를 만들지 않는다.
    dispatch.Observe(0x24,[](std::uint32_t) {});dispatch.Version(0x24,1,0);dispatch.RequireLength(0x24,5);
    dispatch.RegisterCanonicalDirect(0x24,0x004b4600U,[client=&client](auto,auto*,auto packet) { ReceiveClientMetaBattleRemove(*client,packet); });
    dispatch.Observe(0x25,[](std::uint32_t) {});dispatch.Version(0x25,1,0);dispatch.RequireLength(0x25,8);
    dispatch.RegisterCanonicalDirect(0x25,0x004b47e0U,[client=&client](auto,auto*,auto packet) { ReceiveClientMetaSailError(*client,packet); });
    dispatch.Observe(0x88,[](std::uint32_t) {});dispatch.Version(0x88,1,0);dispatch.RequireLength(0x88,98);
    dispatch.RegisterCanonicalDirect(0x88,0x004b4680U,[client=&client](auto,auto*,auto packet) { ReceiveClientMetaParticipantName(*client,packet); });
}
}
