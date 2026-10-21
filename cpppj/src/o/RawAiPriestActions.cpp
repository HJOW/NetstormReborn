// 사제AI의선행탐색·현재재조회·의식/건설·회수요청과원본기한을보존한다.
#include "o/RawAiPriestActions.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kSnapBias=0.99999F,kDaisDistance=2.0F; //PE00502310/00501444의원본float보정과다이스거리경계다.
constexpr double kBuildDelay=20.0,kSearchDelay=3.0,kSalvageDelay=10.0; //PE005012d8/00501448/00501450의원본double기한이다.
constexpr std::uint32_t kUnset=0xc61c4000U; //원본목표미정-10000.0f의DWORD비트다.
// 실제비정렬little endian DWORD를읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 사제 행동 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은지정네BYTE만조합한다.
// 지정DWORD만쓰고주변원문을보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 사제 행동 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은지정네BYTE만쓴다.
// 원본DWORD의signed해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본ftol의signed64절삭하위DWORD다.무효FISTP의하위값은0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
// 선택된실제하위가없으면앞선효과를보존하고명시실패한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 사제 행동 실제 하위 미연결");return function; }
}
// float0.99999더하기뒤중간float축소없이ftol하며하위signed DWORD를float로저장한다.
RawPathPoint SnapAiPriestPoint(RawPathPoint point) { point.x=static_cast<float>(Ftol(static_cast<double>(point.x)+kSnapBias));point.y=static_cast<float>(Ftol(static_cast<double>(point.y)+kSnapBias));return point; }
// 원본은mapLimit을float에한번저장해두축을검사한다.
bool AiPriestPointValid(RawPathPoint point,std::int32_t mapLimit) { const auto limit=static_cast<float>(mapLimit);return point.x>0 && point.x<limit && point.y>0 && point.y<limit; }
// 상태BYTE/물리frame번호자체로의식진행을대신하지않는다.
bool AiPriestInRitual(const SidPool& pool,std::span<const RiftTypeFrames> frames,Sid priest) { const auto raw=pool.Slot(priest);if (raw[10]>=frames.size()) throw std::out_of_range("AI 사제 의식 현재TYPE 범위");const auto phase=RawPathAnimation::Direction(frames[raw[10]],Signed(Read(raw,36)));return phase>9 && phase<15; }
// 제단조회후현재argument WORD를읽으며적owner를WORD로잘라비교하지않는다.
bool AiAltarContainsEnemy(const SidPool& pool,Sid dais,std::uint32_t enemy,std::uint32_t altarType,const std::function<Sid(float,float,std::uint32_t)>& find) { const auto raw=pool.Slot(dais);const auto x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));const auto altar=Need(find)(x,y,altarType);const auto found=pool.Slot(altar);const auto argument=static_cast<std::uint32_t>(found[12])|(static_cast<std::uint32_t>(found[13])<<8);return argument==enemy; }
// 생성시게임행동/장치를호출하지않는다.
RawAiPriestActions::RawAiPriestActions(RawAiBrainMemory& memory,const SidPool& pool,AiPriestActionGlobals globals,AiPriestActionHooks hooks):memory_(memory),pool_(pool),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("AI 사제 행동 같은PATCH풀 누락"); }
// 해제/가짜본체를거부한다.
std::span<std::uint8_t> RawAiPriestActions::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 사제 행동 실제0xb08본체 누락");return raw; }
// 하위효과뒤현재DWORD를다시읽는다.
std::uint32_t RawAiPriestActions::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 지정필드만갱신한다.
void RawAiPriestActions::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { Put(Body(token),at,value); }
// 두DWORD를원본QWORD로읽는다.
double RawAiPriestActions::Time(std::uint32_t token,std::size_t at) const { return std::bit_cast<double>((std::uint64_t(Field(token,at+4))<<32)|Field(token,at)); }
// 현재시각은각기한쓰기위치에서새로읽는다.
void RawAiPriestActions::Deadline(std::uint32_t token,std::size_t at,double delay) const { const auto value=std::bit_cast<std::uint64_t>(globals_.now+delay);Field(token,at,static_cast<std::uint32_t>(value));Field(token,at+4,static_cast<std::uint32_t>(value>>32)); }
// native now→due/C0관문과같이less/unordered를모두거부한다.
bool RawAiPriestActions::Ready(std::uint32_t token,std::size_t at) const { return globals_.now>=Time(token,at); }
// 잘못된DWORD를다른정상SID로잘라쓰지않는다.
Sid RawAiPriestActions::Object(std::uint32_t value) const { if (value>=pool_.Capacity() || value>65535) throw std::out_of_range("AI 사제 행동 실제SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// 현재raw두float를값으로저장한다.
RawPathPoint RawAiPriestActions::Position(Sid actor) const { const auto raw=pool_.Slot(actor);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 현재목표원문을읽는다.
RawPathPoint RawAiPriestActions::Target(std::uint32_t token) const { return {std::bit_cast<float>(Field(token,0xad0)),std::bit_cast<float>(Field(token,0xad4))}; }
// 같은목표원문두성분만쓴다.
void RawAiPriestActions::Target(std::uint32_t token,RawPathPoint point) const { Field(token,0xad0,std::bit_cast<std::uint32_t>(point.x));Field(token,0xad4,std::bit_cast<std::uint32_t>(point.y)); }
// 좌표미정값은원본처럼y→x로지운다.
void RawAiPriestActions::ClearTarget(std::uint32_t token) const { Field(token,0xad4,kUnset);Field(token,0xad0,kUnset); }
// 각초기화호출시현재여덟인자를같은본체에서읽고하위에값으로넘긴다.
void RawAiPriestActions::BeginSearch(std::uint32_t token,std::uint32_t type) const { const AiPriestSearchRequest request{type,{std::bit_cast<float>(Field(token,0x270)),std::bit_cast<float>(Field(token,0x274)),std::bit_cast<float>(Field(token,0x278)),std::bit_cast<float>(Field(token,0x27c))},{std::bit_cast<float>(Field(token,0x2b0)),std::bit_cast<float>(Field(token,0x2b4))},Field(token,0x260)};Need(hooks_.begin)(token,request); }
// HP는최대HPgetter전에저장하고기능800h는그효과뒤현재값을읽는다.
void RawAiPriestActions::Fallback(std::uint32_t token,Sid priest) const { if (Field(token,0x2cc)) { Need(hooks_.home)(token,priest);return; }const auto hp=Signed(Read(pool_.Slot(priest),26));const auto maximum=Need(hooks_.maxHitPoints)(priest);if (maximum<=hp && (Field(token,0x248)&0x800U)) Need(hooks_.collect)(token,priest);else Need(hooks_.home)(token,priest); }
// 원본최초count/pointer를사용하고같은HP이면앞선SID를유지한다.
Sid RawAiPriestActions::BestBuilding(std::uint32_t token) const { const auto count=Signed(Field(token,0x308));std::uint32_t selected{};if (count>0) { const auto list=memory_.Bytes(Field(token,0x300));for (std::int32_t index=0;index<count;++index) { const auto value=Read(list,static_cast<std::size_t>(index)*4);if (!selected || Signed(Read(pool_.Slot(Object(selected)),26))<Signed(Read(pool_.Slot(Object(value)),26))) selected=value; } }return Object(selected); } //반복문은처음목록을정순으로비교하며추가생존필터를넣지않는다.
// 전체prefix/각관문/현재재조회와원래요청순서를보존한다.
void RawAiPriestActions::Act(std::uint32_t token,Sid priest) const {
    if (Signed(Field(token,0x5a4))<Signed(Field(token,0x5a0))) { Need(hooks_.scan)(token,0);return; }
    if (!priest.value || !Ready(token,0x5a8) || Need(hooks_.vulnerable)(priest) || Need(hooks_.busy)(priest)) return;
    if (Need(hooks_.path)(priest).value) { static_cast<void>(Need(hooks_.maxHitPoints)(priest));return; }if (!Ready(token,0x5c0)) return;
    if (Field(token,0x2a4) && (Field(token,0x248)&0x400U)) { if (Need(hooks_.ritual)(Object(Field(token,0x29c)))) return;if (Need(hooks_.altarEnemy)(Field(token,0x264),Object(Field(token,0x2a4)))) { const auto dais=Object(Field(token,0x2a4)),self=Object(Field(token,0x29c));Need(hooks_.target)(token,self,dais,1);return; }const auto distance=Need(hooks_.distance)(Position(priest),Object(Field(token,0x2a4)));if (distance>kDaisDistance) { Need(hooks_.target)(token,priest,Object(Field(token,0x2a4)),0);return; } }
    const auto type=Need(hooks_.select)(token);
    if (Field(token,0x294) && !Field(token,0x2cc) && Signed(Field(token,0x2d8))<400) { Fallback(token,priest);return; }
    const auto target=Target(token);if (!AiPriestPointValid(target,globals_.mapLimit)) { if (!type) { Fallback(token,priest);return; } }else if (!type) { ClearTarget(token);return; }
    const auto snapped=SnapAiPriestPoint(Position(priest));const auto current=Target(token);if (snapped.x==current.x && snapped.y==current.y) { Deadline(token,0x5c0,kBuildDelay);Need(hooks_.deploy)(token,type,Position(priest));ClearTarget(token);return; }
    if (!Ready(token,0x5c8)) return;
    if (type==Field(token,0x574)) { if (Signed(Field(token,0x5a0))<=Signed(Field(token,0x5a4))) { Target(token,{std::bit_cast<float>(Field(token,0x57c)),std::bit_cast<float>(Field(token,0x580))});Field(token,0x574,0);Target(token,SnapAiPriestPoint(Target(token)));if (AiPriestPointValid(Target(token),globals_.mapLimit)) { Need(hooks_.point)(token,priest,Target(token));return; } } }else BeginSearch(token,type);
    if (type==globals_.daisType && Signed(Field(token,0x5a4))>=Signed(Field(token,0x5a0)) && !AiPriestPointValid(Target(token),globals_.mapLimit) && Time(token,0x5e8)==0.0) {
        auto selected=BestBuilding(token);if (!selected.value) { if (!Need(hooks_.enemyLowFunds)(token)) { Deadline(token,0x5c8,kSearchDelay);return; }selected=Object(Field(token,0x294));if (!selected.value) { Deadline(token,0x5c8,kSearchDelay);return; } }
        Need(hooks_.target)(token,priest,selected,0);Need(hooks_.salvage)(selected);Deadline(token,0x5e8,kSalvageDelay);BeginSearch(token,type);Need(hooks_.scan)(token,1);
    }
    Deadline(token,0x5c8,kSearchDelay);
}
// 같은실제풀연결확인용이다.
const SidPool& RawAiPriestActions::Pool() const { return pool_; }
}
