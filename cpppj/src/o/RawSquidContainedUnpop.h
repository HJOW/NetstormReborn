// 공통 contained 해제의 종속 링크와 내용물 UI 요청을 같은 실제 SID 풀에 연결한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include <functional>
#include <memory> //약한Session종단이실제외곽owner를호출끝까지보유한다.
#include <string_view>

namespace netstorm::o { // Form 전용 Unpop과 구별되는 공통 종속 해제 몸체다.
struct SquidContainedUnpopState { // 타입/사건은 현재 DWORD를 읽고 검색 진단은 같은 Client 전역을 빌린다.
    const std::uint32_t& contentFormType; // PATCH00541088/CD0051c978의 부모 통지 대상 타입이다.
    const std::uint32_t& gumpFormType; // PATCH0054107c/CD0051c96c의 UI 종속 검색 타입이다.
    const std::uint32_t& uiEvent; // PATCH005427dc/CD00512964의 현재 내용물 변경 사건이다.
    RawCheckingFlag checkingDead; // PATCH005e4794의 현재 전체 진단 DWORD다.
    std::function<bool()> inMetamapMode; // PATCH00594fc8의 현재 비영 관문이다.
    std::function<bool()> boss; // PATCH00540bc4의 현재 권한 관문이다.
};
struct SquidContainedUnpopHooks { // 원문 가상 호출과 큐/진단의 선택된 실제 몸체만 요구한다.
    std::function<Sid(Sid)> parent; // 현재 VT+0의 부모 getter이며 asset과 Form의 필드가 다르다.
    std::function<Sid(Sid)> previous; // 현재 VT+4의 이전 getter이며 next 저장 뒤 호출한다.
    std::function<void(Sid,Sid)> setPrevious; // 다음 항목의 현재 VT+0xc에 이전 번호를 쓴다.
    std::function<void(Sid,Sid,std::uint32_t)> parentNotice; // 저장 부모의 현재 VT+0x24에 제거 이유2를 전달한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> command; // 실제00452fe0/CD004a6aa0의 현재 입력 큐다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 정상 복귀하면 저장값/현재값을 구별해 계속한다.
};
// 두 PE의 생성자 VT에서 확인한 공통 부모/이전 getter를 실행한다. 모르는 VT는 거부한다.
Sid CommonContainedParent(const SidPool& pool,Sid sid);
Sid CommonContainedPrevious(const SidPool& pool,Sid sid); // Form의+0xe와 자산의+0x10 WORD를 실제 VT로 구별한다.
Sid CommonContainedPreviousAtTable(const SidPool& pool,Sid sid,std::uint32_t table); // 정렬 부모가 이미 선택한 VT+4를 실행한다.
void CommonContainedSetParent(SidPool& pool,Sid sid,Sid parent); // 현재 VT+8의 Form+10/자산+e WORD를 쓴다.
void CommonContainedSetPrevious(SidPool& pool,Sid sid,Sid previous); // 같은 실제 VT의 setPrev 쓰기 폭을 보존한다.
void CommonContainedSetPreviousAtTable(SidPool& pool,Sid sid,std::uint32_t table,Sid previous); // 이웃 getter 앞에서 저장한 VT+c 배치를 사용한다.
class RawSquidContainedUnpop { // PATCH004ac6b0→004ad230/CD004ac160→004acf40의 실제 종단이다.
public:
    RawSquidContainedUnpop(SidPool& pool,SquidContainedUnpopState state,SquidContainedUnpopHooks hooks); // 상태 참조/풀은 처리기보다 오래 살아야 한다.
    void BindRuntimePin(std::function<std::shared_ptr<void>()> pin); //실제현재Session종단만전체owner를pin하며장기기본종단은비워둔다.
    void BindChecking(RawCheckingFlag checking) { state_.checkingDead=checking; } //현재Storage진단view가교체되어도같은종단의다음조회에반영한다.
    void Detach(Sid sid) const; // void를 다시 쓰지 않고 종속 체인/현재 contained와 부모 통지만 처리한다.
    void NotifyUi(Sid sid) const; // 이 객체 자신의 종속 Form에 GumpForm이 있으면 현재 사건을 한 번 큐에 넣는다.
    void Unpop(Sid sid) const; // 분리가 정상 반환하면 부모 dead 여부와 무관하게 자신의 UI 종속을 조회한다.
    std::uint32_t ContentFormType() const; // 실제 부착 꼬리가 같은 Session의 현재 전체 번호를 읽는다.
    const SidPool& Pool() const; // World/Session 연결기가 같은 실제 풀인지 확인한다.
private:
    void Assert(std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) const; // 판본별 Squid.cpp 진단 위치를 보존한다.
    SidPool& pool_; // Form/자산/UI 종속이 함께 사용하는 현재 물리 풀이다.
    SquidContainedUnpopState state_; // 값 사본 대신 살아 있는 현재 전역을 조회한다.
    std::function<std::shared_ptr<void>()> runtimePin_; //Formgetter/부모통지/UI가빌린현재Session의실제전체수명이다.
    SquidContainedUnpopHooks hooks_; // 진단/가상 통지 뒤 원문 재조회 순서를 보존하는 실제 장치다.
};
}
