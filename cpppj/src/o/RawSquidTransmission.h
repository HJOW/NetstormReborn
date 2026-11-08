// 공통 Squid 전송의 같은2048BYTE 버퍼·현재 헤더·압축 상태와 실제 가상 writer를 보관한다.
#pragma once
#include "o/RawFormPacket.h"
#include <array>
#include <functional>
#include <string_view>

namespace netstorm::o { // Client와 World/Session이 동일한 원문 전역 물리를 공유한다.
struct SquidCompressedPacketHeader { //가변패킷의00544c90은고정헤더WORD와달리DWORD입력이다.
    std::uint8_t message{0x7a},protocol{1}; //0052ee92/93의현재두BYTE다.
    std::uint32_t length{}; //00432fe0이읽는실제DWORD이며최소4비교뒤낮은WORD를쓴다.
};
struct RawSquidTransmissionState { // 초기 PE 값과 BSS0을 보존하며 World 대여/반납에서 초기화하지 않는다.
    std::array<std::uint8_t,2048> buffer{}; //005c7c50의 공통 송신 버퍼이며 반환 크기 뒤를 지우지 않는다.
    FormPacketHeaders forms; // 기존 감시/내용물/삭제 writer의 현재 헤더다.
    FormDeletionStamp deletion; // 삭제 writer의 한 번 소비 표식이다.
    FormPacketHeader update{0x2c,1,27}; //0052edc0의고정WORD선언길이는35BYTE반환과별개다.
    SquidCompressedPacketHeader compressed; //00544c90의DWORD선언길이를Word로미리자르지않는다.
    std::int32_t compress{1}; //00531850의 초기1이며 Config의 현재 signed DWORD를 보존한다.
    std::uint32_t clientSidAllowed{}; //005c7a78의 초기BSS0이며 재동기화 부모의 임시 허용 관문이다.
    std::uint32_t sameOwnerEnabled{},lastOwner{},compressedCount{},compressedBytes{}; //0054d8e8/e4/ec/f0의 서로 다른 현재 DWORD다.
    std::uint32_t syncTarget{}; //005c7a7c의 현재 동기화 대상이며 시계 동기화 token과 별개다.
    FormPacketHeader sharedRegular{0x7e,1,28}; //0052ee9c/9d/a0의 공유 Regular 수신 이미지 헤더다.
};
struct RawSquidTransmissionHooks { // 직렬화에 필요한 현재 조회와 양수 반환의 실제 송신 장치다.
    std::function<const RiftTypeRecord&(std::uint8_t)> type; // 압축할 저장 TYPE의 현재 속성을 선택한다.
    std::function<std::uint32_t()> defaultType; //005411a0의 현재 bridge TYPE다.
    std::function<std::int32_t(std::uint8_t)> maxHp; //0049fce0의 현재 weakened-priest 조건을 포함한다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; // 양수 반환이면 현재1..8의 실제 연결을 순회한다.
    std::function<void(std::int32_t,std::string_view)> trace; // 압축 누적255 경계의 실제 수준4 로그다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 전송 진단 정상 복귀를 허용한다.
    std::function<bool(std::int32_t)> connected; //0048f4b0의 현재 대상별 연결 관문이다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //004e2dc0의 같은 대상 연결 송신이다.
    std::function<std::int32_t(Sid,std::span<std::uint8_t>,std::uint32_t)> processPacket; // 실제 ProcessForm+54가 같은 Kernel 객체의 writer로 작성한 패킷을 공통 버퍼에 쓴다.
    std::function<void(std::span<std::uint8_t>)> broadcastMutable; //Native만같은전체2048BYTE를대상별observer에넘기며기존constfixture는위broadcast를사용한다.
    std::function<void(std::int32_t,std::span<std::uint8_t>)> sendMutable; //Native대상송신의실제Player/NULL관문뒤같은전체예약을Registry에넘긴다.
};
// 실제 현재 VT+54와 두 Form writer를 분배한다. 새 MP 검사는 하지 않고 원문/PE로 대조한다.
class RawSquidTransmission {
public:
    RawSquidTransmission(const SidPool& pool,RawSquidTransmissionState& state,RawSquidTransmissionHooks hooks); // 같은 물리와 장기 장치만 빌린다.
    void Pop(Sid sid,std::uint32_t flags) const; //004ac260의(-1,-1,0,flags) 호출이며 일반+54에는 원문 인자0을 전달한다.
    void SendTo(Sid sid,std::int32_t target,std::uint32_t writerArgument) const; //004ac260의 selector=-3/가상+54 경로다. 현재 writer 인자와 대상 연결을 사용하며 방송으로 대체하지 않는다.
    void Transmit(Sid sid,std::int32_t target,std::int32_t selector,std::uint32_t writerArgument,std::uint32_t flags) const; //004ac260의 네 인자를 보존한다. -2만 현재+58에 flags까지 전달하고 나머지는+54다.
    void SameOwnerMode(std::uint32_t mode) const; //00433100: 모드를 쓰고 마지막 owner를 signed -99로 재설정한다.
private:
    std::int32_t Write(Sid sid,std::uint32_t writerArgument) const; // 현재+54와 실제 ProcessForm writer 인자를 선택해 원문 크기를 반환한다.
    std::int32_t WriteDeletion(Sid sid,std::uint32_t flags) const; // 현재+58이 공통004ac340인87VT를 분배하고 같은 헤더/삭제 표식을 소비한다.
    std::int32_t Compress(std::span<const std::uint8_t> input) const; //00432fe0의16필드 인코딩과 현재 owner/누적을 실행한다.
    const SidPool& pool_; // 현재 슬롯과 현재 서버를 조회하는 같은 풀이다.
    RawSquidTransmissionState& state_; // 실제 공통 버퍼/헤더/압축 전역이다.
    RawSquidTransmissionHooks hooks_; //같은Storage의장기장치와현재짧은World조회만보관한다.
};
}
