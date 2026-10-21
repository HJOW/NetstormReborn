// 기술의 원소 분류와 현재 시설 목록·생산 슬롯·행동 기한을 원본 순서대로 처리한다.
#include "o/RawAiFactoryProduction.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kUpgradeBudget=1199; //원본 지갑>0x4af의 업그레이드 후보 관문이다.
// 비정렬 little endian DWORD를 물리 범위 안에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 시설 생산 DWORD 읽기 범위");std::uint32_t value{};
    // 원본 네 BYTE의 낮은 자리부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
// 지정 네 BYTE만 바꾸고 다른 본체/목록 원문을 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 시설 생산 DWORD 쓰기 범위");
    // 네 BYTE의 원본 little endian 표현을 기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// signed 비교에서 DWORD 감김을 보존한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 선택되지 않은 하위는 요구하지 않고 실제 선택 지점에서만 누락을 거부한다.
template<class Function> const Function& Need(const Function& function,const char* message) { if (!function) throw std::logic_error(message);return function; }
}
// 생성할 때 실제 게임 효과를 일으키지 않는다.
RawAiFactoryProduction::RawAiFactoryProduction(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,const ContainedFinderState& finder,AiFactoryProductionGlobals globals,AiFactoryProductionHooks hooks)
    :memory_(memory),pool_(pool),types_(types),finder_(finder),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.empty()) throw std::invalid_argument("AI 시설 생산 같은 PATCH/TYPE 계약 누락"); }
// 가짜/해제 토큰이나 다른 크기의 본체를 거부한다.
std::span<std::uint8_t> RawAiFactoryProduction::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 시설 생산 실제0xb08 본체 누락");return raw; }
// 외부 효과 뒤 현재 같은 본체를 다시 읽는다.
std::uint32_t RawAiFactoryProduction::Field(std::uint32_t token,std::size_t offset) const { return Read(Body(token),offset); }
// 선택된 필드만 현재 본체에 쓴다.
void RawAiFactoryProduction::Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const { Put(Body(token),offset,value); }
// 현재 목록의 같은 소유 backing을 읽는다.
std::uint32_t RawAiFactoryProduction::Item(std::uint32_t token,std::size_t pointer,std::int32_t index) const { if (index<0) throw std::out_of_range("AI 시설 생산 음수 목록 인덱스");return Read(memory_.Bytes(Field(token,pointer)),static_cast<std::size_t>(index)*4); }
// 원본처럼 모든 일치 항목만 제거하며 사용하지 않는 꼬리는 지우지 않는다.
void RawAiFactoryProduction::Remove(std::uint32_t token,std::size_t pointer,std::uint32_t value) const {
    std::uint32_t removed{};
    // 현재 count/포인터를 각 방문 시 다시 읽고 살아남은 항목의 상대 순서를 보존한다.
    for (std::int32_t index=0;index<Signed(Field(token,pointer+8));++index) { const auto item=Item(token,pointer,index);if (item==value) ++removed;else Put(memory_.Bytes(Field(token,pointer)),(static_cast<std::size_t>(index)-removed)*4,item); }
    Field(token,pointer+8,Field(token,pointer+8)-removed);
}
// 원본은 가득 찼을 때 포인터를 읽지 않으며 중복을 검사하지 않는다.
void RawAiFactoryProduction::Append(std::uint32_t token,std::size_t pointer,std::uint32_t value) const {
    const auto count=Signed(Field(token,pointer+8));if (count>=Signed(Field(token,pointer+4))) return;if (count<0) throw std::out_of_range("AI 시설 생산 음수 추가 인덱스");Put(memory_.Bytes(Field(token,pointer)),static_cast<std::size_t>(count)*4,value);Field(token,pointer+8,Field(token,pointer+8)+1U);
}
// 원본 풀 밖 접근을 잘린 다른 SID로 바꾸지 않는다.
Sid RawAiFactoryProduction::PhysicalSid(std::uint32_t value) const { if (value>=pool_.Capacity() || value>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("AI 시설 생산 현재 SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// TYPE0은 원본 getter처럼 허용한다.
const RiftTypeRecord& RawAiFactoryProduction::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("AI 시설 생산 현재 TYPE 범위");return types_[type]; }
// 같은 시설의 현재 BYTE TYPE에서 원소를 다시 읽는다.
std::int32_t RawAiFactoryProduction::Theme(Sid factory) const { return Type(pool_.Slot(factory)[10]).theme; }
// QWORD 시각을 float로 축소하지 않는다.
bool RawAiFactoryProduction::Ready(std::uint32_t token) const { const auto due=std::bit_cast<double>((std::uint64_t(Field(token,0x5ac))<<32)|Field(token,0x5a8));return globals_.now>=due; }
// 부가 행동은 현재 rate와QWORD now로 두 생각 기한을 기록한 뒤 디버그 로그를 처리한다.
void RawAiFactoryProduction::Action(std::uint32_t token,AiFactoryAction action,std::uint32_t type) const {
    const auto rate=static_cast<double>(std::bit_cast<float>(Field(token,0x228)));const auto first=std::bit_cast<std::uint64_t>(rate+globals_.now),second=std::bit_cast<std::uint64_t>((rate+1.0)+globals_.now);Field(token,0x5a8,static_cast<std::uint32_t>(first));Field(token,0x5ac,static_cast<std::uint32_t>(first>>32));Field(token,0x5b0,static_cast<std::uint32_t>(second));Field(token,0x5b4,static_cast<std::uint32_t>(second>>32));if (Field(token,0xb00)) Need(hooks_.log,"AI 시설 생산 현재 행동 로그 미연결")(token,action,type);
}
// 원본 기술 순회는 다음SID를 먼저 저장하며 기술 추가/생산 조회 효과 뒤에도 그 next를 유지한다.
std::uint32_t RawAiFactoryProduction::SelectTechnology(std::uint32_t token,std::int32_t theme) const {
    const auto parent=Need(hooks_.technology,"AI 시설 생산 현재 기술부모 조회 미연결")(Field(token,0x260));RawContainedFinder cursor(pool_,finder_);
    // flags4 기술 내용물만 현재 TYPE/원소로 판정하며 생산이 없는 첫 종류를 돌려준다.
    for (auto form=cursor.Begin(parent);form.value;form=cursor.Next()) {
        const auto raw=pool_.Slot(form);if (!(raw[20]&4)) continue;const auto type=static_cast<std::uint32_t>(raw[18])|(static_cast<std::uint32_t>(raw[19])<<8);const auto& entry=Type(type);bool matches=entry.theme==theme;
        if (entry.flags2&0x800U) { const auto offset=std::int64_t{0x3b4}+static_cast<std::int64_t>(entry.theme)*4;if (offset<0 || static_cast<std::uint64_t>(offset)>RawAiBrainConstructor::kBytes-4) throw std::out_of_range("AI 시설 생산 원소 치환 필드 범위");const auto replacement=Field(token,static_cast<std::size_t>(offset));if (replacement==1) matches=theme==3;else if (replacement==2) matches=matches || theme==3;else if (replacement) matches=false; }
        if (matches && !Need(hooks_.product,"AI 시설 생산 현재 생산 조회 미연결")(token,type).value) return type;
    }
    return 0;
}
// 같은 종류의 중복 생산도 원본 총수에는 각각 포함한다.
std::int32_t RawAiFactoryProduction::CountProducts(Sid factory) const {
    std::uint32_t count{};RawContainedFinder cursor(pool_,finder_);
    // 원본mask10 조건만 확인하며 pending/종류/소유자 필터를 추가하지 않는다.
    for (auto form=cursor.Begin(factory);form.value;form=cursor.Next()) if (pool_.Slot(form)[20]&0x10U) ++count;return Signed(count);
}
// 미처리 첫 시설의 생산 등록 또는 업그레이드 대기 첫 시설의 요청만 처리한다.
void RawAiFactoryProduction::Manage(std::uint32_t token) const {
    if (!Field(token,0x294) || Field(token,0x298)==Need(globals_.invalidGraph,"AI 시설 생산 현재 무효 그래프 조회 미연결")() || !Ready(token)) return;
    if (Signed(Field(token,0x314))<1) {
        if (!Field(token,0x314) && Field(token,0x2cc) && Signed(Field(token,0x2d8))>Signed(kUpgradeBudget) && Signed(Field(token,0x320))>0) { const auto value=Item(token,0x318,0);Need(hooks_.upgrade,"AI 시설 업그레이드00453bf0 실제 요청 미연결")(PhysicalSid(value));Remove(token,0x318,value);Action(token,AiFactoryAction::Upgrade,0); }
        return;
    }
    const auto value=Item(token,0x30c,0);const auto factory=PhysicalSid(value);const auto level=Need(hooks_.level,"AI 시설 현재 가상level 미연결")(factory);const auto capacity=Signed(level+2U),count=CountProducts(factory);const auto type=SelectTechnology(token,Theme(factory));
    if (type && count<capacity) { Need(hooks_.addProduct,"AI 시설 실제 생산 등록00410420 미연결")(token,factory,type);static_cast<void>(Type(type));Action(token,AiFactoryAction::AddedProduct,type);if (Signed(static_cast<std::uint32_t>(count)+1U)<capacity) return; }
    Remove(token,0x30c,value);if (type && Signed(Need(hooks_.level,"AI 시설 현재 가상level 재조회 미연결")(factory))<2 && SelectTechnology(token,Theme(factory))) Append(token,0x318,value);
}
// 같은 실제 월드 연결 확인용 풀이다.
const SidPool& RawAiFactoryProduction::Pool() const { return pool_; }
}
