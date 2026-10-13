// Squid.cpp의 감시 쌍과 부모 삭제 통지를 같은 SID 풀·공통 삭제 처리기에 연결한다.
#pragma once
#include "o/SquidForms.h"
#include "o/RawSquidDestroy.h"

namespace netstorm::o {
// watched 부모가 삭제될 때 watcher 부모의 가상 +0x34(watched,form)를 호출하는 경계다.
using DependFormNotice=std::function<void(Sid watcher,Sid watched,Sid form)>;
// 두 Form이 서로를 참조하는 방향을 명시한다. watchedForm은 watched, watcherForm은 watcher 안에 붙는다.
struct DependFormPair { Sid watchedForm{},watcherForm{}; };
// watcher/watched 쌍 생성·조회·제거·가상 preDestroy와 부모 통지를 복원한다.
class DependFormHost {
public:
    // 풀·삭제 처리기는 이 객체보다 오래 살아야 한다. 다른 Form/자산의 가상 분배는 fallback으로 보존한다.
    DependFormHost(SidPool& pool,RawSquidDestroy& destroy,DependFormNotice notice={},
        SquidDestroyHooks fallback={},FormPopTransmit transmit={});
    // 004b1510/CD 004af940: 어느 한 부모가 client 번호면 두 Form 모두 client 풀에 만든다.
    DependFormPair Create(Sid watched,Sid watcher) const;
    // 004ae900/CD 004afbc0: watched의 Form 체인에서 watcher 부모가 일치하는 첫 Depend를 찾는다.
    bool Has(Sid watched,Sid watcher) const;
    // 004ae780/CD 004afa50: watched가 dead면 무동작이며 일치하는 모든 감시 쌍을 같은 삭제기로 제거한다.
    void Remove(Sid watcher,Sid watched,std::uint32_t flags=0) const;
    // +0x12 watcherForm을 통해 상대 부모를 읽는다. +0x12가 0이면 부모0을 반환한다.
    Sid Watcher(Sid form) const;
    // 가상 +0x1c: client 쌍 또는 서버 권한일 때 watcher 부모에 알리고 watchedFid==0을 재확인한다.
    void Notify(Sid form,Sid watched) const;
    // 가상 +0x10의 실제 공통 삭제를 실행한다. 상대 먼저 삭제→자신 Unpop→SID 반납을 공유한다.
    void Destroy(Sid form,std::uint32_t flags=0) const;
    // 타입1만 처리하고 다른 타입의 기존 분배를 보존한다. 전체 월드의 삭제 분배기로 사용한다.
    SquidDestroyHooks Hooks() const;
    // 원본 타입1 판정과 공유 풀 조회를 연결기/진단에 제공한다. free는 별도로 확인한다.
    bool IsDependForm(Sid form) const;
    const SidPool& Pool() const;
private:
    // 살아 있거나 삭제 중인 타입1 Form의 현재 raw 상태를 확인한다.
    void Form(Sid form) const;
    // getter/store 폭을 WORD로 제한하여 다른 payload를 보존한다.
    Sid Word(Sid sid,std::size_t offset) const;
    void Put(Sid sid,std::size_t offset,Sid value) const;
    // watcher/watched 유효 번호와 원본의 금지 타입·같은 객체 조건을 확인한다.
    void Parents(Sid watcher,Sid watched) const;
    SidPool& pool_;
    RawSquidDestroy& destroy_;
    DependFormNotice notice_;
    SquidDestroyHooks fallback_;
    SquidForms forms_;
};
}
