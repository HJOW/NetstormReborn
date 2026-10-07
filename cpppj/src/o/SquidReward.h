// 삭제/해체 보상(패치 0044c2f0 ↔ CD 004626d0): 비용 곡선·소유자 비율·SP 지급·AI 지갑·샘 풀을 복원한다.
#pragma once
#include "o/SidPool.h"
#include "o/SpStore.h"
#include "o/SquidPostPop.h"
#include <array>
#include <functional>
#include <span>

namespace netstorm::o {
// 원본 타입 번호 전역의 실제 초기값이다(두 PE 공통). 이름은 `--dump-types`의 타입 이름이다.
struct SquidRewardTypes {
    std::uint32_t none{164};         // altar(DAT_005412e8): 보상 대상이 아니며 개수 곡선이 하나 줄어든다.
    std::uint32_t dais{165};         // dais(DAT_005412ec): altar와 같이 개수가 하나 줄어든다.
    std::uint32_t silent{147};       // rainBlocker(DAT_005412a4): 삭제 보상에서는 제외, 해체 보상에서는 포함.
    std::uint32_t alias{150};        // growingRainBlocker(DAT_005412b0): 비용을 rainBlocker 것으로 대체하며 해체 환불은 0.
    std::uint32_t fullValue{154};    // nugget(DAT_005412c0): 해체 환불 비용에 1/4 배율을 곱하지 않는다.
    std::uint32_t priest{158};       // priest(DAT_005412d0): 디버그 플래그가 켜지면 최대 HP를 1/4로 본다.
};
// 보상 계산이 읽거나 갱신하는 판본별 전역과 AI 지갑이다. 패치 주소/CD 주소는 괄호에 둔다.
struct SquidRewardState {
    SquidRewardTypes types;
    std::uint32_t percent{25};           // 삭제 보상 비율(DAT_005424bc / CD 005395d0). 전투 초기화가 25로 정한다.
    std::int32_t geyserPool{};           // 보상하지 않은 가치의 누적(DAT_00595088 / CD 00512110). 샘 생성 비교에 쓰인다.
    bool fixedSpA{},fixedSpB{};          // SP를 고정값으로 만드는 두 전역(DAT_00594fb8,DAT_005c85a4 / CD 00540a1c,00518904).
    bool halfHealthMode{};               // 해체 환불을 HP 절반 미만에서 0으로 만드는 전역(DAT_00594fbc / CD 00540a20).
    bool aiMirrorDisabled{};             // AI 지갑 갱신을 끄는 전역(DAT_00594fc8 / CD 00540a24).
    bool walletClamp{};                  // 지갑을 0x4a6 이상으로 올리는 객체 조건(DAT_005634a0->+0x68 / CD 00549a08->+0x68).
    bool priestQuarterHp{};              // priest 최대 HP 1/4 전역(DAT_005ca8f0 / CD 00511c90).
    bool debugAsserts{};                 // 패치 0049fce0의 디버그 assert 전역(DAT_005e4794). 켜지면 위반 입력을 거부한다.
    std::array<std::int32_t,kPlayerCount+1> aiWallet{}; // AI 객체 +0x2d8. AI가 부착된 소유자만 갱신한다.
    float cdLocalSp{};                   // CD판 로컬 SP 전역(005120d4). 패치판은 ScrambledSpStore를 쓴다.
};
struct SquidRewardHooks {
    std::function<std::int32_t(Sid)> virtualCount; // 파생 객체의 vtable +0x80. 비어 있으면 base(상태 상위 3비트)를 쓴다.
    std::function<void()> refreshSp;               // SP 표시 갱신(0043dad0 / CD 004ee9d0). 표시 효과이므로 비어 있어도 된다.
};
// 한 번의 보상 계산 결과다. applied가 거짓이면 아무 상태도 바뀌지 않았다.
struct SquidRewardResult {
    bool applied{};
    std::int32_t cost{},refund{}; // 정수 비용과 지급액. 지급액은 소유자가 0이면 0이다.
};
class SquidReward {
public:
    // 풀/타입/장부는 호출자가 소유한다. 패치판은 SP 저장소가 필요하고 CD판은 필요 없다.
    SquidReward(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& bookkeeping,
        SquidRewardState& state,ScrambledSpStore* store,SquidRewardHooks hooks={});
    // 상태를 바꾸지 않고 비용/지급액을 계산한다. 손상 입력은 여기서 거부한다.
    SquidRewardResult Plan(Sid sid,std::uint32_t recipient,bool salvage) const;
    // 0044c2f0 / 004626d0. salvage는 원본 세 번째 인자로, 거짓이면 삭제 보상 비율, 참이면 남은 HP 비례 환불이다.
    SquidRewardResult Refund(Sid sid,std::uint32_t recipient,bool salvage);
    // 패치판 정수 비용 표(DAT_0059ab50)의 디코딩 값이다. 로더가 ftol(cost + 번호*23)을 저장하고 읽을 때 번호*23을 뺀다.
    std::int32_t TypeCostUnits(std::uint32_t type) const;
    // 삭제 장부가 같은 풀/장부를 쓰는지 확인하기 위한 연결 대상이다.
    const SidPool& Pool() const;
    const SquidPostPopState& Bookkeeping() const;
    // 원본 vtable +0x80을 물은 횟수다. 기계어 대조에서 호출 순서와 횟수를 확인하는 진단 값이다.
    std::uint32_t VirtualCountCalls() const;
private:
    // vtable +0x80의 개수. 훅이 있으면 파생 구현이고 없으면 base의 상태 상위 3비트다.
    std::int32_t Count(Sid sid) const;
    // 0044b220 / CD 004147c0. 판본별 부동소수 순서로 비용 곡선을 계산한 뒤 0방향으로 절삭한다.
    std::int32_t CostCurve(std::uint32_t type,std::int32_t count) const;
    // 0043cb30 / CD 00414830. 해체 환불용 float 비용이다(nugget 외에는 1/4).
    float SalvageCost(std::uint32_t type,std::int32_t count) const;
    // 0044c240 / CD 00462610. 남은 HP 비례 환불이다.
    std::int32_t SalvageRefund(Sid sid,std::uint32_t type,std::int32_t count) const;
    // 004adc60/004aecf0: HP가 있는 객체인지 판단한다.
    bool HasHitPoints(Sid sid) const;
    // 004adcb0/004aed30와 0049fce0/00444720: 객체의 최대 HP다.
    std::int32_t MaxHitPoints(Sid sid) const;
    std::int32_t TypeHitPoints(std::uint32_t type) const;
    // 로컬/소유자 SP 가산과 AI 지갑 갱신이다. delta는 float로 반올림된 지급액이다.
    void AddSp(std::uint32_t owner,float delta);
    void AddWallet(std::uint32_t owner,std::int32_t amount);
    std::span<const std::uint8_t> Raw(Sid sid) const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    SquidPostPopState& bookkeeping_;
    SquidRewardState& state_;
    ScrambledSpStore* store_{};
    SquidRewardHooks hooks_;
    mutable std::uint32_t virtualCalls_{};
};
}
