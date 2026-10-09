// 원본의 목록→검색→확보→HP→예약→소유자 상태 쓰기 순서를 보존한다.
#include "o/RawPriestPostPop.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 후반 조합의 풀 혼합 검사를 위해 읽기 전용 참조를 반환한다.
const SidPool& RawPriestPostPop::Pool() const { return pool_; }
namespace {
// 판본 공통 raw 필드 위치와 원본 회복 분모/배율이다. 배율은 두 PE에서 float 50.0이다.
constexpr std::size_t kType=10,kOriginalOwner=12;
constexpr std::int32_t kRegenDivisor=6;
constexpr double kRegenScale=50.0;
// 가상 표 DWORD를 정렬과 무관하게 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> raw) {
    std::uint32_t result=0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i=0;i<4;++i) result|=static_cast<std::uint32_t>(raw[i])<<(8*i);
    return result;
}
// 00414450와 CD 몸체의 inline 목록: 용량이 남을 때만 중복을 검색하고 끝에 넣는다.
void AddUnique(SquidPostPopList& list,Sid sid) {
    if (list.count>=list.entries.size()) return;
    // 활성 구간 밖의 오래된 같은 번호는 중복으로 취급하지 않는다.
    for (std::uint32_t i=0;i<list.count;++i) if (list.entries[i]==sid.value) return;
    list.entries[list.count++]=sid.value;
}
}
// 연결 오류는 효과를 발생시키기 전에 거부한다.
RawPriestPostPop::RawPriestPostPop(SidPool& pool,const SquidReward& hp,PriestPostPopState& state,PriestPostPopHooks hooks)
    :pool_(pool),hp_(hp),state_(state),hooks_(std::move(hooks)) {
    if (&hp.Pool()!=&pool) throw std::invalid_argument("사제 postPop/최대 HP의 SID 풀이 다릅니다");
    if (!hooks_.findRegular || !hooks_.reserveRegular || !hooks_.addRegular) throw std::invalid_argument("사제 회복 예약 효과 누락");
}
// carrier 호출 이후 낙하/경로/전용 이벤트 실행은 다음 복원 단계다.
void RawPriestPostPop::Prefix(Sid sid,std::uint32_t flags) const {
    const auto raw=pool_.AllocatedBytes(sid);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (raw[kType]!=kPriestType || Vtable(raw)!=(patch ? kPatchPriestVtable : kCdPriestVtable))
        throw std::invalid_argument("사제 postPop 가상 표/타입 불일치");
    if (state_.priests.count>state_.priests.entries.size()) throw std::invalid_argument("사제 목록 개수 오류");
    if (raw[patch ? 40 : 35]&9) return;
    if (flags&1) {
        AddUnique(state_.priests,sid);
        if (!hooks_.findRegular(sid,kPriestRegenEvent) && hooks_.reserveRegular()) {
            // x87은 int / 6을 0방향으로 절삭한 뒤 곱하고 float으로 저장한다. int를 먼저 float으로 줄이지 않는다.
            const float numerator=static_cast<float>(static_cast<double>(hp_.TypeHitPoints(kPriestType)/kRegenDivisor)*kRegenScale);
            // 원본 FILD의 정수 분모를 유지하고 나눗셈 결과만 float으로 저장한다.
            const float payload=static_cast<float>(static_cast<double>(numerator)/static_cast<double>(hp_.MaxHitPoints(sid)));
            hooks_.addRegular(sid,kPriestRegenEvent,payload);
        }
    }
    // 예약 훅 이후 다시 읽는다. 원본도 현재 소유자를 이 지점에서 조회한다.
    const auto current=pool_.Slot(sid);
    const auto owner=current[patch ? 34 : 32];
    if ((current[kOriginalOwner]&0x7f)==owner && owner>0 && owner<=kPlayerCount) state_.ownerState[owner]=0;
}
// ProcessForm·Kernel을 가진 실제 호스트와 합성한다. reserve는 원본 할당 실패를 주입할 경계다.
PriestPostPopHooks MakePriestPostPopProcessHooks(SquidProcessHost& host) {
    return {
        [&host](Sid sid,std::uint32_t event) { return host.FindEvent(sid,event)!=nullptr; },
        [] { return true; },
        [&host](Sid sid,std::uint32_t event,float payload) { host.AddRegular(sid,event,payload); }
    };
}
}
