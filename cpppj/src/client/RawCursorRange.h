// PATCH10.78의 원형/직선 사정거리 표시를 같은 월드와 Kernel 수명으로 실행한다.
#pragma once
#include "client/RawCursorMana.h"
#include "o/RawGunTarget.h"

namespace netstorm::client {
// 원본 new(0x200)의 필드다.100SID 목록과 패딩도 독립 관찰에서 보존한다.
struct CursorRangeFields {
    std::array<std::uint8_t,0x200> bytes{}; // +14 목록·1a4/1a8 개수·1b0 사거리·1d0/1d8/1e0 시각이다.
};
// 실제 원본 전역이다. 같은 Session에서 모든 사정거리 프로세스가 공유한다.
struct CursorRangeGlobals {
    CursorManaGlobals& coordinates; // 생성 시 {마나 전역,총 전역}으로 연결해 같은00531928/0052f838을 읽는다.
    o::GunTargetState& targeting; // 같은0059a92c/30·00541288·005409c0의 픽셀 배율/Wind 타입/반각이다.
    std::uint32_t rangeType{104},processType{51}; // 005411f8/0054113c의 별도 사정거리 객체/프로세스 번호다.
};
// 현재 TYPE/총 방향/그림과 실제 객체 가상 효과를 연결한다. 몸체의 표시 계산은 대체하지 않는다.
struct CursorRangeHooks {
    CursorManaHooks world; // 같은 create/detach/unpop/destroy/position/center를 사용하며 WORD/theme는 호출하지 않는다.
    std::function<float(std::uint32_t)> range; // 실제 TYPE+44의 현재 사거리다.
    std::function<o::Sid(o::Sid)> firstContent; // 원본 FindContained(parent,1,0,0)이다.
    std::function<std::uint32_t(o::Sid)> mode; // 현재004d4d00의 실제 공격 모드다.
    std::function<bool(const o::GunTargetQuery&,std::int32_t)> cannonDirection; // 실제00469120의8DWORD/지정 방향 판정이다.
    std::function<o::GunTargetRect(o::Sid)> footprint; // 실제004ade70(parent,rect,0)의 확장 높이 없는 현재 발자국이다.
    std::function<std::int32_t(o::Sid,std::int32_t)> directionFrame; // 현재 부모 가상+b8(direction)의 실제 프레임이다.
    std::function<std::array<std::int16_t,2>(std::uint32_t,std::int32_t)> firingOffset; // 실제419850(TYPE,0,frame)의 헤더+32/+34다.
    std::function<o::RiftTypeFrames::LetterRun(std::uint32_t,std::uint32_t)> frameRun; // TYPE+130/+170의 현재 A/B Run이다.
    std::function<void(std::uint32_t,std::string_view)> diagnostic; // Dead circle range!의 실제 color29 출력 경계다.
};
// 실제 생성자 Attach의 네 인자를 같은 Host에 넘긴다.
using CursorRangeAttach=std::function<void(CursorRangeFields&,o::Sid,o::Sid,std::uint32_t)>;
// 원본512바이트 필드와 같은 자료로 생성·갱신·이벤트·소멸을 실행한다.
class RawCursorRange {
public:
    // 모든 자료와 효과는 몸체 및 부착한 프로세스보다 오래 살아야 한다.
    RawCursorRange(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,o::SquidProcessState& clock,
        CursorRangeGlobals& globals,CursorDropState& cursor,CursorRangeHooks hooks);
    // 474520: 현재 group/범위/방향에 따라 직선 또는 원형 객체를 만들고 flags10으로 부착한다.
    void Construct(CursorRangeFields& fields,o::Sid parent,o::Sid form,const CursorRangeAttach& attach) const;
    // 474b80: 종료 시각을 검사하고474c30/474ed0의 실제 갱신 결과로 마지막 시각을 저장한다.
    void Frame(CursorRangeFields& fields,const std::function<void()>& kill) const;
    // 474790의 실제 세 가상 인자다. 현재100칸과 void를 매번 읽는다.
    void Notify(CursorRangeFields& fields,std::uint32_t event,std::uint32_t info,std::uint32_t value) const;
    // 474870: 현재 커서 토큰을 지우고100칸 중0/free가 아닌 객체를 실제 가상 삭제한다.
    void Destroy(CursorRangeFields& fields,std::uint32_t token) const;
    const o::SidPool& Pool() const; // 다른 월드의 Host 연결을 막는 단일 풀 조회다.
    std::uint32_t ProcessType() const; // 부착 시점의 현재 프로세스 타입이다.
private:
    std::span<const std::uint8_t> Raw(o::Sid sid) const; // 현재 raw 슬롯만 읽는다.
    o::Sid Parent(const CursorRangeFields& fields) const; // 실제 Baseprocess 부모/머리 진단이다.
    o::Sid Marker(const CursorRangeFields& fields,std::int32_t index) const; // DWORD SID와100칸 범위를 검증한다.
    std::array<float,2> Clamp(std::array<float,2> point) const; // 실제 엄격한 좌표 경계와 대체점이다.
    void CreateRays(CursorRangeFields& fields,o::Sid parent) const; // 474280의 방향별 현재 개수/생성 순서다.
    void CreateCircle(CursorRangeFields& fields) const; // 474450의8개 객체와 현재 프레임이다.
    bool Rays(CursorRangeFields& fields,const std::function<void()>& kill) const; // 474c30의 실제 직선 이동/애니메이션이다.
    bool Circle(CursorRangeFields& fields,float delta,const std::function<void()>& kill) const; // 474ed0의 원형/관측각 왜곡이다.
    void Stop(CursorRangeFields& fields,const std::function<void()>& kill) const; // preview extra8이면 현재 다른 토큰도0으로 지운다.
    o::SidPool& pool_; // 실제 표시 객체가 공유하는 단일 raw 풀이다.
    std::span<const o::RiftTypeRecord> types_; // 현재 TYPE/발자국/핫스폿을 빌린다.
    o::SquidProcessState& clock_; // 같은 Host의 double 게임 시각이다.
    CursorRangeGlobals& globals_; // 현재 종류/좌표/픽셀/관측각 전역이다.
    CursorDropState& cursor_; // 마나/취득/취소와 같은 현재 커서 프로세스 토큰이다.
    CursorRangeHooks hooks_; // 복원 계산 밖의 실제 자료/객체 효과다.
};
// 같은 실제 ProcessForm/Kernel에 소유권을 넘기는 프로세스다.
class CursorRangeProcess final:public o::SquidProcess {
public:
    explicit CursorRangeProcess(const RawCursorRange& logic); // 단일 필드를 확보한다.
    ~CursorRangeProcess() override; // 실제 마커와 커서 토큰을 정리한다.
    void RunFrame() override; // 자기 삭제가 있어도 로컬 필드 수명을 유지한다.
    void OnAttached() override; // 실제 Form/부모 DWORD를 반영한다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override; // 같은 원본 이벤트를 분배한다.
    std::uint32_t Token() const; // 호스트 주소 대신 Kernel 번호를 UI에 준다.
    CursorRangeFields& Fields(); // 검사/진단이 현재 단일 저장소를 읽는다.
private:
    const RawCursorRange& logic_; // Kernel보다 오래 사는 실제 몸체다.
    std::shared_ptr<CursorRangeFields> fields_; // 자기 삭제 프레임이 끝날 때까지 살아 있는 필드다.
};
// 실제 new/Attach를 실행하며 메모리 확보 실패만nullptr로 구별한다.
CursorRangeProcess* CreateCursorRangeProcess(o::SquidProcessHost& host,const RawCursorRange& logic,o::Sid parent,o::Sid form={});
// 마나가 연결한 같은 제거/이벤트를 유지하고 실제 사정거리 생성만 추가한다.
void BindCursorRangeLifecycle(CursorLifecycleHooks& hooks,o::SquidProcessHost& host,const RawCursorRange& logic);
}
