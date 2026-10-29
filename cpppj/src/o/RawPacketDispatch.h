// 패킷 등록·길이 검증·버전 정책·지연 없는 단일 패킷 분배를 원본 Zocket 몸체에서 복원한다.
#pragma once
#include "o/RawConstructionData.h"
#include <array>

namespace netstorm::o {
// 원본 연결 객체에서 단일 분배가 읽고 쓰는 값이다. 호스트 소켓 포인터 대신 명시적인 참조를 전달한다.
struct PacketConnection {
    std::uint32_t flags{}; // +0x34: bit 0x40이 켜진 연결에는 진입하지 않는다.
    std::uint32_t sender{}; // +0x40: 처리기의 첫 DWORD 인자다.
    std::uint32_t received{}; // +0x44: 일반 패킷의 마지막 수신 시각 기록이다.
    std::uint32_t heartbeat{}; // +0x48: 타입 6 패킷의 별도 시각 기록이다.
    std::uint32_t depth{}; // +0x8c: 즉시 콜백 중 증가하고 정상/예외 종료에서 복구하는 중첩 깊이다.
    bool delayed{}; // +0x7c: 지연 분배 큐가 있으면 이 API에서는 거부한다.
    std::array<std::uint8_t,20> address{}; // +0x4c..5f의 현재 주소20BYTE다. 실제 로컬 두 끝의 생성 값을 보관한다.
};
// 원본 세 등록 표를 호스트 함수/값으로 옮긴 한 항목이다.
struct PacketRegistration {
    std::function<void(std::uint32_t,PacketConnection&,std::span<const std::uint8_t>)> receive; // 처리기(sender,연결,원본 버퍼)다.
    std::function<void(std::uint32_t)> observer; // 집합 패킷에서만 쓰이는 관찰기이며 단일 분배에서는 호출하지 않는다.
    std::function<void(std::span<std::uint8_t>,std::uint32_t)> packetObserver; // 같은 원문 observer 슬롯의 mutable this를 쓰는 장치다. 기존 인자 전용 장치와 동시에 등록하지 않는다.
    void NotifyObserver(std::span<std::uint8_t> packet,std::uint32_t sending) const; // 현재 등록된 한 종류의 observer만 실제 패킷/방향 인자로 호출한다.
    std::uint16_t length{0xffff}; // 0xffff는 가변 길이, 그 밖에는 헤더 WORD와 정확히 비교한다.
    std::uint8_t version{}; // 등록된 프로토콜 값이다. incoming의 bit 0x80만 가리고 비교한다.
    std::uint8_t mismatch{}; // 높은 버전: 4 종료/1 무시, 낮은 버전: 8 종료/2 무시. 0이면 계속 분배한다.
};
// 게임/OS 밖의 시계·기록·연결 종료 수명 효과다. 콜백이 연결 flags/depth를 바꾸는 것도 허용한다.
struct PacketDispatchHooks {
    std::function<std::uint32_t()> now; // 두 판본의 double 시계→DWORD 변환 결과를 공급한다.
    std::function<void(PacketConnection&,std::span<const std::uint8_t>,std::uint16_t)> record; // 타입 10 이상만 원본 선언 길이와 함께 기록한다.
    std::function<void(PacketConnection&)> terminate; // 버전 정책 종료다. 원본의 종료 메시지 재분배는 이 경계의 책임이다.
    std::function<void(PacketConnection&)> close; // 콜백 뒤 depth 0/flags 0x40이면 연결 소멸 몸체를 실행한다.
    std::function<void(PacketConnection&)> release; // 위 소멸 뒤 저장소를 해제한다. 호출 뒤 연결을 다시 읽지 않는다.
};
// 004ded50/70/90·004df240·004df740·004df010 ↔ CD 00418bb0/bd0/bf0·004190a0·00419130·00418d40.
// 단일 패킷과 즉시 콜백 분기만 지원한다. 타입 4 집합/지연 큐·소켓 송수신·연결 생성/종료 몸체는 후속이다.
class RawPacketDispatch {
public:
    // 판본별 최대 타입과 원본 고정 길이 표를 준비한다. 필수 시계/기록/수명 경계는 비울 수 없다.
    RawPacketDispatch(OriginalEdition edition,PacketDispatchHooks hooks);
    // 각 원본 등록 함수처럼 한 표만 바꾼다. 타입 0/최대 밖 번호는 등록 전에 거부한다.
    void Register(std::uint32_t type,decltype(PacketRegistration::receive) receive);
    // 해당 타입의 관찰기 표만 바꾼다. 단일 분배는 등록된 관찰기를 호출하지 않는다.
    void Observe(std::uint32_t type,decltype(PacketRegistration::observer) observer);
    void ObservePacket(std::uint32_t type,decltype(PacketRegistration::packetObserver) observer); // 원본 thiscall observer에 실제 mutable packet을 전달하는 동일 슬롯 등록이다.
    // 해당 타입의 버전 BYTE와 불일치 정책 BYTE만 바꾼다.
    void Version(std::uint32_t type,std::uint8_t version,std::uint8_t mismatch);
    // 원본 길이 표의 caller 공급/검증용 변경이다. 0xffff이면 선언 길이로 제한하지 않는다.
    void RequireLength(std::uint32_t type,std::uint16_t length);
    // 등록 상태를 읽거나 독립 원본 표 관찰과 비교한다.
    const PacketRegistration& Registration(std::uint32_t type) const;
    // 최소 헤더 4바이트가 필요하다. 타입 10 이상은 처리기가 있어야 하며, 그 밖에는 타입/길이만 검사한다.
    bool Validate(std::span<const std::uint8_t> packet) const;
    // 닫힌 연결/버전 종료/콜백 뒤 소멸이면 false, 무시한 패킷/성공이면 true다. 실제 필드 길이는 처리기가 검사한다.
    // 선언 길이와 실제 span을 같게 강제하지 않는다. 콜백에 전달한 버퍼는 호출 동안만 유효하다.
    bool Dispatch(PacketConnection& connection,std::span<const std::uint8_t> packet) const;
private:
    // 잘못된 등록 번호가 다른 항목을 덮어쓰지 않게 한다.
    void CheckType(std::uint32_t type) const;
    std::array<PacketRegistration,256> entries_; // 원본 BYTE 메시지 번호 전체의 호스트 저장소다.
    std::uint32_t lastType_; // 원본 초기 최대 타입: 패치 137/CD 126이다.
    PacketDispatchHooks hooks_; // 전송 계층의 시계·로그·연결 수명 효과다.
};
// 두 원본 초기화처럼 0x37의 observer=null/version=1/flags=0/건설 수신을 등록한다. data는 분배기보다 오래 살아야 한다.
void RegisterConstructionData(RawPacketDispatch& dispatch,RawConstructionData& data);
}
