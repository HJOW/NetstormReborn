// 개발자 문자열 패킷의 현재 길이 관찰과 원문 +8 문자열 수신을 복원한다.
#pragma once
#include "o/RawRemoteTellPacket.h"

namespace netstorm::o { // 전송/기록/권한은 기존 실제 분배기를 사용한다.
void ObserveDeveloperCommandPacket(std::span<std::uint8_t> packet,std::uint32_t sending,const RemoteTellAssertion& assertion); //40a6b0은 송신에서만 len 진단 뒤 현재 lowWORD+8을 헤더에 쓴다.
void RegisterDeveloperCommandPacket(RawPacketDispatch& dispatch,std::function<void(std::string_view)> execute,RemoteTellAssertion assertion); //31h/version1/정책0/가변 길이와44dba0의 현재 문자열 실행기를 등록한다.
void ReceiveDeveloperCommandPacket(std::span<const std::uint8_t> packet,const std::function<void(std::string_view)>& execute); //선언 길이와 별개인 첫 NUL 문자열을 같은 실제 소비자에 전달하며 sender/연결은 조회하지 않는다.

}
