// 두 판본에서 동일한 목록 압축과 보호막 가상 호출 조건을 사용한다.
#include "o/RawPriestPreDestroy.h"
#include "o/RawStateSidLists.h" // 실제 signed40ea00의 압축과 stale 꼬리를 공유한다.
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 사제 타입/상태와 추상·매장 extra 비트다.
constexpr std::uint8_t kPriestTypeNumber=158,kInvalidState=9,kSkipPrefix=9;
// 판본별 실제 사제 가상 표의 원본 32비트 기록값이다.
constexpr std::uint32_t kPatchTable=0x50f210,kCdTable=0x5003e0;
// raw 가상 표 기록을 호스트 정렬과 무관하게 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> raw) {
    std::uint32_t result=0;
    // 낮은 바이트부터 DWORD를 조합한다.
    for (std::size_t i=0;i<4;++i) result|=static_cast<std::uint32_t>(raw[i])<<(8*i);
    return result;
}
}
// 모든 외부 경계를 필수로 받아 부분 복원에 따른 효과 누락을 막는다.
RawPriestPreDestroy::RawPriestPreDestroy(SidPool& pool,PriestPostPopState& state,PriestPreDestroyHooks hooks):
    pool_(pool),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.findForcefield || !hooks_.destroyForcefield || !hooks_.validateCarrier || !hooks_.carrierPre)
        throw std::invalid_argument("사제 삭제 준비 경계 미연결");
}
// 예약/풀 밖 번호 및 다른 파생 표는 사제 경로로 분배하지 않는다.
bool RawPriestPreDestroy::Handles(Sid sid) const {
    if (sid.value<(nativeBody_ ? 1 : 5) || sid.value>=pool_.Capacity()) return false;
    const auto raw=pool_.Slot(sid);
    return (nativeBody_ || raw[10]==kPriestTypeNumber) && Vtable(raw)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchTable : kCdTable);
}
// 장기 Native owner가 최초 조립할 때만 선택하며 게임 전역이나 목록은 읽지 않는다.
void RawPriestPreDestroy::EnableNativeBody() {
    if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Native 사제 전처리 판본 오류");
    nativeBody_=true;
}
// 원본은 공통 Destroy가 dead를 켠 뒤 진입한다. 여기서 dead를 이유로 생략하면 목록/회복이 남는다.
void RawPriestPreDestroy::PreDestroy(Sid sid,std::uint32_t flags) const {
    if (!nativeBody_ && (!Handles(sid) || (pool_.Slot(sid)[11]&kInvalidState))) throw std::invalid_argument("사제 삭제 준비 raw 상태 오류");
    const auto extraOffset=pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U;
    const bool ordinary=(pool_.Slot(sid)[extraOffset]&kSkipPrefix)==0;
    if (!nativeBody_) { if (ordinary) (void)state_.Priests().Items();hooks_.validateCarrier(sid,flags); } // 독립 typed의 기존 실패 순서만 유지한다.
    if (ordinary) {
        RemoveStateSidList(state_.Priests(),sid.value); // Native는 현재 signed count를 소비하며 음수도 정규화하지 않는다.
        const Sid forcefield=hooks_.findForcefield(sid);
        // 원본은 SID 1~4도 주소 범위 안으로 인정한다. 타입/free/dead를 여기서 추가로 검사하지 않는다.
        if (forcefield.value>=1 && forcefield.value<pool_.Capacity()) hooks_.destroyForcefield(forcefield,0);
    }
    hooks_.carrierPre(sid,flags);
}
// 실제 분배기는 다른 월드의 목록과 슬롯을 혼합할 수 없다.
const SidPool& RawPriestPreDestroy::Pool() const { return pool_; }
// 기존 삭제/ProcessHost 연결을 유지하면서 사제의 가상 preDestroy만 처리한다.
SquidDestroyHooks MakePriestPreDestroyHooks(const SidPool& pool,const RawPriestPreDestroy& priest,SquidDestroyHooks fallback) {
    if (&pool!=&priest.Pool() || !fallback.emit) throw std::invalid_argument("사제 삭제 분배 풀/후속 경계 오류");
    const auto emit=fallback.emit;
    fallback.emit=[&priest,emit](const SquidDestroyEvent& event) {
        if (event.effect==SquidDestroyEffect::PreDestroy && priest.Handles(event.sid)) priest.PreDestroy(event.sid,event.flags);
        else emit(event);
    };
    return fallback;
}
}
