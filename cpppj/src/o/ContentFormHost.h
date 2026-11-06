// 생산 목록 ContentForm의 정렬 부착·재부착·부모 알림을 원본 raw 저장소에 연결한다.
#pragma once
#include "o/RawCheckingFlag.h" // 원문 checking의 현재 DWORD 뷰와 독립 입력을 구별한다.
#include "o/SquidForms.h"
#include "o/RawContainedCreate.h"
#include <string_view>

namespace netstorm::o {
// 부모 가상 +0x24(form,reason)의 추가1/제거2 알림이다. 부모의 실제 생산/UI 효과를 연결한다.
using ContentFormNotice=std::function<void(Sid parent,Sid form,std::uint32_t reason)>;
// 실제 월드의 전투·강한 검사 전역이다. 부착 알림 뒤의 최신 값을 읽는다.
struct ContentFormState {
    RawCheckingFlag inBattle{}; // 부모 알림 뒤 원문00594fbc의 현재 전투 값을 읽는다.
    RawCheckingFlag checkingLinks{}; // 두 진단 위치에서 같은005e4794를 다시 읽는다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 식/파일/행을 전달하고 복귀 뒤 현재 상태를 다시 판정한다.
    std::function<std::uint32_t()> contentType; // 정렬 순회마다 현재00541088을 읽는다. 독립 입력만6을 쓴다.
};
// 사제를 워커/벌룬에 담은 전투 부착 뒤 포획 알림(004927f0/CD 0040df80)을 요청한다.
// 두 번째 인자는 내용물 +12의 하위7비트에 저장된 이전 사제 소유자다.
using ContentPriestNotice=std::function<void(Sid parent,std::uint32_t previousOwner)>;
// 유닛 목록의 정렬·그룹 플래그와 순서 BYTE를 관리한다. 특별한 생산/운반 삭제 효과는 월드 분배기의 책임이다.
class ContentFormHost {
public:
    // 풀·타입 입력·부모 알림의 수명은 연결기보다 길어야 한다. 타입 입력은 복사한다.
    ContentFormHost(SidPool& pool,std::span<const RiftTypeRecord> types,ContentFormNotice notice,
        FormPopTransmit transmit={},const ContentFormState* state=nullptr,ContentPriestNotice captured={});
    // 내부 제거 callback이 자신의 주소를 사용하므로 처리기는 복사/이동하지 않고 같은 수명으로 연결한다.
    ContentFormHost(const ContentFormHost&)=delete;
    ContentFormHost& operator=(const ContentFormHost&)=delete;
    // 004aec10/CD 004afff0: flags의 0x300을 제외한 그룹 안에서 기존 순서 번호보다 앞에 삽입한다.
    // 패치는 dead/free 진단 복귀 뒤 현재 상태로 재판정하며 CD의 같은 상태 입력은 무동작이다.
    void Attach(Sid form,Sid parent,std::uint32_t flags=0) const;
    // 공통 Form Unpop 뒤 살아 있는 부모에 제거2를 알린다. 부모 dead/free의 링크는 원본대로 보존한다.
    void Unpop(Sid form) const;
    void BindPop(std::function<void(Sid,std::uint32_t)> pop); // 정렬 링크 뒤 실제 World 공통 Pop 깊이/현재 진단을 대여한다.
    void BindUnpop(std::function<void(Sid)> unpop); // 실제 World의 공통 Factory가 현재 번호/부모 알림을 선택하도록 대여한다.
    void BindReattach(std::function<void(Sid,Sid,std::uint32_t)> reattach); // Regular 분리 뒤 현재 VT+50을 같은 Factory/정렬 호스트로 선택한다.
    void BindCurrentTypes(std::span<const RiftTypeRecord> types); // 같은 현재 표를 빌리고 독립 입력의 기존 사본은 유지한다.
    // 가상 +0x60의 직접 flags 조회다. WORLD 진단 복귀 뒤 현재 0x34를 읽으며 TYPE/상태를 선행 검사하지 않는다.
    bool Robust(Sid form) const;
    // 004ae080/CD 004af1c0: 부모 head에서 모든 next를 따라가 자기 SID 포함 여부를 검사한다.
    // 목록 flags 검사인 가상 +0x60(Robust)와 다르며 부착 알림 뒤 실제 연결 상태를 다시 확인한다.
    bool Linked(Sid form) const;
    // 가상 +0x5c: 양수 피해는 Unpop→0x100 플래그 해제→로컬 재부착 후 0을 반환한다.
    // 피해량이 0/음수이면 그대로 돌려준다. 재부착 부모 알림/원본 순서 BYTE를 함께 갱신한다.
    float Damage(Sid form,std::int32_t damage,float amount) const;
    // 기존 contained 생성기의 타입6 생성과 부착 경계를 같은 저장소로 연결한다.
    ContainedCreateHooks BindCreate(SquidFactory& factory,ContainedCreateHooks hooks) const;
    // 전체 월드의 Form 분배기가 같은 풀을 사용하는지 확인한다.
    const SidPool& Pool() const;
private:
    void Assert(std::string_view expression,std::uint32_t line) const; // 선택한10.78 진단만 요구하고 돌아오면 원문 본문을 계속한다.
    std::uint16_t Word(Sid sid,std::size_t offset) const;
    void Put(Sid sid,std::size_t offset,std::uint16_t value) const;
    // Form/contained 자산이 섞인 체인의 가상 previous getter/setter 위치를 구분한다.
    std::uint16_t Previous(Sid sid) const;
    void SetPrevious(Sid sid,std::uint16_t previous) const;
    std::uint32_t TypeGenus(std::uint32_t type) const; // 통지 뒤 현재 TYPE를 실제 소비 위치에서 읽는다.
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    std::span<const RiftTypeRecord> currentTypes_; // 같은 World의 고정 길이 표이며 호스트보다 오래 살아야 한다.
    // notice_: 통지.
    ContentFormNotice notice_;
    // forms_: 객체에 부착된 Form의 생성·통지·수명 처리기.
    SquidForms forms_;
    // state_: 실제 월드의 전투·강한 검사 전역이다. 부착 알림 뒤의 최신 값을 읽는다 (ContentFormState).
    const ContentFormState* state_{};
    // captured_: 포획된 사제 또는 현재 포획 상태 자료.
    ContentPriestNotice captured_;
    std::function<void(Sid,std::uint32_t)> pop_; // 실제 Session은 같은 World 공장에 후처리를 보내고 독립 입력은 기존 Form 몸체를 사용한다.
    std::function<void(Sid,Sid,std::uint32_t)> reattach_; // Regular의 분리/flags 쓰기 뒤 현재 가상 +50을 선택하는 실제 Session 장치다.
    std::function<void(Sid)> unpop_; // 미연결 독립 Host는 같은 공통 Form 분리와 입력 notice를 사용한다.
};
}
