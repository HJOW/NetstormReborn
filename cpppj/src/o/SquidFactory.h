// 원본 Squid.cpp의 자산 생성/void Take와 공통 가상 초기화를 raw SID 풀에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidForms.h" // 공통 부착의 실제 Pop 전송 장치 타입을 공유한다.
#include "o/RawCheckingFlag.h"
#include <functional>
#include <string_view>
#include <memory> //장기공장은현재World의짧은가상장치를약하게만빌린다.

namespace netstorm::o {
class SquidUnpop;
struct SquidFactoryDiagnostics { // 같은 World/Session의 현재 TYPE와 정상 복귀 가능한 원문 진단을 빌린다.
    std::span<const RiftTypeRecord> types; // 비어 있으면 기존 독립 생성자의 TYPE 사본을 사용한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // Take/PostTake의 원문 식/파일/줄을 실제 진단 장치에 전달한다.
    std::function<void(std::string_view)> detail; // 00436d20의 TYPE 불일치 문구를 같은 Client의005e4798 소유자에 복사한다.
    RawCheckingFlag checking{}; // TYPE HP의 선택된 진단에서 현재005e4794를 읽는다.
};
struct SquidFactoryRuntimeBindings { //실제 현재 World의 공간/Form 대여이며 Factory/Pool의 소유권과 구별한다.
    SquidUnpop* unpop{}; //Take가 선택한 실제 자산+48이며 같은Pool의현재Unpop을빌린다.
    std::function<void(Sid)> formDetach; //현재Session약한종단또는같은World의실제공통종속분리다.
    std::function<std::uint32_t()> contentType; //현재내용물타입getter이며선택한Attach위치에서읽는다.
    FormPopTransmit formTransmit; //실제현재Form전송장치이며빈전송성공을설치하지않는다.
    std::function<std::shared_ptr<void>()> pin; //실제Client가공개한전체Runtime을선택된호출끝까지보유하며독립typed대여는비워둔다.
};
// SID 슬롯을 할당하고 타입별 생성자 필드를 초기화하는 팩토리이다.
class SquidFactory {
public:
    // 타입 입력은 복사하여 호출자 수명과 분리한다. 약화 mana 옵션은 원본 HP /4 분기다.
    SquidFactory(SidPool& pool,std::span<const RiftTypeRecord> types,bool weakenedMana=false,SquidUnpop* unpop=nullptr);
    void BindUnpop(std::function<SquidUnpop&()> unpop); //선택된Take위치에서같은장기Unpop실행기를조회한다. 생성만으로공간/UI를요구하지않는다.
    void BindRuntime(const std::shared_ptr<const SquidFactoryRuntimeBindings>& runtime); //같은현재대여만약하게기록하며실제호출은끝날때까지선택된장치를pin한다.
    // 004af530 ↔ CD 004ab390. 복원된 자산 생성자 또는 constructor=0 fallback을 호출한다.
    Sid Create(std::uint32_t type,std::uint32_t flags=0);
    // 생산 칸 contained Form(타입 6)의 전용 생성이다. 같은 SID 할당 뒤 판본별 가상 표/타입만 쓴다.
    // 현재 TYPE6의 cdecl과 반환 VT+28을 조회한다. 정상 원문6 생성자는004aea40/CD004afe00이며+28은 빈 함수다.
    Sid CreateContained(std::uint32_t flags=0);
    // UI 객체 부착용 GumpForm(타입3)도 같은 현재 Create 부모에 전달한다. 메뉴의 기본 할당 flags는2다.
    // 정상 원문 생성자004ae9f0/CD004afd60의 VT만 쓰고 빈+28을 호출하며 현재 표의 다른 생성자를 고정 VT로 덮지 않는다.
    Sid CreateGumpForm(std::uint32_t flags=2);
    // 생성 효과 어댑터가 같은 실제 SID 풀을 연결하는지 확인한다.
    const SidPool& Pool() const;
    void BindDiagnostics(SquidFactoryDiagnostics diagnostics); // 같은 판본 길이의 현재 TYPE와 실제 진단 장치를 연결한다.
    void BindFormDetach(std::function<void(Sid)> detach); // 현재 World/Session의 공통 종속 분리만 대여하며 UI 검색을 추가하지 않는다.
    void BindFormPopDepth(std::uint32_t& current); // 실제 World의 같은 postPop DWORD만 빌리며 다른 주소 재연결은 거부한다.
    void BindFormPopDepth(std::uint32_t&&)=delete; // 임시 깊이의 수명 밖 참조를 막는다.
    void PopForm(Sid form,std::uint32_t flags,FormPopTransmit transmit={}); // 현재 진단/깊이와 실제 VT+20의 공통 Pop을 실행한다.
    void BindFormAttach(std::function<std::uint32_t()> contentType,FormPopTransmit transmit={}); // 현재 Session 번호/전송을 선택 위치에서 빌린다.
    void AttachForm(Sid form,Sid parent,std::uint32_t flags,FormPopTransmit transmit={}); // 파생 +74 접두 뒤 공통 부모만 실행한다.
    void UnpopForm(Sid sid); // 실제 Form VT+48의 진단/void/현재 getter와 부모 free 관문을 실행한다.
    void BindWeakenedMana(const bool& current); // 실제005ca8f0 주소만 빌린다. 다른 주소 재연결은 거부하며 대상은 이 공장보다 오래 살아야 한다.
    void BindWeakenedMana(bool&&)=delete; // 임시 bool의 주소를 보관해 수명이 끝나는 연결은 컴파일 시 거부한다.
    void BindWeakenedMana(const bool&&)=delete; // const 임시 bool도 같은 장기 참조 계약을 적용한다.
    void BindHitPointTypes(const std::uint32_t* priest,const std::uint32_t* dais); // 같은 장기 TYPE 두 주소를 빌린다. 독립 미연결 입력만 기존158/165를 사용한다.
    void BindFakeSurfaceType(const std::uint32_t& current); // Create가 같은 장기005412e0을 읽는다. 임시/다른 주소는 연결하지 않는다.
    void BindFakeSurfaceType(std::uint32_t&&)=delete; // 임시 DWORD의 수명 밖 참조를 거부한다.
    void BindFakeSurfaceType(const std::uint32_t&&)=delete; // const 임시도 장기 owner가 아니다.
    // 이미 확보된 슬롯에 생성자 쓰기만 적용한다. type/상태/풀 목록은 덮지 않는다.
    Sid Construct(std::uint32_t type,Sid sid);
    void ConstructCurrent(Sid sid); //004af720/CD004ab770: 현재 raw TYPE의 생성자만 실행하고 반환 객체는 무시한다.
    // bare cdecl 주소의 원문 쓰기만 물리 SID에 적용한다. 미지원은false이며 TYPE/할당/가상 초기화를 추가하지 않는다.
    // PATCH NULL 함수는 SID를 읽지 않고true, Dais 보조 진단은 정상 복귀 뒤 같은 슬롯에 VT를 저장한다.
    bool ConstructByAddress(std::uint32_t address,Sid sid);
    void Retype(Sid sid,std::uint32_t type,const std::function<void(std::uint32_t)>& assertion={}); //004b1620의 void/foot 진단→TYPE BYTE→생성자만 실행한다. HP/소유자/할당은 보존한다.
    // 004af610 ↔ CD 004ab440의 지원 자산 수신 경로다. free list/freeCount는 변경하지 않는다.
    // 두 판본의 모든 !free 슬롯은 void 여부와 무관하게 실제 VT+48을 호출하고 현재 생성자 반환 객체를 사용한다.
    Sid Take(std::uint32_t type,Sid sid);
    // 004ad470 ↔ CD 004ae0f0의직접몸체다. TYPE/상태/VT없이extra의abstract/buried두비트만읽는다.
    std::uint32_t FirstPopFlags(Sid sid);
    // 004b0c60 ↔ CD 004ae040. 파생 클래스가 base 초기화를 호출할 때도 사용한다.
    void PostCreate(Sid sid);
    std::int32_t ReadSelectedHitPoints(const RiftTypeRecord& type,std::span<const RiftTypeRecord> selectedBase) const; // 표시/전송이 저장한 TYPE this를 같은 현재 HP getter와 장기 전역으로 읽는다.
    // 004ad4a0 ↔ CD 004ae120. zOrder만 갱신하며 owner·HP는 보존한다.
    void PostTake(Sid sid);
    // 현재 TYPE 생성자의 실제 최종 VT 기록값이다. 메뉴 등 가상 분배기는 이 값과 현재 raw VT를 먼저 대조한다.
    // 호스트 함수 포인터로 역참조하지 않으며 미등록 생성자는 기존 조회와 같이 거부한다.
    std::uint32_t ConstructorVtable(const RiftTypeRecord& type) const;
    static bool SupportsConstructedVtable(OriginalEdition edition,std::uint32_t table); // 같은 판본의 base/검증된 recipe 표에 있는 현재 VT인지 확인한다. TYPE와 VT의 쌍 일치를 강제하지 않는다.
private:
    // 주소 산술 전에 타입 범위와 원본 생성자 지원 여부를 검증한다.
    const RiftTypeRecord& Type(std::uint32_t number,bool requireSupported) const;
    const RiftTypeRecord& TypeRecord(std::uint32_t number) const; // 진단 복귀 뒤 Form 번호도 포함한 현재 TYPE 한 항목을 읽는다.
    void BindCurrentFormPop(SquidForms& forms) const; // 같은 공장의 현재 진단/공유 깊이를 Attach와 직접 Pop에 함께 연결한다.
    std::span<const RiftTypeRecord> CurrentTypes() const; // 현재 borrowed 표 또는 기존 독립 소유 표를 복사 없이 읽는다.
    std::int32_t TypeHitPoints(const RiftTypeRecord& type,std::span<const RiftTypeRecord> selectedBase) const; // 같은 TYPE this와 진단 뒤 현재 base/전역으로 원문 HP를 읽는다.
    void Assert(std::string_view expression,std::uint32_t line) const; // 양판본 진단이 정상 복귀하면 현재 raw를 다시 읽어 계속한다.
    // 원본 생성자의 쓰기 순서/폭/OR 비트를 보존한다. 호출 전에 지원 여부를 검사한다.
    void ApplyConstructor(std::span<std::uint8_t> bytes,const RiftTypeRecord& type) const;
    Sid ConstructRaw(std::uint32_t address,Sid sid,bool requireObject); // base/자산/Form의 bare 쓰기를 실행하며 void NULL의 반환을 SID로 위조하지 않는다.
    void VirtualPostCreate(Sid sid,std::uint32_t table); // 실제 생성자가 남긴VT의+28을 선택한다. 네 Form은 원문 빈 함수다.
    // 원본 32비트 vtable 값은 기록할 뿐 호스트에서 역참조하지 않는다.
    std::uint32_t BaseVtable() const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    SquidFactoryDiagnostics diagnostics_; // 실제 소비자는 현재 TYPE span을 빌리고 독립 검사는 기존 사본을 사용한다.
    // weakenedMana_: 약화된 마나 여부.
    bool weakenedMana_{};
    const bool* sharedWeakenedMana_{}; // 생성자 HP 쓰기가 읽는 현재 장기 전역이며 기존 값형 검사는 미연결이다.
    const std::uint32_t* sharedPriestType_{};const std::uint32_t* sharedDaisType_{}; // TYPE HP가 호출 위치에서 읽는 같은 장기 두 DWORD다.
    const std::uint32_t* sharedFakeSurfaceType_{}; // 같은 Client 수명의 현재 fakeThreeByThreeSurface TYPE이며 독립 미연결만162다.
    // unpop_: 객체의 공간 등록을 해제하는 Unpop 처리기.
    SquidUnpop* unpop_{};
    std::function<SquidUnpop&()> currentUnpop_; //실제Storage의같은공간부모이며독립raw포인터계약은유지한다.
    std::weak_ptr<const SquidFactoryRuntimeBindings> runtime_; //World반납뒤raw장치/Session주소를공장에남기지않으며독립공장은종전필드를쓴다.
    std::uint32_t* formPostPopDepth_{}; // 실제 공장보다 오래 사는 World bookkeeping의 같은 현재 깊이다.
    std::function<std::uint32_t()> formContentType_; // 약한 현재 Session 또는 World 기본 번호 getter다.
    FormPopTransmit formTransmit_; // 실제 World의 현재 Form Pop 전송이며 호출자 명시 장치가 있으면 그것을 사용한다.
    std::function<void(Sid)> formDetach_; // 실제 소비자는 현재 Session의 약한 대여 또는 World 기본 종단을 호출한다.
};
}
