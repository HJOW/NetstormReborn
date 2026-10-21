// 수집/포획의 우선순위·원문 재조회·현재 권한과 NaN/목록/경로 분기를 보존한다.
#include "o/RawAiCarrierActions.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kDaisOffset=3.0F; //00501908의 다이스 접근 좌표 보정이다.
// 실제 비정렬 little endian DWORD만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 운반 행동 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은 지정 네 BYTE를 합친다.
// 원본 DWORD의 네 BYTE만 쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 운반 행동 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은 주변 원문을 보존한다.
// 원본 WORD의 상위 BYTE도 유지한다.
std::uint32_t Word(std::span<const std::uint8_t> raw,std::size_t at) { return static_cast<std::uint32_t>(raw[at])|(static_cast<std::uint32_t>(raw[at+1])<<8); }
// 원본 signed DWORD 비교/나머지의 비트 해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 선택된 실제 하위 누락을 가짜 행동 성공으로 처리하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 운반 행동 실제 하위 미연결");return function; }
// 후보 kind는BYTE로 자르지 않고0049a840의전체 인덱스로 검사한다.
std::uint32_t TypeGenus(std::span<const RiftTypeRecord> types,std::uint32_t type) { if (type>=types.size()) throw std::out_of_range("AI 운반 행동 현재TYPE 범위");return types[type].flags2; }
}
// 원본은 mask 탈락 후보라도 먼저 kind TYPE를 조회한다.
Sid FindContainedGenus(const SidPool& pool,const ContainedFinderState& state,std::span<const RiftTypeRecord> types,Sid parent,std::uint32_t mask,std::uint32_t genus,std::uint32_t index) {
    RawContainedFinder finder(pool,state);
    // 현재 종속 체인을 원래 순서로 방문하고 일치 순번만 DWORD로 감소시킨다.
    for (auto sid=finder.Begin(parent);sid.value;sid=finder.Next()) { const auto raw=pool.Slot(sid);const bool matches=(TypeGenus(types,Word(raw,18))&genus)==genus;if ((!mask || (Word(raw,20)&mask)) && matches) { if (!index) return sid;--index; } }
    return {};
}
// 진단 callback 뒤 현재 genus/좌표/실패점도 다시 읽으며 free/dead 필터를 추가하지 않는다.
RawPathPoint DaisApproachPoint(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& checking,const RawPathPoint& invalid,const std::function<void(std::string_view)>& assertion,Sid dais) {
    const auto valid=[&] { return dais.value && dais.value<pool.Capacity(); }; //원본의 현재 물리 SID 범위만 검사한다.
    const auto isDais=[&] { return (TypeGenus(types,pool.Slot(dais)[10])&0x400000U)!=0; }; //현재 TYPE genus를 다시 읽는다.
    if (checking && (!valid() || !isDais())) Need(assertion)("dais.isValid() && dais.getGenus() & gDAIS");
    if (!valid() || !isDais()) return invalid;const auto raw=pool.Slot(dais);const float x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));return {static_cast<float>(static_cast<double>(x)-kDaisOffset),static_cast<float>(static_cast<double>(y)-kDaisOffset)};
}
// 생성은 어떤 원본 행동/장치 효과도 실행하지 않는다.
RawAiCarrierActions::RawAiCarrierActions(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,AiCarrierActionGlobals globals,AiCarrierActionHooks hooks):memory_(memory),pool_(pool),types_(types),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("AI 운반 행동 같은PATCH188 누락"); }
// 가짜/해제 토큰은 즉시 거부한다.
std::span<std::uint8_t> RawAiCarrierActions::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 운반 행동 실제0xb08 누락");return raw; }
// 현재 본체의 값을 읽는다.
std::uint32_t RawAiCarrierActions::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 지정된 본체 필드만 바꾼다.
void RawAiCarrierActions::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { Put(Body(token),at,value); }
// signed count를 현재 조회한다.
std::int32_t RawAiCarrierActions::Count(std::uint32_t token,std::size_t list) const { return Signed(Field(token,list+8)); }
// 현재 포인터를 사용하되 손상된 음수 주소 산술은 명시 거부한다.
std::uint32_t RawAiCarrierActions::Item(std::uint32_t token,std::size_t list,std::int32_t index) const { if (index<0) throw std::out_of_range("AI 운반 행동 음수 인덱스");return Read(memory_.Bytes(Field(token,list)),static_cast<std::size_t>(index)*4); }
// 일치하지 않으면 원문을 전혀 쓰지 않는다.
bool RawAiCarrierActions::Contains(std::uint32_t token,std::size_t list,std::uint32_t value) const { for (std::int32_t index=0;index<Count(token,list);++index) if (Item(token,list,index)==value) return true;return false; } //반복문은 현재 active 목록만 검사한다.
// full은 추가를 생략하며 꼬리를 정리하지 않는다.
void RawAiCarrierActions::Append(std::uint32_t token,std::size_t list,std::uint32_t value) const { const auto count=Count(token,list);if (count>=Signed(Field(token,list+4))) return;if (count<0) throw std::out_of_range("AI 운반 행동 음수 추가 인덱스");Put(memory_.Bytes(Field(token,list)),static_cast<std::size_t>(count)*4,value);Field(token,list+8,static_cast<std::uint32_t>(count)+1U); }
// 모든 중복을 안정 제거하고 비활성 꼬리를 보존한다.
void RawAiCarrierActions::Remove(std::uint32_t token,std::size_t list,std::uint32_t value) const { std::int32_t removed{};for (std::int32_t index=0;index<Count(token,list);++index) { const auto current=Item(token,list,index);if (current==value) ++removed;else Put(memory_.Bytes(Field(token,list)),static_cast<std::size_t>(index-removed)*4,current); }Field(token,list+8,static_cast<std::uint32_t>(Count(token,list))-static_cast<std::uint32_t>(removed)); } //반복문은 원본 순서로 현재 목록을 압축한다.
// 원본 isValid에는 free/dead/종류 조건이 없다.
bool RawAiCarrierActions::Valid(std::uint32_t value) const { return value && value<pool_.Capacity(); }
// 잘못된DWORD를 다른SID로 잘라서 사용하지 않는다.
Sid RawAiCarrierActions::Object(std::uint32_t value) const { if (value>=pool_.Capacity() || value>65535) throw std::out_of_range("AI 운반 행동 물리SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// 실제 현재 TYPE의 genus다.
std::uint32_t RawAiCarrierActions::Genus(Sid actor) const { return TypeGenus(types_,pool_.Slot(actor)[10]); }
// 원본0 wrapper만 sentinel을 반환하며 그 외는 현재 실제 그래프다.
std::uint32_t RawAiCarrierActions::Graph(Sid actor) const { return actor.value ? Need(hooks_.graph)(actor) : Need(globals_.invalidGraph)(); }
// C0가1인 less/unordered는 모두 미도달이다.디컴파일의단순 now<due와구별한다.
bool RawAiCarrierActions::Ready(std::uint32_t token,std::size_t at) const { const auto due=std::bit_cast<double>((std::uint64_t(Field(token,at+4))<<32)|Field(token,at));return globals_.now>=due; }
// 현재 다이스 유무는 실제 이동 직전에 읽는다.
void RawAiCarrierActions::Deliver(std::uint32_t token,Sid actor) const { const auto dais=Field(token,0x2a4);if (dais) { const auto point=Need(hooks_.daisPoint)(Object(dais));Need(hooks_.point)(token,actor,point); }else Need(hooks_.home)(token,actor); }
// 각 효과 뒤 원문의 현재 적 SID/기한/분류를 다시 조회한다.
bool RawAiCarrierActions::MayCapture(std::uint32_t token,Sid actor) const {
    if (!(Field(token,0x248)&0x100U) || Count(token,0x384)>=Signed(Field(token,0x388)) || !Valid(Field(token,0x2a0)) || !Need(hooks_.enemyAvailable)(Field(token,0x264)) || !Need(hooks_.vulnerable)(Object(Field(token,0x2a0))) || !Ready(token,0x5e0)) return false;
    if ((Genus(actor)&0x20000U) && !Need(hooks_.flyingGetter)(token)) return true;const auto source=Graph(actor);return source==Graph(Object(Field(token,0x2a0)));
}
// 첫 크리스털 조회 뒤 현재 boss를 읽는다.비서버 요청은 이동/등록 없이 반환한다.
void RawAiCarrierActions::Capture(std::uint32_t token,Sid actor) const { if (Need(hooks_.crystals)(actor)) { const auto crystal=Need(hooks_.firstCrystal)(actor);if (!globals_.boss) { Need(hooks_.dropRequest)(actor);return; }Need(hooks_.destroy)(crystal); }Need(hooks_.target)(token,actor,Object(Field(token,0x2a0)));Need(hooks_.remember)(token,actor); }
// 전체 원본의 prefix/포획귀환/순환 한 항목/정지 우선순위를 유지한다.
void RawAiCarrierActions::Act(std::uint32_t token) const {
    const auto enemy=Field(token,0x2a0);if (Valid(enemy)) static_cast<void>(Need(hooks_.maxHitPoints)(Object(enemy)));
    // 포획 사제를 든 정지 운반자를 가장 먼저 귀환시키며 생각 기한/일반 수집자보다 우선한다.
    for (std::int32_t index=0;index<Count(token,0x384);++index) { const auto value=Item(token,0x384,index);if (value && Need(hooks_.prisoner)(Object(value)).value && !Need(hooks_.path)(Object(value)).value) { const auto actor=Object(value);Deliver(token,actor);Field(token,0x38c,0);Append(token,0x384,value);return; } }
    const auto count=Count(token,0x2dc);if (!count || !Ready(token,0x5a8)) return;const auto next=Signed(Field(token,0x6d0)+1U);if (next==std::numeric_limits<std::int32_t>::min() && count==-1) throw std::domain_error("AI 운반 행동 원본IDIV overflow");const auto cursor=next%count;Field(token,0x6d0,static_cast<std::uint32_t>(cursor));const auto actor=Object(Item(token,0x2dc,cursor));
    if (globals_.checking && (pool_.Slot(actor)[11]&1U)) Need(hooks_.assertion)("!s.isFree()",0xc04);if (pool_.Slot(actor)[11]&1U) { Remove(token,0x2dc,actor.value);return; }
    if (Genus(actor)&0x200000U) Need(hooks_.assertion)("!( s.getGenus() & gPRIEST )",0xc0a);
    if (!Need(hooks_.path)(actor).value) { if (MayCapture(token,actor)) { Capture(token,actor);return; }if (!actor.value) Need(hooks_.assertion)("sid",0xaeb);if (Need(hooks_.prisoner)(actor).value) { Deliver(token,actor);return; }Need(hooks_.collect)(token,actor);if (Contains(token,0x384,actor.value)) Remove(token,0x384,actor.value);return; }
    if (Contains(token,0x384,actor.value)) return;
    if (Field(token,0x294) && Field(token,0x298)!=Need(globals_.invalidGraph)()) { const auto targets=Need(hooks_.targets)(actor);if (!targets[0]) return;const auto target=Object(targets[0]);if (!(pool_.Slot(target)[11]&3U) && (Genus(target)&0x10000000U) && Contains(token,0x330,target.value)) { if (Genus(actor)&0x20000U) return;const auto destination=Graph(target);if (destination==Graph(actor)) return; }Need(hooks_.reclassify)(token); }
    Need(hooks_.stop)(actor);
}
// 같은 실제 월드의 연결 확인용이다.
const SidPool& RawAiCarrierActions::Pool() const { return pool_; }
}
