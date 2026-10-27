// PATCH10.78의 커서 마나 표시를 실제 SID 객체와 BaseProcess 수명으로 실행한다.
#pragma once
#include "client/RawCursorLifecycle.h"
#include "o/SquidProcess.h"

namespace netstorm::client {
class RawSurfaceWorld; // 같은 실제 자산 생성/표시/가상 삭제를 소유하는 월드다.
// 원본 new(0xe0)의 필드이며 미사용 padding도 독립 관측에서 보존한다.
struct CursorManaFields {
    std::array<std::uint8_t,0xe0> bytes{}; // +c의30SID·84개수·8c반경·c0/c8/d0시각 등을 원본 폭으로 저장한다.
};
// 생성/좌표 제한이 사용하는 현재 원본 전역이다. 프로세스 사이에서 하나를 공유한다.
struct CursorManaGlobals {
private:
    std::int32_t localMapLimit_{256}; // 독립 원문 검사의 기존 값형256이다. 실제 Client는 이 사본을 읽지 않는다.
public:
    std::uint32_t manaType{103},processType{50}; // 005411f4/00541138의 마나 객체·마나 프로세스 번호다.
    std::int32_t& mapLimit; //00531928의 실제 장기 주소 또는 독립 입력을 직접 읽고 쓰는 참조다.
    float invalidPoint{1},radiusLimit{22}; // 0052f838/0052f488의 범위 밖 위치와 마나 최대 반경이다.
    explicit CursorManaGlobals(std::int32_t* sharedMapLimit=nullptr):mapLimit(sharedMapLimit ? *sharedMapLimit : localMapLimit_) {} // 처음 한 번 같은 현재 주소를 빌린다.
    CursorManaGlobals(const CursorManaGlobals&)=delete;CursorManaGlobals& operator=(const CursorManaGlobals&)=delete; // 지역 fallback에 매인 참조를 값 복사하지 않는다.
};
// 계산 밖의 실제 객체 생성·가상 표시/삭제와 중심 조회다. 선택한 경계의 누락은 예외다.
struct CursorManaHooks {
    std::function<o::Sid(std::uint32_t,std::uint32_t)> create; // 004af530의 현재 타입/flags2 생성이다.
    std::function<void(o::Sid,std::uint32_t)> setWord; // 004ac220의 하위WORD와 타입별 후속 통지다.
    std::function<std::uint32_t(std::int32_t)> themeFrame; // 초기화된00592d60의 현재 theme 문자에 대응하는 프레임이다.
    std::function<void(o::Sid,std::uint32_t)> detach,unpop,destroy; // 현재 가상+48/+4c/+10이며 인자는0이다.
    std::function<void(o::Sid,float,float,std::uint32_t)> position; // 실제 가상+90의 표시/공간 갱신이다.
    std::function<std::array<float,2>(o::Sid)> center; // 현재 부모의004adea0 중심이며 콜백마다 다시 읽는다.
};
// 생성자의 실제 Attach는 Kernel 소유권 이전을 위해 호출자가 제공한다.
using CursorManaAttach=std::function<void(CursorManaFields&,o::Sid,o::Sid,std::uint32_t)>;
// 생성·원형/사각형 프레임·이벤트·소멸의 실제 몸체를 같은 현재 필드에 적용한다.
class RawCursorMana {
public:
    // 모든 참조와 효과는 이 몸체 및 연결한 프로세스보다 오래 살아야 한다.
    RawCursorMana(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,o::SquidProcessState& clock,
        CursorManaGlobals& globals,CursorDropState& cursor,CursorManaHooks hooks);
    // 004737f0: 객체 생성 뒤 Attach(type50,parent,form,10), 그 뒤 현재 시각/표시 상태를 초기화한다.
    void Construct(CursorManaFields& fields,o::Sid parent,o::Sid form,const CursorManaAttach& attach) const;
    // 00473d00: kill은 실제0041bf10(this,0)이며 사각형 꼬리 시각 쓰기 뒤에도 필드가 살아 있어야 한다.
    void Frame(CursorManaFields& fields,const std::function<void()>& kill) const;
    // 00473c30의 실제 세 가상 인자다. 세 번째 인자는 원본에서도 사용하지 않는다.
    void Notify(CursorManaFields& fields,std::uint32_t event,std::uint32_t info,std::uint32_t value) const;
    // 00473b90: 현재 커서 프로세스 해제→표시 객체 삭제→Baseprocess 필드 무효화 순서다.
    void Destroy(CursorManaFields& fields,std::uint32_t token) const;
    // 실제 풀/프로세스 종류를 Host 연결 시 확인한다.
    const o::SidPool& Pool() const;
    std::uint32_t ProcessType() const;
private:
    // 도달한 SID와 미리보기 배열 인덱스만 호스트 범위 안에서 읽는다.
    std::span<const std::uint8_t> Raw(o::Sid sid) const;
    o::Sid Marker(const CursorManaFields& fields,std::int32_t index) const;
    o::Sid Parent(const CursorManaFields& fields) const;
    // 0041d150/0040e800의 엄격한 좌표 경계와 현재 범위 밖 대체점을 적용한다.
    std::array<float,2> Clamp(std::array<float,2> point) const;
    o::SidPool& pool_; // 표시 객체를 생성/변경하는 단일 SID 풀이다.
    std::span<const o::RiftTypeRecord> types_; // 현재 부모의 TYPE/발자국/theme를 빌린다.
    o::SquidProcessState& clock_; // Host/Kernel과 같은 현재 double 게임 시각이다.
    CursorManaGlobals& globals_; // 현재 마나 종류/최대 반경/맵 경계다.
    CursorDropState& cursor_; // 소멸 시 같은 현재 커서 프로세스 토큰을 지운다.
    CursorManaHooks hooks_; // 월드 객체 가상 효과를 실제 소비자에게 보낸다.
};
// 실제 Kernel 객체다. 원본 사각형 Kill 뒤 쓰기를 안전하게 유지하도록 프레임 동안 필드를 공유 소유한다.
class CursorManaProcess final:public o::SquidProcess {
public:
    // 새 필드는 값 초기화하되 원본 몸체는 자신이 쓰는 바이트만 변경한다.
    explicit CursorManaProcess(const RawCursorMana& logic);
    ~CursorManaProcess() override; // 실제 표시 객체와 현재 커서 토큰을 원본 순서로 정리한다.
    void RunFrame() override; // 같은 현재 필드로 원본 frame을 실행한다.
    void OnAttached() override; // Baseprocess의 실제 Form/부모 DWORD를 필드에 반영한다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override; // 같은 가상+1c를 분배한다.
    std::uint32_t Token() const; // 호스트 포인터 대신 현재 Kernel 등록 번호를 UI에 전달한다.
    CursorManaFields& Fields(); // 독립 관찰/진단이 현재 단일 저장소를 읽는다.
private:
    const RawCursorMana& logic_; // 프로세스보다 오래 사는 실제 몸체다.
    std::shared_ptr<CursorManaFields> fields_; // 자기 삭제 프레임의 꼬리 쓰기까지 살아 있는 원본 필드다.
};
// 실제 new/Attach를 실행한다. 메모리 확보 실패는nullptr이며 게임 효과를 반환0으로 대체하지 않는다.
CursorManaProcess* CreateCursorManaProcess(o::SquidProcessHost& host,const RawCursorMana& logic,o::Sid parent,o::Sid form={});
// 같은 Host/Kernel의 생성·제거·가상 이벤트를 수명 몸체에 연결한다. 사정거리 생성은 별도 몸체가 담당한다.
void BindCursorManaLifecycle(CursorLifecycleHooks& hooks,o::SquidProcessHost& host,o::Kernel& kernel,const RawCursorMana& logic);
// 원본 wrtsWRTS.?> 표를 초기화한 뒤의 theme→대문자 프레임이다. 보호 PE 관측과 대조한다.
std::uint32_t CursorManaThemeFrame(std::int32_t theme);
// 실제 월드의 생성/WORD/Unpop/Repop/Pop/삭제를 연결한다. 중심은 같은 풀의 현재 계산기를 받는다.
void BindCursorManaWorld(CursorManaHooks& hooks,RawSurfaceWorld& world,
    std::function<std::array<float,2>(o::Sid)> center,std::function<void(o::Sid)> notifySurface={});
}
