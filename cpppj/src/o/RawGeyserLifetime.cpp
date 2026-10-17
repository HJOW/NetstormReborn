// 간헐천 고유 수명 접두를 일반 Damageable로 대체하지 않는다.
#include "o/RawGeyserLifetime.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 현재 비정렬 raw DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) { return std::uint32_t(raw[offset])|(std::uint32_t(raw[offset+1])<<8)|(std::uint32_t(raw[offset+2])<<16)|(std::uint32_t(raw[offset+3])<<24); }
// 실제 선택된 큰 하위를 빈 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("간헐천 수명의 실제 하위 미연결");return hook; }
}
// 서로 다른 CD 수명은 이 PATCH 본문으로 실행하지 않는다.
RawGeyserLifetime::RawGeyserLifetime(SidPool& pool,SquidPostPopList& geysers,const std::uint32_t& editor,GeyserLifetimeHooks hooks):pool_(pool),geysers_(geysers),editor_(editor),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("간헐천 수명 판본 오류");
}
// 타입 번호가 같더라도 다른 가상 표면 파생으로 확장하지 않는다.
bool RawGeyserLifetime::Handles(Sid sid) const { return Read(pool_.Slot(sid),0)==0x50da88; }
// 원본40ea00의 앞방향 압축과 남은 물리 꼬리를 그대로 보존한다.
void RawGeyserLifetime::Remove(Sid sid) const {
    static_cast<void>(geysers_.Items());std::uint32_t removed{};
    // AI 제거 효과 뒤의 현재 count/내용으로 모든 중복을 제거한다.
    for (std::uint32_t i=0;i<geysers_.count;++i) { const auto value=geysers_.entries[i];if (value==sid.value) ++removed;else geysers_.entries[i-removed]=value; }geysers_.count-=removed;
}
// getter 뒤 프레임 직접 저장이며 일반 SetFrame의 추가 Unpop/두 표시 갱신을 넣지 않는다.
void RawGeyserLifetime::PreDestroy(Sid sid,std::uint32_t flags) const {
    const auto frame=Need(hooks_.defaultFrame)(sid);auto raw=pool_.AllocatedBytes(sid);
    // 현재 기본 프레임 DWORD만 기록한다.
    for (std::size_t i=0;i<4;++i) raw[36+i]=static_cast<std::uint8_t>(frame>>(i*8));
    Need(hooks_.update)(sid);
    if (!(pool_.Slot(sid)[40]&9U)) { Need(hooks_.registerAi)(sid,0);Remove(sid); }
    Need(hooks_.damageablePre)(sid,flags);
}
// 원본의 진입 조건을 한 번 평가하고 섬 정리 뒤에는 동일 flags로 공통 수명을 호출한다.
void RawGeyserLifetime::PostDestroy(Sid sid,std::uint32_t flags) const {
    const auto raw=pool_.Slot(sid);
    if (editor_ && !(raw[40]&9U)) Need(hooks_.dropIsland)(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),flags,raw[34]);
    Need(hooks_.damageablePost)(sid,flags);
}
// 같은 실제 raw 풀이다.
const SidPool& RawGeyserLifetime::Pool() const { return pool_; }
}
