// Squid.cpp의 감시 쌍과 부모 삭제 통지를 같은 SID 풀·공통 삭제 처리기에 연결한다.
#pragma once
#include "o/SquidForms.h"
#include "o/RawSquidDestroy.h"

namespace netstorm::o {
class SquidFactory; // 실제 World의 공통 생성/현재 생성자 표를 짧게 빌리는 전방 선언이다.
// watched 부모가 삭제될 때 watcher 부모의 가상 +0x34(watched,form)를 호출하는 경계다.
using DependFormNotice=std::function<void(Sid watcher,Sid watched,Sid form)>;
// 두 Form이 서로를 참조하는 방향을 명시한다. watchedForm은 watched, watcherForm은 watcher 안에 붙는다.
struct DependFormPair { /* watchedForm: 암묵적인 런타임 SquidId 변환을 막는 원본 슬롯 번호다 (Sid). */ /* watcherForm: 암묵적인 런타임 SquidId 변환을 막는 원본 슬롯 번호다 (Sid). */ Sid watchedForm{},watcherForm{}; };
// watcher/watched 쌍 생성·조회·제거·가상 preDestroy와 부모 통지를 복원한다.
class DependFormHost {
public:
    void BindUnpop(std::function<void(Sid)> unpop); // 실제 World 공장의 현재 진단/종속 분리를 연결한다.
    void BindVirtualDestroy(std::function<void(Sid,std::uint32_t)> destroy,std::function<void(Sid)> beforeFind); // Native는 현재 상대 VT의 실제 삭제와 FormFinder 시작 진단을 같은 장기 owner에 연결한다.
    void BindCreation(SquidFactory& factory,const std::uint32_t& type,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion={}); // 같은 풀의 공장/장기 현재 번호와 원문 진단을 연결한다.
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
    // 실제 감시 클래스 VT 판정과 공유 풀 조회를 연결기/진단에 제공한다. free는 별도로 확인한다.
    bool IsDependForm(Sid form) const;
    const SidPool& Pool() const;
private:
    std::function<void(Sid,std::uint32_t)> virtualDestroy_; // Native 상대 삭제는 현재 클래스의 가상+10을 다시 선택하며 독립 typed 계약은 기존 검사/삭제를 유지한다.
    std::function<void(Sid)> beforeFind_; // 현재 checking/메타맵/권한 진단 뒤 부모 head를 읽게 하는 원문 FormFinder 시작 경계다.
    std::function<void(Sid)> unpop_; // 독립 입력의 미연결은 기존 forms_의 같은 공통 해제를 사용한다.
    SquidFactory* factory_{}; // 실제 World와 같은 수명의 공장이며 독립 입력은 같은 공통 Factory를 호출마다 구성한다.
    const std::uint32_t* dependType_{}; // 미연결 독립 입력만 원문 초기1을 쓰고 실제 World는 장기 DWORD를 빌린다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> creationAssertion_; // Create 이전 server 진단이 정상 복귀하면 원문 flags0을 유지한다.
    std::uint32_t DependType() const; // 검색/금지 타입 검사와 각 Create가 같은 현재 전체 번호를 읽는다.
    // 살아 있거나 삭제 중인 감시 클래스의 현재 raw/VT 상태를 확인한다.
    void Form(Sid form) const;
    // getter/store 폭을 WORD로 제한하여 다른 payload를 보존한다.
    Sid Word(Sid sid,std::size_t offset) const;
    void Put(Sid sid,std::size_t offset,Sid value) const;
    // watcher/watched 유효 번호와 원본의 금지 타입·같은 객체 조건을 확인한다.
    void Parents(Sid watcher,Sid watched) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // destroy_: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    RawSquidDestroy& destroy_;
    // notice_: 통지.
    DependFormNotice notice_;
    // fallback_: emit은 같은 풀을 바꿀 수 있다. 종속 next는 ReleaseDependent 전에 저장한다 (SquidDestroyHooks).
    SquidDestroyHooks fallback_;
    // forms_: 객체에 부착된 Form의 생성·통지·수명 처리기.
    FormPopTransmit transmit_; // 독립/실제 생성에서 같은 명시 Pop 전송을 공통 부모에 전달한다.
    SquidForms forms_;
};
}
