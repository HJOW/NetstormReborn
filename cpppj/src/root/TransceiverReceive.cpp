// R listener 수락과 실제 부분 수신/관찰/분배를 같은 고정 노드에서 수행한다.
#include "root/TransceiverInternals.h"

namespace netstorm::root { // 수신 버퍼는 callback 동안 같은 물리에 남는다.
using transceiver_detail::Need;
ReceivedFrame TransceiverTransport::ReceiveOne(std::uint32_t token,bool dispatch) {
    const CallGuard guard(*this);auto& node=Get(token);if (node.listener) { Accept(node);return {}; }return Receive(node,dispatch); // 실제 가상 함수 종류는 socket의 상속된 listener bit와 별개다.
}
void TransceiverTransport::Accept(Node& node) {
    while (std::bit_cast<std::int32_t>(connections_.pendingCount)<31) { //485be0의 실제 SETL이며 data초기1을 포함해 최대30개를 허용한다.
        std::unique_ptr<SocketDevice> socket;const auto result=node.socket->Accept(socket);if (result<0) { Need(hooks_.log)(0,"listening socket shutdown\n");return; }if (!result) return;
        const auto token=Adopt(std::move(socket),0,node.acceptedCapacity);RegisterPending(connections_,token,Lifecycle()); // 새 buffer/현재 sender 음수 공개/개수 증가를 기존 raw 부모에 맡긴다.
    }
}
ReceivedFrame TransceiverTransport::Receive(Node& node,bool dispatch) {
    if (node.connection.delayed) throw std::logic_error("R 지연 호출 큐 실제 장치 미연결"); // 현재84150/842a0 기본 경로의 callList는NULL이다.
    if (node.connection.flags&0x40U) { Destroy(node);node.retired=true;return {}; }
    if (node.connection.flags&1U) { if (dispatch) Emit(node,2);return {}; } // 원문 일반 실패 flag는 선택 통지만 수행한다.
    if ((node.connection.flags&0x20U) && (node.socket->Flags()&0x418U)) {
        const auto status=node.socket->Status();if (status<0) { if (dispatch) Emit(node,2);return {}; }
        if (status) {
            node.connection.flags&=~0x20U;if (dispatch) Emit(node,1);if (node.retired || node.destroyed) return {}; // callback이 실제 반납한 물리를 다시 읽지 않는다.
            if (node.socket->Flags()&0x418U) node.connection.address=node.socket->QueryPeerAddress();
        }
    }
    if (!(node.socket->Flags()&0x418U)) {
        const auto received=node.socket->ReceiveDatagram(node.input,node.connection.address);node.received=std::bit_cast<std::uint32_t>(received);if (received<0) { if (dispatch) Emit(node,2);return {}; }
    } else if (node.received<4) {
        const auto received=node.socket->Receive(std::span(node.input).subspan(node.received,4U-node.received));if (received<0) { if (dispatch) Emit(node,2);return {}; }node.received+=static_cast<std::uint32_t>(received);
    }
    if (node.received<4) return {ReceiveKind::Sentinel,{}};
    const auto type=node.input[1];const auto expected=transceiver_detail::Length(node.input); // observer 호출 전 원문 Type/선언 길이를 캡처한다.
    if (!expected || node.bufferSize<=expected || expected<4 || expected>node.input.size()) { Need(hooks_.log)(0,"packet overflow from R transceiver\n");if (dispatch) Emit(node,2);return {}; } // 논리 크기와 실제 입력 할당을 별도로 검사해 원본 realloc 뒤의 물리 초과를 막는다.
    if (!(node.socket->Flags()&0x418U)) {
        if ((node.socket->Flags()&0x220U) && expected!=node.received) { Need(hooks_.log)(0,"udp packet size underflow\n");if (dispatch) Emit(node,2);return {}; }
    } else {
        if (node.received>expected) { if (dispatch) Emit(node,2);return {}; } // 잘못된 헤더를 음수 길이의 OS 접근으로 바꾸지 않는다.
        const auto received=node.socket->Receive(std::span(node.input).subspan(node.received,expected-node.received));if (received<0) { if (dispatch) Emit(node,2);return {}; }node.received+=static_cast<std::uint32_t>(received);
        if (node.received<expected) return {ReceiveKind::Sentinel,{}};if (node.received>expected) { if (dispatch) Emit(node,2);return {}; }
    }
    node.received=0;auto packet=std::span(node.input).first(expected); // 완성된 뒤 검증/관찰보다 앞에서 부분 길이를0으로 쓴다.
    if (type==0x7f || type==0x80) return {ReceiveKind::Sentinel,{}}; // 폐기 검증은 observer/실제 처리기 전에 제외한다.
    if (!dispatch_.Validate(packet)) {
        const auto failure=!type || type>137 ? "type":(type>=10 && !dispatch_.Registration(type).receive ? "handler":"length");Need(hooks_.log)(0,"Packet of type "+std::to_string(type)+" failed "+failure+" validation\n");return {ReceiveKind::Sentinel,{}};
    }
    if (type!=4) { const auto entry=dispatch_.Registration(type);entry.NotifyObserver(packet,0); } // 첫 Type의 observer를 호출한 뒤 공통 분배는 현재 헤더/등록을 다시 읽는다.
    if (node.retired || node.destroyed) return {};const auto currentLength=transceiver_detail::Length(node.input);
    if (currentLength<4 || currentLength>node.input.size()) { if (dispatch) Emit(node,2);return {}; }packet=std::span(node.input).first(currentLength);
    if (dispatch && !o::DispatchPacketFrame(dispatch_,node.connection,packet,DispatchHooks())) return {};
    return {ReceiveKind::Packet,packet}; // 기존848c0의 실제 현재 버퍼 반환이며 다음 변경 호출까지만 빌린다.
}
}
