// 아이스 타워147의 파괴·성장형150의 예약/성장/삭제를 원본 순서대로 복원한다.
#pragma once
#include "o/SidPool.h"
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o {
// 원본48ead0은 할당 성공 뒤 현재TYPE의 numFrames를 읽고 공통4b0d30만 호출한다.
void PostPopGrowingIce(const SidPool& pool,std::span<const RiftTypeFrames> frames,Sid parent,std::uint32_t flags,
    const std::function<bool()>& reserve,const std::function<void(Sid,std::uint32_t,float)>& regular,
    const std::function<void(Sid,std::uint32_t)>& base);
// 현재 원본 TYPE/로딩 전역이다. 모든 효과 뒤에도 같은 저장소를 읽는다.
struct IceTowerTypes {
    std::uint32_t ice{147},growing{150},noIsland{157}; //5412a4/5412b0/5412cc의 실제 현재값이다.
};
// 게임 세계의 실제 생성·공간·소유자·삭제와 공통 후처리를 공급한다.
struct IceTowerHooks {
    std::function<bool()> reserveRegular; //48ead0의 실제new40 성공/실패다.
    std::function<void(Sid,std::uint32_t,float)> regular; //같은 예약 메모리를 원본event3a6/주기로 부착한다.
    std::function<void(Sid,std::uint32_t)> basePop; //성장형의 마지막4b0d30이다.
    std::function<Sid(float,float)> grid; //40eaf0의 절삭한표면칸 첫SID다.
    std::function<Sid(float,float,std::uint32_t)> genusAt; //4b2070의 현재일반finder다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //4af530의 실제 생성자/예약이다.
    std::function<void(Sid,std::uint32_t)> unpop; //가상48이며 후처리중에도flags0으로 공간만 해제한다.
    std::function<void(Sid,std::uint16_t)> word; //4ac220의 현재WORD와 다리표면통지다.
    std::function<void(Sid,std::uint8_t)> owner; //가상74의 현재소유자 지정이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; //가상90의 같은현재좌표에 실제 재배치다.
    std::function<void(Sid,std::uint32_t)> destroy; //성장완료때 자기 가상10을 flags0으로 호출한다.
    std::function<void(float,float,std::uint32_t,std::uint32_t)> dropIsland; //46e6c0의 실제 받침정리다.
    std::function<void(Sid,std::uint32_t)> damageablePost; //완성형의 마지막44b9e0이다.
    std::function<void(Sid,std::uint32_t)> basePost; //성장형의 마지막4b0840이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; //4acee0의 표시/프레임 갱신이다.
};
// 참조는 같은 월드의 현재 원본 상태이며 이 객체보다 오래 살아야 한다.
class RawIceTower {
public:
    RawIceTower(SidPool& pool,std::span<const RiftTypeFrames> frames,const IceTowerTypes& types,
        const bool& battle,const bool& boss,const std::uint32_t& loadingDepth,IceTowerHooks hooks); //10.78의 현재188타입 프레임을 빌린다.
    void PostPop(Sid parent,std::uint32_t flags) const; //성장형48ead0의 실제 예약과 공통후처리다.
    void PostDestroy(Sid parent,std::uint32_t flags) const; //48edf0의 완성형 파괴 후 성장형 생성이다.
    void GrowingPostDestroy(Sid parent,std::uint32_t flags) const; //48eb80의 받침정리와 공통후처리다.
    float Handle(Sid parent,std::uint32_t event,std::uint32_t count,float payload) const; //48ebe0의 event3a6과 현재프레임 진행이다.
private:
    std::array<float,2> Position(Sid sid) const; //효과 뒤 현재좌표를 다시 읽는다.
    SidPool& pool_; //같은 raw50바이트를 읽고 QA/프레임을 쓴다.
    std::span<const RiftTypeFrames> frames_; //현재TYPE numFrames의 원본 코드표다.
    const IceTowerTypes& types_; //효과 뒤에도 현재타입전역을 읽는다.
    const bool& battle_; const bool& boss_; //재성장 생성과 최종타워 생성의 서로 다른 권한이다.
    const std::uint32_t& loadingDepth_; //완성형 파괴의 최초 로딩 관문이다.
    IceTowerHooks hooks_; //같은 월드의 실제 하위 몸체다.
};
}
