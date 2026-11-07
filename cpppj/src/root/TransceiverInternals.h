// R 전송의 여러 번역 단위가 같은 고정 노드/외곽 대여 정의를 공유한다.
#pragma once
#include "root/TransceiverTransport.h"
#include "root/SocketDevice.h"
#include "o/RawPacketFrame.h"
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include <utility>

namespace netstorm::root { // 공개 게임 상태와 분리된 실제 R 전송 물리다.
struct TransceiverTransport::Node {
    o::PacketConnection connection;std::unique_ptr<SocketDevice> socket; // 실제 패킷/OS 수명을 한 안정된 주소에서 보관한다.
    std::vector<std::uint8_t> input,write; // 처음만 ff로 채우는 실제 읽기/쓰기 할당이다. 쓰기 재할당은 읽기 물리를 바꾸지 않는다.
    std::uint32_t token{},bufferSize{},acceptedCapacity{},noMainTimeout{}; // 현재 숫자 토큰/원문 공유 logical크기/수락 버퍼크기/+90이다.
    std::uint32_t received{};std::size_t cursor{4},aggregate{}; // 실제+3c의부분길이/쓰기+68/현재집합+6c의 offset이다.
    std::int32_t writeLimit{100000}; // 원문+70의 최초100000이다.0은 이 관문을 끈다.
    bool listener{},destroying{},destroyed{},retired{}; // 가상 수신 종류와 소멸/논리 반납/물리 수거를 구분한다.
};
struct TransceiverTransport::CallGuard {
    TransceiverTransport& owner; // callback이 재진입해도 같은 외곽 호출 대여를 빌린다.
    explicit CallGuard(TransceiverTransport& value):owner(value) { ++owner.activeCalls_; } // 노드 물리의 반납을 미룬다.
    CallGuard(const CallGuard&)=delete; // 외곽 대여를 복제해 두 번 반납하지 않는다.
    ~CallGuard() { if (!--owner.activeCalls_) owner.Collect(); } // 정상/예외 복귀의 최외곽에서만 실제 수거한다.
};
namespace transceiver_detail { // 동일한 물리 헤더/원문 통지와 선택 장치 검사를 공유한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("R transceiver 실제 장치 미연결");return function; } // 실제로 선택된 효과만 요구한다.
inline std::uint16_t Length(std::span<const std::uint8_t> bytes) { if (bytes.size()<4) throw std::out_of_range("R 패킷 헤더 물리 범위");return static_cast<std::uint16_t>(bytes[2]|(std::uint16_t(bytes[3])<<8U)); } // 선언 WORD를 비정렬로 읽는다.
inline void Header(std::span<std::uint8_t> bytes,std::uint8_t type,std::size_t length) { if (bytes.size()<4 || length>65535) throw std::out_of_range("R 패킷 헤더 쓰기 범위");bytes[1]=type;bytes[2]=static_cast<std::uint8_t>(length);bytes[3]=static_cast<std::uint8_t>(length>>8U); } // BYTE0은 원문444b40/836d0처럼 보존한다.
inline bool Retired(std::span<const std::uint8_t> packet) { // 사용자 지시로 단일/집합 내부의 폐기 검증을 실제 송수신 전에 거른다.
    if (packet.size()<4) throw std::out_of_range("R 검증 정책 헤더 범위");if (packet[1]==0x7f || packet[1]==0x80) return true;if (packet[1]!=4) return false;
    const auto total=Length(packet);if (total<4 || total>packet.size()) throw std::out_of_range("R 집합 송신 길이 범위");
    for (std::size_t at=4;at<total;) { // 한 집합의 실제 내부 패킷 경계만 순회한다.
        if (total-at<4) throw std::out_of_range("R 집합 내부 헤더 범위");const auto length=Length(packet.subspan(at));if (length<4 || length>total-at) throw std::out_of_range("R 집합 내부 길이 범위");if (packet[at+1]==0x7f || packet[at+1]==0x80) return true;if (packet[at+1]==4) throw std::logic_error("R 중첩 집합 미복원");at+=length;
    }
    return false;
}
}
}
