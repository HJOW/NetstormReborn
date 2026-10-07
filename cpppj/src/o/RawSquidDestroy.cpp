// 종속/파생 효과 뒤 필드를 다시 읽고 기존 SquidUnpop·SidPool::Release를 실제로 호출한다.
#include "o/RawSquidDestroy.h"
#include <stdexcept>
#include <unordered_set>

namespace netstorm::o {
namespace {
// 두 판본의 공통 raw 위치와 dead/free/void/contained 비트다.
constexpr std::size_t kType=10,kState=11,kNext=4,kHead=6;
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kContained=8;
// 원본 서버/전파/종속 삭제 flags다. 서버의 client SID는 전파 억제 비트를 자동으로 추가한다.
constexpr std::uint32_t kServer=8,kClient=0x10,kDependent=0x40;
}
// 기존 공간/수명 상태를 공유하는 어댑터만 허용한다.
RawSquidDestroy::RawSquidDestroy(SidPool& pool,SquidUnpop& unpop,std::span<const RiftTypeRecord> types)
    :pool_(pool),unpop_(unpop),types_(types) {
    if (&unpop_.Pool()!=&pool_) throw std::invalid_argument("Destroy pool mismatch");
}
// 커서로 저장할 WORD는 콜백보다 먼저 읽는다.
std::uint16_t RawSquidDestroy::Word(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);return static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8));
}
// 원본 카운터는 DWORD 감김을 허용한다. 이 완료 호출만으로 공통 훅 전체를 복원했다고 보지 않는다.
void RawSquidDestroy::CompletePreDestroy() { --preDepth_; }
// 공통 postDestroy가 정상적으로 끝났음을 상위 destroy에 돌려준다.
void RawSquidDestroy::CompletePostDestroy() { --postDepth_; }
// 카운터는 원본 주소/호스트 포인터와 독립적이다.
std::uint32_t RawSquidDestroy::PreDepth() const { return preDepth_; }
// 현재 postDestroy 카운터를 읽는다.
std::uint32_t RawSquidDestroy::PostDepth() const { return postDepth_; }
// 공통 삭제/훅 어댑터의 풀 소유자를 읽기 전용으로 제공한다.
const SidPool& RawSquidDestroy::Pool() const { return pool_; }
// 실제 공간 해제 어댑터가 사용하는 해시/spot에 대조한다.
void RawSquidDestroy::ValidateGraph(const RawGraph& graph) const { unpop_.ValidateGraph(graph); }
// 원본은 dead를 먼저 켜므로 중첩된 같은 객체 삭제는 모든 외부 효과를 건너뛴다.
void RawSquidDestroy::Destroy(Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) {
    if (sid.value<5 || sid.value==pool_.Layout().predictableFirst || sid.value>=pool_.Capacity())
        throw std::out_of_range("Destroy SID");
    auto raw=pool_.Slot(sid);
    if (raw[kState]&kDead) return;
    if ((raw[kState]&(kFree|kContained)) || raw[kType]<kFirstAssetTypeNumber || raw[kType]>=types_.size())
        throw std::logic_error("Destroy unsupported root");
    if (!hooks.emit || !hooks.selected) throw std::invalid_argument("Destroy hooks");
    const bool client=sid.value<pool_.Layout().serverFirst;
    // 패치 원본의 assert 앞 부분 변경은 새 코드에서 피한다. 서버 객체의 수신 삭제는 flag 8이 필요하다.
    if (!pool_.IsServer() && !client && !(flags&kServer)) throw std::logic_error("Destroy server authority");
    if (pool_.IsServer() && client) flags|=kClient;
    pool_.AllocatedBytes(sid)[kState]|=kDead;
    const auto pre=preDepth_++;
    hooks.emit({SquidDestroyEffect::PreDestroy,sid,{},flags});
    if (preDepth_!=pre) throw std::logic_error("Destroy pre completion");
    Sid dependent{Word(sid,kHead)};std::unordered_set<std::uint16_t> visited;
    // preDestroy가 바꾼 현재 head부터 처리하며 각 next는 종속 release 전에 저장한다.
    while (dependent.value) {
        if (dependent.value>=pool_.Capacity() || !visited.insert(dependent.value).second)
            throw std::logic_error("Destroy dependent chain");
        const auto child=pool_.Slot(dependent);
        if ((child[kState]&kFree) || (!(child[kState]&kContained) && child[kType]>=kFirstAssetTypeNumber))
            throw std::logic_error("Destroy dependent state");
        const Sid next{Word(dependent,kNext)};
        hooks.emit({SquidDestroyEffect::ReleaseDependent,dependent,sid,0});
        if (!(pool_.Slot(dependent)[kState]&kFree))
            hooks.emit({SquidDestroyEffect::DestroyDependent,dependent,sid,flags|kDependent});
        dependent=next;
    }
    if (pool_.Slot(sid)[kState]&kFree) throw std::logic_error("Destroy root released during pre");
    if (!(flags&(kClient|kDependent)) && pool_.IsServer() && !client)
        hooks.emit({SquidDestroyEffect::Transmit,sid,{},flags|kServer});
    if (hooks.selected()==sid) hooks.emit({SquidDestroyEffect::ClearSelection,sid,{},0});
    raw=pool_.Slot(sid);
    // 파생 효과 뒤 현재 타입/void를 사용한다. 실제 공간 해제는 flags 0으로 호출한다.
    if (!(raw[kState]&kVoid)) {
        if (raw[kType]>=types_.size()) throw std::out_of_range("Destroy live type");
        unpop_.Unpop(sid,types_[raw[kType]],0);
    }
    const auto post=postDepth_++;
    hooks.emit({SquidDestroyEffect::PostDestroy,sid,{},flags});
    if (postDepth_!=post) throw std::logic_error("Destroy post completion");
    pool_.Release(sid);
}
}
