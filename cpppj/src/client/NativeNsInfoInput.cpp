// 원문은 모드/known 관문을 추가하지 않으며 실패한 행은 원래 입력으로 기본 처리를 계속한다.
#include "client/NativeNsInfoInput.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // CS 뷰/현재 표는 호출 중 같은 실제 owner의 주소를 빌린다.
namespace { // 선택되지 않은 장치를 요구하지 않고 원문 BYTE/DWORD 폭을 유지한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("NSInfo 루트 입력의 실제 장치 미연결");return hook; } // 효과를 선택한 원문 위치에서만 callback을 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { if (offset>bytes.size() || 4>bytes.size()-offset) throw std::out_of_range("NSInfo 루트 입력의 실제 CS DWORD 부족");return std::uint32_t(bytes[offset])|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24); } // 비정렬 물리를 같은 낮은 BYTE부터 조립한다.
bool Active(std::span<const std::uint8_t> record) { if (record.size()<=0x64) throw std::out_of_range("NSInfo 루트 입력의 실제 CS flag BYTE 부족");return (record[0x64]&1U)!=0; } // DWORD 대신 원문의 test BYTE 한 개만 읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { if (offset>bytes.size() || 4>bytes.size()-offset) throw std::out_of_range("NSInfo 루트 입력의 실제 패킷 DWORD 부족");for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } // 원문의 DWORD 저장을 같은 네 BYTE에만 기록한다.
}
// 비교만 26비트로 마스킹하며 기본 처리에는 수정하지 않은 event/first/second를 전달한다.
std::uint32_t HandleNativeNsInfo(std::uint32_t event,std::uint32_t first,std::uint32_t second,const NativeNsInfoInputHooks& h) {
    const auto code=event&0x03ffffffU;
    if (code==Need(h.command)(0x542790) && std::bit_cast<std::int32_t>(second)>0) {
        const auto index=Need(h.roster)(second);const auto record=Need(h.participant)(index);
        if (Active(record)) { const auto point=Need(h.mouse)();Need(h.menu)(index,point);return 1; }
    }
    if (code==Need(h.command)(0x54278c) && std::bit_cast<std::int32_t>(second)>0) {
        const auto index=Need(h.roster)(second);const auto record=Need(h.participant)(index);
        if (Active(record)) { Need(h.center)(Read(record,0xb8));return 1; }
    }
    if (code==Need(h.command)(0x5429dc) && std::bit_cast<std::int32_t>(second)>0) {
        const auto record=Need(h.participant)(second);
        if (Active(record)) { Need(h.status)([record,&h](auto packet) { Put(packet,4,Read(record,4));Put(packet,8,8);Need(h.kickoffSound)(); });return 1; }
    }
    return Need(h.base)(event,first,second);
}
}
