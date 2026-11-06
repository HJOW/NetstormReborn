// 건설 프로세스 메시지 0x37의 쓰기·읽기·수신과 실제 프로세스의 전송 래퍼를 복원한다.
#pragma once
#include "o/RawConstructionProcess.h"

namespace netstorm::o {
// 실제 진단 창의 정상 복귀 뒤 원문 수신을 계속한다. 미연결 진단은 호스트 오류로 드러낸다.
using ConstructionDataAssertion=std::function<void(std::string_view,std::string_view,std::uint32_t)>;
// 원본 전역의 두 헤더 바이트다. 기본값은 세 PE에 저장된 1/0x37이며 호출자가 현재 전역을 공급할 수 있다.
struct ConstructionDataHeader {
    std::uint8_t protocol{1}; // 패치 005325b9 / CD 0053fc30: 첫 헤더 바이트다.
    std::uint8_t message{0x37}; // 패치 005325b8 / CD 0053fc2c: 등록된 메시지 번호다.
};
// 수신의 외부 효과다. 새 객체 저장소와 부착은 호출자가 소유하며 lookup은 실제 host가 없을 때만 필요하다.
struct ConstructionDataHooks {
    std::function<ConstructionProcessFields*()> allocate; // new(0x38)에 대응한다. null이면 원본의 null 접근 대신 bad_alloc으로 거부한다.
    std::function<void(ConstructionProcessFields&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> attach; // 필드·현재 타입·부모 DWORD·캡처된 form DWORD·flags 0x10을 받는다.
    std::function<ConstructionProcessFields*(ProcessId)> lookup; // 현재 form의 DWORD Kernel 번호를 그대로 조회한다. 없는 슬롯은 null이다.
};
// 필드 비트에 따른 상태 패킷과 원본 수신 분기를 다룬다. 실제 host를 주면 기존 form 갱신이 실제 Kernel에 연결된다.
// hooks 생성자는 새 form 부착을 명시 경계로 둔다. 실제 logic/host 생성자는 지정 서버 form까지 직접 부착한다.
class RawConstructionData {
public:
    // 최대 패킷은 헤더 9 + 부모 4 + 시작 8 + 끝 8 + 추가 1바이트다.
    static constexpr std::size_t kMaximumSize=30;
    // pool/state/hooks/host는 이 객체보다 오래 살아야 한다. 실제 host와 pool이 다르거나 필수 경계가 없으면 거부한다.
    RawConstructionData(SidPool& pool,ConstructionProcessState& state,ConstructionDataHooks hooks,SquidProcessHost* host=nullptr,ConstructionDataAssertion assertion={});
    // 같은 풀의 실제 건설 몸체와 host에 새/기존 수신을 연결한다. 새 객체의 소유권은 Kernel에 넘기며 실패 시 자동 해제한다.
    RawConstructionData(SidPool& pool,ConstructionProcessState& state,RawConstructionProcess& logic,SquidProcessHost& host,ConstructionDataAssertion assertion={});
    // 004426d0 / CD 004d0430: 현재 필드를 little endian으로 쓰고 dirty를 0으로 만든다. force이면 먼저 0xff로 둔다.
    // 부모는 인자가 0이 아닐 때만 쓴다. 기존 dirty의 bit 1과 부모 0의 조합도 원본 그대로 유지한다.
    // 부족한 버퍼는 쓰기/dirty 변경 전에 거부한다. 반환은 패킷에 기록한 길이이며 CD의 불명확한 EAX 반환은 사용하지 않는다.
    static std::uint16_t Write(ConstructionProcessFields& fields,std::span<std::uint8_t> packet,
        std::uint32_t form,std::uint32_t parent,bool force,ConstructionDataHeader header={});
    // 00443bd0 / CD 004d2b20: includeParent 또는 force가 참일 때 실제 프로세스의 부모를 전송한다.
    static std::uint16_t WriteProcess(ConstructionProcess& process,std::span<std::uint8_t> packet,
        bool includeParent,bool force,ConstructionDataHeader header={});
    // 00442760 / CD 004d04d0: 선택된 시각/추가 바이트만 복원하고 부모 DWORD(없으면 0)를 반환한다. dirty/sound는 보존한다.
    // fields=null은 시각/추가 비트가 없을 때만 허용한다. 원본처럼 선언 길이/헤더 종류는 읽지 않으며 실제 버퍼 폭만 검사한다.
    static std::uint32_t Read(ConstructionProcessFields* fields,std::span<const std::uint8_t> packet);
    // 004427b0 / CD 004d0540: free의ff/live의부모bit1 진단 뒤 정상 복귀하면 같은 생성/읽기/부착을 계속한다.
    // 현재 PID가 없고 객체 필드가 선택되지 않으면 읽기만 끝낸다. 실제 필드 저장/물리 범위만 호스트 예외로 드러낸다.
    void Receive(std::span<const std::uint8_t> packet) const;
private:
    SidPool& pool_; // free 상태와 form의 현재 Kernel 번호를 읽는 월드다.
    ConstructionProcessState& state_; // 할당 경계 뒤의 현재 프로세스 타입을 읽는다.
    ConstructionDataHooks hooks_; // 새 객체/부착/선택적 Kernel 조회의 명시 경계다.
    SquidProcessHost* host_; // 있으면 lookup 경계 대신 현재 실제 프로세스를 찾는다.
    RawConstructionProcess* logic_{}; // 실제 새 수신 객체의 진행/완료 몸체다. hooks 생성자에서는 null이다.
    ConstructionDataAssertion assertion_; // 실제 Client의 수명 안정 진단 경계이며 정상 복귀를 중단으로 바꾸지 않는다.
    void Assert(std::string_view expression,std::uint32_t line) const; // 원문 파일/행을 같은 진단 장치에 전달한다.
};
}
