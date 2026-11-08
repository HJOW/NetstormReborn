// 월드·UI·프로세스의 내장 Form 가상 삭제를 같은 SID 풀/공통 삭제 깊이로 분배한다.
#pragma once
#include "o/ContentFormLifetime.h"
#include "o/DependFormHost.h"
#include "o/GumpFormHost.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 자산의 가상 +0x10 재정의(예: 다리 삭제 거부)를 보존하는 루트 삭제 경계다.
using SquidAssetDestroy=std::function<bool(Sid,std::uint32_t,const SquidDestroyHooks&)>;
// 현재가상클래스별fallback사슬을같은분배기로모아혼합종속체인의중첩삭제를지원한다.
class SquidFormDispatch {
public:
    // 자산 기본 효과와 선택 경계를 유지한다. 풀·공통 삭제기·등록한 호스트들은 이 객체보다 오래 살아야 한다.
    SquidFormDispatch(SidPool& pool,RawSquidDestroy& destroy,SquidDestroyHooks assets,SquidAssetDestroy assetDestroy={});
    // 각 호스트 생성 시 fallback에 이 객체의 Hooks를 넘긴 뒤 등록한다. 등록 전에 실제 삭제하지 않는다.
    void Bind(DependFormHost& forms);
    void Bind(GumpFormHost& forms);
    void Bind(ContentFormLifetime& forms);
    void Bind(SquidProcessHost& processes);
    // 같은 풀의 현재 자산 훅을 교체한다. 실제 PATCH는 원문83VT의 최초 World +10 부모를 보존하고 typed/CD만 assetDestroy도 교체한다.
    void SetAssets(SquidDestroyHooks assets,SquidAssetDestroy assetDestroy);
    // 실제 PATCH는 현재83자산/네Form VT의 +10을 선택한다. 미연결 typed/CD는 기존 TYPE별 자산/공통 분배를 유지한다.
    bool Destroy(Sid sid,std::uint32_t flags=0) const;
    // Form범위의효과/Unpop은현재VT로분배하며전송·선택해제는공통자산경계로전달한다.
    SquidDestroyHooks Hooks() const;
    // 초기화/진단 시 같은 SID 풀이 연결되어 있는지 확인한다.
    const SidPool& Pool() const;
private:
    // 등록하지 않은 Form 타입은 임의 기본 동작을 만들지 않고 필요한 실제 가상 호스트를 요구한다.
    const SquidDestroyHooks& Route(Sid sid) const;
    // 서로 다른 풀을 연결하지 않고 동일 타입 호스트의 재등록도 명시적으로 거부한다.
    void Register(const SidPool& pool,std::uint32_t type,SquidDestroyHooks hooks);
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // destroy_: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    RawSquidDestroy& destroy_;
    // assets_: 타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계.
    SquidDestroyHooks assets_;
    // assetDestroy_: 자산 삭제.
    SquidAssetDestroy assetDestroy_;
    // routes_: 타입 번호별로 선택할 삭제·통지 처리 경로.
    std::array<SquidDestroyHooks,kFirstAssetTypeNumber> routes_;
    // bound_: 타입 번호별 분배 경계가 연결됐는지.
    std::array<bool,kFirstAssetTypeNumber> bound_{};
};
}
