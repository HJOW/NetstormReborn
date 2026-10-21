// 원본 거리 비교·저장 폭·양수 동률·현재 owner 재조회와 선택된 효과 순서를 보존한다.
#include "o/RawAiDropEvaluation.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kDistanceValue=1000.0F; //PE00501480의원본float부족좌표점수기준이다.
constexpr std::size_t kDryStride=12; //ae4의각기록은x/y float와TYPE DWORD다.
// 지정비정렬DWORD를현재물리블록안에서읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 평가 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은지정네BYTE만조합한다.
// 원본DWORD를signed로읽는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본ftol의signed64절삭하위DWORD이며무효FISTP의하위값은0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
// 선택된실제하위가없으면그지점앞효과를보존하며거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 배치 평가 실제 하위 미연결");return function; }
}
// 생성중게임판단/장치를실행하지않는다.
RawAiDropEvaluation::RawAiDropEvaluation(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,const RawAiDropTargets& targets,AiDropEvaluationGlobals globals,AiDropEvaluationHooks hooks):memory_(memory),pool_(pool),types_(types),targets_(targets),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || &pool!=&targets.Pool()) throw std::invalid_argument("AI 배치 평가 같은 PATCH풀 누락"); }
// 현재본체의정확한크기/수명을확인한다.
std::span<std::uint8_t> RawAiDropEvaluation::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 배치 평가 실제0xb08본체 누락");return raw; }
// 하위뒤현재본체를다시빌린다.
std::uint32_t RawAiDropEvaluation::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 지정 DWORD 이외의 원문을 보존한다.
void RawAiDropEvaluation::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { auto raw=Body(token);if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 평가 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은지정네BYTE만쓴다.
// TYPE0도원본처럼조회하며물리표밖만거부한다.
const RiftTypeRecord& RawAiDropEvaluation::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("AI 배치 평가 현재TYPE 범위");return types_[type]; }
// 각개선비교는축소전값으로하되선택된최고값은원본처럼float저장후재사용한다.
float RawAiDropEvaluation::DryDistance(std::uint32_t token,RawPathPoint point,std::int32_t& index) const {
    float best{};
    // 매기록에서현재포인터/count를다시읽고TYPE/용량/cursor로임의필터하지않는다.
    for (std::int32_t current=0;current<Signed(Field(token,0xae0));++current) { const auto raw=memory_.Bytes(Field(token,0xae4));const auto at=static_cast<std::size_t>(current)*kDryStride;const double distance=std::abs(static_cast<double>(std::bit_cast<float>(Read(raw,at)))-point.x)+std::abs(static_cast<double>(std::bit_cast<float>(Read(raw,at+4)))-point.y);if (distance>0) { const double value=static_cast<double>(kDistanceValue)-distance;if (value>best) { index=current;best=static_cast<float>(value); } } }
    return best;
}
// 공급문자열은C의첫NUL까지세고실제공급이선택한동일selected목록효과를유지한다.
std::int32_t RawAiDropEvaluation::SupplyScore(std::uint32_t token,RawPathPoint point) const {
    std::int32_t unusedIndex{};const auto value=DryDistance(token,point,unusedIndex);if (!(value>static_cast<double>(kDistanceValue)-(static_cast<double>(globals_.manaRadius)+globals_.manaRadius))) return 0;
    std::string text;const auto owner=Field(token,0x260),probe=globals_.probeType;Need(hooks_.collect)(text,point.x,point.y,probe,owner,-1);const auto count=text.find('\0');const auto length=count==std::string::npos ? text.size() : count;if (length>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()-1)) throw std::out_of_range("AI 배치 공급 C문자열 길이 범위");const auto signedLength=static_cast<std::int32_t>(length);if (!signedLength || signedLength>=Signed(Field(token,0xad8))) return 0;return Ftol(value)/(signedLength+1);
}
// 두TYPE조회의flags/group을먼저읽고원본우선순위대로단하나의평가만선택한다.
std::int32_t RawAiDropEvaluation::Evaluate(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,Sid& target) const {
    const auto flags=Type(type).flags2;const auto group=Type(type).group;if (flags&0x800U) return SupplyScore(token,point);if (flags&0x4000000U) return targets_.Blockers(token,type,point,direction,target);if (group==3) return targets_.Ground(token,type,point,target);if (group==0 || group==1) return targets_.Gun(token,type,point,direction,target);if (group!=6) { static_cast<void>(Type(type));if (Field(token,0xb00)) Need(hooks_.unsupported)(token,type); }return 0;
}
// 후보를채택하지않아도대상SID의평가효과는보존하고승인뒤목표Y→X→점수순서로만쓴다.
bool RawAiDropEvaluation::ConsiderScore(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,RawPathPoint& bestPoint,Sid& target,const std::function<std::int32_t()>& read,const std::function<void(std::int32_t)>& write) const {
    const CanonPlacementQuery query{type,0,point.x,point.y,0,Field(token,0x260)&255U,1};static_cast<void>(Type(type));if (!Need(hooks_.mayPlace)(query)) return false;const auto value=Evaluate(token,type,point,direction,target);const auto bestScore=read();if (!(bestScore<value || (value>0 && value==bestScore))) return false;
    std::string text;const auto owner=Field(token,0x260);if (Need(hooks_.energy)(text,point.x,point.y,type,owner)) { bestPoint.y=point.y;bestPoint.x=point.x;write(value);return true; }
    if (Need(hooks_.dry)(token,point.x,point.y,type)) { const auto y=Ftol(point.y),x=Ftol(point.x);if (Field(token,0xb00)) Need(hooks_.dryLog)(token,x,y); }return false;
}
// 명시외부점수참조도평가가끝난그시점에읽는다.
bool RawAiDropEvaluation::Consider(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,RawPathPoint& bestPoint,std::int32_t& bestScore,Sid& target) const { return ConsiderScore(token,type,point,direction,bestPoint,target,[&bestScore] { return bestScore; },[&bestScore](auto value) { bestScore=value; }); }
// 실제스캔의3e8을임시사본/호출뒤일괄동기화로대신하지않는다.
bool RawAiDropEvaluation::ConsiderStored(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,RawPathPoint& bestPoint,Sid& target) const { return ConsiderScore(token,type,point,direction,bestPoint,target,[this,token] { return Signed(Field(token,0x3e8)); },[this,token](auto value) { Field(token,0x3e8,std::bit_cast<std::uint32_t>(value)); }); }
// 동일실제풀연결확인용이다.
const SidPool& RawAiDropEvaluation::Pool() const { return pool_; }
}
