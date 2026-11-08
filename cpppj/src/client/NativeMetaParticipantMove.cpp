// 원문 지역 캡처와 하위 효과 뒤 현재 packet/CS 재조회를 분리하고 부분 효과를 보존한다.
#include "client/NativeMetaParticipantMove.h"
#include "client/NativeMetaSpriteDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //새 active/phase/known 관문으로 원문 이동 분기를 숨기지 않는다.
namespace { //비정렬 원문 폭/float 비트와 선택된 물리만 처리한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("메타 참가자 이동의 실제 장치 미연결");return hook; } //실제 도달한 효과에서만 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("참가자 이동의 실제 읽기 물리 부족");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //지정한 BYTE/WORD/DWORD만 낮은 BYTE부터 조립한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("참가자 이동의 실제 쓰기 물리 부족");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //이웃 플래그와 미작성 필드를 보존한다.
std::span<std::uint8_t> Record(std::span<std::uint8_t> cs,std::uint32_t id,bool battle) { const auto count=battle ? o::kChallengeBattleCount:o::kChallengeParticipantCount,offset=battle ? o::kChallengeBattleOffset:o::kChallengeParticipantOffset,stride=battle ? o::kChallengeBattleBytes:o::kChallengeParticipantBytes;if (cs.size()!=o::kChallengeBytes || id>=count) throw std::out_of_range("참가자 이동의 실제 캡처 CS 범위");return cs.subspan(offset+id*stride,stride); } //같은 this CS의 포인터를 current 조회로 바꾸지 않는다.
std::uint32_t Flags(std::span<const std::uint8_t> packet) { return std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(packet,5,2))))); } //원문의 MOVSX WORD→전체 DWORD 플래그를 보존한다.
std::size_t Slot(std::uint32_t slot) { if (slot>=8) throw std::out_of_range("참가자 이동의 실제 좌석8칸 물리 범위");return 0x38U+slot*4U; } //원문8개 playerId 배열 밖 쓰기는 호스트 경계에서 거부한다.
void Bounds(ClientChallengeGlobals& challenge,std::span<std::uint8_t> cs,std::uint32_t battle,const NativeMetaParticipantMoveHooks& h) { challenge.UpdateBattleVersionBounds(cs,std::bit_cast<std::int32_t>(battle),[&h](auto expression,auto line) { Need(h.assertion)(expression,"\\Ns\\O\\Ch.cpp",line); }); } //표시 뒤에도 캡처CS의 같은 버전을 집계한다.
}
// 지면 효과의 내부 clamp가 뒤 음향에 전달할 원래 위치를 바꾸지 않는다.
void ExplodeNativeMetaBattleSlot(ClientChallengeGlobals& challenge,std::int32_t battle,std::int32_t slot,const RawCameraViewState& snapshot,const NativeMetaBattleSlotHooks& h) {
    const auto b=challenge.Battle(std::bit_cast<std::uint32_t>(battle));if (!(Read(b,0,1)&1U)) { Need(h.assertion)("c->isValid()",".\\Metadisplay.cpp",0x737);if (!(Read(b,0,1)&1U)) return; }
    if (slot<0 || slot>7) Need(h.assertion)("slot >= 0 && slot < MAX_PLAYER_PER_BATTLE",".\\Metadisplay.cpp",0x1a5);if (slot<0 || slot>7) throw std::out_of_range("메타 슬롯 효과의 실제8자리 포구 표 범위");
    const auto raw=Need(h.raw)(Read(b,0x20));const auto type=Read(raw,10,1);const auto frame=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(Need(h.shape.firstFrame)(type))+std::bit_cast<std::uint32_t>(kMetaBattleSeatOrder[static_cast<std::size_t>(slot)]));const auto muzzle=Need(h.shape.muzzle)(type,frame);
    const auto x=static_cast<float>(static_cast<long double>(std::bit_cast<float>(Read(raw,0xe)))+static_cast<long double>(muzzle[0])/std::bit_cast<float>(snapshot.baseCellFloatBits[0]));const auto y=static_cast<float>(static_cast<long double>(std::bit_cast<float>(Read(raw,0x12)))+static_cast<long double>(muzzle[1])/std::bit_cast<float>(snapshot.baseCellFloatBits[1]));Need(h.ground)(x,y);Need(h.sound)(x,y);
}
// 초기 owner/자기 전투/전투변경과 NSInfo 필요 여부는 지역값이며 이후 전역 재조회와 구별한다.
void ReceiveNativeMetaParticipantMove(ClientChallengeGlobals& challenge,std::span<std::uint8_t> packet,const std::uint32_t& subscriber,const std::uint32_t& known,const NativeMetaParticipantMoveHooks& h) {
    const auto cs=challenge.Bytes();const auto r=Record(cs,Read(packet,4,1),false);const auto owner=Need(h.owner)();const auto newBattle=Read(packet,0xf,1);const auto own=Record(cs,owner,false);const auto ownBattle=Read(own,0xa4),oldBattle=Read(r,0xa4);const bool changed=oldBattle!=newBattle;const bool refresh=(Read(packet,4,1)==owner && changed) || (ownBattle && (ownBattle==oldBattle || ownBattle==newBattle));
    if (changed || Read(r,0xa8)!=Read(packet,0x10,1)) {
        if (Read(packet,5,1)&8U) { Need(h.kick)(oldBattle,Read(r,0xa8));if (Read(r,4)==subscriber) Need(h.kickedSound)();if (!Read(r,0xa4)) Need(h.assertion)("p->battle != 0","\\Ns\\O\\Ch.cpp",0x5fb); }
        const auto from=Read(r,0xa4);if (from) { const auto b=Record(cs,from,true);Put(b,Slot(Read(r,0xa8)),0);Need(h.battle)(Read(b,4));Bounds(challenge,cs,Read(r,0xa4),h);Put(r,0x64,Read(r,0x64)&0xfffffffbU); }
        const auto to=Read(packet,0xf,1);if (to) { const auto participant=Read(packet,4,1),slot=Read(packet,0x10,1);const auto b=Record(cs,to,true);const auto at=Slot(slot);if (Read(b,at)) Need(h.assertion)("!playerIds[slot]","\\Ns\\O\\Ch.cpp",0x1c8);Put(b,at,participant);Need(h.battle)(Read(b,4));Bounds(challenge,cs,Read(packet,0xf,1),h);const auto y=Read(r,0xa0),x=Read(r,0x9c);Need(h.thunder)(std::bit_cast<float>(x),std::bit_cast<float>(y));if (known && Read(packet,4,1)==Need(h.owner)()) Need(h.music)(); }
    }
    Put(packet,5,Read(packet,5,1)&0xf7U,1);const auto difference=Flags(packet)^Read(r,0x64);if ((difference&2U) && Read(packet,0xf,1) && ownBattle==Read(packet,0xf,1)) Need(h.dropSound)();Put(r,0x64,Flags(packet));
    const auto battle=Read(packet,0xf,1);std::array<std::uint32_t,2> position{Read(packet,7),Read(packet,0xb)};Put(r,0xa4,battle);Put(r,0xa8,Read(packet,0x10,1));if (battle) { const auto point=Need(h.seat)(r,battle,Read(r,0xa8));position={std::bit_cast<std::uint32_t>(point[0]),std::bit_cast<std::uint32_t>(point[1])}; }
    if (std::bit_cast<float>(Read(r,0x9c))!=std::bit_cast<float>(position[0]) || std::bit_cast<float>(Read(r,0xa0))!=std::bit_cast<float>(position[1])) { Put(r,0x9c,position[0]);Put(r,0xa0,position[1]);Need(h.animate)(r); }Put(r,0x9c,position[0]);Put(r,0xa0,position[1]); //Rig callback 뒤에도 같은 지역 좌표를 다시 쓴다.
    if ((difference&4U) && (Read(packet,5,1)&4U) && Read(packet,0xf,1) && Read(packet,4,1)==owner) { Need(h.resetOptions)(Record(cs,Read(packet,0xf,1),true).subspan(0x59,87));const auto current=Read(packet,0xf,1);Need(h.sendOptions)(current,Record(cs,current,true).subspan(0x59,87)); }
    if (changed) { if (Read(packet,4,1)==owner) { Need(h.chatMode)(0);if (Read(r,0xa4)) Need(h.chatMode)(1); }else if (Read(own,0xa4)) Need(h.chatMembers)(); }
    if (refresh) Need(h.refreshInfo)();Need(h.update)(Read(packet,4,1),0xff);Need(h.refreshCore)();Need(h.rebuildName)(Read(packet,4,1));
}
}
