// Fort Money 섹션의 실제모드/SP·게임난수·로딩깊이를 공유 상태에 적용한다.
#pragma once
#include "o/SquidReward.h"

namespace netstorm::o {
// MemStream이 실제로 읽는 pos/end DWORD다. 저장금액의float값은 이함수에서 사용하지 않는다.
struct FortMoneyCursor {
    std::uint32_t position{}; // 현재스트림pos이며 실제몸체가4를감김가산한다.
    std::uint32_t end{}; // 현재스트림end이며 증가한pos와signed비교한다.
};
// Money wrapper와현재플레이어SP 지정이 읽는 나머지전역이다.
struct FortMoneyState {
    bool economyBlocked{}; // 패치005c85d4/CD0051894c가nonzero이면 시작SP적용을생략한다.
    bool clientActive{}; // 패치00595344의로컬플레이어SP/표시경로를선택한다.
    bool templatePresent{true}; // 실제Template의첫DWORD가nonzero인지표현하며 없으면진단뒤계속한다.
    std::uint8_t difficulty{}; // 패치0052f7ee의0..2 인덱스로1000/2500/3000을고른다.
    std::int32_t cdStartingSp{}; // CD005120d0의현재signed시작SP를float로변환한다.
    std::uint32_t spCheckCount{}; // 패치005c89c4의SP검사잔여횟수이며조건성립시99를쓴다. 지형CRTcache와별개다.
};
// 원본진단의호출순서와조건부SP표시를실제소비자에게전달한다.
enum class FortMoneyAssertion {
    MissingTemplate, // Template의첫DWORD가0인원본진단이다.
    StreamPastEnd // 실제4바이트소비뒤signed pos>end인원본진단이다.
};
// 원본효과를생략하지않고같은로딩상태/표시에연결하는경계다.
struct FortMoneyHooks {
    std::function<void(FortMoneyAssertion)> assertion; // 원본assert보고뒤계속하는진단경계다.
    std::function<void(std::uint32_t)> loadingDepthChanged; // 동일월드모듈에현재깊이DWORD를전달한다.
    std::function<void()> refreshSp; // 패치로컬SetPlayerSp의실제0043dad0 표시갱신이다.
};
// 패치004bf340→00483300 및CD004297f0→00414720의현재Money 실행사슬이다.
class RawFortMoneyLoad {
public:
    // SP/장부/실제9개Player기록/현재깊이는이객체보다오래살아야한다. patchStore는같은게임GameRandom을쓴다.
    RawFortMoneyLoad(OriginalEdition edition,SquidRewardState& reward,SquidPostPopState& players,
        std::span<const std::uint8_t> playerRecords,std::uint32_t& loadingDepth,
        FortMoneyState& state,ScrambledSpStore* patchStore,FortMoneyHooks hooks);
    // 실제4바이트소비→모드별현재SP/난수/99회검사카운터를적용하고깊이를복구한다.
    void Load(FortMoneyCursor& stream) const;
    // 패치00483300/CD00414720의Money본문만실행한다. wrapper/조회/깊이/99회설정은별도다.
    void ReadMoney(FortMoneyCursor& stream) const;
    // 패치00447810: 로컬이면현재special 고정SP와표시를적용하고1..8의비로컬은분산SP만지정한다.
    void SetPlayerSp(std::int32_t owner,std::int32_t amount) const;
private:
    // 원본Player+50의현재flagsBYTE를다시읽는다. 로컬/반복번호와다른수명사본을만들지않는다.
    std::uint8_t PlayerFlags(std::uint32_t owner) const;
    OriginalEdition edition_; // 현재SP저장/Player기록stride의판본이다.
    SquidRewardState& reward_; // 기존fort/mission/special모드와CD현재SP를공유한다.
    SquidPostPopState& players_; // 현재localOwner DWORD가있는동일월드장부다.
    std::span<const std::uint8_t> playerRecords_; // 원본9개Player의실제바이트배열이다.
    std::uint32_t& loadingDepth_; // 기존Fort/사제/표면과동일한현재로딩깊이다.
    FortMoneyState& state_; // 현재guard/client/difficulty/시작SP/99회검사전역이다.
    ScrambledSpStore* patchStore_; // 실제게임난수를소비하는패치분산SP저장소이며CD에서는null이다.
    FortMoneyHooks hooks_; // 실제진단/로딩상태전달/표시효과다.
};
}
