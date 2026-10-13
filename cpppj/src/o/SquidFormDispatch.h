// 월드·UI·프로세스의 내장 Form 가상 삭제를 같은 SID 풀/공통 삭제 깊이로 분배한다.
#pragma once
#include "o/ContentFormLifetime.h"
#include "o/DependFormHost.h"
#include "o/GumpFormHost.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 자산의 가상 +0x10 재정의(예: 다리 삭제 거부)를 보존하는 루트 삭제 경계다.
using SquidAssetDestroy=std::function<bool(Sid,std::uint32_t,const SquidDestroyHooks&)>;
// 타입별 서로 다른 fallback 사슬을 같은 분배기로 모아 혼합 종속 체인의 중첩 삭제를 지원한다.
class SquidFormDispatch {
public:
    // 자산 기본 효과와 선택 경계를 유지한다. 풀·공통 삭제기·등록한 호스트들은 이 객체보다 오래 살아야 한다.
    SquidFormDispatch(SidPool& pool,RawSquidDestroy& destroy,SquidDestroyHooks assets,SquidAssetDestroy assetDestroy={});
    // 각 호스트 생성 시 fallback에 이 객체의 Hooks를 넘긴 뒤 등록한다. 등록 전에 실제 삭제하지 않는다.
    void Bind(DependFormHost& forms);
    void Bind(GumpFormHost& forms);
    void Bind(ContentFormLifetime& forms);
    void Bind(SquidProcessHost& processes);
    // 현재 타입별 가상 루트 삭제다. 자산의 별도 재정의는 assetDestroy를 사용하고 Form은 공통 몸체에 들어간다.
    bool Destroy(Sid sid,std::uint32_t flags=0) const;
    // 모든 Form 효과/Unpop은 현재 타입에 따라 분배하며 전송·선택 해제는 공통 자산 경계로 곧바로 전달한다.
    SquidDestroyHooks Hooks() const;
    // 초기화/진단 시 같은 SID 풀이 연결되어 있는지 확인한다.
    const SidPool& Pool() const;
private:
    // 등록하지 않은 Form 타입은 임의 기본 동작을 만들지 않고 필요한 실제 가상 호스트를 요구한다.
    const SquidDestroyHooks& Route(Sid sid) const;
    // 서로 다른 풀을 연결하지 않고 동일 타입 호스트의 재등록도 명시적으로 거부한다.
    void Register(const SidPool& pool,std::uint32_t type,SquidDestroyHooks hooks);
    SidPool& pool_;
    RawSquidDestroy& destroy_;
    SquidDestroyHooks assets_;
    SquidAssetDestroy assetDestroy_;
    std::array<SquidDestroyHooks,kFirstAssetTypeNumber> routes_;
    std::array<bool,kFirstAssetTypeNumber> bound_{};
};
}
