// 원문 단일/집합의 서로 다른 시각/버전/observer/수신 순서와 종료 깊이를 공유한다.
#include "o/RawPacketFrame.h"
#include <stdexcept>
namespace netstorm::o { // OS 소켓/Player와 독립인 프레임 분배 계층이다.
namespace { // 최소 헤더와 폐기된 외부 검증 타입만 분류한다.
constexpr std::size_t kHeader=4; // 원문 단일/집합 패킷의 공통 헤더 물리 폭이다.
constexpr bool IsRetiredCdValidationPacket(std::uint8_t type) { return type==0x7fU || type==0x80U; } // 사용자 지시로 수신/송신에서 은퇴한 검증 쌍이다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width) { // 실제 프레임의 지정 원문 폭을 읽는다.
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("공통 프레임 읽기 범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8U); // 낮은 BYTE부터 합친다.
    return value;
}
}
bool DispatchPacketFrame(RawPacketDispatch& dispatch,PacketConnection& connection,std::span<const std::uint8_t> frame,const PacketDispatchHooks& hooks) {
    if (frame.size()>=kHeader && IsRetiredCdValidationPacket(frame[1])) return !(connection.flags&0x40U); // 단일 검증 요청은 등록 여부와 무관하게 파일/응답 처리 전에 무시한다.
    if (frame.size()<4 || frame[1]!=4) return dispatch.Dispatch(connection,frame);if (connection.flags&0x40U) return false;
    const auto total=Read(frame,2,2);std::size_t cursor=4;std::int32_t delay=-1; //같은 집합의 첫 선택 지연을 뒤 수신기에도 전달한다.
    if (total>frame.size()) { hooks.terminate(connection);return false; }
    // 현재 집합의 원문 순서를 보존하고 callback 뒤 다음 등록/연결 필드를 다시 읽는다.
    while (cursor<total) {
        if (total-cursor<4) { hooks.terminate(connection);return false; }const auto length=Read(frame,cursor+2,2);
        if (!length || length>total-cursor) { hooks.terminate(connection);return false; }const auto packet=frame.subspan(cursor,length);cursor+=length;
        if (!dispatch.Validate(packet)) continue;if (IsRetiredCdValidationPacket(packet[1])) continue; // 집합 내부에서도 폐기된 검증의 기록/observer/수신기를 호출하지 않는다.
        if (packet[1]==4) throw std::logic_error("공통 중첩 집합 패킷 미복원");const auto type=packet[1];
        const auto now=hooks.now();if (type==6) connection.heartbeat=now;else connection.received=now;
        const auto& registration=dispatch.Registration(type);const auto incoming=packet[0]&0x7f;
        if (incoming!=registration.version) {
            const bool lower=incoming<registration.version;if (registration.mismatch&(lower ? 8U : 4U)) { hooks.terminate(connection);return false; }
            if (registration.mismatch&(lower ? 2U : 1U)) continue;
        }
        const auto recordPin=registration.Pin(); // 집합의 record도 실제 전체 Runtime 정리보다 먼저 대여를 올린다.
        hooks.record(connection,packet,static_cast<std::uint16_t>(length));const auto observerEntry=dispatch.Registration(type);observerEntry.NotifyObserver({const_cast<std::uint8_t*>(packet.data()),packet.size()},0); // 실제 수신 frame은 전송이 소유한 mutable vector이며 같은 this를 전달한다.
        const auto selectedDelay=dispatch.DeliverReceiver(type,connection,packet,delay);if (selectedDelay>=0) delay=selectedDelay; //observer 전 저장한 type으로 F를 선택하며 NULL0/음수 반환의 지연 규칙도 보존한다.
    }
    if (cursor!=total) { hooks.terminate(connection);return false; }
    if (!connection.depth && (connection.flags&0x40U)) { hooks.close(connection);hooks.release(connection);return false; }return true;
}
}
