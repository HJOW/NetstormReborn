// 삭제/해체 보상의 판본별 x87 연산 순서를 복원한다. 파생 vtable과 AI 객체 안쪽은 훅/지갑 배열로 둔다.
#include "o/SquidReward.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 raw 슬롯의 타입/상태 바이트와 HP 필드의 위치다(HP는 패치 DWORD, CD WORD).
constexpr std::size_t kType=10,kState=11,kHp=26;
// 지갑을 올리는 하한과 SP를 고정할 때의 판본별 값이다(패치 0x500ff8, CD 0x47c35000).
constexpr std::int32_t kWalletFloor=0x4a6;
constexpr double kPatchFixedSp=194162.0,kCdFixedSp=100000.0;
// 인코딩된 비용 표가 쓰는 번호 계수와 해체 환불 배율(패치 0x501550, CD 0x500648)이다.
constexpr std::int64_t kCostBias=23;
constexpr double kSalvageScale=0.25;
// 타입 flags2의 dais 부류와 flags1의 HP 보유 비트다.
constexpr std::uint32_t kDaisKind=0x400000,kHasHp=0x10;
// 비용 곡선이 정확히 더해지는 한계와 비용 곡선 항 수의 상한이다.
constexpr double kExactLimit=9007199254740992.0;
constexpr std::int64_t kMaxTerms=0x01000000;
// 원본 __ftol: 0방향 절삭 뒤 하위 32비트만 쓴다. 비유한/64비트 범위 밖은 원본에서 부정 정수이므로 거부한다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9.2e18) throw std::out_of_range("보상 정수 변환 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 32비트 감김 덧셈/뺄셈이다.
std::int32_t Wrap(std::int64_t value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)); }
// 정렬되지 않은 little endian 필드를 호스트 엔디언과 무관하게 읽는다.
std::uint32_t Dword(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 낮은 바이트부터 모은다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
}
SquidReward::SquidReward(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& bookkeeping,
    SquidRewardState& state,ScrambledSpStore* store,SquidRewardHooks hooks)
    :pool_(pool),types_(types.begin(),types.end()),bookkeeping_(bookkeeping),state_(state),store_(store),hooks_(std::move(hooks)) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    if (types.size()!=(patch ? 188U : 171U)) throw std::invalid_argument("보상 타입 판본/크기 오류");
    // 패치판의 SP는 저장소에만 있다. CD판은 전역 float이므로 저장소를 연결하지 않는다.
    if (patch!=(store!=nullptr)) throw std::invalid_argument(patch ? "패치판 보상은 SP 저장소가 필요합니다" : "CD판 보상은 SP 저장소를 쓰지 않습니다");
}
std::span<const std::uint8_t> SquidReward::Raw(Sid sid) const {
    if (sid.value==0 || sid.value>=pool_.Capacity()) throw std::out_of_range("보상 SID 범위 오류");
    const auto raw=pool_.Slot(sid);
    if (raw[kType]>=types_.size()) throw std::out_of_range("보상 타입 번호 범위 오류");
    return raw;
}
// 로더의 ftol(cost + 번호*23)을 그대로 따라 한 뒤 읽는 쪽의 감산까지 합친 값이다. CD판은 float 비용을 그대로 쓰므로 호출하지 않는다.
std::int32_t SquidReward::TypeCostUnits(std::uint32_t type) const {
    const double encoded=static_cast<double>(types_.at(type).cost)+static_cast<double>(type*kCostBias);
    if (encoded<-2147483648.0 || encoded>=2147483648.0) throw std::out_of_range("보상 패치 비용 표 범위 오류");
    return Wrap(static_cast<std::int64_t>(encoded)-static_cast<std::int64_t>(type)*kCostBias);
}
// base vtable +0x80은 004ad0a0이 항상 참이므로 상태 바이트의 상위 3비트다.
const SidPool& SquidReward::Pool() const { return pool_; }
const SquidPostPopState& SquidReward::Bookkeeping() const { return bookkeeping_; }
std::uint32_t SquidReward::VirtualCountCalls() const { return virtualCalls_; }
std::int32_t SquidReward::Count(Sid sid) const {
    ++virtualCalls_;
    if (hooks_.virtualCount) return hooks_.virtualCount(sid);
    return Raw(sid)[kState]>>5;
}
std::int32_t SquidReward::TypeHitPoints(std::uint32_t type) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto& record=types_.at(type);
    // 패치판 디버그 assert는 HP 비트가 없는 타입(dais 제외)의 조회를 보고한다. 보고 UI는 복원하지 않으므로 거부한다.
    if (patch && state_.debugAsserts && !(record.flags1&kHasHp) && type!=state_.types.dais)
        throw std::logic_error("보상 타입 HP assert 위반");
    return state_.priestQuarterHp && type==state_.types.priest ? record.maxHitPoints/4 : record.maxHitPoints;
}
bool SquidReward::HasHitPoints(Sid sid) const {
    const auto raw=Raw(sid);const auto& type=types_[raw[kType]];
    const std::size_t extra=pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35;
    return (type.flags2&kDaisKind) ? (raw[extra]&1)!=0 : (type.flags1&kHasHp)!=0;
}
std::int32_t SquidReward::MaxHitPoints(Sid sid) const {
    const auto raw=Raw(sid);const auto& type=types_[raw[kType]];
    const std::size_t extra=pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35;
    if (!(type.flags2&kDaisKind)) return TypeHitPoints(raw[kType]);
    // dais 부류는 켜져 있을 때 altar의 HP를 쓰고 꺼져 있으면 0이다.
    return (raw[extra]&1) ? TypeHitPoints(state_.types.none) : 0;
}
std::int32_t SquidReward::CostCurve(std::uint32_t type,std::int32_t count) const {
    if (type>=types_.size()) throw std::out_of_range("보상 비용 타입 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    // altar/dais는 개수를 하나 줄여서 곡선에 넣는다.
    const auto n=(type==state_.types.none || type==state_.types.dais) ? Wrap(static_cast<std::int64_t>(count)-1) : count;
    if (n<0) return 0;
    if (patch) {
        // 같은 float 단가를 n+1번 누적한다. 정수 단가×항 수가 정확히 표현되는 범위만 지원한다.
        if (n>=kMaxTerms) throw std::out_of_range("보상 비용 곡선 항 수 범위 오류");
        const double unit=static_cast<double>(static_cast<float>(TypeCostUnits(type)));
        const double sum=unit*static_cast<double>(n+1);
        if (std::fabs(sum)>=kExactLimit) throw std::out_of_range("보상 비용 곡선 누적 범위 오류");
        return Ftol(sum);
    }
    // CD판은 항 수와 float 단가를 곱해 float로 저장한다.
    const float product=static_cast<float>(static_cast<double>(static_cast<std::int64_t>(n)+1)*static_cast<double>(types_[type].cost));
    return Ftol(static_cast<double>(product));
}
float SquidReward::SalvageCost(std::uint32_t type,std::int32_t count) const {
    if (type>=types_.size()) throw std::out_of_range("보상 환불 타입 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto n=(type==state_.types.none || type==state_.types.dais) ? Wrap(static_cast<std::int64_t>(count)-1) : count;
    float result=0.0f;
    if (patch) {
        double sum=0.0;
        if (n>=0) {
            if (n>=kMaxTerms) throw std::out_of_range("보상 비용 곡선 항 수 범위 오류");
            sum=static_cast<double>(static_cast<float>(TypeCostUnits(type)))*static_cast<double>(n+1);
            if (std::fabs(sum)>=kExactLimit) throw std::out_of_range("보상 비용 곡선 누적 범위 오류");
        }
        // 합계는 x87 스택에 남은 채 배율을 곱하고 호출자가 float로 저장한다.
        result=static_cast<float>(type!=state_.types.fullValue ? sum*kSalvageScale : sum);
        return result;
    }
    if (n>=0) result=static_cast<float>(static_cast<double>(static_cast<std::int64_t>(n)+1)*static_cast<double>(types_[type].cost));
    return type!=state_.types.fullValue ? static_cast<float>(static_cast<double>(result)*kSalvageScale) : result;
}
std::int32_t SquidReward::SalvageRefund(Sid sid,std::uint32_t type,std::int32_t count) const {
    if (type==state_.types.alias) return 0;
    const float cost=SalvageCost(type,count);
    if (!HasHitPoints(sid)) return Ftol(static_cast<double>(cost));
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto raw=Raw(sid);
    const std::int32_t hp=patch ? std::bit_cast<std::int32_t>(Dword(raw,kHp))
                                : static_cast<std::int16_t>(raw[kHp]|(static_cast<std::uint16_t>(raw[kHp+1])<<8));
    const auto max=MaxHitPoints(sid);
    // 절반 미만이면 환불 없음. 반 값은 부호 있는 2 나눗셈(0방향)이다.
    if (state_.halfHealthMode && hp<max/2) return 0;
    // 최대 HP 0은 x87에서 무한대/NaN이 되고 절삭 변환이 정수 불확정값의 하위 DWORD 0을 돌려준다(두 판본 기계어 확인).
    if (max==0) return 0;
    if (patch) {
        // 먼저 HP×비용을 float로 저장하고 최대 HP로 나눈다.
        const float product=static_cast<float>(static_cast<double>(hp)*static_cast<double>(cost));
        return Ftol(static_cast<double>(product)/static_cast<double>(max));
    }
    // CD판은 HP/최대 HP 비율에 비용을 곱해 float로 저장한다.
    const float scaled=static_cast<float>((static_cast<double>(hp)/static_cast<double>(max))*static_cast<double>(cost));
    return Ftol(static_cast<double>(scaled));
}
SquidRewardResult SquidReward::Plan(Sid sid,std::uint32_t recipient,bool salvage) const {
    const auto raw=Raw(sid);
    if (recipient>kPlayerCount) throw std::out_of_range("보상 수신자 번호는 0~8이어야 합니다");
    const auto type=static_cast<std::uint32_t>(raw[kType]);
    if (type==state_.types.none || (!salvage && type==state_.types.silent)) return {};
    // 가상 개수는 원본처럼 호출마다 다시 묻는다. 훅은 상태를 바꾸지 않아야 한다.
    auto cost=CostCurve(type,Count(sid));
    if (raw[kType]==state_.types.alias) cost=CostCurve(state_.types.silent,Count(sid));
    if (cost==0) return {};
    std::int32_t refund=salvage ? SalvageRefund(sid,raw[kType],Count(sid))
                                : Wrap(static_cast<std::int64_t>(static_cast<std::int32_t>(
                                      static_cast<std::uint32_t>(state_.percent)*static_cast<std::uint32_t>(cost)))/100);
    if (recipient==0) refund=0;
    return {true,cost,refund};
}
// 패치 00413bb0 / CD 004d8cc0·004d81f0. 소유자 1~8 중 AI가 부착된 경우만 지갑을 올린다.
void SquidReward::AddWallet(std::uint32_t owner,std::int32_t amount) {
    if (state_.aiMirrorDisabled || owner<1 || owner>kPlayerCount || !bookkeeping_.aiAttached[owner]) return;
    auto& wallet=state_.aiWallet[owner];
    wallet=Wrap(static_cast<std::int64_t>(wallet)+amount);
    if (state_.walletClamp && wallet<=kWalletFloor) wallet=kWalletFloor;
}
// 패치 0040eec0/0041df80 및 CD 00414680의 SP 가산과 AI 지갑 갱신, 표시 갱신이다.
void SquidReward::AddSp(std::uint32_t owner,float delta) {
    const bool fixed=state_.fixedSpA || state_.fixedSpB;
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        const float sp=std::bit_cast<float>(store_->Get(owner));
        if (!std::isfinite(sp)) throw std::invalid_argument("보상 SP 저장값이 유한하지 않습니다");
        // SP 고정 모드에서는 지급액이 고정값과 현재 SP의 차이로 바뀐다.
        if (fixed) delta=static_cast<float>(kPatchFixedSp-static_cast<double>(sp));
        const float sum=static_cast<float>(static_cast<double>(sp)+static_cast<double>(delta));
        const auto mirror=Ftol(static_cast<double>(delta));
        store_->Set(owner,std::bit_cast<std::uint32_t>(sum));
        AddWallet(owner,mirror);
    } else {
        // CD판은 로컬 SP만 전역 float으로 두고 고정 모드가 값을 덮어쓴다. 지갑에는 입력 지급액이 간다.
        float sp=static_cast<float>(static_cast<double>(delta)+static_cast<double>(state_.cdLocalSp));
        if (fixed) sp=static_cast<float>(kCdFixedSp);
        state_.cdLocalSp=sp;
        AddWallet(owner,Ftol(static_cast<double>(delta)));
    }
    if (hooks_.refreshSp) hooks_.refreshSp();
}
SquidRewardResult SquidReward::Refund(Sid sid,std::uint32_t recipient,bool salvage) {
    const auto plan=Plan(sid,recipient,salvage);
    if (!plan.applied) return plan;
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (recipient==bookkeeping_.localOwner) AddSp(recipient,static_cast<float>(plan.refund));
    // CD판의 다른 소유자는 SP 저장소가 없고 정수 지급액이 AI 지갑으로만 간다.
    else if (patch) AddSp(recipient,static_cast<float>(plan.refund));
    else AddWallet(recipient,plan.refund);
    state_.geyserPool=Wrap(static_cast<std::int64_t>(state_.geyserPool)+Wrap(static_cast<std::int64_t>(plan.cost)-plan.refund));
    return plan;
}
}
