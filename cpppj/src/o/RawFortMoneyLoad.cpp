// 저장금액을직접대입하지않고원본의현재모드/플레이어정책과분산SP쓰기순서를보존한다.
#include "o/RawFortMoneyLoad.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::array<std::int32_t,3> kInitialSp{1000,2500,3000}; // 패치0041c7d0의실제지역배열이며difficulty0..2로조회한다.
constexpr std::uint8_t kExcludedPlayer=0x20; // Money시작SP지정에서건너뛰는현재Player+50 비트다.
constexpr float kPatchFortSp=194162.0f,kCdFortSp=100000.0f,kSpecialLocalSp=194162.0f; // 실제판본별Money fort값과모든유효owner의special 지정값이다.
constexpr std::uint32_t kBonusSp=4000,kSpCheckCount=99; // 패치시작SP의가산DWORD와검사잔여횟수다.
}

// 판본별Player기록크기와필수효과/분산저장소를구성전에확인한다.
RawFortMoneyLoad::RawFortMoneyLoad(OriginalEdition edition,SquidRewardState& reward,SquidPostPopState& players,
    std::span<const std::uint8_t> playerRecords,std::uint32_t& loadingDepth,
    FortMoneyState& state,ScrambledSpStore* patchStore,FortMoneyHooks hooks)
    :edition_(edition),reward_(reward),players_(players),playerRecords_(playerRecords),loadingDepth_(loadingDepth),
     state_(state),patchStore_(patchStore),hooks_(std::move(hooks)) {
    const auto stride=edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU;
    if (playerRecords.size()!=9U*stride || !hooks_.assertion || !hooks_.loadingDepthChanged || !hooks_.refreshSp ||
        (edition==OriginalEdition::Patch1078 && !patchStore)) throw std::invalid_argument("FortMoney 실제Player/SP/필수효과누락");
}

// 실제physical Player0..8의현재BYTE만읽는다.
std::uint8_t RawFortMoneyLoad::PlayerFlags(std::uint32_t owner) const {
    if (owner>=9) throw std::out_of_range("FortMoney 실제Player범위오류");
    const auto stride=edition_==OriginalEdition::Patch1078 ? 0xb4U : 0xacU;
    return playerRecords_[owner*stride+0x50U];
}

// AI지갑가산이나SP차감용AddSp를사용하지않고실제지정몸체의분기를따른다.
void RawFortMoneyLoad::SetPlayerSp(std::int32_t owner,std::int32_t amount) const {
    if (edition_!=OriginalEdition::Patch1078) throw std::logic_error("CD Money에패치SetPlayerSp를호출할수없습니다");
    if (state_.clientActive && static_cast<std::uint32_t>(owner)==players_.localOwner) {
        const float value=reward_.fixedSpB ? kSpecialLocalSp : static_cast<float>(amount);
        patchStore_->Set(players_.localOwner,std::bit_cast<std::uint32_t>(value));hooks_.refreshSp();
    } else if (owner>0 && owner<9) {
        const float value=reward_.fixedSpB ? kSpecialLocalSp : static_cast<float>(amount);
        patchStore_->Set(static_cast<std::uint32_t>(owner),std::bit_cast<std::uint32_t>(value));
    }
}

// wrapper의현재깊이/Template진단뒤Money4바이트소비·현재모드별SP를순서대로적용한다.
void RawFortMoneyLoad::Load(FortMoneyCursor& stream) const {
    const bool patch=edition_==OriginalEdition::Patch1078;
    if (patch && reward_.halfHealthMode && !state_.economyBlocked) state_.spCheckCount=kSpCheckCount;
    ++loadingDepth_;hooks_.loadingDepthChanged(loadingDepth_);
    try {
        if (!state_.templatePresent) hooks_.assertion(FortMoneyAssertion::MissingTemplate);
        stream.position=0; // 실제DataManager의Money 섹션조회는현재pos를0으로초기화한다.
        ReadMoney(stream);
    } catch (...) {
        --loadingDepth_;hooks_.loadingDepthChanged(loadingDepth_);throw;
    }
    --loadingDepth_;hooks_.loadingDepthChanged(loadingDepth_);
}

// 섹션4바이트소비와현재모드별SP설정을그대로실행한다. 저장float비트를SP로대입하지않는다.
void RawFortMoneyLoad::ReadMoney(FortMoneyCursor& stream) const {
        const bool patch=edition_==OriginalEdition::Patch1078;
        stream.position+=4U;
        if (std::bit_cast<std::int32_t>(stream.end)<std::bit_cast<std::int32_t>(stream.position)) hooks_.assertion(FortMoneyAssertion::StreamPastEnd);
        if (reward_.fixedSpA) {
            if (patch) patchStore_->Set(players_.localOwner,std::bit_cast<std::uint32_t>(kPatchFortSp));
            else reward_.cdLocalSp=kCdFortSp;
        }
        if (reward_.halfHealthMode && !state_.economyBlocked) {
            if (patch) {
                if (state_.difficulty>=kInitialSp.size()) throw std::out_of_range("FortMoney 실제difficulty배열범위오류");
                const auto amount=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(kInitialSp[state_.difficulty])+kBonusSp);
                // 원본은현재Player1..8의flags를각반복에서읽고같은게임난수를소비하는SP지정몸체를호출한다.
                for (std::int32_t owner=1;owner<9;++owner) if (!(PlayerFlags(static_cast<std::uint32_t>(owner))&kExcludedPlayer)) SetPlayerSp(owner,amount);
            } else if (!(PlayerFlags(players_.localOwner)&kExcludedPlayer)) reward_.cdLocalSp=static_cast<float>(state_.cdStartingSp);
        }
}
}
