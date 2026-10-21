// 원본 배치 행동의 현재 필드 조회/조건 단락·부분 효과와 성공 판정 위치를 보존한다.
#include "o/RawAiDropActions.h"
#include "o/RawAiPriestActions.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::int32_t kWalletMinimum=1200,kChoices=30; //원본 signed 지갑 하한과 물리 후보 배열 크기다.
// 비정렬 little endian DWORD를 실제 블록 안에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 행동 DWORD 읽기 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은 지정 네 BYTE만 합친다.
// 원본 DWORD의 signed 비교/방향을 보존한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// CRT signed64 절삭의 하위 DWORD이며 무효 FISTP의 하위값은0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
// 도달한 실제 하위가 없으면 앞선 상태 쓰기를 유지하고 그 위치에서 거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 배치 행동 실제 하위 미연결");return function; }
}
// 생성 중 선택/난수/게임 장치를 실행하지 않는다.
RawAiDropActions::RawAiDropActions(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,AiDropActionGlobals globals,AiDropActionHooks hooks):memory_(memory),pool_(pool),types_(types),random_(random),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("AI 배치 행동 같은 PATCH188 TYPE 누락"); }
// 해제 토큰/작은 가짜 본체를 성공으로 취급하지 않는다.
std::span<std::uint8_t> RawAiDropActions::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 배치 행동 실제0xb08 본체 누락");return raw; }
// 하위 뒤 현재 본체를 다시 빌린다.
std::uint32_t RawAiDropActions::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 주변 필드를 보존하며 지정한 네 BYTE만 기록한다.
void RawAiDropActions::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { auto raw=Body(token);if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 행동 DWORD 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은 지정 폭만 쓴다.
// TYPE0도 원래 getter처럼 조회하고 상위 비트가 있는 번호를 BYTE로 자르지 않는다.
const RiftTypeRecord& RawAiDropActions::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("AI 배치 행동 현재 TYPE 범위");return types_[type]; }
// 두 기한은 각 비교 위치의 현재 시각이며 unordered도 거부한다.
bool RawAiDropActions::Ready(std::uint32_t token,std::size_t at) const { const auto due=std::bit_cast<double>((static_cast<std::uint64_t>(Field(token,at+4))<<32)|Field(token,at));return globals_.now>=due; }
// 생산 조각은 free/dead/새 소유자 필터 없이 원래 pending bit100만 제외한다.
Sid RawAiDropActions::AvailableProduct(std::uint32_t token,std::uint32_t type) const { const auto product=Need(hooks_.product)(token,type);return product.value && (pool_.Slot(product)[21]&1U) ? Sid{} : product; }
// 회전을 배치쿼리 flags로 넣지 않으며 owner의 하위 BYTE를 사용한다.
bool RawAiDropActions::MayPlace(std::uint32_t token,std::uint32_t type,RawPathPoint point) const { const CanonPlacementQuery query{type,0,point.x,point.y,0,Field(token,0x260)&255U,1};static_cast<void>(Type(type));return Need(hooks_.mayPlace)(query); }
// 손상 count/index를 다른 유효 후보로 clamp하지 않는다.
std::size_t RawAiDropActions::PointOffset(std::int32_t index) const { if (index<0 || index>=kChoices) throw std::out_of_range("AI 배치 행동30후보 물리 범위");return 0x408+static_cast<std::size_t>(index)*8; }
// 현재 후보를 하위/로그 뒤 다시 조회한다.
RawPathPoint RawAiDropActions::Point(std::uint32_t token,std::int32_t index) const { const auto at=PointOffset(index);return {std::bit_cast<float>(Field(token,at)),std::bit_cast<float>(Field(token,at+4))}; }
// 같은 후보의 현재 전체 표적 번호다.
std::uint32_t RawAiDropActions::Target(std::uint32_t token,std::int32_t index) const { static_cast<void>(PointOffset(index));return Field(token,0x4f8+static_cast<std::size_t>(index)*4); }
// 원본0슬롯 조회는 허용하며 물리 풀 밖을 잘라 다른 객체로 바꾸지 않는다.
Sid RawAiDropActions::Object(std::uint32_t number) const { if (number>65535 || number>=pool_.Capacity()) throw std::out_of_range("AI 배치 행동 현재 표적 SID 물리 범위");return Sid{static_cast<std::uint16_t>(number)}; }
// 출력이 꺼져도 원래 TYPE getter의 범위 검사는 앞서 수행한다.
void RawAiDropActions::Log(std::uint32_t token,AiDropActionNotice notice) const { if (notice.action!=AiDropActionLog::PartialScan) static_cast<void>(Type(notice.type));if (Field(token,0xb00)) Need(hooks_.log)(token,notice); }
// 후보 좌표는 MayPlace 인자의 사본이 아니라 그 이후 현재 값이다.
void RawAiDropActions::PointLog(std::uint32_t token,AiDropActionLog action,std::int32_t index) const { const auto at=PointOffset(index);const auto y=Ftol(std::bit_cast<float>(Field(token,at+4))),x=Ftol(std::bit_cast<float>(Field(token,at)));Log(token,{action,Field(token,0x3e0),0,x,y}); }
// 모든 관문/출력·선택 효과 뒤 현재 필드를 원본 위치에서 재조회한다.
void RawAiDropActions::Run(std::uint32_t token) const {
    auto type=Field(token,0x3e0);if (type && !Need(hooks_.product)(token,type).value) Log(token,{AiDropActionLog::LostAbility,Field(token,0x3e0)});
    type=Field(token,0x3e0);if (!type || Field(token,0x3f0)) { Need(hooks_.select)(token);type=Field(token,0x3e0);if (!type) return; }
    if (!Field(token,0x3ec) && !Need(hooks_.scan)(token,type,Signed(Field(token,0x3e4)))) { const auto direction=Signed(Field(token,0x3e4));type=Field(token,0x3e0);Field(token,0x3ec,1);Log(token,{AiDropActionLog::ScanComplete,type,direction}); }
    if (!Ready(token,0x5a8) || !Ready(token,0x5e8) || Signed(Field(token,0x2d8))<kWalletMinimum || (Field(token,0x400) && !Field(token,0x3ec))) return;
    if (!AvailableProduct(token,Field(token,0x3e0)).value || Signed(Field(token,0x5a4))<Signed(Field(token,0x5a0))) return;
    if (AiPriestPointValid({std::bit_cast<float>(Field(token,0xad0)),std::bit_cast<float>(Field(token,0xad4))},globals_.mapLimit)) return;
    if (!Field(token,0x3ec)) Log(token,{AiDropActionLog::PartialScan});
    if (Signed(Field(token,0x570))<1) {
        if (globals_.checking && Field(token,0x3e8)) Need(hooks_.assertion)("drop.value == 0",0xf8b);
        if (Type(Field(token,0x3e0)).flags2&0x800U) { Need(hooks_.fake)(token);Field(token,0x3f0,1);return; }
        Need(hooks_.seed)(token);
    } else {
        if (!Need(hooks_.canAfford)(token,Field(token,0x3e0))) { Log(token,{AiDropActionLog::CannotAfford,Field(token,0x3e0)});Field(token,0x3f0,1);return; }
        Field(token,0x3f4,0);Field(token,0x3fc,0);
        // 실패 후보도 실제 압축하고 압축 뒤 현재 성공 플래그로만 중단한다.
        while (Signed(Field(token,0x570))>0) {
            const auto index=Signed(random_.Next(Field(token,0x570)));const auto selectedType=Field(token,0x3e0);const auto point=Point(token,index);
            if (!MayPlace(token,selectedType,point)) PointLog(token,AiDropActionLog::BadChoice,index);
            else {
                PointLog(token,AiDropActionLog::Laying,index);const auto target=pool_.Slot(Object(Target(token,index)));const auto targetType=target[10];const auto y=Ftol(std::bit_cast<float>(Read(target,18))),x=Ftol(std::bit_cast<float>(Read(target,14)));Log(token,{AiDropActionLog::Target,targetType,0,x,y});
                const auto direction=Field(token,0x3e4);const auto currentPoint=Point(token,index);const auto currentType=Field(token,0x3e0);Need(hooks_.deploy)(token,currentType,currentPoint,0,direction);
                const bool focused=(Field(token,0x248)&0x40U)!=0;Field(token,0x3f4,1);Field(token,0x3fc,Target(token,index));if (focused) Field(token,0x3f8,Target(token,index));
            }
            Need(hooks_.remove)(token,index);if (Field(token,0x3f4)) { Field(token,0x3f0,1);return; }
        }
    }
    Field(token,0x3f0,1);
}
// 같은 실제 월드 연결을 확인한다.
const SidPool& RawAiDropActions::Pool() const { return pool_; }
}
