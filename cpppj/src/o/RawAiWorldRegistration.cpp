// 기존 자산의 원본 방문 순서와 간헐천 다섯 목록의 서로 다른 역할을 보존한다.
#include "o/RawAiWorldRegistration.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr std::uint32_t kDelayedLimit=300; //00415720의 실제0x4b0 임시 목록 용량이다.
// 원본 비정렬 DWORD를 실제 backing 범위에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("AI 월드 등록 DWORD 범위");std::uint32_t value{};for (std::size_t i=0;i<4;++i) value|=std::uint32_t(bytes[at+i])<<(i*8);return value; } // 반복문은 낮은 네 바이트만 합친다.
// 현재 DWORD의 네 바이트만 바꾸며 inactive/padding을 유지한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("AI 월드 등록 쓰기 범위");for (std::size_t i=0;i<4;++i) bytes[at+i]=static_cast<std::uint8_t>(value>>(i*8)); } // 반복문은 지정한 폭만 쓴다.
// 원본 x87의 두 float 좌표 차이는 중간 float 저장 없이 계산한다.
double Distance(std::array<float,2> first,std::array<float,2> second) { const double dx=static_cast<double>(first[0])-second[0],dy=static_cast<double>(first[1])-second[1];return dx*dx+dy*dy; }
// 원본 CRT signed64 절삭의 낮은 DWORD다. 주소 계산 불가능한 비유한 입력은 호스트 계약 오류다.
std::int32_t Truncate(float value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) throw std::out_of_range("AI 간헐천 CRT 좌표 범위");return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); }
// 선택되지 않은 하위를 미리 요구하지 않고 도달한 경계에서만 거부한다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("AI 월드 등록 실제 하위 미연결");return effect; }
}
// 실제 같은 세계의 저장소만 연결한다.
RawAiWorldRegistration::RawAiWorldRegistration(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> players,std::span<const std::uint32_t> allies,std::span<const std::uint16_t> surface,const RawAiObjectNotice& objects,AiWorldRegistrationHooks hooks)
    :memory_(memory),pool_(pool),types_(types),players_(players),allies_(allies),surface_(surface),objects_(objects),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || &objects.Pool()!=&pool || types.size()!=188 || players.size()!=1620 || allies.size()!=81 || surface.size()!=65536) throw std::invalid_argument("AI 월드 등록의 같은 PATCH188/Player9/동맹81/표면65536 누락");
}
// 해제 포인터나 크기 없는 가짜 본체는 성공으로 대신하지 않는다.
std::span<std::uint8_t> RawAiWorldRegistration::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 월드 등록 실제0xb08 본체 누락");return raw; }
// 슬롯0은 진단 뒤의 원본 분류에도 사용되므로 임의 금지하지 않는다.
Sid RawAiWorldRegistration::Object(std::uint32_t sid) const { if (sid>=pool_.Capacity() || sid>65535) throw std::out_of_range("AI 월드 등록 SID 물리 범위");return Sid{static_cast<std::uint16_t>(sid)}; }
// free/dead 판정은 각 caller의 원문 위치에만 둔다.
std::uint32_t RawAiWorldRegistration::Genus(Sid sid) const { const auto type=pool_.Slot(sid)[10];if (type>=types_.size()) throw std::out_of_range("AI 월드 등록 현재 TYPE 범위");return types_[type].flags2; }
// 비양수 count의 자연 루프를 유지한다.
std::int32_t RawAiWorldRegistration::Count(std::uint32_t token,std::size_t at) const { return std::bit_cast<std::int32_t>(Read(Body(token),at+8)); }
// 용량이 남을 때만 실제 현재 포인터로 쓴다.
void RawAiWorldRegistration::Append(std::uint32_t token,std::size_t at,std::uint32_t sid) const { auto raw=Body(token);const auto count=Count(token,at);if (count>=std::bit_cast<std::int32_t>(Read(raw,at+4))) return;if (count<0) throw std::out_of_range("AI 월드 등록 음수 목록 인덱스");Put(memory_.Bytes(Read(raw,at)),static_cast<std::size_t>(count)*4,sid);Put(raw,at+8,static_cast<std::uint32_t>(count)+1); }
// count 밖의 꼬리나 여유 메모리를0으로 정리하지 않는다.
void RawAiWorldRegistration::Remove(std::uint32_t token,std::size_t at,std::uint32_t sid) const {
    std::int32_t removed{};
    // 현재 count와 실제 pointer를 원본처럼 재조회하며 생존 항목의 순서를 유지한다.
    for (std::int32_t index=0;index<Count(token,at);++index) { auto list=memory_.Bytes(Read(Body(token),at));const auto value=Read(list,static_cast<std::size_t>(index)*4);if (value==sid) ++removed;else Put(list,static_cast<std::size_t>(index-removed)*4,value); }
    Put(Body(token),at+8,static_cast<std::uint32_t>(Count(token,at))-static_cast<std::uint32_t>(removed));
}
// 자신과 동맹의 현재 신전이 같은 거리이면 자신의 후보를 유지한다.
bool RawAiWorldRegistration::CloserToAlly(std::uint32_t token,std::uint32_t number) const {
    const auto sid=Object(number);const auto object=pool_.Slot(sid);const std::array<float,2> point{std::bit_cast<float>(Read(object,14)),std::bit_cast<float>(Read(object,18))};const auto temple=Read(Body(token),0x294);
    const auto own=temple ? Need(hooks_.center)(Object(temple)) : std::array<float,2>{std::bit_cast<float>(Read(Body(token),0x268)),std::bit_cast<float>(Read(Body(token),0x26c))};
    const auto ownDistance=static_cast<float>(Distance(point,own)); //00415312의 fstp DWORD이며 상대 거리에는 이 축소를 넣지 않는다.
    // Player0은 방문하지 않고 현재 owner행의 방향 동맹과 현재 Player+7c만 선택한다.
    for (std::uint32_t candidate=1;candidate<9;++candidate) { const auto owner=Read(Body(token),0x260);if (owner>=9) throw std::out_of_range("AI 간헐천 현재 동맹 owner 범위");if (candidate==owner || !allies_[owner*9+candidate]) continue;const auto other=Read(players_,candidate*180+0x7c);if (other && Distance(point,Need(hooks_.center)(Object(other)))<ownDistance) return true; }
    return false;
}
// 표면0 진단 뒤 원본처럼 현재0슬롯을 분류한다. own surface만330, 섬/기타는348이다.
void RawAiWorldRegistration::ClassifySurface(std::uint32_t token,std::uint32_t number) const {
    const auto object=pool_.Slot(Object(number));const auto x=Truncate(std::bit_cast<float>(Read(object,14))),y=Truncate(std::bit_cast<float>(Read(object,18)));std::uint32_t surface{};
    if (x>=0 && y>=0 && x<256 && y<256) surface=surface_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)];if (!surface) Need(hooks_.missingSurface)(number);
    const auto sid=Object(surface);if ((Genus(sid)&0x1000002U)==2U) { const auto owner=pool_.Slot(sid)[34];if (!owner || owner==Read(Body(token),0x260)) Append(token,0x330,number); }
    else Append(token,0x348,number);
}
// 제거는 좌표/TYPE를 읽지 않고, 등록은 가까움 판정 뒤 현재 표면을 읽는다.
void RawAiWorldRegistration::Geyser(std::uint32_t token,std::uint32_t sid,std::uint32_t state) const {
    Body(token);if (!state) { for (const auto at:{0x324U,0x330U,0x33cU,0x348U,0x354U}) Remove(token,at,sid);return; } // 반복문은 원본 다섯 목록의 순서대로 모든 중복을 제거한다.
    if (CloserToAlly(token,sid)) Need(hooks_.fartherGeyser)(token,sid);else { Append(token,0x324,sid);ClassifySurface(token,sid); }
}
// 기존 수도·시설·수집자를 먼저 등록한 뒤 방어 자산을 한 번씩 지연 통지한다.
void RawAiWorldRegistration::FullScan(std::uint32_t token) const {
    Body(token);const auto temporary=memory_.Allocate(kDelayedLimit*4);std::uint32_t count{};const auto end=pool_.Capacity();std::uint32_t cursor=1;
    //0040eb40의 최초 호출도1을 먼저 증가시키므로 SID1은 방문하지 않는다.
    const auto next=[&] { ++cursor;while (cursor<end && (pool_.Slot(Object(cursor))[11]&1U)) ++cursor; }; // 반복문은 원본의 free bit만 건너뛴다. dead는 제외하지 않는다.
    try {
        next();
        // 하위 로그 뒤 현재 pool/owner/type을 다시 읽되 iterator 끝은 최초 cap을 유지한다.
        while (cursor>=1 && cursor<pool_.Capacity()) { const auto sid=Object(cursor);if (pool_.Slot(sid)[10]>=70 && !(Genus(sid)&2U)) { const auto genus=Genus(sid);if (genus&0x10000000U) Geyser(token,cursor,1);else if (pool_.Slot(sid)[34]==Read(Body(token),0x260)) { if (!(genus&0x4000000U)) objects_.Update(token,sid,true);else if (count<kDelayedLimit) { Put(memory_.Bytes(temporary),count*4,cursor);++count; } } }next(); }
        // 방어용300개 제한을 넘은 항목은 통지하지 않으며 SID순서를 유지한다.
        for (std::uint32_t index=0;index<count;++index) objects_.Update(token,Object(Read(memory_.Bytes(temporary),index*4)),true);
        Put(Body(token),0x3c8,3);
    } catch (...) { memory_.Free(temporary);throw; }
    memory_.Free(temporary);
}
}
