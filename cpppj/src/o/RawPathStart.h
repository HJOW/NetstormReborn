// 실제 이동 시작 상위 몸체를 같은 SID 풀·검색·서버 Host 및 부모 통지에 연결한다.
#pragma once
#include "o/RawContainedLookup.h"
#include "o/RawPathProcess.h"
#include "o/SquidFrame.h"

namespace netstorm::o {
// 실제 타입 +e0/+e4/+118을 매 조회에서 읽는 메타다. 현재 부모 타입 변경 뒤에도 다시 조회한다.
struct RawPathStartType {
    float speed{},turningSpeed{}; // 현재 원본 타입의 이동/회전 속도다.
    std::int32_t defaultFrame{}; // 사제 +40이 두 사건을 삭제한 뒤 읽는 현재 기본 프레임이다.
};
// 같은 월드 전역을 참조한다. 타입·보스/게임 시각·목적지 스냅·실패 재귀 guard를 복사하지 않는다.
struct RawPathStartState {
    SquidProcessState& clock; // 권한 및 Prepare가 사용하는 현재 공유 게임 시계다.
    std::uint32_t& dudeType; // 00541170/CD0051ca5c: client Path 부착 대상이다.
    std::uint32_t& teleportType; // 00541270/CD0051cb5c: type23 생성/자연 부모 통지를 선택한다.
    std::uint32_t& manaBoltType; // 005411fc/CD0051cae8: client Path 부착 대상이다.
    std::uint32_t& priestType; // 005412d0/CD0051cbbc: 실제 사제 +44/+40의 부모 타입이다.
    std::uint32_t& geyserType; // 00541240/CD0051cb2c: 사제 목적지 간헐천 finder가 마지막 일치를 targetA로 기록한다.
    bool& snapPriestDestination; // 00595180/CD0052da98: 사제 목적지 거의올림-0.5 전역이다.
    bool& retrying; // 패치005951b0: 무효 경로 정리 후 재귀를 한 번 제어하는 실제 guard다.
};
// 원본8인자 명령이다. 목적지는 clamp 뒤 통지/저장되며 다른 값은 해석 없이 DWORD로 전달한다.
struct RawPathStartRequest {
    Sid parent{}; // 경로의 실제 부모 SID다.
    RawPathPoint destination{}; // 원하는 float32 목적지 좌표다.
    std::uint32_t replaceStart{}; // nonzero면 연속한 시작 꼭짓점을 실제 현재 부모 좌표로 교체한다.
    std::uint32_t helper{},tailWord{},tailByte{},reserved{}; // 실제 인자5~8; helper/tail은 raw DWORD이며 reserved는 재귀에서 그대로 전달한다.
};
// 미복원 효과는 필수 경계이며 다른 부모/파생 클래스에 빈 구현을 공급하지 않는다.
struct RawPathStartHooks {
    std::function<std::uint32_t(std::uint32_t)> processType; // 복원된00488fe0/CD0047d020 타입 선택 몸체다.
    std::function<std::unique_ptr<RawPathProcess>(bool priest)> create; // 실제 같은 풀/hash/search/state를 쓰는 type54 또는23 생성이다.
    std::function<RawPathStartType(Sid)> metadata; // 현재 부모의 실제 타입+e0/+e4/+118 조회다.
    std::function<void(RawPathPoint,std::uint32_t,std::uint32_t)> clearBlocked; // 00443800/CD004d2450의 실제 겹침 정리; 현재 부모 위치/타입/소유자다.
};
// 패치00489080/CD0047d150의 실제 상위 순서와 지원 부모158/134의 두 통지를 실행한다.
class RawPathStart {
public:
    // 모든 참조는 Start보다 오래 살아야 한다. lookup/Host/Frame이 다른 풀을 처리하면 생성에서 거부한다.
    RawPathStart(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,SquidProcessHost& host,
        const RawContainedLookup& lookup,const SquidFrame& frame,RawPathStartState state,RawPathStartHooks hooks);
    // 실제 경로를 만든 후 Host가 소유한다. 실패 정리 재귀가 성공해도 외부 호출은 원본처럼nullptr를 반환한다.
    RawPathProcess* Start(RawPathStartRequest request);
    // 후속 실제 월드 연결에서 다른 풀을 합성하지 않도록 참조를 제공한다.
    const SidPool& Pool() const;
private:
    // 원본의 거의올림 좌표·현재 float32 raw 위치·무효 경로의 발자국을 조회한다.
    RawPathPoint Position(Sid sid) const;
    static RawPathPoint CeilPoint(RawPathPoint point);
    SquidSearchArea Footprint(Sid sid) const;
    // 사제의 목적지 간헐천 조회와 사건604/605 전체 삭제·기본 프레임 지정이다. teleport 통지는 실제 빈 ret다.
    void NotifyStarted(Sid parent,RawPathPoint destination,RawPathProcess& process);
    SidPool& pool_; // 유일한 실제 SID 저장소다.
    const SquidHash& hash_; // Init 검색/부모 목적지/실패 정리와 같은 공간 해시다.
    std::span<const RiftTypeRecord> types_; // callback 뒤 현재 타입/genus/발자국을 읽는 원본 표다.
    SquidProcessHost& host_; // 실제 Kernel/Form 생성·부착·삭제를 수행한다.
    const RawContainedLookup& lookup_; // 첫 기존 같은 종류 경로를 현재 종속 체인에서 찾는다.
    const SquidFrame& frame_; // 사제의 현재 defaultFrame을 실제 공간/표시 갱신에 반영한다.
    RawPathStartState state_; // 공유 전역의 참조 묶음이다.
    RawPathStartHooks hooks_; // 실제 타입/생성/메타/겹침 정리 경계다.
};
}
