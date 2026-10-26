// Native 연결표를 Session 교체보다 오래 보관한다. 실제 생성/재배치 writer의 연결은 별도다.
#pragma once
#include "o/RawConnectionCleanup.h"
#include <array>

namespace netstorm::client { //동기화 토큰과 서버 권한은 기존 실제 Client owner에서 빌린다.
// 최초 BSS0만 구성하며 Session 획득/반납이 표나 개수를 다시 초기화하지 않는다.
class ClientConnectionGlobals {
public:
    ClientConnectionGlobals()=default; //새 Client의 원문 고정 배열 수명이다.
    ClientConnectionGlobals(const ClientConnectionGlobals&)=delete; //실제 연결 주소 표를 복제하지 않는다.
    ClientConnectionGlobals& operator=(const ClientConnectionGlobals&)=delete; //생존 연결의 현재 칸을 값 대입으로 바꾸지 않는다.
    ClientConnectionGlobals(ClientConnectionGlobals&&)=delete; //연결된 실제 표의 주소를 이동하지 않는다.
    ClientConnectionGlobals& operator=(ClientConnectionGlobals&&)=delete; //현재 span의 주소 수명을 유지한다.
    o::ConnectionCleanupGlobals Cleanup(std::uint32_t& synchronization) { return {transceivers_,buffers_,pending_,synchronization,pendingCount_}; } //기존005c8bf4를 직접 빌리며 별도 사본을 만들지 않는다.
private:
    std::array<std::uint32_t,o::kTransceiverSlots> transceivers_{}; //005e4a18의22현재 토큰이며 미연결 기존 전송의 부재 증거로 사용하지 않는다.
    std::array<std::uint32_t,o::kTransceiverSlots> buffers_{}; //005e4a70의 별도22버퍼다.
    std::array<std::uint32_t,o::kPendingConnectionSlots> pending_{}; //005e46f0의32현재 pending이다.
    std::uint32_t pendingCount_{}; //00543604의 독립 개수이며 전체 pending 정리도 보존한다.
};
}
