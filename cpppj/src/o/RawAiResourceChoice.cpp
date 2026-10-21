// 원본 AI의 거리 절삭과 현재 그래프/목록 재조회·중복/용량/로그 순서를 보존한다.
#include "o/RawAiResourceChoice.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kInitialDistance=65536.0F; //00410a42의 최소 거리 제곱 초기값이다.
// 원본 비정렬 little endian DWORD를 물리 범위 안에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("AI 자원 선택 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(bytes[at+index])<<(index*8);return value; } //반복문은 네 BYTE만 합친다.
// 현재 지정 DWORD만 바꾸고 주변 본체·여유 원문을 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("AI 자원 선택 쓰기 범위");for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은 지정 네 BYTE만 쓴다.
// 원본 DWORD의 signed 비교를 구현 의존 변환 없이 적용한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본 CRTftol의 signed64 절삭 하위DWORD다.무효 FISTP의 하위DWORD도0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
// 선택된 실제 하위 누락을 빈 목록/가짜 이동 성공으로 대신하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 자원 선택 실제 하위 미연결");return function; }
}
// 생성은 현재 원문을 읽거나 어떤 게임 효과도 발생시키지 않는다.
RawAiResourceChoice::RawAiResourceChoice(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,std::function<std::uint32_t()> invalidGraph,AiResourceChoiceHooks hooks)
    :memory_(memory),pool_(pool),types_(types),invalidGraph_(std::move(invalidGraph)),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("AI 자원 선택 같은 PATCH188 누락"); }
// 해제 핸들이나 작은 가짜 본체를 거부한다.
std::span<std::uint8_t> RawAiResourceChoice::Body(std::uint32_t token) const { auto bytes=memory_.Bytes(token);if (bytes.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 자원 선택 실제0xb08 본체 누락");return bytes; }
// 실제 현재 본체의 DWORD를 다시 읽는다.
std::uint32_t RawAiResourceChoice::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 비양수 count는 원본의 자연 루프 종료를 유지한다.
std::int32_t RawAiResourceChoice::Count(std::uint32_t token,std::size_t list) const { return Signed(Field(token,list+8)); }
// 손상된 음수 인덱스의 호스트 주소 접근만 명시 거부한다.
std::uint32_t RawAiResourceChoice::Item(std::uint32_t token,std::size_t list,std::int32_t index) const { if (index<0) throw std::out_of_range("AI 자원 선택 음수 인덱스");return Read(memory_.Bytes(Field(token,list)),static_cast<std::size_t>(index)*4); }
// 용량이 없으면 실제 목록 포인터에 접근하지 않는다.
void RawAiResourceChoice::Append(std::uint32_t token,std::size_t list,std::uint32_t value) const { const auto count=Count(token,list);if (count>=Signed(Field(token,list+4))) return;if (count<0) throw std::out_of_range("AI 자원 선택 음수 추가 인덱스");Put(memory_.Bytes(Field(token,list)),static_cast<std::size_t>(count)*4,value);Put(Body(token),list+8,static_cast<std::uint32_t>(count)+1U); }
// 제거한 중복 수만 count에서 빼며 남은 원소의 상대 순서를 유지한다.
void RawAiResourceChoice::Remove(std::uint32_t token,std::size_t list,std::uint32_t value) const { std::int32_t removed{};for (std::int32_t index=0;index<Count(token,list);++index) { const auto current=Item(token,list,index);if (current==value) ++removed;else Put(memory_.Bytes(Field(token,list)),static_cast<std::size_t>(index-removed)*4,current); }Put(Body(token),list+8,static_cast<std::uint32_t>(Count(token,list))-static_cast<std::uint32_t>(removed)); } //반복문은 현재 count/포인터로 전체 active 목록을 안정 압축한다.
// 잘못된 DWORD를 다른 정상 SID로 잘라서 사용하지 않는다.
Sid RawAiResourceChoice::Object(std::uint32_t value) const { if (value>=pool_.Capacity() || value>65535) throw std::out_of_range("AI 자원 선택 실제SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// free/dead/소유자 필터를 추가하지 않는다.
std::uint32_t RawAiResourceChoice::Genus(std::uint32_t value) const { const auto type=pool_.Slot(Object(value))[10];if (type>=types_.size()) throw std::out_of_range("AI 자원 선택 TYPE 범위");return types_[type].flags2; }
// 신전이 없으면 center 하위를 요구하지 않고 두 raw float를 그대로 반환한다.
RawPathPoint RawAiResourceChoice::HomeCenter(std::uint32_t token) const { const auto temple=Field(token,0x294);if (temple) return Need(hooks_.center)(Object(temple));return {std::bit_cast<float>(Field(token,0x268)),std::bit_cast<float>(Field(token,0x26c))}; }
// 거리값이 같으면 먼저 방문한 대상을 유지한다.그래프는 거리 탈락 후보에서도 조회한다.
Sid RawAiResourceChoice::Select(std::uint32_t token,std::size_t list,RawPathPoint point,std::uint32_t excludedGraph,std::uint32_t requiredGraph) const {
    float best=kInitialDistance;std::uint32_t selected{};
    // 그래프 getter의 효과 뒤 다음 원소/현재 count와 포인터를 다시 읽는다.
    for (std::int32_t index=0;index<Count(token,list);++index) { const auto value=Item(token,list,index);const auto raw=pool_.Slot(Object(value));const double x=static_cast<double>(point.x)-std::bit_cast<float>(Read(raw,14)),y=static_cast<double>(point.y)-std::bit_cast<float>(Read(raw,18));const auto distance=Ftol(x*x+y*y);const auto graph=Need(hooks_.graph)(Object(value));if (!(static_cast<double>(distance)<best)) continue;const auto invalid=Need(invalidGraph_)();if ((excludedGraph==invalid || graph!=excludedGraph) && (requiredGraph==invalid || requiredGraph==graph)) { best=static_cast<float>(distance);selected=value; } }
    return Object(selected);
}
// 첫 관문 뒤에는 초기 신전/그래프 상태를 다시 검사하지 않고 원래 순서의 실제 하위를 호출한다.
void RawAiResourceChoice::CollectOrHome(std::uint32_t token,Sid actor) const {
    if (!Field(token,0x294) || Field(token,0x298)==Need(invalidGraph_)()) return;
    if (!Need(hooks_.crystals)(actor) && (Field(token,0x248)&0x10U)) { const auto center=HomeCenter(token);const auto graph=Need(hooks_.graph)(actor);const auto excluded=Need(invalidGraph_)();const auto target=Select(token,0x330,center,excluded,graph);if (target.value) { Need(hooks_.target)(token,actor,target);return; } }
    Need(hooks_.home)(token,actor);
}
// 물리 슬롯0이나 free 항목도 원본처럼 현재 TYPE로만 분류한다.
bool RawAiResourceChoice::HasFlyingCaptureCarrier(std::uint32_t token) const { for (std::int32_t index=0;index<Count(token,0x384);++index) if (Genus(Item(token,0x384,index))&0x20000U) return true;return false; } //반복문은 현재 포획 대기 목록만 방문한다.
// 중복은 로그도 생략하고,full은 로그까지 진행한다.기한/사제 HP를 갱신하지 않는다.
void RawAiResourceChoice::RememberCaptureCarrier(std::uint32_t token,Sid actor) const { const auto count=Count(token,0x384);if (count>0) { const auto list=memory_.Bytes(Field(token,0x384));for (std::int32_t index=0;index<count;++index) if (Read(list,static_cast<std::size_t>(index)*4)==actor.value) return; }Append(token,0x384,actor.value);if (Field(token,0xb00)) Need(hooks_.captureLog)(token,actor); } //반복문은 최초 count/포인터의 중복을 확인한다.
// 로그 뒤 현재 원문을 재조회하며 연속된 제거 항목을 건너뛰지 않는다.
void RawAiResourceChoice::ReclassifyDisconnected(std::uint32_t token) const {
    std::int32_t index{};
    // 다른 그래프 항목은 같은 인덱스에서 중복 모두 제거한 뒤 다음 현재 원소를 검사한다.
    while (index<Count(token,0x330)) { const auto value=Item(token,0x330,index);if (!value || value>=pool_.Capacity() || !(Genus(value)&0x10000000U)) Need(hooks_.assertion)("s.isValid() && (s.getGenus()&gGEYSER)");const auto graph=value ? Need(hooks_.graph)(Object(value)) : Need(invalidGraph_)();if (graph==Field(token,0x298)) ++index;else { if (Field(token,0xb00)) Need(hooks_.disconnectedLog)(token,Object(value));Remove(token,0x330,value);Append(token,0x348,value); } }
}
// 같은 실제 월드의 풀인지 연결 단계에서 확인한다.
const SidPool& RawAiResourceChoice::Pool() const { return pool_; }
}
