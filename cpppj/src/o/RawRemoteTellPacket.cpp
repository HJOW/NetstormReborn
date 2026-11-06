// 원문은 수신에서tellLength를 검사하지 않고+8의첫NUL을 현재Tell에 전달한다.
#include "o/RawRemoteTellPacket.h"
#include "o/RawPacketDispatch.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
namespace netstorm::o { // 버전/기록/생존 관문은 기존 실제 분배기에 맡긴다.
namespace { // 비정렬 현재DWORD와 선택된 효과를 처리한다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("원격 Tell 실제 장치 미연결");return effect; } // 선택되지 않은 진단은 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes) { if (bytes.size()<8) throw std::out_of_range("원격 Tell 헤더 물리 누락");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[4+index])<<(index*8);return value; } // 현재+4의네BYTE만읽는다.
}
std::array<std::uint8_t,264> WriteRemoteTellPacket(std::string_view text,const RemoteTellAssertion& assertion,std::uint32_t line) {
    text=text.substr(0,text.find('\0'));std::array<std::uint8_t,264> packet;packet.fill(0xff);packet[0]=1;packet[1]=0x70;packet[2]=4;packet[3]=0;const auto length=text.size()+1;
    if (length>0xffffffffU) throw std::out_of_range("원격 Tell 실제 문자열 길이 범위");for (std::size_t index=0;index<4;++index) packet[4+index]=static_cast<std::uint8_t>(length>>(index*8)); // 원문의현재strlen+1 DWORD를진단전에쓴다.
    if (length>255) Need(assertion)("z->tellLength < 256",".\\Ui.cpp",line);if (length>256) throw std::out_of_range("원격 Tell264BYTE 복사 물리 범위");std::copy(text.begin(),text.end(),packet.begin()+8);packet[8+text.size()]=0;return packet;
}
void ObserveRemoteTellPacket(std::span<std::uint8_t> packet,std::uint32_t sending,const RemoteTellAssertion& assertion) {
    if (!sending) return;if (Read(packet)==0xffffffffU) Need(assertion)("tellLength != (int)-1","\\Ns\\O\\_p_packets.cpp",0xac7);const auto length=Read(packet)+8U;packet[2]=static_cast<std::uint8_t>(length);packet[3]=static_cast<std::uint8_t>(length>>8); // 진단뒤현재+4를재조회하고하위WORD만쓴다.
}
void RegisterRemoteTellPacket(RawPacketDispatch& dispatch,std::function<void(std::string_view)> tell,RemoteTellAssertion assertion) {
    dispatch.Version(0x70,1,0);dispatch.RequireLength(0x70,0xffff);dispatch.ObservePacket(0x70,[assertion=std::move(assertion)](auto packet,auto sending) { ObserveRemoteTellPacket(packet,sending,assertion); });
    dispatch.Register(0x70,[tell=std::move(tell)](auto,auto&,auto packet) { if (packet.size()<9) throw std::out_of_range("원격 Tell 실제 문자열 물리 누락");const auto body=packet.subspan(8);const auto end=std::find(body.begin(),body.end(),0);if (end==body.end()) throw std::out_of_range("원격 Tell 실제 NUL 누락");Need(tell)({reinterpret_cast<const char*>(body.data()),static_cast<std::size_t>(end-body.begin())}); }); // cf980은sender/lengthDWORD를읽지않는다.
}
}
