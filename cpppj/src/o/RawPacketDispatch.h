// 패킷 등록·길이 검증·버전 정책·지연 없는 단일 패킷 분배를 원본 Zocket 몸체에서 복원한다.
#pragma once
#include "o/RawConstructionData.h"
#include <array>
#include <memory>

namespace netstorm::o {
class PacketRegistrationTable;class PacketRegistrationLease; // backend별 hooks와 별개로 공유할 실제 등록표/Session 대여다.
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
//원문 receiver 함수F의 코드 정체성과 장기 데이터만 담는 불변 본문값이다. 현재 타입표나 packet[1]로 G를 다시 선택하지 않는다.
struct CanonicalPacketReceiver {
    using Receive=std::function<void(std::uint32_t,PacketConnection*,std::span<const std::uint8_t>)>; //선택된 F가 쓰는원래인자이며 nullable연결의허용은각본문계약이다.
    const std::uint32_t nativeEntry; //원문함수주소는정체성표식이며호스트함수포인터로캐스팅하지않는다.
    const Receive receive; //Client장기owner와고정코드본문만보유하며oldSession/lease/enqueuepin을담지않는다.
    CanonicalPacketReceiver(std::uint32_t entry,Receive body); //실제비NULL원문본문/정체성만허용하며가짜성공본문을만들지않는다.
    void Invoke(std::uint32_t sender,PacketConnection* connection,std::span<const std::uint8_t> packet) const; //저장된F값만실행하며Validate/version/observer/record/depth를추가하지않는다.
};
// 원본 세 등록 표를 호스트 함수/값으로 옮긴 한 항목이다.
struct PacketRegistration {
    std::weak_ptr<PacketRegistrationLease> registrationLease; // 실행 중 실제 Session 전체 owner를 pin할 약한 대여다.
    bool leased{}; // 전역 기본 등록과 Session 대여 등록을 구별한다.
    std::shared_ptr<void> Pin() const; // receiver/observer 호출이 끝날 때까지 실제 참조 모듈을 유지한다.
    std::function<void(std::uint32_t,PacketConnection&,std::span<const std::uint8_t>)> receive; // 처리기(sender,연결,원본 버퍼)다.
    std::function<void(std::uint32_t,PacketConnection*,std::span<const std::uint8_t>)> directReceive; // R 내부 직접 호출의 실제NULL 연결을 지원한다. 일반 framed receive와 같은 callable 소유자를 공유한다.
    std::shared_ptr<const CanonicalPacketReceiver> canonicalReceive; //검토된장기F만기록한다. 일반짧은closure는NULL이며계획중인큐가안전하다고가장하지않는다.
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
    std::function<void(PacketConnection&,std::span<const std::uint8_t>,std::uint16_t)> sent; //실제 가상 송신의 복사/쓰기 뒤 원문 방향1 기록이다. 수신 record와 구별하며 장치 없는 독립 전송은 비워 둔다.
    std::function<std::span<const std::uint8_t>()> players; //실제 Client는 Player 재할당 뒤에도 현재0059534c를 조회한다. 독립 고정뷰 장치는 기존 입력 span을 사용한다.
};
bool PacketPlayerActive(std::span<const std::uint8_t> fallback,const std::function<std::span<const std::uint8_t>()>& current,std::uint32_t owner); //4e2dc0의 선택된 owner 기록+50 BYTE만 현재 실제 Player9에서 읽는다.
// 004ded50/70/90·004df240·004df740·004df010 ↔ CD 00418bb0/bd0/bf0·004190a0·00419130·00418d40.
// 단일 패킷과 즉시 콜백 분기만 지원한다. 타입 4 집합/지연 큐·소켓 송수신·연결 생성/종료 몸체는 후속이다.
class RawPacketDispatch {
public:
    // 판본별 최대 타입과 원본 고정 길이 표를 준비한다. 필수 시계/기록/수명 경계는 비울 수 없다.
    RawPacketDispatch(OriginalEdition edition,PacketDispatchHooks hooks,std::shared_ptr<PacketRegistrationTable> registrations={});
    const std::shared_ptr<PacketRegistrationTable>& Registrations() const; // Local/TCP/Editor가 같은 등록표 owner를 생성 때 공유한다.
    // 각 원본 등록 함수처럼 한 표만 바꾼다. 타입 0/최대 밖 번호는 등록 전에 거부한다.
    void Register(std::uint32_t type,decltype(PacketRegistration::receive) receive);
    void RegisterDirect(std::uint32_t type,decltype(PacketRegistration::directReceive) receive); // 같은 등록표의 수신기이며 일반 프레임은 실제 연결 주소, 직접 호출은 선택한 nullable 포인터를 전달한다.
    void RegisterCanonicalDirect(std::uint32_t type,std::uint32_t nativeEntry,CanonicalPacketReceiver::Receive receive); //고정Native본문/장기데이터를불변값으로등록하고현재즉시두입구도같이설치한다.
    void RegisterCanonical(std::uint32_t type,const std::shared_ptr<const CanonicalPacketReceiver>& receive); //이미선택한같은F를재등록해lease표식만바꾸며본문을type현재표에서다시선택하지않는다.
    void PersistCanonicalBaseline(std::uint32_t type); //검토된 현재 장기 F/observer를 활성 Session 반납 뒤에도 같은 표에 보존한다. 실제 전역 setter 직후에만 호출한다.
    std::shared_ptr<const CanonicalPacketReceiver> SnapshotReceiver(std::uint32_t type) const; //enqueue선택시한번F값을복사한다. NULLreceiver는NULL이고미전환짧은본문은명시오류다. Runtime pin은하지않는다.
    void InvokeDirect(std::uint32_t type,std::uint32_t sender,PacketConnection* connection,std::span<const std::uint8_t> packet) const; // 원문 함수 포인터 호출처럼 검증/observer·기록/시각/연결 깊이·닫힘을 추가하지 않는다. 등록 owner pin만 유지한다.
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
    std::shared_ptr<PacketRegistrationTable> registrations_; // 원문 전역 등록표 하나이며 backend별 hooks는 아래에 별도로 남긴다.
    std::uint32_t lastType_; // 원본 초기 최대 타입: 패치 137/CD 126이다.
    PacketDispatchHooks hooks_; // 전송 계층의 시계·로그·연결 수명 효과다.
};
// 두 원본 초기화처럼 0x37의 observer=null/version=1/flags=0/건설 수신을 등록한다. data는 분배기보다 오래 살아야 한다.
void RegisterConstructionData(RawPacketDispatch& dispatch,RawConstructionData& data);
}
