// 일반운반체의4f 포획요청과실제서버객체→내용물변환을복원한다.
#pragma once
#include "o/RawContainedFinder.h"
#include "o/RawFormPacket.h"
#include <functional>
namespace netstorm::o {
// 헤더/명령/특정타입은효과뒤현재값을다시읽는다.
struct CarrierPickupState {
    FormPacketHeader request{0x4f,1,9}; //40a9a0의현재BYTE/BYTE/WORD다.
    std::uint32_t command{0x030000b1},silentType{168}; //5429b8의입력명령과5412f8의기존내용물삭제후보충생략타입이다.
};
// 큰게임하위와장치경계만빌리며서버객체의dead와종속이동순서는본문이소유한다.
struct CarrierPickupHooks {
    std::function<std::pair<Sid,bool>(Sid,float,float)> candidate; //426900의실제현재지점후보다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //4e2dc0(9,packet)의실제전송이다.요청에서로컬수신하지않는다.
    std::function<void(std::uint32_t)> assertion; //Carrier.cpp의20c 권한진단이며복귀뒤현재boss를다시읽는다.
    std::function<Sid(Sid)> reveal; //4620c0의오벨리스크보상객체변환이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> queue; //452fe0(현재명령,현재대상,packet부모WORD)다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> find; //4ac9f0(parent,1,0,0)의현재기존내용물이다.
    std::function<void()> afterDiscard; //485e40의원본인자없는전역보충부모다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> create; //4afa70의같은실제ContainedForm 생성·부착이다.
    std::function<void(Sid,std::uint32_t)> destroy,detach; //현재가상+10/+48을flags0으로실행한다.
    std::function<void(Sid,Sid,std::uint32_t)> attach; //현재가상+50(newContent,0)이다.
};
// 같은실제풀/TYPE/내용물전역과현재권한·미션모드를공유한다.
class RawCarrierPickup {
public:
    RawCarrierPickup(SidPool& pool,std::span<const RiftTypeRecord> types,const ContainedFinderState& contained,
        const bool& boss,const bool& single,CarrierPickupState& state,CarrierPickupHooks hooks); //PATCH10.78의장기참조를받는다.
    void Request(Sid parent) const; //428e00의후보/현재권한/미션/헤더/서버요청이다.
    void Receive(std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> packet) const; //426520은선행owner/flags를읽지않는다.
    void Discard(Sid parent) const; //4264d0의현재첫mask1 삭제와현재특정타입비교다.
    const SidPool& Pool() const; //다른월드의생성·수명처리기를섞지않는다.
private:
    std::uint32_t Genus(Sid sid) const; //현재BYTE TYPE의실제flags2를읽는다.
    void Dead(Sid sid,bool value) const; //유효물리슬롯의+11에현재다른비트를보존해bit2만쓴다.
    SidPool& pool_; //실제지도객체/내용물의단일물리저장소다.
    std::span<const RiftTypeRecord> types_; //효과뒤현재TYPE를다시조회한다.
    const ContainedFinderState& contained_; //서버종속이동의현재타입/권한/진단이다.
    const bool& boss_; //요청과서버의현재권한이다.
    const bool& single_; //594fa4의현재단독미션모드다.5c85a4 편집모드와구별한다.
    CarrierPickupState& state_; //현재명령과요청헤더/특정타입전역이다.
    CarrierPickupHooks hooks_; //선택한실제하위가없으면명시거부한다.
};
}
