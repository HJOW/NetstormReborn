// packet 길이 재조회와 CS0일 때 실제 자식을 요구하지 않는 원문 State 부모를 유지한다.
#include "o/RawStateMetaBattleDescription.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 실제 등록표가 반환한 전체 packet의 물리 범위만 선택 위치에서 검사한다.
namespace { //도달한 필드/장치만 읽고 쓰며 전체 packet을 선검사하지 않는다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("전투 설명 State의 선택된 실제 장치 미연결");return value; } //CS0/정상 길이는 미사용 callback을 요구하지 않는다.
std::uint8_t Byte(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>=bytes.size()) throw std::out_of_range("전투 설명 observer BYTE 물리 범위");return bytes[at]; } //선택한 현재BYTE를 읽는다.
void Length(std::span<std::uint8_t> bytes,std::uint16_t length) { if (bytes.size()<4) throw std::out_of_range("전투 설명 observer WORD 물리 범위");bytes[2]=static_cast<std::uint8_t>(length);bytes[3]=static_cast<std::uint8_t>(length>>8); } //현재packet+2의WORD만 쓴다.
}
// receive/진단 함수는 등록표가 소유하고 현재 CS 참조는 실제 외부 owner가 유지한다.
void RegisterStateMetaBattleDescription(RawPacketDispatch& dispatch,const std::uint32_t& currentCs,StateMetaBattleDescriptionReceive receive,StateMetaBattleDescriptionAssert assertion) {
    const auto baseline=dispatch.Registration(0x71).canonicalReceive; //실제Client기본F를값으로선택하며현재packet[1]이나genericReceive를중계하지않는다.
    if (baseline && baseline->nativeEntry==0x004b49c0U) { dispatch.RegisterCanonical(0x71,baseline);return; } //동일F만이번Session에대여하고장기observer/원문metadata는그대로유지한다.독립입력은기존본문을사용한다.
    dispatch.ObservePacket(0x71,[assertion=std::move(assertion)](auto packet,auto sending) { if (!sending) return;if (Byte(packet,9)==0xffU) Need(assertion)("descLen != (unsigned char)-1");Length(packet,static_cast<std::uint16_t>(std::uint32_t(Byte(packet,9))+10U)); });dispatch.Version(0x71,1,0);dispatch.RequireLength(0x71,0xffff);
    dispatch.Register(0x71,[&currentCs,receive=std::move(receive)](auto sender,auto&,auto packet) { if (currentCs) Need(receive)(sender,packet); }); //원문은 mode/sender/known 관문 없이 현재 CS 비영일 때만 자식을 호출한다.
}
}
