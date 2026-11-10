// 선언 len과 별개인 현재 첫 NUL을 사용하며 수신에 송신 길이 검사를 추가하지 않는다.
#include "o/RawDeveloperCommandPacket.h"
#include "o/RawPacketDispatch.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 실제 Session/입력의 실행기 수명만 빌린다.
void ObserveDeveloperCommandPacket(std::span<std::uint8_t> packet,std::uint32_t sending,const RemoteTellAssertion& assertion) {
    if (!sending) return;if (packet.size()<8) throw std::out_of_range("개발자 명령의 현재 len DWORD 부족");
    const auto length=[packet] { return std::uint32_t(packet[4])|(std::uint32_t(packet[5])<<8)|(std::uint32_t(packet[6])<<16)|(std::uint32_t(packet[7])<<24); }; // 진단이 돌아온 뒤 동일 현재 바이트를 다시 읽는다.
    if (length()==0xffffffffU) { if (!assertion) throw std::logic_error("개발자 명령 len 진단 장치 미연결");assertion("len != (int)-1","\\Ns\\O\\_p_packets.cpp",0x74b); }
    const auto count=length()+8U;packet[2]=static_cast<std::uint8_t>(count);packet[3]=static_cast<std::uint8_t>(count>>8); // 현재 하위WORD의 감김을 보존한다.
}
void RegisterDeveloperCommandPacket(RawPacketDispatch& dispatch,std::function<void(std::string_view)> execute,RemoteTellAssertion assertion) {
    dispatch.Version(0x31,1,0);dispatch.RequireLength(0x31,0xffff);dispatch.ObservePacket(0x31,[assertion=std::move(assertion)](auto packet,auto sending) { ObserveDeveloperCommandPacket(packet,sending,assertion); });
    dispatch.Register(0x31,[execute=std::move(execute)](auto,auto&,auto packet) { ReceiveDeveloperCommandPacket(packet,execute); });
}
// 기존 등록과 Native 고정 F가 같은 첫 NUL/물리 범위/현재 문자열 효과 본문을 공유한다.
void ReceiveDeveloperCommandPacket(std::span<const std::uint8_t> packet,const std::function<void(std::string_view)>& execute) { //44dba0→40a6f0(packet,0)은 sender/len DWORD를 읽지 않는다.
        if (packet.size()<9) throw std::out_of_range("개발자 명령 문자열 물리 부족");const auto body=packet.subspan(8);const auto end=std::find(body.begin(),body.end(),0);if (end==body.end()) throw std::out_of_range("개발자 명령 문자열 NUL 누락");
        if (!execute) throw std::logic_error("개발자 명령 실제 해석기 미연결");execute({reinterpret_cast<const char*>(body.data()),static_cast<std::size_t>(end-body.begin())});
    }

}
