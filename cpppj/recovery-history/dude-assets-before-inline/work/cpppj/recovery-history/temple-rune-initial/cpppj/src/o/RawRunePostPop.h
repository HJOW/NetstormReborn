// Rune166 후처리의304 예약·신전조회·건설시작과 공통 postPop 호출을 복원한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/SquidPostPop.h"

namespace netstorm::o {
class RawConstructionProcess; // 기존 실제건설 시작/프로세스 몸체를 재사용한다.
class SquidProcessHost; // 같은Kernel에 실제로컬304 Regular를 부착한다.
inline constexpr std::uint32_t kPatchRuneVtable=0x005075d8,kCdRuneVtable=0x00504148; // 두PE의 Rune166 생성자가 기록하는 실제가상표다.
// Rune 조회/건설시간 helper가 효과뒤에도읽는 현재 원본 전역이다.
struct RunePostPopState {
    std::uint32_t runeType{166},daisType{165},altarType{164}; // 원본 현재타입 번호이며 각조회시다시읽는다.
    bool suppressDuration{}; // 00594fa4/CD0052e170: altar/player 시간선택을끄고10초로반환한다.
    std::array<std::uint32_t,kPlayerCount+1> playerFlags{}; // patchPlayer+50/CDPlayer+50의현재bit1을읽는다. 원래ownerWORD가index다.
};
// 예약/건설/공통후처리는 모두 실제호스트/몸체로 연결해야 한다.
struct RunePostPopHooks {
    std::function<bool()> reserveRegular; // 최초flags1의new0x28 성공/실패다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // Host.AddRegular(Rune,0x304,0.01F)의로컬예약이다.
    std::function<void(Sid,float)> startConstruction; // 가까운같은owner신전이있으면기존Construction.Start(Rune,7/10/25)를호출한다.
};
// 실제 Rune166 postPop 전체몸체다. generic Pop에는 같은SquidPostPop의명시등록으로만연결한다.
class RawRunePostPop {
public:
    // 풀/해시/현재표/상태/공통몸체/효과참조는이객체보다오래살아야한다.
    RawRunePostPop(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,RunePostPopState& state,SquidPostPop& base,RunePostPopHooks hooks);
    // 실제Rune타입/가상표와이모듈등록여부를판단한다. free/void를지원주소판단과섞지않는다.
    bool Handles(Sid sid) const;
    // Pop의공간쓰기전에Rune타입/가상표/공통장부조건을검사한다.
    void Validate(Sid sid,std::uint32_t flags,const RawGraphPop* projected=nullptr) const;
    // 최초304→flags2의현재신전/시간/건설→공통PostPopBase를동일flags로한번실행한다.
    void PostPop(Sid rune,std::uint32_t flags) const;
    // 거의올림한Rune위치±1의일반finder에서현재같은owner의첫dais를읽는다.
    Sid FindOwnedDais(Sid rune) const;
    // Rune기준점의첫altar원래ownerWORD/playerFlags와현재전역으로7/10/25초를읽는다.
    std::int32_t ConstructionSeconds(Sid rune) const;
    // 실제Pop/Host/Construction어댑터의같은풀연결을확인한다.
    const SidPool& Pool() const;
private:
    // 현재Rune/altar의단정도좌표와ownerBYTE를원본폭대로읽는다.
    std::array<float,2> Position(Sid sid) const;
    std::uint32_t Owner(Sid sid) const; // 조회할때마다현재소유자BYTE를읽는다.
    SidPool& pool_; // 현재raw슬롯을읽고공통몸체와공유한다.
    const SquidHash& hash_; // Rune/dais/altar가공유하는네단계일반해시다.
    std::span<const RiftTypeRecord> types_; // 현재genus/발자국조회용타입표다.
    RunePostPopState& state_; // 예약효과뒤현재타입/시간전역을다시읽는다.
    SquidPostPop& base_; // 같은실제공통장부/깊이를한번갱신한다.
    RunePostPopHooks hooks_; // 생략할수없는예약/건설몸체연결이다.
};
// 같은실제Host/Construction을연결한다. 예약확보실패는caller가reserveRegular로공급한다.
RunePostPopHooks MakeRunePostPopHooks(const SidPool& pool,SquidProcessHost& host,const RawConstructionProcess& construction,RunePostPopHooks hooks);
}
