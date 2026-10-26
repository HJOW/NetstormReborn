// 실제 Native 직접 store 위치만 바인딩하며 새 주소 사전·전역 복사·UI 갱신 효과를 만들지 않는다.
#include "client/ClientStateWorldGlobals.h"
#include "client/ClientMain.h"
#include "client/ClientAudio.h"
#include "client/CampaignTemplateStorage.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientNsInfoGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientRandomGlobals.h"
#include "client/RawMenuControls.h"
#include "client/RawSurfaceWorld.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // Client 수명 물리만 빌리고 Session 객체 자체는 보관하지 않는다.
namespace { // Native 주소의 직접 읽기·폭별 쓰기만 구현하는 지역 연결 함수다.
// 기존 실제 bool 소비자는 0/1 모델이다. 임의 DWORD 비트 보존을 가장하지 않고 잘못된 폭을 쓰기 전에 거부한다.
bool StateBoolean(std::uint32_t value) { if (value>1) throw std::invalid_argument("State bool 전역의 정상0/1 밖 쓰기");return value!=0; }
// 동일한 22DWORD 표 안의 정렬된 Native 주소만 실제 인덱스로 변환한다.
bool ActivityWord(std::uint32_t address) { return address>=0x5c84e8U && address<0x5c8540U && (address-0x5c84e8U)%4U==0; }
// 전체 직접 읽기는 각 호출에서 같은 현재 owner 필드를 다시 조회한다.
std::uint32_t ReadStateWord(const ClientStateWorldContext& c,std::uint32_t address) {
    auto& game=*c.client.GameplayGlobals();auto& local=*c.client.LocalStateGlobals();auto& aux=*game.GameAux();
    if (ActivityWord(address)) return c.client.ConnectionActivityGlobals()->Frames()[(address-0x5c84e8U)/4U];
    switch (address) {
    case 0x540bc0:return game.Server();case 0x540bc4:return game.Boss();
    case 0x540bdc:return std::bit_cast<std::uint32_t>(aux.DefaultSp());case 0x540c70:return game.player.battleOwner;case 0x540cac:return game.player.templateOwner;case 0x541240:return aux.GeyserType();
    case 0x5424bc:return aux.RefundPercent();case 0x542534:return local.CachedPeerIndex();case 0x542538:return local.CachedPeerValue();case 0x54253c:return local.CachedPeerFieldA4();case 0x542540:return local.CachedPeerFieldA8();case 0x542544:return local.ScriptBreak();
    case 0x5453e4:return std::bit_cast<std::uint32_t>(aux.AllowAnyCapture());case 0x5467b8:return c.client.NsInfoGlobals()->zone;case 0x5467bc:return aux.SubscriberMarker();
    case 0x54c8a0:return c.surface.chunks.Mode();case 0x54db14:return game.FullDeck();case 0x55a420:return local.SourceType();
    case 0x5949d4:return game.MapRevision();case 0x594fa0:return c.surface.links.mission;case 0x594fa4:return game.world.missionPost;
    case 0x594fb8:return c.surface.ownerMode.fort;case 0x594fbc:return c.surface.ownerMode.battle;case 0x594fc8:return c.surface.ownerMode.challenge;
    case 0x595088:return aux.ResourcePoolBits();case 0x59508c:return std::bit_cast<std::uint32_t>(aux.TotalCost());case 0x595090:return aux.SavedGeyserCount();case 0x5950ac:return aux.OrdinaryGeysers();case 0x5950b0:return aux.GeyserFrameFlag();case 0x595344:return game.player.playerKnown;
    case 0x59a8ac:return game.HideBuildings();case 0x59a8b0:return c.client.KencloudGlobals()->WorldRedraw();
    case 0x5c7a74:return local.Begin();case 0x5c848c:return std::bit_cast<std::uint32_t>(aux.LostX());case 0x5c8490:return std::bit_cast<std::uint32_t>(aux.LostY());
    case 0x5c85a0:return local.CentralCreated();case 0x5c85a4:return game.world.editor;case 0x5c85b4:return local.ConnectionNotified();case 0x5c85c0:return local.ZonePending();case 0x5c85c8:return local.MetadataMode();case 0x5c85cc:return local.StateAuxCC();case 0x5c85d4:return local.SavedGame();case 0x5c85e0:return local.AbortPending();
    case 0x5c8758:return local.State().phase[0];case 0x5c8bb0:return aux.Archipelago();case 0x5ca8f0:return aux.PriestQuarterHp();case 0x5ca938:return c.menus.PopupState().command;
    default:throw std::out_of_range("State 직접 읽기의 미등록 Native DWORD 주소");
    }
}
// 원문 직접 쓰기는 해당 저장소 한 필드만 바꾸며 효과 callback·후속 재초기화를 실행하지 않는다.
void WriteStateWord(const ClientStateWorldContext& c,std::uint32_t address,std::uint32_t value) {
    auto& game=*c.client.GameplayGlobals();auto& local=*c.client.LocalStateGlobals();auto& aux=*game.GameAux();
    if (ActivityWord(address)) { c.client.ConnectionActivityGlobals()->Frames()[(address-0x5c84e8U)/4U]=value;return; }
    switch (address) {
    case 0x540bc0:game.Server()=StateBoolean(value);return;case 0x540bc4:game.Boss()=StateBoolean(value);return;
    case 0x5424bc:aux.RefundPercent()=value;return;case 0x542534:local.CachedPeerIndex()=value;return;case 0x542538:local.CachedPeerValue()=value;return;case 0x54253c:local.CachedPeerFieldA4()=value;return;case 0x542540:local.CachedPeerFieldA8()=value;return;case 0x542544:local.ScriptBreak()=value;return;
    case 0x5453e4:aux.AllowAnyCapture()=std::bit_cast<std::int32_t>(value);return;case 0x5467b8:c.client.NsInfoGlobals()->zone=value;return;case 0x5467bc:aux.SubscriberMarker()=value;return;
    case 0x54c8a0:c.surface.chunks.Mode()=value;return;case 0x54db14:game.FullDeck()=value;return;case 0x55a420:local.SourceType()=value;return;
    case 0x5949d4:game.MapRevision()=value;return;case 0x594fa4:game.world.missionPost=StateBoolean(value);return;
    case 0x594fb8:c.surface.ownerMode.fort=StateBoolean(value);game.player.fortMode=c.surface.ownerMode.fort;return; //같은 Native 주소의 기존 Player 어댑터 별칭도 동일 쓰기를 반영한다.
    case 0x594fbc:c.surface.ownerMode.battle=StateBoolean(value);c.surface.links.battle=c.surface.ownerMode.battle;return; //같은00594fbc을 읽는 기존 Bridge 어댑터 별칭이다.
    case 0x594fc8:c.surface.ownerMode.challenge=StateBoolean(value);return;
    case 0x595088:aux.StoreResourcePoolBits(value);return;case 0x59508c:aux.TotalCost()=std::bit_cast<std::int32_t>(value);return;case 0x595090:aux.SavedGeyserCount()=value;return;case 0x5950ac:aux.OrdinaryGeysers()=value;return;case 0x5950b0:aux.GeyserFrameFlag()=value;return;case 0x595344:game.player.playerKnown=value;return;
    case 0x59a8ac:game.HideBuildings()=value;return;case 0x59a8b0:c.client.KencloudGlobals()->WorldRedraw()=value;return;
    case 0x5c7a74:local.Begin()=value;return;case 0x5c848c:aux.LostX()=std::bit_cast<float>(value);return;case 0x5c8490:aux.LostY()=std::bit_cast<float>(value);return;
    case 0x5c85a0:local.CentralCreated()=value;return;case 0x5c85a4:game.world.editor=value;return;case 0x5c85b4:local.ConnectionNotified()=value;return;case 0x5c85c0:local.ZonePending()=value;return;case 0x5c85c8:local.MetadataMode()=value;return;case 0x5c85cc:local.StateAuxCC()=value;return;case 0x5c85d4:local.SavedGame()=StateBoolean(value);return;case 0x5c85e0:local.AbortPending()=value;return;
    case 0x5c8bb0:aux.Archipelago()=value;return;case 0x5ca8f0:aux.PriestQuarterHp()=StateBoolean(value);return;case 0x5ca938:c.menus.PopupState().command=value;return;
    default:throw std::out_of_range("State 직접 쓰기의 미등록 Native DWORD 주소");
    }
}
// double 쓰기의 비트/폭은 유지하고 음악·UI·시계 효과를 추가하지 않는다.
void WriteStateWide(const ClientStateWorldContext& c,std::uint32_t address,std::uint64_t bits) {
    auto& local=*c.client.LocalStateGlobals();auto& aux=*c.client.GameplayGlobals()->GameAux();const auto value=std::bit_cast<double>(bits);
    switch (address) {
    case 0x565e68:c.audio.WriteSongEndBits(bits);return;
    case 0x5950b8:aux.GeyserDeadline()=value;return;case 0x5950c0:aux.CleanupDeadline()=value;return;case 0x5950c8:aux.SecondaryGeyserDeadline()=value;return;case 0x595358:aux.ScheduledDrawDeadline()=value;return;
    case 0x5c85f0:local.LoginEnd()=value;return;case 0x5c85f8:local.ConnectEnd()=value;return;case 0x5c8610:local.ReconnectSaveEnd()=value;return;case 0x5c8618:local.CaptureEnd()=value;return;
    default:throw std::out_of_range("State 직접 쓰기의 미등록 Native double 주소");
    }
}
}
// 실제 Client의 동일 소유자만 받아 원문 전체 직접 접근을 반환한다. 조회 자체는 Native 정리를 실행하지 않는다.
o::StateWorldCleanupGlobals BindClientStateWorldGlobals(ClientStateWorldContext c) {
    const auto& current=c.client.SurfaceStorageIfInitialized(); // 초기 Client의 NULL은 직접 구성한 같은 권한·GameAux 표면을 허용하며 조회가 새 저장소를 만들지 않는다.
    if (c.surface.pool.Edition()!=o::OriginalEdition::Patch1078 || c.surface.Authority()!=c.client.GameplayGlobals()->Authority() || c.surface.GameAux()!=c.client.GameplayGlobals()->GameAux() || (current && current.get()!=&c.surface) || &c.audio!=&c.client.WorldAudio())
        throw std::invalid_argument("State 전역의 같은 실제 Client/표면/권한/음향 owner 필요");
    o::StateWorldCleanupGlobals result;
    result.read32=[c](auto address) { return ReadStateWord(c,address); };result.write32=[c](auto address,auto value) { WriteStateWord(c,address,value); };
    result.write16=[c](auto address,auto value) { if (address!=0x5c89b4U) throw std::out_of_range("State 직접 쓰기의 미등록 Native WORD 주소");auto& seed=c.client.RandomGlobals()->FortCoreSeed();seed=(seed&0xffff0000U)|value; };
    result.write64Bits=[c](auto address,auto bits) { WriteStateWide(c,address,bits); };
    result.players=[c] { return c.client.PlayerRecords()->Records(); }; // 원문 각 DWORD store가 재할당 후 현재 base를 다시 읽는다.
    result.templateManager=[c](auto slot) { if (slot>=9) throw std::out_of_range("State Template9의 슬롯 범위");const auto bytes=c.templates.Headers();std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[slot*20+index])<<(8*index);return value; }; // 현재 header 첫 DWORD만 읽는다.
    result.typeDefault=[c](auto type)->float& { return c.surface.types.at(type).cost; }; // 자산 원본 사본 대신 수정 가능한 실제 TYPE float를 빌린다.
    return result;
}
}
