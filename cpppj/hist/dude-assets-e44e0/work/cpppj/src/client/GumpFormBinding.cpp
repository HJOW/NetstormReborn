// 호스트 Form 토큰에는 원본 SID를 사용한다. 32비트 원본 포인터/가상 표를 실행 주소로 변환하지 않는다.
#include "client/GumpFormBinding.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본의 long SID를 WORD로 조용히 절단하지 않고 실제 번호 표현 범위를 먼저 확인한다.
o::Sid Sid(std::uint32_t value) {
    if (value>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("GumpForm SID 표현 범위 초과");
    return o::Sid{static_cast<std::uint16_t>(value)};
}
}
// 매 알림에서 현재 등록 표를 읽으므로 생성 당시의 UI 주소/토큰을 저장하지 않는다.
o::GumpFormNotice MakeGumpFormNotice(GumpCreateState& state,GumpObjectRelease releaseObject) {
    if (!releaseObject) throw std::invalid_argument("Gump 가상 객체 삭제 알림 경계 필요");
    // 소유 ID 0 생략은 Form의 Notify가 담당하며 직접 조회에는 원본 등록 표 전체를 사용한다.
    return [&state,releaseObject=std::move(releaseObject)](std::uint32_t owner,o::Sid object) {
        if (owner>=state.entries.size()) throw std::out_of_range("GumpForm 소유 등록 ID 범위 초과");
        const auto token=state.entries[owner];
        if (!token) throw std::runtime_error("GumpForm 소유 UI 등록이 없습니다");
        releaseObject(token,object);
    };
}
// 원본 기본 통지는 부모 SID를 버린다. Form을 먼저 지우고 UI는 기존 지연 삭제 큐에서 나중에 소멸한다.
void ReleaseGumpObject(const GumpLifetime& lifetime,std::uint32_t token,o::Sid) { lifetime.Kill(token); }
// +0x28에는 같은 풀의 SID를 기록한다. 토큰/index를 동일 번호로 제공하면 원본 stride 나눗셈과 같은 결과다.
void BindMenuGumpForms(GumpCreateState& create,MenuGumpWidgetState& state,MenuGumpWidgetHooks& hooks,o::GumpFormHost& forms) {
    if (create.collection.edition!=forms.Pool().Edition()) throw std::invalid_argument("메뉴와 GumpForm 판본이 다릅니다");
    state.objectFormType=o::kGumpFormType;
    // 원본 타입 전역의 다른 값은 별도 Form 재정의가 필요하므로 GumpForm 생성에서 거부한다.
    hooks.objectForm=[&forms](std::uint32_t type,std::uint32_t flags) {
        if (type!=o::kGumpFormType) throw std::invalid_argument("메뉴 객체 부착 GumpForm 타입 오류");
        const auto form=forms.Create(flags);return MenuGumpObjectForm{form.value,form.value};
    };
    // 등록 ID는 DWORD 전체를 기록하고 객체 SID는 원본 번호 범위를 확인해 부착한다.
    hooks.formOwner=[&forms](std::uint32_t form,std::uint32_t owner) { forms.SetOwner(Sid(form),owner); };
    // 가상 +0x74는 소유 ID를 쓴 뒤 같은 풀의 객체 종속 체인에 연결한다.
    hooks.formObject=[&forms](std::uint32_t form,std::uint32_t object,std::uint32_t flags) { forms.Attach(Sid(form),Sid(object),flags); };
}
// Form 삭제는 UI 알림 없이 공통 삭제에 들어간다. 객체 삭제 알림에서 중첩 호출돼도 dead 분기가 순서를 유지한다.
void BindGumpFormLifetime(GumpLifetimeHooks& hooks,o::GumpFormHost& forms) {
    const auto previous=hooks.deleteForm;
    // 현재 타입으로 분배하며 반납된 GumpForm의 dead 기록도 공통 몸체에 전달한다.
    hooks.deleteForm=[&forms,previous](std::uint32_t form,std::uint32_t flags) {
        const auto sid=Sid(form);
        if (forms.IsGumpForm(sid)) forms.Destroy(sid,flags);
        else { if (!previous) throw std::invalid_argument("GumpForm 외 UI Form 삭제 경계 필요");previous(form,flags); }
    };
}
}
