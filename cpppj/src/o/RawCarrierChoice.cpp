// 경로를 찾지 못한 후보도 거친 거리 점수로 비교하는 원본 분기를 유지한다.
#include "o/RawCarrierChoice.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr double kInt64Boundary=9223372036854775808.0; // _ftol의 masked invalid signed64 경계다.
// 선택된 실제 하위가 없으면 빈 효과나 성공값으로 바꾸지 않는다.
template<class F> const F& Need(const F& f) { if (!f) throw std::logic_error("운반 후보의 실제 하위 미연결");return f; }
// 비정렬 raw/Player의 DWORD만 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // 원본 필드의 네 바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value;
}
//41cfc0의 넓은 거리/speed와 _ftol 하위 WORD다. 디컴파일 C에 누락된428210의 FDIV를 보존한다.
std::uint16_t TravelWord(RawPathPoint first,RawPathPoint second,float speed) {
    const double dx=static_cast<double>(first.x)-second.x,dy=static_cast<double>(first.y)-second.y;
    const double time=std::sqrt(dy*dy+dx*dx)/static_cast<double>(speed);
    if (!std::isfinite(time) || time>=kInt64Boundary || time< -kInt64Boundary) return 0;
    return static_cast<std::uint16_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(time)));
}
}
// Player 기록과 슬롯 폭이 다른 CD의 후보 몸체를 PATCH로 대신 실행하지 않는다.
RawCarrierChoice::RawCarrierChoice(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> players,
    SquidPostPopList& returns,SquidPostPopList& geysers,const double& now,const bool& checking,
    const RawBoardTarget& board,CarrierChoiceState& state,CarrierChoiceHooks hooks)
    :pool_(pool),types_(types),players_(players),returns_(returns),geysers_(geysers),now_(now),checking_(checking),board_(board),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || players.size()!=9*0xb4) throw std::invalid_argument("운반 후보의 판본/TYPE/Player9 오류");
}
//0번 슬롯도 물리 범위에는 포함되며 free/dead 필터를 넣지 않는다.
Sid RawCarrierChoice::Number(std::uint32_t number) const { if (number>=pool_.Capacity() || number>0xffffU) throw std::out_of_range("운반 후보의 물리 SID 범위 오류");return Sid{static_cast<std::uint16_t>(number)}; }
// 순수 조회 사이에도 getter 효과가 있을 수 있는 호출 순서를 바꾸지 않는다.
RawPathPoint RawCarrierChoice::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 원래 BYTE 타입 번호와 같은 표를 읽는다.
const RiftTypeRecord& RawCarrierChoice::Type(Sid sid) const { const auto kind=pool_.Slot(sid)[10];if (kind>=types_.size()) throw std::out_of_range("운반 후보의 현재 TYPE 범위 오류");return types_[kind]; }
// 처음에 만든 AI bool 사본으로 후보 경로 분기를 결정하지 않는다.
bool RawCarrierChoice::Controlled(Sid sid) const { const auto owner=pool_.Slot(sid)[34];if (!owner) return false;if (owner>=9) throw std::out_of_range("운반 후보의 현재 Player 범위 오류");return Read(players_,owner*0xb4+0xb0)!=0; }
// 이 필드는 객체의 실제 임시 scratch이며 할당 여부나 타입으로 쓰기를 막지 않는다.
void RawCarrierChoice::Metric(Sid sid,std::uint16_t value) const { const auto offset=pool_.Offset(sid)+8;pool_.bytes_[offset]=static_cast<std::uint8_t>(value);pool_.bytes_[offset+1]=static_cast<std::uint8_t>(value>>8); }
// 음수 count를 큰 양수 목록으로 순회하지 않는다.
std::int32_t RawCarrierChoice::Count(const SquidPostPopList& list) { return std::bit_cast<std::int32_t>(list.count); }
// 두 순회는 nonzero 카운터/표면 후보 처리/현재 count 재조회 시점이 서로 다르다.
Sid RawCarrierChoice::Evaluate(SquidPostPopList& candidates,Sid anchor,Sid mover) const {
    std::array<float,2> movement{1.0F,90.0F};bool skipProbe{}; // 무효 mover의 원본 기본값과 처음에 고정한 탐색 생략 여부다.
    if (mover.value && mover.value<pool_.Capacity()) { movement=Need(hooks_.movement)(mover);skipProbe=Controlled(mover) || (Type(mover).flags2&0x20000U)!=0; }
    std::int32_t index{},seen{},total=Count(candidates); // 첫 순회의 현재 위치/실제 계산 수/현재 count다.
    if (total>0) {
        // 첫 순회에서 surface는 진단 후 재검사하고 여전히 surface면 seen을 늘리지 않는다.
        do {
            if (index>=static_cast<std::int64_t>(candidates.entries.size())) break;
            if (total) {
                const auto candidate=Number(candidates.entries.at(static_cast<std::size_t>(index%total)));
                if (candidate.value) {
                    if (Type(candidate).flags1&0x800U) Need(hooks_.assertion)(0xace);
                    if (!(Type(candidate).flags1&0x800U)) {
                        const auto point=Position(candidate);const auto anchorRect=Need(hooks_.footprint)(anchor);const auto first=board_.Nearest(anchorRect,point,1);
                        const auto rectangle=Need(hooks_.footprint)(candidate);const auto second=board_.Nearest(rectangle,first,1);Metric(candidate,TravelWord(second,first,movement[0]));++seen;
                    }
                }
            }
            total=Count(candidates);++index;
        } while (seen<total);
    }
    if (candidates.count>candidates.entries.size()) throw std::out_of_range("운반 후보 정렬의 count/capacity 오류");
    // 실제 원본 비교기와 구형 CRT를 사용한다. 관측의 주소 변환은 배열 문맥만 추가한다.
    RawCarrierCandidateSort sorter(pool_,hooks_.relocate ? std::function<std::uint32_t(const void*)>([&](const void* pointer) { return hooks_.relocate(candidates.entries,pointer); }) : std::function<std::uint32_t(const void*)>{});
    sorter.Sort(std::span(candidates.entries).first(candidates.count));
    float best=10000.0F;Sid chosen{};index=0;seen=0;total=Count(candidates); // 원본의 엄격한 최소값과 두 번째 순회다.
    if (total>0) {
        // 경로 탐색은 거친 WORD 점수가 현재 최소값보다 작을 때만 수행한다.
        do {
            if (index>=static_cast<std::int64_t>(candidates.entries.size())) break;
            if (total) {
                const auto candidate=Number(candidates.entries.at(static_cast<std::size_t>(index%total)));
                if (candidate.value) {
                    const auto raw=pool_.Slot(candidate);double metric=raw[8]|(std::uint32_t(raw[9])<<8);
                    if (state_.comparePaths && !skipProbe && metric<static_cast<double>(best)) {
                        RawPathRuntime temporary(OriginalEdition::Patch1078); // 원본처럼 매 탐색 전에 count와5000개 시각을0으로 만든다.
                        const auto startPoint=Position(anchor),candidatePoint=Position(candidate); // 둘 다 첫 발자국 효과 전에 보관한다.
                        const auto candidateRect=Need(hooks_.footprint)(candidate);const auto destination=board_.Nearest(candidateRect,startPoint,1);
                        const auto anchorRect=Need(hooks_.footprint)(anchor);const auto start=board_.Nearest(anchorRect,candidatePoint,1);
                        const auto kind=pool_.Slot(mover)[10];const auto cardinal=std::find(state_.cardinalTypes.begin(),state_.cardinalTypes.end(),kind)!=state_.cardinalTypes.end();
                        Need(hooks_.search)(temporary,mover,start,destination,cardinal ? 1U : 0U);
                        if (temporary.Count()) {
                            const auto now=now_;temporary.Prepare(now,now,movement[0],movement[1],0);metric=temporary.Time(temporary.Count()-1)-now_;
                            state_.currentMetricBits=std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<double>(movement[0])*metric));
                        }
                    }
                    if (metric<static_cast<double>(best)) { best=static_cast<float>(metric);chosen=candidate; }
                    ++seen;
                }
            }
            total=Count(candidates);++index;
        } while (seen<total);
    }
    if (checking_ && seen!=Count(candidates)) Need(hooks_.assertion)(0xb03);
    state_.currentMetricBits=state_.baselineBits;return chosen;
}
// 현재 home을 선두에 넣고 outposts의 nonzero 수를 세며 원래500개 상한과 중복을 유지한다.
Sid RawCarrierChoice::ChooseReturn(Sid parent) const {
    SquidPostPopList candidates{std::vector<std::uint32_t>(500),0};const auto owner=pool_.Slot(parent)[34];if (owner>=9) throw std::out_of_range("귀환 후보의 현재 Player 범위 오류");
    const auto home=Read(players_,owner*0xb4+0x7c);if (home && Need(hooks_.validReturn)(parent,Number(home))) candidates.entries[candidates.count++]=home;
    std::int32_t index{},seen{},total=Count(returns_); // 원래 ring 목록의 nonzero 수/현재 count다.
    if (total>0) {
        // 빈 항목은 seen을 늘리지 않고 capacity에 도달하면 멈춘다.
        do {
            if (index>=static_cast<std::int64_t>(returns_.entries.size())) break;
            if (total) { const auto target=returns_.entries.at(static_cast<std::size_t>(index%total));if (target) { if (Need(hooks_.validReturn)(parent,Number(target)) && candidates.count<500) candidates.entries[candidates.count++]=target;++seen; } }
            total=Count(returns_);++index;
        } while (seen<total);
    }
    return Evaluate(candidates,parent,parent);
}
// 귀환점이 물리 범위 안일 때만 Graph 조회/간헐천 수집을 수행한다.
Sid RawCarrierChoice::ChooseGeyser(Sid parent) const {
    const auto home=ChooseReturn(parent);if (!home.value || home.value>=pool_.Capacity()) return {};
    static_cast<void>(Need(hooks_.graphFor)(parent));SquidPostPopList candidates{std::vector<std::uint32_t>(500),0};
    std::int32_t index{},total=Count(geysers_); // 여기서는 nonzero 수가 아니라 원래 index와 count를 비교한다.
    if (total>0) {
        // 원본에는 capacity 조기 종료가 없다. 확보 범위 밖 접근만 at으로 거부한다.
        do {
            if (total) { const auto target=geysers_.entries.at(static_cast<std::size_t>(index%total));if (target && Need(hooks_.validGeyser)(parent,Number(target)) && candidates.count<500) candidates.entries[candidates.count++]=target; }
            total=Count(geysers_);++index;
        } while (index<total);
    }
    return Evaluate(candidates,home,parent);
}
// 같은 실제 풀만 공개한다.
const SidPool& RawCarrierChoice::Pool() const { return pool_; }
}
