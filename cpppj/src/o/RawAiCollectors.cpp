// 수집자 확보의 원본 단락 평가·난수 소비·목록/시각 재조회 순서를 보존한다.
#include "o/RawAiCollectors.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::int32_t kAttempts=20; //00415920의 후보 배치 최대 반복 횟수다.
// 비정렬 원본 DWORD를 실제 물리 범위에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 수집자 DWORD 읽기 범위");std::uint32_t value{};
    // 네 BYTE의 낮은 자리부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
// 선택된 DWORD만 바꾸고 주변 본체를 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 수집자 DWORD 쓰기 범위");
    // 원본 little endian 네 BYTE만 쓴다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 원본 signed 분기의 비트 해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// CRTftol의 signed64 절삭 후 하위DWORD다. 무효 FISTP의 하위DWORD는0이다.
std::uint32_t Truncate(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value))); }
// 선택된 하위의 누락을 명시 오류로 남긴다.
template<class Function> const Function& Need(const Function& function,const char* message) { if (!function) throw std::logic_error(message);return function; }
}
// 게임 판단 전에 같은 저장소의 물리 계약만 확인한다.
RawAiCollectors::RawAiCollectors(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,AiCollectorsGlobals globals,AiCollectorsHooks hooks)
    :memory_(memory),pool_(pool),types_(types),random_(random),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.empty()) throw std::invalid_argument("AI 수집자 같은 PATCH/TYPE 계약 누락"); }
// 해제되거나 다른 크기의 가짜 본체를 거부한다.
std::span<std::uint8_t> RawAiCollectors::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 수집자 실제0xb08 본체 누락");return raw; }
// 외부 효과 뒤에도 현재 본체를 다시 읽는다.
std::uint32_t RawAiCollectors::Field(std::uint32_t token,std::size_t offset) const { return Read(Body(token),offset); }
// 현재 본체의 해당 DWORD만 쓴다.
void RawAiCollectors::Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const { Put(Body(token),offset,value); }
// 목록의 개수와 별개로 현재 backing 안의 실제 칸만 읽는다.
std::uint32_t RawAiCollectors::Item(std::uint32_t token,std::size_t pointer,std::int32_t index) const { if (index<0) throw std::out_of_range("AI 수집자 음수 목록 인덱스");return Read(memory_.Bytes(Field(token,pointer)),static_cast<std::size_t>(index)*4); }
// 현재 TYPE0도 허용하되 표 밖 접근은 거부한다.
const RiftTypeRecord& RawAiCollectors::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("AI 수집자 현재 TYPE 범위");return types_[type]; }
// 원본 적 유효성은 별도로0을 제외하며 이 물리 getter는0도 보존한다.
Sid RawAiCollectors::PhysicalSid(std::uint32_t value) const { if (value>=pool_.Capacity() || value>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("AI 수집자 현재 SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// 원본 QWORD 비교의 unordered/기한 전은 모두 거짓이다.
bool RawAiCollectors::Ready(std::uint32_t token,std::size_t offset) const { const auto due=std::bit_cast<double>((std::uint64_t(Field(token,offset+4))<<32)|Field(token,offset));return globals_.now>=due; }
// 같은 원본 생산 조회를 다시 호출한 뒤 현재 조각의 pending bit100만 제외한다.
Sid RawAiCollectors::AvailableProduct(std::uint32_t token,std::uint32_t type) const { const auto form=Need(hooks_.product,"AI 수집자 현재 생산 조회 미연결")(token,type);return form.value && (pool_.Slot(form)[21]&1) ? Sid{} : form; }
// free/dead/추가 소유자 필터 없이 현재 수집자 목록의 TYPE genus만 검사한다.
bool RawAiCollectors::HasFlying(std::uint32_t token) const {
    // 하위가 현재 목록을 바꾸면 다음 원문 읽기에서 반영한다.
    for (std::int32_t index=0;index<Signed(Field(token,0x2e4));++index) if (Type(pool_.Slot(PhysicalSid(Item(token,0x2dc,index)))[10]).flags2&0x20000U) return true;return false;
}
// 최대HP getter의 효과가 HP를 바꾸더라도 먼저 읽은 현재HP로 원본 비교를 한다.
bool RawAiCollectors::Vulnerable(Sid priest) const { const auto hp=Signed(Read(pool_.Slot(priest),26));const auto maximum=Need(hooks_.maxHitPoints,"AI 수집자 현재 최대HP 조회 미연결")(priest);return hp<maximum/2; }
// 원본 로그 좌표는 y→x 절삭이며 생각 기한은 float rate/QWORD now를 사용한다.
void RawAiCollectors::Notice(std::uint32_t token,std::uint32_t type,float x,float y) const {
    const auto iy=Signed(Truncate(y)),ix=Signed(Truncate(x));static_cast<void>(Type(type));const auto rate=static_cast<double>(std::bit_cast<float>(Field(token,0x228)));
    const auto first=std::bit_cast<std::uint64_t>(rate+globals_.now);Field(token,0x5a8,static_cast<std::uint32_t>(first));Field(token,0x5ac,static_cast<std::uint32_t>(first>>32));const auto second=std::bit_cast<std::uint64_t>((rate+1.0)+globals_.now);Field(token,0x5b0,static_cast<std::uint32_t>(second));Field(token,0x5b4,static_cast<std::uint32_t>(second>>32));
    if (Field(token,0xb00)) Need(hooks_.log,"AI 수집자 현재 Lay 로그 미연결")(token,type,ix,iy);
}
// 일반 배치와 생각 기한/로그·현재 로컬 소유자의 드롭 음향을 같은 수명에서 이어 실행한다.
void RawAiCollectors::DeployWithNotice(std::uint32_t token,std::uint32_t type,float x,float y,std::uint32_t argument,std::uint32_t direction) const {
    Need(hooks_.deploy,"AI 수집자 실제 생산/건설 배치 미연결")(token,type,x,y,argument,direction);Notice(token,type,x,y);if (Field(token,0x260)==globals_.localOwner) Need(hooks_.dropSound,"AI 수집자 로컬 드롭 음향 미연결")();
}
// 매 시도에 경계를 snapshot하고 y→x 순서로 두 원본 난수를 소비한다.
void RawAiCollectors::PlaceInHome(std::uint32_t token,std::uint32_t type) const {
    // 전체 MayPlace가 허용한 첫 후보만 배치하고 최대20회 뒤에는 아무 조각도 만들지 않는다.
    for (std::int32_t remaining=kAttempts;remaining!=0;--remaining) {
        const auto left=std::bit_cast<float>(Field(token,0x270)),top=std::bit_cast<float>(Field(token,0x274)),right=std::bit_cast<float>(Field(token,0x278)),bottom=std::bit_cast<float>(Field(token,0x27c));
        const auto y=static_cast<float>(static_cast<double>(Signed(random_.Next(Truncate(static_cast<double>(bottom)-top))))+top);const auto x=static_cast<float>(static_cast<double>(Signed(random_.Next(Truncate(static_cast<double>(right)-left))))+left);const CanonPlacementQuery query{type,0,x,y,0,Field(token,0x260)&255U,1};static_cast<void>(Type(type));
        if (Need(hooks_.mayPlace,"AI 수집자 전체 MayPlace 미연결")(query)) { DeployWithNotice(token,type,x,y,0,0);return; }
    }
}
// 원본 취약 사제용 비행 수집자 경로는 일반 비용/대기 조각 판정을 추가로 적용하지 않는다.
void RawAiCollectors::Ensure(std::uint32_t token) const {
    if (!Field(token,0x294) || Field(token,0x298)==Need(globals_.invalidGraph,"AI 수집자 현재 무효 그래프 조회 미연결")() || !Field(token,0x28c)) return;
    const auto enemySid=Field(token,0x2a0);
    if (Ready(token,0x5a8) && random_.Next(1000)<100 && (Field(token,0x248)&0x100U) && Signed(Field(token,0x38c))<Signed(Field(token,0x388)) && enemySid>=1 && enemySid<pool_.Capacity() &&
        Need(hooks_.enemyAvailable,"AI 수집자 현재 적 사제 참여 조회 미연결")(Field(token,0x264)) && Vulnerable(PhysicalSid(Field(token,0x2a0))) && Ready(token,0x5e0) && !HasFlying(token)) {
        // 현재 후보 목록을 원래 순서로 방문해 첫 생산 보유 비행 타입에서 끝낸다.
        for (std::int32_t index=0;index<Signed(Field(token,0x28c));++index) { const auto type=Item(token,0x284,index);if (Need(hooks_.product,"AI 수집자 현재 생산 조회 미연결")(token,type).value && (Type(type).flags2&0x20000U)) { PlaceInHome(token,type);return; } }
    }
    if (Field(token,0x2cc)) return;const auto index=Signed(random_.Next(Field(token,0x28c)));auto type=Item(token,0x284,index);if (!Need(hooks_.product,"AI 수집자 현재 생산 조회 미연결")(token,type).value) type=Item(token,0x284,0);
    if (Need(hooks_.product,"AI 수집자 현재 생산 재조회 미연결")(token,type).value && Ready(token,0x5a8) && AvailableProduct(token,type).value && Need(hooks_.canAfford,"AI 수집자 실제 비용 비교 미연결")(token,type)) PlaceInHome(token,type);
}
// 실제 같은 월드 연결 확인용 풀이다.
const SidPool& RawAiCollectors::Pool() const { return pool_; }
}
