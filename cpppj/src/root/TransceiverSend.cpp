// R의 직접/집합 송신과 flush·부분 쓰기 포인터를 같은 실제 노드에 연결한다.
#include "root/TransceiverInternals.h"
#include <algorithm>

namespace netstorm::root { // packet observer/기록과 OS 쓰기의 원문 순서를 보존한다.
using transceiver_detail::Need;
void TransceiverTransport::Send(std::uint32_t token,std::span<std::uint8_t> packet) { const CallGuard guard(*this);Send(Get(token),packet,nullptr); } // 가상+4의 실제 현재 노드를 선택한다.
void TransceiverTransport::SendTo(std::uint32_t token,std::span<std::uint8_t> packet,std::span<const std::uint8_t,20> destination) {
    const CallGuard guard(*this);std::array<std::uint8_t,20> address{};std::copy(destination.begin(),destination.end(),address.begin());Send(Get(token),packet,&address); // 현재 목적지의20BYTE를 동기 OS 호출까지 고정한다.
}
void TransceiverTransport::Send(Node& node,std::span<std::uint8_t> packet,const std::array<std::uint8_t,20>* destination) {
    if (transceiver_detail::Retired(packet)) return;
    if (destination && !(node.socket->Flags()&0x220U)) Need(hooks_.assertion)("getFlags() & zoDGRAMBASED","c:\\ns\\zacket\\zacket.cpp",1032);
    const auto type=packet[1];const auto entry=dispatch_.Registration(type);entry.NotifyObserver(packet,1); //835f0=0/83610=no-op이므로 Byte0의80을 송신 완료 marker로 사용하지 않는다.
    if (node.retired || node.destroyed) return;const auto length=transceiver_detail::Length(packet);if (length>packet.size()) throw std::out_of_range("R 송신 선언/물리 길이 범위");
    if (destination) {
        if (node.socket->SendTo(packet.first(length),*destination)<length) Emit(node,2);if (node.retired || node.destroyed) return; // 짧은 송신 뒤에도 생존한 노드는 원문처럼 기록한다.
    } else if (!(node.connection.flags&2U)) {
        if (node.socket->Send(packet.first(length))<length) { Need(hooks_.log)(0,"write buffer underflow from R transceiver\n");Emit(node,2);return; }
    } else {
        if (node.cursor+length>65535) Flush(node);if (node.retired || node.destroyed) return;
        if (node.cursor+length>65535) { Need(hooks_.log)(0,"write buffer overflow from R transceiver\n");Emit(node,2);return; }
        if (node.bufferSize<=node.cursor+length) {
            const auto data=reinterpret_cast<std::uintptr_t>(packet.data()),first=reinterpret_cast<std::uintptr_t>(node.write.data()); // 재할당이 원본 자체를 반납하는 비정상 별칭은 물리 경계로 거부한다.
            if (!node.write.empty() && data>=first && data-first<node.write.size()) throw std::out_of_range("R 재할당 중 쓰기 버퍼 자체 별칭");
            const auto capacity=node.cursor+0x400U+length;node.write.resize(capacity);node.bufferSize=static_cast<std::uint32_t>(capacity); // 원문+38의 공유 논리 크기를 바꾸지만 입력의 실제 할당은 별도 보존한다.
            if (node.bufferSize<=node.cursor+length) Need(hooks_.assertion)("currentWriteBufferPtr-writeBuffer+length < bufferSize","c:\\ns\\zacket\\zacket.cpp",1008);
        }
        if (node.cursor>node.write.size() || length>node.write.size()-node.cursor) throw std::out_of_range("R 집합 쓰기 물리 범위");std::memmove(node.write.data()+node.cursor,packet.data(),length);node.cursor+=length;
    }
    const auto currentLength=transceiver_detail::Length(packet);Need(hooks_.record)(true,node.connection,packet,currentLength); // 기록은 현재 헤더/주소/sender를 다시 읽고 앞선 쓰기를 되돌리지 않는다.
}
void TransceiverTransport::Flush(std::uint32_t token) { const CallGuard guard(*this);Flush(Get(token)); } // 외곽 대여가 끝나기 전에는 실제 노드를 수거하지 않는다.
void TransceiverTransport::Flush(Node& node) {
    if (!(node.connection.flags&2U)) return;
    if (node.aggregate>node.cursor) throw std::out_of_range("R flush 실제 버퍼 포인터 범위");
    const auto aggregateLength=node.cursor-node.aggregate; // 현재 집합의 예약 헤더4까지 포함한다. 빈 다음 헤더는 아직 실제 쓰기가 아니다.
    if (aggregateLength>4) {
        if (node.cursor>node.write.size()) throw std::out_of_range("R flush 확정 본문 물리 범위");transceiver_detail::Header(std::span(node.write).subspan(node.aggregate),4,aggregateLength);node.aggregate=node.cursor;node.cursor+=4; // 예약 offset은 끝보다 최대3BYTE 커도 다음 append의 재할당 전에는 역참조하지 않는다.
    }
    if (node.cursor<4 || aggregateLength<4) throw std::out_of_range("R flush 예약 헤더 범위");const auto length=node.cursor-4;if (length>node.write.size()) throw std::out_of_range("R flush 실제 송신 물리 범위");
    if (node.writeLimit && static_cast<std::int32_t>(length)>node.writeLimit) { Emit(node,2);return; }
    if (length>4) {
        const auto written=node.socket->Send(std::span(node.write).first(length));if (written<0) { Need(hooks_.log)(0,"flush overflow from R transceiver\n");Emit(node,2);return; }
        if (written>0 && static_cast<std::size_t>(written)<length) {
            if (static_cast<std::size_t>(written)>node.aggregate) throw std::out_of_range("R 부분 송신 집합 포인터 범위");std::memmove(node.write.data(),node.write.data()+written,node.aggregate-static_cast<std::size_t>(written)); // 원문처럼 확정 접두만 옮기고 예약 헤더 tail을 새로 채우지 않는다.
        }
        if (static_cast<std::size_t>(written)>node.aggregate || static_cast<std::size_t>(written)>node.cursor) throw std::out_of_range("R 실제 송신 반환 길이 범위");node.cursor-=static_cast<std::size_t>(written);node.aggregate-=static_cast<std::size_t>(written);
    }
}
void TransceiverTransport::SetBuffered(std::uint32_t token,bool enabled) {
    const CallGuard guard(*this);auto& node=Get(token);if (!(node.socket->Flags()&0x418U)) Need(hooks_.assertion)("getFlags() & zoCONNBASED","c:\\ns\\zacket\\zacket.cpp",1139);
    if (enabled) { node.connection.flags|=2U;node.aggregate=0;node.cursor=4; } // 이미 켜져 있어도 원문처럼 현재 포인터를 다시 쓴다.
    else {
        Flush(node);if (node.retired || node.destroyed) return;
        if (node.aggregate) Need(hooks_.assertion)("startOfCurrentBufferedPacket == writeBuffer","c:\\ns\\zacket\\zacket.cpp",1147);node.connection.flags&=~2U; // 진단이 복귀하면 원문처럼 bit를 내린다.
    }
}
}
