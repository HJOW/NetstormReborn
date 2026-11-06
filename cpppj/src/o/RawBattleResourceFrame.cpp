// PE의 x87 ordered 비교·두 번의 정수화·각 효과 뒤 재조회와 후보 순서를 유지한다.
#include "o/RawBattleResourceFrame.h"
#include "o/RawBattleResourceOptions.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace netstorm::o { // 새 프레임/Player/Ready/권한을 합성하지 않는다.
namespace { // 같은 현재 바이트와 Native signed DWORD만 처리한다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("일반 자원 프레임의 실제 장치 미연결");return effect; } // 선택된 외부 효과만 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(index*8);return value; } // 원문 비정렬 네BYTE만 읽는다.
std::span<const std::uint8_t> Players(const BattleResourceFrameGlobals& g) { const auto rows=Need(g.players)();if (rows.size()!=9*180) throw std::out_of_range("자원 프레임 Player9 물리 누락");return rows; } // 수신/생성 뒤 현재 base를 재조회한다.
bool AllLogged(const BattleResourceFrameGlobals& g) { bool result=true;for (std::uint32_t owner=1;owner<9;++owner) { const auto row=Players(g).subspan(owner*180,180);if (Read(row,0x2c) && !(row[0x50]&0x22U)) result=false; }return result; } //0048f4e0은 subscriber 행마다 flags2/20 중 하나를 요구한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 감긴 DWORD를 부호 확장한다.
std::int32_t Truncate(double value) { return !std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0 ? 0 : Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); } // x87_ftol signed64 하위DWORD와 indefinite0이다.
std::int32_t Target(std::int32_t cell,std::int32_t corner,std::int32_t center) { return cell==5 ? center : cell==1 || cell==3 || cell==7 || cell==9 ? corner : center/2; } // PE는 모서리corner/변(center/2)/중앙center를 사용한다.
}
void RunBattleResourceFrame(BattleResourceFrameGlobals g,const BattleResourceFrameHooks& h) {
    if (g.options.size()!=87) throw std::out_of_range("일반 자원 프레임 옵션87BYTE 누락");if (g.loginEnd!=0.0) return;auto& aux=g.aux;
    if (Signed(aux.SpellCount())>0 && aux.SpellDeadline()!=0.0 && g.now>=aux.SpellDeadline()) { if (g.boss) Need(h.replenish)();const auto next=g.now+20.0;--aux.SpellCount();aux.SpellDeadline()=next; } // PE는 현재 개수 감산을 새 기한 store보다 먼저 한다.
    if (!aux.GeyserFrameFlag() && g.now>=aux.GeyserDeadline()) {
        if (!g.battle) aux.IncomePrimed()=0;
        if (g.boss && g.battle && AllLogged(g) && BattleIncomePeriod(g.options)) {
            const auto period=BattleIncomePeriod(g.options);const auto primed=aux.IncomePrimed();aux.GeyserDeadline()=static_cast<double>(period)+g.now;
            if (!primed) aux.IncomePrimed()=1;
            else for (std::uint32_t owner=1;owner<9;++owner) { // 원문 숨김BYTE를 먼저 읽고 현재subscriber가 있는 행만 지급한다.
                if (!g.options[62+owner] && Read(Players(g),owner*180+0x2c)) { const auto amount=BattleIncomeAmount(g.options);Need(h.income)(owner,g.owner,amount); }
            }
        }
    }
    if (aux.OrdinaryGeysers() || !(g.now>=aux.SecondaryGeyserDeadline())) return;const auto period=g.single ? 30 : BattleGeyserPeriod(g.options);const bool boss=g.boss;aux.SecondaryGeyserDeadline()=static_cast<double>(period)+g.now;
    if (!boss || g.editor || !BattleGeyserPeriod(g.options) || !AllLogged(g)) return;GameRandom random(0xbeefU);if (!Need(h.predictable)()) random.SetState(RawFortFinishOwner::TimeSeed(Need(h.wallSeconds)()));
    if (!g.single && g.battle) {
        if (g.options[18]==2) {
            const auto budget=Need(h.budget)(),density=Need(h.density)();const double original=budget;const double densityAddition=density==1 ? 0.1 : density==2 ? 0.2 : 0.0;const auto adjusted=Truncate((densityAddition+1.0)*original);const auto spacing=Need(h.spacing)(); // 두 getter 사이에 원문 첫_ftol을 수행한다.
            const double sizeAddition=spacing==3 ? -0.2 : spacing==4 ? -0.1 : spacing==6 ? 0.1 : 0.0;const auto corner=Truncate(sizeAddition*original+adjusted)/2/4,center=Signed(static_cast<std::uint32_t>(corner)+1U);std::array<std::int32_t,10> counts{};
            for (std::int32_t cell=1;cell<=9;++cell) { const auto rectangle=Need(h.partition)(cell);counts[static_cast<std::size_t>(cell)]=Signed(Need(h.count)(rectangle)); } // 현재TYPE와 실제flags1 finder로 아홉 구역을 먼저 센다.
            std::vector<std::int32_t> choices;for (std::int32_t cell=1;cell<=9;++cell) if (counts[static_cast<std::size_t>(cell)]<Target(cell,corner,center)) choices.push_back(cell); // 후보는 원문1..9 순서다.
            if (!choices.empty()) { const auto selected=choices.size()==1 ? 0U:Need(h.choose)(static_cast<std::uint32_t>(choices.size()));if (selected>=choices.size()) throw std::out_of_range("일반 간헐천 실제 난수 후보 범위");const auto cell=choices[selected];const auto missing=Signed(static_cast<std::uint32_t>(Target(cell,corner,center))-static_cast<std::uint32_t>(counts[static_cast<std::size_t>(cell)]));const auto quantity=missing<=1 ? 1:missing/2;const auto rectangle=Need(h.partition)(cell);Need(h.populate)(quantity,random,rectangle,{1,1,1}); } // 선택한 구역 범위를 다시 읽어 현재 자원으로 실제 배치한다.
        }
        if (g.options[18]!=1) return;
    }
    const auto resource=static_cast<float>(aux.ResourcePool());if (!(Need(h.typeCost)()<resource)) { const auto target=Need(h.budget)()/3;if (Signed(Need(h.geyserCount)())>=target) return; }
    const auto rectangle=Need(h.whole)();Need(h.populate)(1,random,rectangle,{0,1,1}); // 원문의 전체 한 개 배치는 재사용0/지형1/허공1이다.
}
}
