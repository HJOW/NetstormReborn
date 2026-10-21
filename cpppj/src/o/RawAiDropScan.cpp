// 원본 스캔의 X선증가/예산/현재상태·난수순서와30후보 교체/압축을 보존한다.
#include "o/RawAiDropScan.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::int32_t kChoices=30,kNormalBudget=1000,kSupplyBudget=500; //원본최대후보와flags800별호출예산이다.
constexpr std::size_t kWidth=256; //원본지도행stride다.
// 비정렬DWORD의실제물리범위를확인해읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 스캔 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은지정네BYTE만조합한다.
// 원본DWORD의signed해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본DWORD의덧셈/뺄셈감김이다.
std::int32_t Add(std::int32_t value,std::uint32_t delta) { return Signed(static_cast<std::uint32_t>(value)+delta); }
// ftol의signed64절삭하위DWORD다.무효FISTP의하위값은0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
// 선택된실제하위가없으면앞선상태/효과를보존하며거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 배치 스캔 실제 하위 미연결");return function; }
}
// 생성중스캔/난수/장치를실행하지않는다.
RawAiDropScan::RawAiDropScan(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> spots,std::span<const std::uint16_t> surfaces,std::span<const std::uint32_t> alliances,GameRandom& random,AiDropScanGlobals globals,AiDropScanHooks hooks):memory_(memory),pool_(pool),types_(types),frames_(frames),spots_(spots),surfaces_(surfaces),alliances_(alliances),random_(random),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=types.size() || spots.size()!=65536 || surfaces.size()!=65536 || alliances.size()!=81) throw std::invalid_argument("AI 배치 스캔 실제PATCH월드누락"); }
// 해제/작은가짜본체를거부한다.
std::span<std::uint8_t> RawAiDropScan::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 배치 스캔 실제0xb08본체 누락");return raw; }
// 각하위효과뒤현재본체를다시읽는다.
std::uint32_t RawAiDropScan::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 지정필드만쓰고다른상태를동기화하지않는다.
void RawAiDropScan::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { auto raw=Body(token);if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 스캔 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은지정네BYTE만쓴다.
// 행경계를clamp하면원본세칸검사의주소가달라지므로전체선형범위만확인한다.
std::uint8_t RawAiDropScan::Spot(std::int32_t x,std::int32_t y) const { const auto at=static_cast<std::int64_t>(y)*static_cast<std::int64_t>(kWidth)+x;if (at<0 || at>=static_cast<std::int64_t>(spots_.size())) throw std::out_of_range("AI 다리 후보 spot 물리 범위");return spots_[static_cast<std::size_t>(at)]; }
// 정상후보배열의범위를넘는손상count/index는명시거부한다.
std::size_t RawAiDropScan::Point(std::int32_t index) const { if (index<0 || index>=kChoices) throw std::out_of_range("AI 배치 후보 좌표 인덱스");return 0x408+static_cast<std::size_t>(index)*8; }
// 같은후보의표적은별도30DWORD배열이다.
std::size_t RawAiDropScan::Target(std::int32_t index) const { if (index<0 || index>=kChoices) throw std::out_of_range("AI 배치 후보 표적 인덱스");return 0x4f8+static_cast<std::size_t>(index)*4; }
// 각쓰기순서와현재count재조회를스캔호출자가정한다.
void RawAiDropScan::Choice(std::uint32_t token,std::int32_t index,RawPathPoint point,Sid target) const { const auto at=Point(index);Field(token,at,std::bit_cast<std::uint32_t>(point.x));Field(token,at+4,std::bit_cast<std::uint32_t>(point.y));Field(token,Target(index),target.value); }
// 양쪽모두가능하면J의위쪽/K의왼쪽이우선이며실패때난수를소비하지않는다.
bool RawAiDropScan::AdjustBridge(Sid surface,std::int32_t& x,std::int32_t& y) const {
    const auto raw=pool_.Slot(surface);if (raw[10]!=globals_.bridgeType) return false;if (raw[10]>=frames_.size()) throw std::out_of_range("AI 다리 후보 현재TYPE 범위");const auto side=RawPathAnimation::Side(frames_[raw[10]],Signed(Read(raw,36)));
    if (side=='J') { if (!Spot(x,Add(y,0xffffffffU)) && !Spot(x,Add(y,0xfffffffeU)) && !Spot(x,Add(y,0xfffffffdU))) y=Add(y,0xffffffffU);else { if (Spot(x,Add(y,1)) || Spot(x,Add(y,2)) || Spot(x,Add(y,3))) return false;y=Add(y,3); }const auto draw=random_.Next(3);x=Add(x,draw);return true; }
    if (side=='K') { if (!Spot(Add(x,0xffffffffU),y) && !Spot(Add(x,0xfffffffeU),y) && !Spot(Add(x,0xfffffffdU),y)) x=Add(x,0xffffffffU);else { if (Spot(Add(x,1),y) || Spot(Add(x,2),y) || Spot(Add(x,3),y)) return false;x=Add(x,3); }const auto draw=random_.Next(3);y=Add(y,draw);return true; }return false;
}
// 원본 유일 호출과 같은3e8의 현재 점수를 그 자리에서 평가·비교한다.
bool RawAiDropScan::Scan(std::uint32_t token,std::uint32_t type,std::int32_t direction) const {
    if (type>=types_.size()) throw std::out_of_range("AI 배치 스캔 현재TYPE 범위");const auto flags=types_[type].flags2;const auto bounds=globals_.bounds;const auto left=Ftol(bounds.left),top=Ftol(bounds.top),right=Ftol(bounds.right),bottom=Ftol(bounds.bottom);auto budget=(flags&0x800U) ? kSupplyBudget : kNormalBudget;
    // 각호출의flags/경계는snapshot이고본체진행/소유자/지도를각원문위치에서새로읽는다.
    do { auto x=Add(Signed(Field(token,0xaf8)),1);Field(token,0xaf8,static_cast<std::uint32_t>(x));if (x>right) { Field(token,0xafc,Field(token,0xafc)+1U);Field(token,0xaf8,static_cast<std::uint32_t>(left));if (Signed(Field(token,0xafc))>bottom) { Field(token,0xafc,static_cast<std::uint32_t>(top));return false; } }
        x=Signed(Field(token,0xaf8));auto y=Signed(Field(token,0xafc));if (x>=0 && y>=0 && x<256 && y<256) { const Sid surface{surfaces_[static_cast<std::size_t>(y)*kWidth+static_cast<std::size_t>(x)]};if (surface.value) { const bool bridge=AdjustBridge(surface,x,y);budget-=5;bool eligible=bridge;
                if (!bridge && (flags&0x4040800U) && (Spot(x,y)&0x3fU)==2U && Field(token,0x2c8)) { const auto surfaceOwner=pool_.Slot(surface)[34];const auto owner=Field(token,0x260);const auto at=static_cast<std::uint64_t>(surfaceOwner)*9U+owner;if (at>=alliances_.size()) throw std::out_of_range("AI 배치 스캔 현재 동맹 인덱스");if (alliances_[static_cast<std::size_t>(at)]) { eligible=surfaceOwner==owner && Need(hooks_.free)(type,x,y);budget-=10; } }
                if (eligible) { const auto previous=Signed(Field(token,0x3e8));RawPathPoint point{};Sid target{};if (Need(hooks_.consider)(token,type,{static_cast<float>(x),static_cast<float>(y)},direction,point,target)) { if (Signed(Field(token,0x3e8))>previous) { Field(token,0x570,0);Choice(token,0,point,target);Field(token,0x570,Field(token,0x570)+1U); }else { const auto count=Signed(Field(token,0x570));if (count>=kChoices) { const auto index=Signed(random_.Next(static_cast<std::uint32_t>(count)));Choice(token,index,point,target); }else { Choice(token,count,point,target);Field(token,0x570,Field(token,0x570)+1U); } } }budget-=125; }
            } }
        --budget;
    }while (budget>0);return true;
}
// 진단 뒤 현재 count를 먼저 줄이고 복사하지 않는 꼬리/점수/진행 위치는 보존한다.
void RawAiDropScan::Remove(std::uint32_t token,std::int32_t index) const { if (Signed(Field(token,0x570))<1) Need(hooks_.assertion)("totalChoices > 0",0x1ac);Field(token,0x570,Field(token,0x570)-1U);
    // 현재count까지XY→표적순서로안정압축하고꼬리바이트는그대로둔다.
    while (index<Signed(Field(token,0x570))) { const auto at=Point(index),next=Point(Add(index,1));Field(token,at,Field(token,next));Field(token,at+4,Field(token,next+4));Field(token,Target(index),Field(token,Target(Add(index,1))));index=Add(index,1); }
}
// 동일실제풀연결확인용이다.
const SidPool& RawAiDropScan::Pool() const { return pool_; }
}
