// 현재 역할과 후보를 매번 다시 읽고 선행 삭제의 부분 효과를 실패 뒤에도 보존한다.
#include "o/RawDaisDeletion.h"
#include "o/RawSquidFinder.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace netstorm::o { // 원문 고유 함수와 호스트의 물리/Kernel 경계를 구별한다.
namespace { // 원문 폭의 읽기와 선택 시점의 함수 수명만 관리한다.
template<class F> F Need(const F& function) { if (!function) throw std::logic_error("다이스 삭제의 선택된 실제 하위 미연결");return function; } // 앞선 효과 뒤 현재 callable을 복사한다.
constexpr double kAlmostCeil=0.9999899864196777; // 원문41def0의 우하단만 거의 올리는 float 상수의 정확한 값이다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) { // 지정한 비정렬 WORD/DWORD만 읽는다.
    if (offset>bytes.size() || bytes.size()-offset<width) throw std::out_of_range("다이스 삭제의 현재 Player 물리 범위");
    std::uint32_t value{};for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[offset+i])<<(8*i);return value; // 인접 필드를 섞지 않는다.
}
void Write(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { // 같은 현재 Player DWORD의 네 바이트만 저장한다.
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("다이스 삭제의 현재 Player 물리 범위");
    for (std::size_t i=0;i<4;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); // 실제 폭을 유지한다.
}
int Cell(double value) { if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("다이스 삭제의 사각형 절삭 범위");return static_cast<int>(value); } // 정상 좌표를0방향으로 절삭한다.
SquidSearchArea Area(const SquidFootprint& box) { return {Cell(box.left),Cell(box.top),Cell(double(box.right)+kAlmostCeil),Cell(double(box.bottom)+kAlmostCeil)}; } // 원문 float 사각형 ctor의 우하단만 거의 올린다.
}
RawDaisDeletion::RawDaisDeletion(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,const RawSquidNeighbors& footprint,
    const std::uint32_t& altarType,const std::uint32_t& runeType,DaisDeletionHooks hooks)
    :pool_(pool),hash_(hash),types_(types),footprint_(footprint),altarType_(altarType),runeType_(runeType),hooks_(std::move(hooks)) {
    if (&pool!=&footprint.Pool()) throw std::invalid_argument("다이스 삭제의 같은 발자국 Pool 계약");
}
// 제단의 추상/매장 비트는 이 고유 전처리의 생략 조건이 아니다.
void RawDaisDeletion::AltarPre(Sid sid,std::uint32_t flags) const {
    if (!(flags&0x400000U)) { const auto dais=Need(hooks_.findDais)(sid);if (dais.value && dais.value<pool_.Capacity() && !(pool_.Slot(dais)[11]&2U)) Need(hooks_.destroy)(dais,flags); }
    RawSquidFinder finder(pool_,hash_,types_);const auto extra=pool_.Edition()==OriginalEdition::Patch1078 ? 40U:35U;
    for (auto current=finder.Begin(Area(footprint_.Footprint(sid)));current.value;current=finder.Next()) { // 다음 링크는 기존 finder가 반환 전에 저장한다.
        if (pool_.Slot(current)[10]==runeType_ && (pool_.Slot(current)[extra]&1U) && (Need(hooks_.boss)() || (current.value>4 && current.value<pool_.Layout().serverFirst))) Need(hooks_.destroy)(current,0);
    }
    Need(hooks_.pre)(sid,flags);
}
// 추상/매장 관문은 한 번만 평가하며 큰 완료/삭제가 바꾼 현재 공간과 역할은 뒤에서 다시 읽는다.
void RawDaisDeletion::DaisPre(Sid sid,std::uint32_t flags) const {
    if (!(pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40U:35U]&9U)) {
        if (flags&0x400000U) { const auto altar=Need(hooks_.findAltar)(sid);Need(hooks_.finishForms)(sid,altar); }
        const auto priest=Need(hooks_.findPriest)(sid);if (priest.value && priest.value<pool_.Capacity()) Need(hooks_.stopPriest)(priest);
        RawSquidFinder finder(pool_,hash_,types_);
        for (auto current=finder.Begin(Area(footprint_.Footprint(sid)));current.value;current=finder.Next()) if (pool_.Slot(current)[10]==altarType_) Need(hooks_.destroy)(current,0x48U); // 현재 제단마다 원래 flags48을 전달한다.
        for (std::uint32_t owner=1;owner<=kPlayerCount;++owner) Need(hooks_.refreshOwner)(owner); // 현재1..8 갱신 순서를 보존한다.
    }
    Need(hooks_.pre)(sid,flags);
}
// PATCH449580의 ESI base 보유/조건부 재조회와 각 현재 owner 계산을 실제 Player view/pin으로 보존한다.
void RawDaisDeletion::DaisPost(Sid sid,std::uint32_t flags) const {
    if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("현재 Player 다이스 삭제의 PATCH 계약");
    if (pool_.Slot(sid)[34]) {
        auto access=Need(hooks_.players)(); // 원문 첫 ESI base이며 다음 조건까지 원래 포인터를 유지한다.
        const auto at=[&](std::size_t field) { return std::size_t(pool_.Slot(sid)[34])*0xb4U+field; }; // 각 주소 계산에서 현재 owner BYTE를 읽는다.
        if (Read(access.records,at(0x94))==sid.value) {
            Write(access.records,at(0x94),0);access=Need(hooks_.players)();const auto primary=at(0x94);Write(access.records,primary+4,Read(access.records,primary));access=Need(hooks_.players)();
        }
        if (Read(access.records,at(0x8c))==sid.value) {
            Write(access.records,at(0x8c),0);access=Need(hooks_.players)();const auto primary=at(0x8c);Write(access.records,primary+4,Read(access.records,primary));
        }
    } // Player span/pin은 외부 Damageable 후처리에 들어가기 전에 반납한다.
    Need(hooks_.post)(sid,flags);
}
// 실제 ProcessForm의 type helper41bff0은 해당 프로세스 Form의 현재 BYTE를 읽는다.
void RawDaisDeletion::FinishForms(const SidPool& pool,SquidProcessHost& host,Sid dais,Sid altar,const std::uint32_t& regularType,
    const std::uint32_t& sharedType,const std::function<void(Sid)>& finish) {
    Sid current{static_cast<std::uint16_t>(Read(pool.Slot(altar),6,2))};std::unordered_set<std::uint16_t> visited; // 원문 parent0도 물리 슬롯으로 조회하며 가짜 제단 유효성 검사를 넣지 않는다.
    while (current.value) { // 현재 후보의 next는 완료 콜백보다 먼저 저장한다.
        if (current.value>=pool.Capacity() || !visited.insert(current.value).second) throw std::logic_error("다이스 제단 프로세스 체인 손상");
        const auto raw=pool.Slot(current);const Sid next{static_cast<std::uint16_t>(Read(raw,4,2))};
        if (raw[10]<=69) {
            if (raw[10]<10 || raw[10]>=host.ProcessTypeEnd()) throw std::logic_error("다이스 제단 process finder isProcess assert");
            auto* process=host.FromForm(current);if (!process) throw std::logic_error("다이스 제단 실제 Kernel 프로세스 누락");
            if (pool.Slot(process->Form())[10]==regularType || pool.Slot(process->Form())[10]==sharedType) {
                const auto* regular=dynamic_cast<const RegularProcess*>(process);if (!regular) throw std::logic_error("다이스 제단의 실제 Regular 필드 미복원");if (regular->Event()==24) Need(finish)(dais);
            }
        }
        current=next;
    }
}
const SidPool& RawDaisDeletion::Pool() const { return pool_; } // 같은 물리 정체성만 공개한다.
// 현재 타입 인자를 한 번 저장하고 같은 일반 finder의 현재 발자국/버킷/매장 조건을 사용한다.
Sid RawDaisDeletion::FindTypeAt(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,Sid source,std::uint32_t type) {
    const auto raw=pool.Slot(source);return FindTypeAt(pool,hash,types,std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),type);
}
// 좌표를 원문 절삭한 뒤 같은 TYPE별 시작 단계/첫 일치를 사용한다.
Sid RawDaisDeletion::FindTypeAt(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,float px,float py,std::uint32_t type) {
    const auto x=Cell(px),y=Cell(py);
    if (type>=types.size()) throw std::out_of_range("다이스 삭제의 현재 typed 조회 TYPE 범위");RawSquidFinder finder(pool,hash,types);
    for (auto current=finder.Begin({x,y,x,y},(types[type].flags2&6U)==0 ? 1U:0U);current.value;current=finder.Next()) if (pool.Slot(current)[10]==type) return current; // 첫 일치에서 즉시 반환한다.
    return {};
}
}
