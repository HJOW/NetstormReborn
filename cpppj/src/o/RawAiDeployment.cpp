// 생산 조각 교체와 실제 건설을 원본 읽기·쓰기·호출 순서대로 실행한다.
#include "o/RawAiDeployment.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kDemoFloor=1190; //원본 signed 지갑의0x4a6 하한이다.
constexpr double kDeploymentDelay=20.0; //보호 PE005012d8의 QWORD 시설 재판단 기한이다.
// 비정렬 원문 DWORD를 물리 범위 안에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 배치 DWORD 읽기 범위");std::uint32_t value{};
    // 네 BYTE의 원본 little endian 표현을 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
// 지정 필드만 쓰고 나머지 본체는 유지한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 배치 DWORD 쓰기 범위");
    // 네 BYTE의 낮은 자리부터 기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 원본ftol은 signed64 절삭의 하위DWORD를 사용하며 무효 FISTP의 하위DWORD는0이다.
std::uint32_t Truncate(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value))); }
// 선택된 하위의 누락을 빈 성공으로 처리하지 않는다.
template<class Function> const Function& Need(const Function& function,const char* message) { if (!function) throw std::logic_error(message);return function; }
}
// 실제 게임 계산을 시작하지 않고 공유 저장소의 물리 계약만 확인한다.
RawAiDeployment::RawAiDeployment(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,const RawContainedCreate& create,AiDeploymentGlobals globals,AiDeploymentHooks hooks)
    :memory_(memory),pool_(pool),types_(types),create_(create),globals_(globals),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || &pool!=&create.Pool() || types.empty()) throw std::invalid_argument("AI 배치 같은 PATCH Pool/TYPE/내용물 생성 계약 누락");
}
// 해제 토큰이나 작은 본체를 거부한다.
std::span<std::uint8_t> RawAiDeployment::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 배치 실제0xb08 본체 누락");return raw; }
// 하위 뒤 같은 본체의 현재 필드를 다시 읽는다.
std::uint32_t RawAiDeployment::Field(std::uint32_t token,std::size_t offset) const { return Read(Body(token),offset); }
// 선택된 필드만 현재 본체에 기록한다.
void RawAiDeployment::Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const { Put(Body(token),offset,value); }
// 원본 TYPE getter의 물리 범위만 검사한다.
const RiftTypeRecord& RawAiDeployment::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("AI 배치 현재 TYPE 범위");return types_[type]; }
// 실제 getter 반환을 WORD로 미리 잘라 잘못된 부모로 바꾸지 않는다.
Sid RawAiDeployment::Technology(std::uint32_t token) const { const auto parent=Need(hooks_.technology,"AI 배치 현재 기술부모 조회 미연결")(Field(token,0x260));if (parent.value>=pool_.Capacity()) throw std::out_of_range("AI 배치 현재 기술부모 SID 범위");return parent; }
// 비용은 반올림이 아닌 원본 signed 절삭 뒤 현재 지갑과 비교한다.
bool RawAiDeployment::CanAfford(std::uint32_t token,std::uint32_t type) const { return std::bit_cast<std::int32_t>(Truncate(Type(type).cost))<=std::bit_cast<std::int32_t>(Field(token,0x2d8)); }
// 원본은 비용을 먼저 음수로 만든 뒤 절삭하며 SP 자체는 이 위치에서 쓰지 않는다.
void RawAiDeployment::Charge(std::uint32_t token,std::uint32_t type) const {
    const auto delta=Truncate(-static_cast<double>(Type(type).cost));Field(token,0x2d8,Field(token,0x2d8)+delta);
    if (Need(hooks_.demo,"AI 배치 비용의 현재 데모 조회 미연결")()) { const auto wallet=Field(token,0x2d8);Field(token,0x2d8,std::bit_cast<std::int32_t>(wallet)>static_cast<std::int32_t>(kDemoFloor) ? wallet : kDemoFloor); }
}
// 시설 종류별 기한은 실제 배치 하위가 반환한 뒤의 현재 시각을 사용한다.
void RawAiDeployment::Deadline(std::uint32_t token,std::size_t offset) const { const auto bits=std::bit_cast<std::uint64_t>(globals_.now+kDeploymentDelay);Field(token,offset,static_cast<std::uint32_t>(bits));Field(token,offset+4,static_cast<std::uint32_t>(bits>>32)); }
// 성공을 가정한 임의 유닛 생성 대신 기존 생산/내용물/건설 몸체를 실행한다.
void RawAiDeployment::Place(std::uint32_t token,std::uint32_t type,float x,float y,std::uint32_t argument,std::uint32_t direction) const {
    const auto product=Need(hooks_.product,"AI 배치 현재 생산 조회 미연결")(token,type);bool charge{};
    if (product.value) {
        Need(hooks_.destroy,"AI 배치 실제 생산 조각 삭제 미연결")(product,0);const auto replacement=create_.Create(Technology(token),type,0,0x108,0x10,0);
        if (Need(hooks_.reserveRegular,"AI 배치 Regular 예약 미연결")()) Need(hooks_.addRegular,"AI 배치 Regular 실제 부착 미연결")(replacement,std::bit_cast<float>(Field(token,0x224)));charge=true;
    } else {
        if (Type(type).flags2&0x70000U) Need(hooks_.assertion,"AI 배치 생산 조각 진단 미연결")("!( ::getType(type)->genus & (gEMPLACEMENT|gCARRIER) )");charge=(Type(type).flags2&0x4200U)!=0;
    }
    if (charge) Charge(token,type);const auto authority=globals_.authority;globals_.moveLocalBuilder=true;
    if (authority) { const ConstructionConfirmRequest request{type,x,y,Field(token,0x260),argument,direction,0,0,0,0};Need(hooks_.request,"AI 배치 실제 서버 건설 요청 미연결")(request); }
    else { const auto preview=create_.Create(Technology(token),type,0,0x108,0x10,0);Need(hooks_.localPlace,"AI 배치 실제 로컬 예측 미연결")({preview,x,y,argument,direction}); }
    globals_.moveLocalBuilder=false;if (Type(type).flags2&0x800U) Deadline(token,0x5b8);if (Type(type).flags2&0x4200U) Deadline(token,0x5c0);
}
// 실제 게임 조립 확인용 동일 풀이다.
const SidPool& RawAiDeployment::Pool() const { return pool_; }
}
