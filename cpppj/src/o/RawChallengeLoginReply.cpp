// 로그→Subscriber→현재 응답코드/28→버전 거부 시12/0→동기 Run을 그대로 수행한다.
#include "o/RawChallengeLoginReply.h"
#include <stdexcept>
#include <string>

namespace netstorm::o { // 실제 분배/장치 등록은 메타 부모가 완성된 뒤 수행한다.
namespace { // 선택된 읽기와 실제 callback만 현재 위치에서 요구한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("1dh 로그인 응답의 실제 장치 미연결");return function; } // 이미 쓴 필드/로그는 실패 시에도 남긴다.
std::uint32_t Read(std::span<const std::uint8_t> packet,std::size_t at,std::size_t width) { //비정렬 x86 필드의 현재 폭만 읽는다.
    if (at>packet.size() || width>packet.size()-at) throw std::out_of_range("1dh 로그인 응답의 실제 패킷 부족");
    std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(packet[at+byte])<<(8*byte);return value; //낮은BYTE부터 조합한다.
}
}
void ReceiveChallengeLoginReply(std::span<const std::uint8_t> packet,LocalMissionStateStorage state,std::uint32_t& subscriber,const ChallengeLoginReplyHooks& h) {
    Need(h.log)("Received ZChalLoginReply, code="+std::to_string(Read(packet,12,1))+"\n");subscriber=Read(packet,8,4);state.current[0x38/4]=Read(packet,12,1);state.current[0]=28;
    if (!Need(h.version)(Read(packet,4,2),Read(packet,6,2))) { state.current[0]=12;state.current[0x38/4]=0; }
    Need(h.run)(); //원문은 거부한 버전도 같은 현재블록으로 즉시 Run한다.
}
}
