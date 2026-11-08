// 삭제/해체 보상의 판본별 x87 연산 순서를 복원한다. PATCH 실제 AI 지갑은 같은 본체 처리기에 연결한다.
#include "o/SquidReward.h"
#include "o/RawRiftHitPoints.h" // Factory와 같은 현재 TYPE HP 몸체를 사용한다.
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
// SP 가산의 원본 CRT는 비유한/64비트 초과에서 정수 불확정값의 하위DWORD0을 반환한다. 수신자0의 미정 SP 비트도 원문대로 처리한다.
std::int32_t SpMirrorFtol(float value) {
    const auto exact=static_cast<double>(value);
    if (!std::isfinite(exact) || exact>=9223372036854775808.0 || exact< -9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(exact)));
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
// 같은 실제 TYPE 배열을 소비 위치에서 읽으며 명시 해제 시에만 생성 시 사본을 다시 쓴다.
std::span<const RiftTypeRecord> SquidReward::CurrentTypes() const { return currentTypes_.empty() ? std::span<const RiftTypeRecord>(types_) : currentTypes_; }
// 잘못된 대여 요청은 기존 연결을 바꾸기 전에 거부한다.
void SquidReward::BindCurrentTypes(std::span<const RiftTypeRecord> types) {
    if (!types.empty() && (pool_.Edition()!=OriginalEdition::Patch1078 || types.size()!=188))
        throw std::invalid_argument("보상 현재 TYPE 대여의 판본/길이 오류");
    currentTypes_=types;
}
void SquidReward::BindHitPointDiagnostics(std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion) { hitPointAssertion_=std::move(assertion); } // 호출 위치에서 선택된 진단만 요구하며 빈 연결은 명시 미연결이다.
// 최초 비용표만 대여하고 현재 HP/플래그 표의 주소와 독립적으로 보존한다.
void SquidReward::BindLoadedCosts(std::span<const RiftTypeRecord> types) {
    if (!types.empty() && (pool_.Edition()!=OriginalEdition::Patch1078 || types.size()!=188))
        throw std::invalid_argument("보상 로딩 비용표 대여의 판본/길이 오류");
    loadedCostTypes_=types;
}
// span에는 at이 없으므로 기존 vector::at과 같은 범위 거부를 역참조 직전에 유지한다.
const RiftTypeRecord& SquidReward::TypeRecord(std::uint32_t type) const {
    const auto types=CurrentTypes();
    if (type>=types.size()) throw std::out_of_range("보상 TYPE 기록 범위 오류");
    return types[type];
}
std::span<const std::uint8_t> SquidReward::Raw(Sid sid) const {
    if (sid.value==0 || sid.value>=pool_.Capacity()) throw std::out_of_range("보상 SID 범위 오류");
    const auto raw=pool_.Slot(sid);
    if (raw[kType]>=CurrentTypes().size()) throw std::out_of_range("보상 타입 번호 범위 오류");
    return raw;
}
// 로더의 ftol(cost + 번호*23)을 그대로 따라 한 뒤 읽는 쪽의 감산까지 합친 값이다. CD판은 float 비용을 그대로 쓰므로 호출하지 않는다.
std::int32_t SquidReward::TypeCostUnits(std::uint32_t type) const {
    // PATCH0059ab50은 TYPE+c4 float와 별도다. 실제 Session은 최초 로딩 표, 독립 재생은 생성 시 사본을 사용한다(단위865).
    const auto costs=loadedCostTypes_.empty() ? std::span<const RiftTypeRecord>(types_) : loadedCostTypes_;
    if (type>=costs.size()) throw std::out_of_range("보상 로딩 비용표 번호 범위 오류");
    const double encoded=static_cast<double>(costs[type].cost)+static_cast<double>(type*kCostBias);
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
    const auto selectedBase=CurrentTypes();const auto& record=TypeRecord(type); // 진단이 표 기준을 옮겨도 처음 선택한 물리 this를 유지한다.
    RiftHitPointHooks hooks;
    hooks.number=[this,selectedBase,&record] { return CurrentRiftTypeNumber(record,selectedBase,CurrentTypes()); }; // 원문 getNum이 소비되는 시점의 현재 base를 읽는다.
    hooks.assertion=[this](auto expression,auto file,auto line) { const auto selected=hitPointAssertion_;if (!selected) throw std::logic_error("보상 TYPE HP의 실제 진단 장치 미연결");selected(expression,file,line); };
    return ReadRiftHitPoints(pool_.Edition(),record,{state_.debugAsserts,RawCheckingFlag{state_.sharedPriestQuarterHp ? *state_.sharedPriestQuarterHp : state_.priestQuarterHp},state_.sharedPriestType ? *state_.sharedPriestType : state_.types.priest,state_.sharedDaisType ? *state_.sharedDaisType : state_.types.dais},hooks);
}
bool SquidReward::HasHitPoints(Sid sid) const {
    const auto raw=Raw(sid);const auto& type=TypeRecord(raw[kType]);
    const std::size_t extra=pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35;
    return (type.flags2&kDaisKind) ? (raw[extra]&1)!=0 : (type.flags1&kHasHp)!=0;
}
std::int32_t SquidReward::MaxHitPoints(Sid sid) const {
    const auto raw=Raw(sid);const auto& type=TypeRecord(raw[kType]);
    const std::size_t extra=pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35;
    if (!(type.flags2&kDaisKind)) return TypeHitPoints(raw[kType]);
    // dais 부류는 켜져 있을 때 altar의 HP를 쓰고 꺼져 있으면 0이다.
    return (raw[extra]&1) ? TypeHitPoints(state_.AltarType()) : 0;
}
std::int32_t SquidReward::CostCurve(std::uint32_t type,std::int32_t count) const {
    if (type>=CurrentTypes().size()) throw std::out_of_range("보상 비용 타입 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    // altar/dais는 개수를 하나 줄여서 곡선에 넣는다.
    const auto n=(type==state_.AltarType() || type==state_.DaisType()) ? Wrap(static_cast<std::int64_t>(count)-1) : count;
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
    const float product=static_cast<float>(static_cast<double>(static_cast<std::int64_t>(n)+1)*static_cast<double>(TypeRecord(type).cost));
    return Ftol(static_cast<double>(product));
}
float SquidReward::SalvageCost(std::uint32_t type,std::int32_t count) const {
    if (type>=CurrentTypes().size()) throw std::out_of_range("보상 환불 타입 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto n=(type==state_.AltarType() || type==state_.DaisType()) ? Wrap(static_cast<std::int64_t>(count)-1) : count;
    float result=0.0f;
    if (patch) {
        double sum=0.0;
        if (n>=0) {
            if (n>=kMaxTerms) throw std::out_of_range("보상 비용 곡선 항 수 범위 오류");
            sum=static_cast<double>(static_cast<float>(TypeCostUnits(type)))*static_cast<double>(n+1);
            if (std::fabs(sum)>=kExactLimit) throw std::out_of_range("보상 비용 곡선 누적 범위 오류");
        }
        // 합계는 x87 스택에 남은 채 배율을 곱하고 호출자가 float로 저장한다.
        result=static_cast<float>(type!=state_.NuggetType() ? sum*kSalvageScale : sum);
        return result;
    }
    if (n>=0) result=static_cast<float>(static_cast<double>(static_cast<std::int64_t>(n)+1)*static_cast<double>(TypeRecord(type).cost));
    return type!=state_.NuggetType() ? static_cast<float>(static_cast<double>(result)*kSalvageScale) : result;
}
std::int32_t SquidReward::SalvageRefund(Sid sid,std::uint32_t type,std::int32_t count) const {
    if (type==state_.AliasType()) return 0;
    const float cost=SalvageCost(type,count);
    if (!HasHitPoints(sid)) return Ftol(static_cast<double>(cost));
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto raw=Raw(sid);
    const std::int32_t hp=patch ? std::bit_cast<std::int32_t>(Dword(raw,kHp))
                                : static_cast<std::int16_t>(raw[kHp]|(static_cast<std::uint16_t>(raw[kHp+1])<<8));
    const auto max=MaxHitPoints(sid);
    // 절반 미만이면 환불 없음. 반 값은 부호 있는 2 나눗셈(0방향)이다.
    if (state_.HalfHealthMode() && hp<max/2) return 0;
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
    if (type==state_.AltarType() || (!salvage && type==state_.SilentType())) return {};
    // 첫 TYPE는 가상Count 전에 저장하며 이후 별칭/환불의 현재 조회 순서는 원문 판본대로 구별한다.
    auto cost=CostCurve(type,Count(sid));
    if (raw[kType]==state_.AliasType()) { const auto count=Count(sid);cost=CostCurve(state_.SilentType(),count); } // 양판본은 Count 뒤의 현재005412a4/CD0051cb90을 읽는다.
    if (cost==0) return {};
    std::int32_t refund;
    if (salvage) {
        auto refundType=static_cast<std::uint32_t>(raw[kType]);const auto count=Count(sid);
        if (pool_.Edition()==OriginalEdition::Cd1072) refundType=raw[kType]; // PATCH0044c37f는 Count 전, CD00462784는 Count 후의 현재 TYPE를 저장한다.
        refund=SalvageRefund(sid,refundType,count);
    } else refund=Wrap(static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(state_.Percent())*static_cast<std::uint32_t>(cost)))/100);
    if (recipient==0) refund=0;
    return {true,cost,refund};
}
// 패치 00413bb0 / CD 004d8cc0·004d81f0. 소유자 1~8 중 AI가 부착된 경우만 지갑을 올린다.
void SquidReward::AddWallet(std::uint32_t owner,std::int32_t amount) {
    if (creditWallet_) { creditWallet_(owner,amount);return; }
    if (state_.aiMirrorDisabled || owner<1 || owner>kPlayerCount || !bookkeeping_.aiAttached[owner]) return;
    auto& wallet=state_.aiWallet[owner];
    wallet=Wrap(static_cast<std::int64_t>(wallet)+amount);
    if (state_.walletClamp && wallet<=kWalletFloor) wallet=kWalletFloor;
}
// 상위 내용물 삭제가 요구하는 선가산을 기존 실제 지갑 몸체에 전달한다.
void SquidReward::CreditWallet(std::uint32_t owner,std::int32_t amount) { AddWallet(owner,amount); }
// CD/불완전 바인딩으로 기존 판본의 값형 지갑을 바꾸지 않는다.
void SquidReward::BindPatchWallet(std::function<void(std::uint32_t,std::int32_t)> credit) {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || !credit) throw std::invalid_argument("PATCH 실제 AI 지갑 가산 처리 누락/판본 불일치");creditWallet_=std::move(credit);
}
// 패치 0040eec0/0041df80 및 CD 00414680의 SP 가산과 AI 지갑 갱신, 표시 갱신이다.
void SquidReward::AddSp(std::uint32_t owner,float delta) {
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        const float sp=std::bit_cast<float>(store_->Get(owner));
        // SP 고정 모드에서는 지급액이 고정값과 현재 SP의 차이로 바뀐다.
        if (state_.FixedSpA() || state_.FixedSpB()) delta=static_cast<float>(kPatchFixedSp-static_cast<double>(sp));
        const float sum=static_cast<float>(static_cast<double>(sp)+static_cast<double>(delta));
        const auto mirror=SpMirrorFtol(delta);
        store_->Set(owner,std::bit_cast<std::uint32_t>(sum));
        AddWallet(owner,mirror);
    } else {
        // CD판은 로컬 SP만 전역 float으로 두고 고정 모드가 값을 덮어쓴다. 지갑에는 입력 지급액이 간다.
        float sp=static_cast<float>(static_cast<double>(delta)+static_cast<double>(state_.cdLocalSp));
        if (state_.FixedSpA() || state_.FixedSpB()) sp=static_cast<float>(kCdFixedSp);
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
    state_.GeyserPool()=Wrap(static_cast<std::int64_t>(state_.GeyserPool())+Wrap(static_cast<std::int64_t>(plan.cost)-plan.refund));
    return plan;
}
}
