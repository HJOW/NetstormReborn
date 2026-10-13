// 생산 목록 ContentForm의 정렬 부착·재부착·부모 알림을 원본 raw 저장소에 연결한다.
#pragma once
#include "o/SquidForms.h"
#include "o/RawContainedCreate.h"

namespace netstorm::o {
// 부모 가상 +0x24(form,reason)의 추가1/제거2 알림이다. 부모의 실제 생산/UI 효과를 연결한다.
using ContentFormNotice=std::function<void(Sid parent,Sid form,std::uint32_t reason)>;
// 실제 월드의 전투·강한 검사 전역이다. 부착 알림 뒤의 최신 값을 읽는다.
struct ContentFormState { bool inBattle{},checkingLinks{}; };
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
    // 패치의 dead/free assert 경로는 거부하며 CD의 같은 상태 입력은 무동작이다.
    void Attach(Sid form,Sid parent,std::uint32_t flags=0) const;
    // 공통 Form Unpop 뒤 살아 있는 부모에 제거2를 알린다. 부모 dead/free의 링크는 원본대로 보존한다.
    void Unpop(Sid form) const;
    // 가상 +0x60: WORLD(0x80) 목록은 assert 경로이며 0x34 중 하나라도 있으면 true다.
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
    // 타입6 가상 표·free 상태·원본 WORD 필드를 확인한다.
    void Form(Sid form) const;
    std::uint16_t Word(Sid sid,std::size_t offset) const;
    void Put(Sid sid,std::size_t offset,std::uint16_t value) const;
    // Form/contained 자산이 섞인 체인의 가상 previous getter/setter 위치를 구분한다.
    std::uint16_t Previous(Sid sid) const;
    void SetPrevious(Sid sid,std::uint16_t previous) const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    ContentFormNotice notice_;
    SquidForms forms_;
    const ContentFormState* state_{};
    ContentPriestNotice captured_;
};
}
