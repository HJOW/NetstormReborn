#include "client/CameraHome.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// resetCamera가 선택하는 사제 genus와 free/dead 공통 상태 비트다.
constexpr std::uint32_t kPriestGenus=0x200000U;
constexpr std::uint32_t kCarrierGenus=0x30000U; //4ac200의 TYPE.flags2에서 일반/벌룬 Carrier를 함께 검사한다.
constexpr std::uint8_t kFree=1,kDead=2;
// 원본 슬롯의 정렬되지 않은 좌표 DWORD를 직접 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{}; // 주변 payload를 읽지 않는 현재 좌표의 비트다.
    // little endian DWORD의 네 바이트만 조합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(8*index);
    return std::bit_cast<float>(value);
}
}
// 실제 현재 상태와 화면 소비자를 첫 풀 접근 전에 확인한다.
CameraHome::CameraHome(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CameraHomeState& state,
    std::function<void(float,float)> center,CameraHomeHooks hooks):pool_(pool),types_(types),state_(state),center_(std::move(center)),hooks_(std::move(hooks)) {
    if (types.empty() || !center_ || state.width<=0 || state.height<=0 || pool.Capacity()>65536 || bool(hooks_.suppressionDepth)!=bool(hooks_.setSuppressionDepth))
        throw std::invalid_argument("초기 카메라의 실제 풀/타입/화면 소비자 범위 오류");
}
// 외부 화면 효과가 로딩 깊이를 바꿨으면 예전 사본이 아니라 현재 실제 값을 읽는다.
std::uint32_t CameraHome::SuppressionDepth() const {
    if (hooks_.suppressionDepth) state_.suppressionDepth=hooks_.suppressionDepth();return state_.suppressionDepth;
}
// 실제 소유자 갱신을 먼저 끝내고 현재 state에도 원문의 깊이를 반영한다.
void CameraHome::SetSuppressionDepth(std::uint32_t depth) const {
    if (hooks_.setSuppressionDepth) hooks_.setSuppressionDepth(depth);state_.suppressionDepth=depth;
}
// 원문433ba0/4340d0처럼 float 저장 뒤의 좌표만 부모에 넘기며 카운터/변환을 여기서 먼저 실행하지 않는다.
void CameraHome::CenterPoint(float x,float y) const { center_(x,y); }
// PATCH433ba0/CD488eb0은 dead를 고른 경우 다른 후보를 다시 찾지 않는다.
void CameraHome::CenterOn(o::Sid sid) const {
    if (!sid.value) return;
    const auto raw=pool_.Slot(sid); // 선택된 같은 슬롯을 중심 이동 직전에 다시 읽는다.
    if (raw[11]&(kFree|kDead)) return;
    if (raw[10]>=types_.size()) throw std::out_of_range("초기 카메라의 현재 TYPE 누락");
    const auto& type=types_[raw[10]]; // 원본 가상 TYPE getter의 현재 자료다.
    const auto x=static_cast<float>(static_cast<double>(Coordinate(raw,14))-type.footX); // 원본은 전체 footX를 빼며 발자국 절반을 쓰지 않는다.
    const auto y=static_cast<float>(static_cast<double>(Coordinate(raw,18))-type.footY); // 원본은 계산 뒤 float 저장을 거친다.
    CenterPoint(x,y);
    const auto currentType=pool_.Slot(sid)[10]; // 화면 적용 뒤 원본4ac200이 읽는 현재 타입을 다시 조회한다.
    if (currentType>=types_.size()) throw std::out_of_range("중심 이동 뒤 현재 TYPE 누락");
    if (types_[currentType].flags2&kCarrierGenus) {
        if (hooks_.contentPreview) hooks_.contentPreview(sid);
        else if (!SuppressionDepth()) throw std::logic_error("카메라 Carrier의 실제 내용물 표시 미연결");
    }
}
// 원본 반복자는2부터 시작하며 free 비트만 보고 다음 현재 번호로 간다.
void CameraHome::Reset() const {
    o::Sid selected{};float sumX{},sumY{},count{}; // 마지막 후보와 원본 단정도 누적 자료다.
    float challengeX{},challengeY{};bool foundChallenge=false; // 정의된 Challenge 좌표를 실제로 읽었는지 구별한다.
    const auto ownerOffset=pool_.Edition()==o::OriginalEdition::Patch1078 ? 34U : 32U; // 두 판본의 실제 소유자 BYTE 위치다.
    // free/dead/void를 합쳐 걸러내지 않고 원본과 같은 SID 순서를 유지한다.
    for (std::uint32_t number=2;number<pool_.Capacity();++number) {
        const o::Sid sid{static_cast<std::uint16_t>(number)};const auto raw=pool_.Slot(sid); // 반복 중의 현재 원본 슬롯이다.
        if (raw[11]&kFree) continue;
        if (raw[10]>=types_.size()) throw std::out_of_range("초기 카메라 순회의 TYPE 범위 오류");
        if ((state_.fort || state_.battle) && raw[ownerOffset]==state_.localPlayer && (types_[raw[10]].flags2&kPriestGenus)) {
            selected=sid;sumX+=Coordinate(raw,14);sumY+=Coordinate(raw,18);count+=1.0f;
        }
        if (state_.challenge && raw[10]==state_.challengeType) {
            selected=sid;
            if (raw[ownerOffset]==state_.localPlayer) { challengeX=Coordinate(raw,14);challengeY=Coordinate(raw,18);foundChallenge=true;break; }
        }
    }
    // 이 원본 분기는 좌표를 쓰지 않은 스택을 읽는다. 정의된 미션 초기화 경로와 구별해 실행을 거부한다.
    if (state_.challenge && selected.value && !foundChallenge) throw std::logic_error("원본 Challenge 초기 카메라의 미초기화 좌표 경로");
    const auto depth=SuppressionDepth(); // 실제 object loadingDepth와 같은 현재 전역을 증가 직전에 읽는다.
    if (depth==std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("카메라 억제 깊이 감김의 외부 표시 생성 경로 미연결");
    SetSuppressionDepth(depth+1);
    try {
        if (state_.challenge) {
            if (!selected.value) {
                challengeX=static_cast<float>(static_cast<double>(state_.worldWidth)*0.5*0.0625f);
                challengeY=static_cast<float>(static_cast<double>(state_.worldHeight)*0.5*0.09090909361839294f);
            }
            CenterPoint(challengeX,challengeY);
        } else if (selected.value) CenterOn(selected);
        else if (count>1.0f) CenterPoint(sumX*(1.0f/count),sumY*(1.0f/count));
        else CenterPoint(128.0f,128.0f);
    } catch (...) { SetSuppressionDepth(SuppressionDepth()-1U);throw; }
    SetSuppressionDepth(SuppressionDepth()-1U);
}
}
