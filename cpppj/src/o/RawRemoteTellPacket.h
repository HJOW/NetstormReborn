// 원본004c9150/4c9200의70h 문자열 쓰기와40b090/cf980 관찰·수신이다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
namespace netstorm::o { // 실제 공유 분배/전송/현재 Tell 장치만 caller가 빌려준다.
class RawPacketDispatch; // 새 연결표/콜백 수명을 만들지 않는다.
using RemoteTellAssertion=std::function<void(std::string_view,std::string_view,std::uint32_t)>; // 진단 정상 복귀 뒤 원문 쓰기를 계속한다.
std::array<std::uint8_t,264> WriteRemoteTellPacket(std::string_view text,const RemoteTellAssertion& assertion,std::uint32_t line); // Native264BYTE ff fill/헤더/길이DWORD/NUL 복사이며 line은4ef/4fb다.
void ObserveRemoteTellPacket(std::span<std::uint8_t> packet,std::uint32_t sending,const RemoteTellAssertion& assertion); // 송신에서만 현재tellLength 진단 뒤 헤더WORD를감겨쓴다.
void RegisterRemoteTellPacket(RawPacketDispatch& dispatch,std::function<void(std::string_view)> tell,RemoteTellAssertion assertion); // 실제70h/version1/정책0/가변길이/관찰기와현재cf730 장치를 등록한다.
}
