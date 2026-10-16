// 실제 목록 추가의 중복·용량과 콜백 뒤 현재 raw 재조회를 보존한다.
#include "o/RawAiDamage.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr std::size_t kGuard=0x360,kHate=0x36c,kVery=0x378,kTracked=0x384; // 실제 네 pointer/capacity/count 기록이다.
// 비정렬 little endian DWORD만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset+4>raw.size()) throw std::out_of_range("AI 피해 DWORD 범위 오류");std::uint32_t value=0;
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(i*8); // 실제 네 바이트만 조합한다.
    return value;
}
// 주변 padding을 보존하는 실제 DWORD 쓰기다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset+4>raw.size()) throw std::out_of_range("AI 피해 쓰기 범위 오류");
    for (std::size_t i=0;i<4;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8)); // 낮은 바이트부터 쓴다.
}
// 정상 유한 좌표의 원본 signed64 절삭 후 낮은 DWORD를 보존한다.
std::int32_t Coordinate(float value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) throw std::out_of_range("AI 피해 CRT 좌표 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 같은 실제 PATCH 타입과 명시 효과만 보관한다.
RawAiDamage::RawAiDamage(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,AiDamageHooks hooks)
    :memory_(memory),pool_(pool),types_(types),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("AI 피해는 PATCH188의 같은 실제 세계가 필요합니다");
}
// 해제되었거나 크기가 다른 본체를 목록 주소로 해석하지 않는다.
std::span<std::uint8_t> RawAiDamage::Brain(std::uint32_t token) const {
    auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 피해 실제0xb08 본체 누락");return raw;
}
// 원본은 count>0일 때 현재 목록을 앞에서부터 검색한다.
bool RawAiDamage::Contains(std::uint32_t token,std::size_t offset,Sid sid) const {
    const auto raw=Brain(token);const auto count=std::bit_cast<std::int32_t>(Read(raw,offset+8));if (count<=0) return false;
    const auto list=memory_.Bytes(Read(raw,offset));if (static_cast<std::size_t>(count)>list.size()/4) throw std::out_of_range("AI 피해 목록 count 범위 오류");
    for (std::int32_t index=0;index<count;++index) if (Read(list,static_cast<std::size_t>(index)*4)==sid.value) return true; // 원문 순서의 첫 중복에서 반환한다.
    return false;
}
// full이면 검색도 하지 않는00414450 순서다.
void RawAiDamage::Append(std::uint32_t token,std::size_t offset,Sid sid) const {
    auto raw=Brain(token);const auto count=std::bit_cast<std::int32_t>(Read(raw,offset+8)),capacity=std::bit_cast<std::int32_t>(Read(raw,offset+4));if (count>=capacity) return;if (count<0) throw std::out_of_range("AI 피해 음수 목록 count 오류");if (Contains(token,offset,sid)) return;
    auto list=memory_.Bytes(Read(raw,offset));Put(list,static_cast<std::size_t>(count)*4,sid.value);Put(raw,offset+8,static_cast<std::uint32_t>(count)+1);
}
// 호출 위치마다 현재 공격자 타입을 다시 읽는다.
std::uint32_t RawAiDamage::Genus(Sid sid) const {
    const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("AI 피해 현재 TYPE 오류");return types_[raw[10]].flags2;
}
// 원본의 포인터 범위는 SID1도 허용하며 free bit만 검사한다.
bool RawAiDamage::ValidHome(Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) return false;const auto raw=pool_.Slot(sid);if (raw[11]&1U) return false;if (raw[10]>=types_.size()) throw std::out_of_range("AI 피해 홈 TYPE 오류");return types_[raw[10]].group==3;
}
// 좌표는 Y/X 순서로 절삭하고 TYPE는 그 뒤 현재 슬롯에서 조회한다.
void RawAiDamage::Log(std::uint32_t token,Sid sid,bool more) const {
    if (!hooks_.log) throw std::logic_error("AI 피해 실제 로그004105b0 미연결");const auto raw=pool_.Slot(sid);const auto x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));const auto iy=Coordinate(y),ix=Coordinate(x);hooks_.log(token,{more,sid,pool_.Slot(sid)[10],ix,iy});
}
// 실제 hate의 full 자연 경로도 로그 요청을 남긴다.
void RawAiDamage::Hate(std::uint32_t token,Sid sid) const {
    if (!sid.value || Contains(token,kHate,sid)) return;Append(token,kHate,sid);Log(token,sid,false);
}
// 피해 대상 genus와 네 현재 목록으로 두 단계 hate 반응을 순서대로 수행한다.
void RawAiDamage::Notice(std::uint32_t token,Sid victim,Sid attacker) const {
    if (!attacker.value) return;Brain(token);auto genus=Genus(victim);if (pool_.Slot(victim)[10]==164) genus|=0x400000U;
    bool hate=Contains(token,kTracked,victim),very=hate;
    if (genus&0x80604200U) {
        hate=very=true;
        if ((genus&0x80404200U) && !Contains(token,kVery,attacker)) { auto raw=Brain(token);Put(raw,0x404,(Read(raw,0x248)>>1)&1U); }
    }
    if ((genus&0x4000000U) && Contains(token,kGuard,victim)) hate=true;
    if (((genus&0x30000U) && Contains(token,kTracked,victim)) || hate) {
        Hate(token,attacker);
        if (Genus(attacker)&0x100000U) { if (!hooks_.home) throw std::logic_error("AI 피해 실제 비행 홈0045c100 미연결");const auto home=hooks_.home(attacker);if (ValidHome(home)) Hate(token,home); }
    }
    if (very && !Contains(token,kVery,attacker)) {
        Append(token,kVery,attacker);
        if (Genus(attacker)&0x100000U) { if (!hooks_.home) throw std::logic_error("AI 피해 실제 very 비행 홈0045c100 미연결");const auto home=hooks_.home(attacker);if (ValidHome(home)) Append(token,kVery,home); }
        Log(token,attacker,true);
    }
}
// 어댑터는 다른 Pool과 섞이지 않았는지 구성 시 확인한다.
const SidPool& RawAiDamage::Pool() const { return pool_; }
}
