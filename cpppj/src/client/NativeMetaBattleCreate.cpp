// 호출부가값으로전달한식별자/좌표/마지막표시를보존하고원문순서로부분효과를수행한다.
#include "client/NativeMetaBattleCreate.h"
#include <bit>
#include <stdexcept>
#include <string>

namespace netstorm::client { //실제공장/QA/owner/가상Pop·Rig가연결되기전에는등록/성공을합성하지않는다.
namespace { //호스트패딩을원문기록에복사하지않고실제1/2/4BYTE폭만처리한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("메타전투 생성의 실제 장치 미연결");return hook; } //선택한원문효과위치에서만장치를요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타전투 생성의 실제 필드 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //원문비정렬필드를낮은BYTE부터읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타전투 생성의 실제 필드 쓰기 범위");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //선택한폭만써서식별자패딩/이미지헤더등미작성필드를보존한다.
}
// 입력전체는값으로캡처하여하위효과가caller입력을변경하더라도원문스택인자를유지한다.
void ConstructNativeMetaBattle(std::span<std::uint8_t> record,NativeMetaBattleCreateData input,const NativeMetaBattleCreateHooks& h) {
    if (Read(record,0,1)&1U) Need(h.assertion)("!isValid()","\\Ns\\O\\Ch.cpp",0x1d1);
    Put(record,4,input.id);Put(record,8,Read(input.identity,0));Put(record,0xc,Read(input.identity,4,2),2);Put(record,0x10,input.position[0]);Put(record,0,1);Put(record,0x14,input.position[1]);
    for (std::uint32_t slot=0;slot<8;++slot) Put(record,0x38U+slot*4U,0); //원문여덟playerId DWORD를같은순서로만지운다.
    Put(record,0x18,0xffffffffU);Put(record,0x1c,0x100U);const auto type=Need(h.battleType)();const auto object=Need(h.create)(type,2);Need(h.setKind)(object,Read(record,4));const auto raw=Need(h.raw)(object);Put(raw,8,0,2);Need(h.owner)(object,0);
    const auto y=Read(record,0x14),x=Read(record,0x10);Need(h.pop)(object,std::bit_cast<float>(x),std::bit_cast<float>(y),0);Put(record,0x20,object);if (!input.existing) Need(h.animate)(object,10);
}
// 첫id/active관문뒤에는스택호출순서대로기존표시/y/x/식별자DWORD·WORD를읽는다.
void ReceiveNativeMetaBattleAdd(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleCreateHooks& h) {
    const auto id=Read(packet,4,1);const auto record=challenge.Battle(id);if (Read(record,0,1)&1U) { Need(h.log)("ERROR in handleChalBattleAdd: battle "+std::to_string(id)+" already exists!\n");return; }
    NativeMetaBattleCreateData input;input.id=id;input.existing=Read(packet,5,1)&2U;input.position[1]=Read(packet,0x10);input.position[0]=Read(packet,0xc);const auto word=Read(packet,6),tag=Read(packet,10,2);Put(input.identity,0,word);Put(input.identity,4,tag,2);ConstructNativeMetaBattle(record,input,h);
}
}
