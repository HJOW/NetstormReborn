// 원본 정상 경로의 현재 필드 읽기/외부 호출/표 쓰기 순서를 보존한다.
#include "client/GumpLifetime.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 Gump flags와 판본별 기본 가상 표의 raw 주소다.
constexpr std::uint32_t kIdle=0x80,kOverlay=0x80000,kVoid=8,kDead=0x10000,kDestroying=4,kUnregistered=0x100000;
constexpr std::uint32_t kPatchVtable=0x50ac70,kCdVtable=0x5048a8;
}
// 저장소 조회만 필수다. 실제 도달한 외부 호출에 한해 해당 경계의 존재를 검사한다.
GumpLifetime::GumpLifetime(GumpCreateState& state,GumpLifetimeState& lifetime,GumpLifetimeHooks hooks)
    :state_(state),lifetime_(lifetime),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("Gump 삭제 저장소 필요");
}
// 원본의 유효 포인터 계약을 호스트의 안정된 토큰 조회로 대체한다.
GumpCreateNode& GumpLifetime::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node) throw std::runtime_error("Gump 삭제 토큰/저장소 없음");return *node;
}
// CD의 idle 한도 80과 10.78의 한도 500을 선택한다.
std::size_t GumpLifetime::IdleSize() const { return state_.collection.edition==o::OriginalEdition::Patch1078 ? 500U : 80U; }
// 첫 빈 칸 또는 첫 일치 칸 하나만 갱신하고 플래그를 마지막에 쓴다.
void GumpLifetime::SetIdle(std::uint32_t token,bool enabled) const {
    auto& node=Node(token);if (enabled==bool(node.flags&kIdle)) return;
    // 원본은 idle 플래그가 표와 일치한다고 가정한다. 정상 입력에서 첫 후보가 선택된다.
    for (std::size_t i=0;i<IdleSize();++i) if (lifetime_.idle[i]==(enabled ? 0U : token)) {
        lifetime_.idle[i]=enabled ? token : 0U;
        node.flags=enabled ? node.flags|kIdle : node.flags&~kIdle;return;
    }
    throw std::length_error("Gump idle 원본 표/플래그 계약 위반");
}
// 원본 overlay 검색은 끝까지 훑어 마지막 후보를 사용한다.
void GumpLifetime::SetOverlay(std::uint32_t token,bool enabled) const {
    auto& node=Node(token);std::size_t found=lifetime_.overlay.size(),empty=found;
    // 중복 disable도 마지막 일치만 지운다. 앞의 중복 포인터는 남는다.
    for (std::size_t i=0;i<lifetime_.overlay.size();++i) {
        if (lifetime_.overlay[i]==token) found=i;if (!lifetime_.overlay[i]) empty=i;
    }
    if (enabled) {
        if (found!=lifetime_.overlay.size() || empty==lifetime_.overlay.size()) throw std::length_error("Gump overlay 원본 등록 계약 위반");
        lifetime_.overlay[empty]=token;node.flags|=kOverlay;
    } else if (found!=lifetime_.overlay.size()) { lifetime_.overlay[found]=0;node.flags&=~kOverlay; }
    else if (state_.collection.edition==o::OriginalEdition::Patch1078 && lifetime_.diagnostics) throw std::runtime_error("Gump overlay 원본 진단 계약 위반");
}
// 남은 항목의 순서를 유지하고 첫 일치만 제거한다. 없는 토큰에는 노드 조회가 필요 없다.
bool GumpLifetime::RemoveDialog(std::uint32_t token) const {
    if (lifetime_.dialogCount>lifetime_.dialogs.size()) throw std::length_error("Gump dialog 표 범위 초과");
    // 원본 memmove와 같은 앞쪽 겹침 복사다. 표의 사용하지 않는 뒤쪽은 보존한다.
    for (std::uint32_t i=0;i<lifetime_.dialogCount;++i) if (lifetime_.dialogs[i]==token) {
        std::move(lifetime_.dialogs.begin()+i+1,lifetime_.dialogs.begin()+lifetime_.dialogCount,lifetime_.dialogs.begin()+i);
        lifetime_.dialogs[--lifetime_.dialogCount]=0;
        lifetime_.currentDialog=lifetime_.dialogCount ? lifetime_.dialogs[lifetime_.dialogCount-1] : 0U;return true;
    }
    return false;
}
// 공개 재귀 진입은 독립 경로 기록을 만들어 잘못된 링크의 무한 반복을 거부한다.
void GumpLifetime::Kill(std::uint32_t token) const { std::vector<std::uint32_t> active;Recurse(token,false,active); }
// 즉시 파괴도 별도 재귀 경로를 만들어 자식 우선으로 가상 소멸자까지 진행한다.
void GumpLifetime::Destroy(std::uint32_t token) const { std::vector<std::uint32_t> active;Recurse(token,true,active); }
// 자식 next는 삭제 호출 전에 보관한다. 콜백 이후 부모의 현재 플래그/표/개수를 다시 읽는다.
void GumpLifetime::Recurse(std::uint32_t token,bool destroy,std::vector<std::uint32_t>& active) const {
    auto& node=Node(token);
    if (std::find(active.begin(),active.end(),token)!=active.end()) throw std::runtime_error("Gump 삭제 자식 순환");
    active.push_back(token);std::vector<std::uint32_t> siblings;
    // 자식 소멸자가 링크/큐를 바꾸거나 객체를 해제할 수 있어 호출 뒤 자식을 다시 읽지 않는다.
    for (auto child=node.child;child;) {
        if (std::find(siblings.begin(),siblings.end(),child)!=siblings.end()) throw std::runtime_error("Gump 삭제 형제 순환");
        siblings.push_back(child);const auto next=Node(child).next;Recurse(child,destroy,active);child=next;
    }
    if (destroy) {
        if (!(node.flags&kUnregistered)) {
            if (node.id>=state_.entries.size()) throw std::out_of_range("Gump 등록 해제 id 범위 초과");
            if (state_.collection.edition==o::OriginalEdition::Patch1078) { state_.entries[node.id]=0;--state_.count; }
            else { --state_.count;state_.entries[node.id]=0; }
            state_.collection.root=state_.entries[0];
            if (std::bit_cast<std::int32_t>(state_.count)<0) throw std::runtime_error("Gump 등록 개수 음수");
        }
        node.flags=(node.flags&~kDead)|kDestroying;
        if (!hooks_.deletingDestructor) throw std::invalid_argument("Gump 가상 소멸자 경계 필요");
        hooks_.deletingDestructor(token,1);
    } else {
        if (node.flags&kIdle) SetIdle(token,false);
        if (node.flags&kOverlay) SetOverlay(token,false);
        if (node.flags&0x40) {
            if (!hooks_.deleteForm) throw std::invalid_argument("Gump Form 삭제 경계 필요");hooks_.deleteForm(node.form,0);
        }
        if (!(node.flags&kVoid)) { if (!hooks_.push) throw std::invalid_argument("Gump 가상 Push 경계 필요");hooks_.push(token); }
        if (lifetime_.killCount>=999) throw std::length_error("Gump 삭제 큐 원본 한도 초과");
        lifetime_.kill[lifetime_.killCount++]=token;
        if (lifetime_.focus==token) lifetime_.focus=0;
        if (state_.collection.overrideHead==token) state_.collection.overrideHead=0;
        RemoveDialog(token);node.flags|=kDead;
    }
    active.pop_back();
}
// null 항목에서는 로컬 개수를 갱신하지 않는다. 소멸자 직후 현재 개수를 읽는 원본 동작이다.
void GumpLifetime::Cleanup() const {
    auto count=lifetime_.killCount;
    if (count>lifetime_.kill.size()) throw std::length_error("Gump 정리 큐 범위 초과");
    // 비어 있지 않은 항목마다 dead를 검사하고 등록 해제/가상 소멸자를 실제 실행한다.
    for (std::uint32_t i=0;i<count;++i) {
        const auto token=lifetime_.kill[i];if (!token) continue;
        if (!(Node(token).flags&kDead)) throw std::runtime_error("Gump 정리 대상 dead 필요");
        Destroy(token);count=lifetime_.killCount;
        if (count>lifetime_.kill.size()) throw std::length_error("Gump 정리 콜백 큐 범위 초과");
    }
    std::fill_n(lifetime_.kill.begin(),count,0U);lifetime_.killCount=0;
}
// 텍스트 해제 콜백 이후 각 포인터를 0으로 쓴다. 두 번째 포인터는 그 시점의 현재 값을 읽는다.
void GumpLifetime::DestructBase(std::uint32_t token) const {
    auto& node=Node(token);node.vtable=state_.collection.edition==o::OriginalEdition::Patch1078 ? kPatchVtable : kCdVtable;
    if (state_.collection.edition==o::OriginalEdition::Patch1078 && lifetime_.diagnostics && !(node.flags&kDestroying)) throw std::runtime_error("Gump 기본 소멸자 destroying 필요");
    if (node.textPointer) { if (!hooks_.release) throw std::invalid_argument("Gump 텍스트 해제 경계 필요");hooks_.release(node.textPointer);node.textPointer=0; }
    if (node.alternateTextPointer) { if (!hooks_.release) throw std::invalid_argument("Gump 대체 텍스트 해제 경계 필요");hooks_.release(node.alternateTextPointer);node.alternateTextPointer=0; }
    // 소멸자는 각 표에서 모든 중복 일치를 지운다. 플래그/개수/dialog는 직접 변경하지 않는다.
    for (auto& entry:lifetime_.overlay) if (entry==token) entry=0;
    // 판본의 실제 idle 표 밖은 보존한다.
    for (std::size_t i=0;i<IdleSize();++i) if (lifetime_.idle[i]==token) lifetime_.idle[i]=0;
    // 전체 삭제 큐를 훑어 뒤의 중복 항목도 null로 만든다.
    for (auto& entry:lifetime_.kill) if (entry==token) entry=0;
}
// 객체를 해제한 뒤 필드를 읽지 않고 원래 this 토큰을 반환한다.
std::uint32_t GumpLifetime::DeleteScalar(std::uint32_t token,std::uint32_t flags) const {
    DestructBase(token);
    if (flags&1) { if (!hooks_.release) throw std::invalid_argument("Gump 객체 해제 경계 필요");hooks_.release(token); }
    return token;
}
}
