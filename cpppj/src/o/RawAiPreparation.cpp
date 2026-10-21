// 현재 목표·지역·지갑·등록 기한을 원본 분기/읽기 순서대로 준비한다.
#include "o/RawAiPreparation.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kPlayerSlots=9; // 원본 Player0~8의 물리 기록 수다.
constexpr std::size_t kPlayerBytes=180; // 패치판 Player 한 기록의 실제 BYTE 수다.
constexpr float kNearestDistance=99999.0F,kHalf=0.5F; //00417027의 초기 거리와00500ee4의 중심 배율이다.
constexpr std::int32_t kRichEnough=12000; //00417394의 signed 지갑 초과 시 간헐천 부착 충족 기준이다.
// 비정렬 DWORD를 지정 원문 범위 안에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 준비 현재 DWORD 물리 범위");std::uint32_t value{};
    // 실제 네 BYTE만 little endian 순서로 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
// 지정한 네 BYTE만 덮어쓰고 다른 미초기화 원문을 유지한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 준비 현재 DWORD 쓰기 범위");
    // 낮은 BYTE부터 원본 DWORD를 기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 선택된 원본 하위가 없으면 그 지점에서 명시 오류를 낸다.
template<class Function> const Function& Need(const Function& function,const char* message) { if (!function) throw std::logic_error(message);return function; }
// DWORD의 signed 분기에서 구현 의존 변환을 피한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 현재 사제 raw 좌표를 float 저장값 그대로 읽는다.
std::array<float,2> Point(std::span<const std::uint8_t> raw) { return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
}

// 본문이 선택하기 전에는 외부 월드 효과나 목록 포인터에 접근하지 않는다.
RawAiPreparation::RawAiPreparation(RawAiBrainMemory& memory,const SidPool& pool,std::span<const std::uint8_t> players,std::span<const std::uint32_t> allies,AiPreparationGlobals globals,AiPreparationHooks hooks)
    :memory_(memory),pool_(pool),players_(players),allies_(allies),globals_(globals),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || players.size()!=kPlayerSlots*kPlayerBytes || allies.size()!=kPlayerSlots*kPlayerSlots) throw std::invalid_argument("AI 준비 같은 PATCH/Player9/동맹81 계약 누락");
}
// 실제 소유 블록을 매번 다시 확인하며 값형 사본을 만들지 않는다.
std::span<std::uint8_t> RawAiPreparation::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 준비 실제0xb08 본체 누락");return raw; }
// 콜백 뒤에도 같은 token의 현재 DWORD를 다시 읽는다.
std::uint32_t RawAiPreparation::Field(std::uint32_t token,std::size_t offset) const { return Read(Body(token),offset); }
// 현재 본체의 해당 원문 DWORD에만 쓴다.
void RawAiPreparation::Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const { Put(Body(token),offset,value); }
// 손상된 owner만 물리 범위 밖에서 거부하고0번 기록은 허용한다.
std::uint32_t RawAiPreparation::Player(std::uint32_t owner,std::size_t offset) const { if (owner>=kPlayerSlots || offset>kPlayerBytes-4) throw std::out_of_range("AI 준비 현재 Player 물리 범위");return Read(players_,owner*kPlayerBytes+offset); }
// 대칭화하지 않고 원본 owner 행을 현재 읽는다.
bool RawAiPreparation::Allied(std::uint32_t owner,std::uint32_t target) const { if (owner>=kPlayerSlots || target>=kPlayerSlots) throw std::out_of_range("AI 준비 현재 방향별 동맹 범위");return allies_[owner*kPlayerSlots+target]!=0; }
// 현재 풀에 실제로 존재하는 물리 번호만 좁힌다. free/dead 필터는 추가하지 않는다.
Sid RawAiPreparation::PhysicalSid(std::uint32_t value) const { if (value>=pool_.Capacity() || value>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("AI 준비 현재 사제 SID 물리 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// 강제 대상은 사제/동맹 확인 없이 선택하고 일반 탐색은 사제 조회 뒤 현재 owner 행을 읽는다.
void RawAiPreparation::SelectEnemy(std::uint32_t token) const {
    Field(token,0x264,0);const auto forced=Field(token,0x240);if (forced) { Field(token,0x264,forced);return; }
    // 첫 살아 있는 사제 보유 비동맹 플레이어에서 즉시 끝낸다.
    for (std::uint32_t owner=1;owner<kPlayerSlots;++owner) if (Need(hooks_.priest,"AI 준비 현재 사제 조회 미연결")(owner) && !Allied(Field(token,0x260),owner)) { Field(token,0x264,owner);return; }
}
// 원본은324목록을338개만 읽는다. 이 불일치를32c로 교정하지 않는다.
std::int32_t RawAiPreparation::CountAttachedGeysers(std::uint32_t token) const {
    std::uint32_t count{};
    // graph 하위가 개수/포인터/기준 그래프를 바꿀 수 있으므로 각 원문 위치에서 다시 읽는다.
    for (std::int32_t index=0;index<Signed(Field(token,0x338));++index) {
        const auto list=memory_.Bytes(Field(token,0x324));const auto sid=Read(list,static_cast<std::size_t>(index)*4);const auto graph=Need(hooks_.graph,"AI 준비 간헐천 그래프 미연결")(sid);if (graph==Field(token,0x298)) ++count;
    }
    return Signed(count);
}
// 원본의 enemy 재선택·데모 기한·취약 사제 목표·SP·최초 등록·현재 수량 판정을 순서대로 실행한다.
void RawAiPreparation::Update(std::uint32_t token) const {
    if (!Field(token,0x264) || Allied(Field(token,0x260),Field(token,0x264))) SelectEnemy(token);
    if (!Field(token,0x264)) Need(hooks_.missingEnemyAssertion,"AI 준비 적 소유자 진단 미연결")();
    if (Need(hooks_.demo,"AI 준비 현재 demo 조회 미연결")()!=0 && Field(token,0x260)==globals_.localOwner &&
        (Need(hooks_.knowledgeOpen,"AI 준비 현재 지식 창 조회 미연결")() || Need(hooks_.blockingGumpOpen,"AI 준비 현재 차단 창 조회 미연결")())) {
        const auto due=std::bit_cast<std::uint64_t>(static_cast<double>(std::bit_cast<float>(Field(token,0x228)))+globals_.now);Field(token,0x5a8,static_cast<std::uint32_t>(due));Field(token,0x5ac,static_cast<std::uint32_t>(due>>32));
    }
    const auto ownRegion=Signed(Player(Field(token,0x260),0x78));Field(token,0x294,ownRegion==globals_.noRegion ? 0U : Need(hooks_.temple,"AI 준비 현재 신전 조회 미연결")(ownRegion));
    if (Field(token,0x248)&0x100U) {
        // 참여 DWORD가 비제로인 적 사제 중 절반 HP 미만인 대상을 원본 순서로 살핀다.
        for (std::uint32_t enemy=1;enemy<kPlayerSlots;++enemy) {
            if (!Player(enemy,0x2c) || Allied(Field(token,0x260),enemy)) continue;const auto priest=Need(hooks_.priest,"AI 준비 취약 적 사제 조회 미연결")(enemy);if (!priest) continue;
            const auto sid=PhysicalSid(priest);const auto hitPoints=Signed(Read(pool_.Slot(sid),0x1a));const auto maxHitPoints=Need(hooks_.maxHitPoints,"AI 준비 현재 사제 최대 HP 미연결")(sid);if (hitPoints>=maxHitPoints/2) continue;if (Field(token,0x264)==enemy) break;
            float nearest=kNearestDistance,nearestOwner{};
            // 같은 방향 동맹의 모든 참여 사제를 방문한다. 먼저 가까운 자신을 발견해도 중단하지 않는다.
            for (std::uint32_t ally=1;ally<kPlayerSlots;++ally) {
                if (!Player(ally,0x2c) || !Allied(Field(token,0x260),ally)) continue;const auto friendPriest=Need(hooks_.priest,"AI 준비 동맹 사제 조회 미연결")(ally);if (!friendPriest) continue;
                const auto source=Point(pool_.Slot(PhysicalSid(friendPriest))),target=Point(pool_.Slot(sid));const auto distance=Need(hooks_.pointDistance,"AI 준비 현재 사제 Point 거리 미연결")(source,target);
                if (distance<static_cast<double>(nearest)) { nearest=static_cast<float>(distance);nearestOwner=static_cast<float>(ally); }
            }
            if (static_cast<double>(nearestOwner)==static_cast<double>(Signed(Field(token,0x260)))) { Field(token,0x264,enemy);break; }
        }
    }
    const auto enemyRegion=Signed(Player(Field(token,0x264),0x78));Field(token,0x2a8,enemyRegion==globals_.noRegion ? 0U : Need(hooks_.temple,"AI 준비 현재 적 신전 조회 미연결")(enemyRegion));
    const auto ownTemple=Field(token,0x294);const auto ownGraph=ownTemple ? Need(hooks_.graph,"AI 준비 현재 신전 그래프 미연결")(ownTemple) : Need(globals_.invalidGraph,"AI 준비 현재 무효 그래프 조회 미연결")();Field(token,0x298,ownGraph);Field(token,0x5f8,ownGraph);Field(token,0x604,Field(token,0x238));Field(token,0x5fc,Field(token,0x260));Field(token,0x600,Field(token,0x220));
    const auto enemyTemple=Field(token,0x2a8);Field(token,0x2ac,enemyTemple ? Need(hooks_.graph,"AI 준비 현재 적 신전 그래프 미연결")(enemyTemple) : Need(globals_.invalidGraph,"AI 준비 현재 적 무효 그래프 조회 미연결")());
    const auto currentEnemyRegion=Signed(Player(Field(token,0x264),0x78));
    if (currentEnemyRegion!=globals_.noRegion) {
        const auto bounds=Need(hooks_.regionBounds,"AI 준비 현재 적 청크 경계 미연결")(currentEnemyRegion);
        // 하위가 돌려준 네 float를 원래 연속 필드에 복사한다.
        for (std::size_t index=0;index<bounds.size();++index) Field(token,0x2b8+index*4,std::bit_cast<std::uint32_t>(bounds[index]));Field(token,0x2b0,Field(token,0x2b8));Field(token,0x2b4,Field(token,0x2bc));
        const auto left=std::bit_cast<float>(Field(token,0x2b8)),right=std::bit_cast<float>(Field(token,0x2c0));const auto x=static_cast<float>((static_cast<double>(right)-left)*kHalf+std::bit_cast<float>(Field(token,0x2b0)));Field(token,0x2b0,std::bit_cast<std::uint32_t>(x));
        const auto top=std::bit_cast<float>(Field(token,0x2bc)),bottom=std::bit_cast<float>(Field(token,0x2c4));const auto y=static_cast<float>((static_cast<double>(bottom)-top)*kHalf+std::bit_cast<float>(Field(token,0x2b4)));Field(token,0x2b4,std::bit_cast<std::uint32_t>(y));
    }
    Field(token,0x29c,Need(hooks_.priest,"AI 준비 현재 소유 사제 조회 미연결")(Field(token,0x260)));Field(token,0x2a0,Need(hooks_.priest,"AI 준비 현재 적 사제 조회 미연결")(Field(token,0x264)));
    if (Field(token,0x260)==globals_.localOwner) {
        const auto current=globals_.playerKnown ? Need(hooks_.storedSp,"AI 준비 현재 SP 조회 미연결")(globals_.localOwner) : 0.0F;
        if (static_cast<double>(current)!=static_cast<double>(Signed(Field(token,0x2d8)))) {
            const auto reread=globals_.playerKnown ? Need(hooks_.storedSp,"AI 준비 두 번째 현재 SP 조회 미연결")(globals_.localOwner) : 0.0F;const auto delta=static_cast<float>(static_cast<double>(Signed(Field(token,0x2d8)))-reread);Need(hooks_.addLocalSp,"AI 준비 실제 로컬 SP 가산 미연결")(delta);
        }
    }
    if (!Field(token,0x25c)) { Need(hooks_.initialScan,"AI 준비 초기 전체 월드 등록 미연결")(token);Field(token,0x25c,1); }
    const auto attached=CountAttachedGeysers(token);const auto collectors=attached<1 ? std::uint32_t(Field(token,0x234)!=0) : Field(token,0x234);Field(token,0x2cc,Signed(collectors)<=Signed(Field(token,0x2e4)) ? 1U : 0U);
    if (Signed(Field(token,0x32c))<2) Field(token,0x2d0,1);
    else { const auto desired=Signed(Field(token,0x230));const auto minimum=desired>0 ? desired : 1;Field(token,0x2d0,minimum<=CountAttachedGeysers(token) ? 1U : 0U); }
    if (Signed(Field(token,0x2d8))>kRichEnough) Field(token,0x2d0,1);
}
// 실제 월드 조립 검사용 동일 풀 참조다.
const SidPool& RawAiPreparation::Pool() const { return pool_; }
}
