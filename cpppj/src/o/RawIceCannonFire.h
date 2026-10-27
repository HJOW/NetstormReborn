// 아이스 캐논의80BYTE 파생004685e0/00468660을 기존 공통 발사와 같은 Host에 연결한다.
#pragma once
#include "o/RawCannonFire.h"

namespace netstorm::o { // 원본 객체 상태와 순수 동작을 실제 캠페인 장치에서 분리한다.
struct IceCannonFireFields { // 공통64BYTE와 원문+40..+4f의 미초기화 padding을 각각 보존한다.
    GunAnimationFields common; // 기존00467330이 읽고 쓰는 같은 접두다.
    std::array<std::uint8_t,16> extra{}; // +40 active DWORD/+44 padding/+48 double 전환 시각이다.
};
struct IceCannonFireHooks { // 선택된 원본 하위에서만 실제 효과를 요구한다.
    std::function<void(Sid)> cooling; //004a8770은 현재 프레임의 포인트에서 냉각 표시를 생성한다.
    std::function<std::array<std::uint32_t,8>(Sid)> context; //0045ae60의 현재8DWORD 발사 문맥이다.
    std::function<bool(const std::array<std::uint32_t,8>&,Sid)> mayTarget; //0046af80의 실제 표적 판정이다.
};
class RawIceCannonFire { // 공통 발사는 재사용하고 아이스 전용 상태/가상 단계만 소유한다.
public:
    // 동일 Pool/TYPE/프레임표·현재 시계와 실제 하위의 수명은 호출자가 보장한다.
    RawIceCannonFire(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const PriestPlainCanonType> frames,
        CannonFireState& state,GunAnimationHooks common,IceCannonFireHooks hooks);
    void Initialize(IceCannonFireFields& fields,Sid gunForm,Sid target) const; // 실제 공통 ctor 뒤 두 가상 표/active/4초 기한만 쓴다.
    void Frame(IceCannonFireFields& fields) const; // 같은00467330의 실제 가상 Step으로 진행한다.
    std::uint32_t Step(IceCannonFireFields& fields,std::int32_t frame) const; //00468660의 냉각/문자 구간/표적 판정을 수행한다.
    bool Launch(IceCannonFireFields& fields) const; // 동일 실제 투사체 생성의 가상 getter만 아이스 상태다.
    const SidPool& Pool() const; // 실제 factory가 같은 저장소를 검증한다.
    std::uint32_t Type() const; // 현재 아이스 부모 TYPE를 반환한다.
private:
    SidPool& pool_; // 현재 부모 프레임을 읽는 같은 실제 SID 풀이다.
    std::span<const PriestPlainCanonType> frames_; // 타입+124/+130/+170의 같은 실제 문자 구간이다.
    CannonFireState& state_; // callback 뒤에도 현재 now를 다시 읽는다.
    IceCannonFireHooks hooks_; // 실제 냉각/문맥/표적 소비자다.
    RawCannonFire common_; // 기존 공통64BYTE 추적/공간/발사/완료 몸체다.
};
class IceCannonFire final : public SquidProcess { // 실제 Kernel35의 아이스 발사 파생이다.
public:
    IceCannonFire(const RawIceCannonFire& logic,IceCannonFireFields fields); // 같은 장기 논리와 현재80BYTE를 보유한다.
    void RunFrame() override; // Kernel의 현재 틱을 같은 원문 몸체로 전달한다.
    void OnAttached() override; // 실제 Host가 확정한 Form/Parent DWORD만 기록한다.
    IceCannonFireFields& Fields(); // 관찰/전송은 같은 실제 객체 상태를 사용한다.
private:
    const RawIceCannonFire& logic_; // Process보다 오래 사는 월드 논리다.
    IceCannonFireFields fields_; // padding을 포함한 원본80BYTE의 상태다.
};
GunAnimationHooks MakeIceCannonFireHostHooks(SquidProcessHost& host,GunAnimationHooks hooks); // 현재 GunProcess 완료와 실제 아이스 파생 삭제를 연결한다.
IceCannonFire* CreateIceCannonFire(SquidProcessHost& host,const RawIceCannonFire& logic,Sid gunForm,Sid target); // 같은 부모에35/flags10을 부착한다.
}
