// TYPE+44의 현재 사거리를 실제 자산 로딩과 속성 순서로 확인한다.
#include "DudeSpawnSupport.h"

namespace { //보호 자산의 실제 정의와 TYPE 기록을 독립 경계에서 대조한다.
using namespace netstorm::o; //제품의 실제 타입/판본을 사용한다.
}

// 양판본 모든 자산의 사거리와 내장 타입의 초기0을 실제 같은 Table 로더로 확인한다.
TEST_CASE(RiftTypeRange_OriginalAssetsAndBuiltinZero) {
    std::size_t checked{}; //양판본 실제 자산을 빠짐없이 방문한 개수다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { //보호된 두 판본을 읽기 전용으로 조회한다.
        const auto& game=netstorm::test::dudespawn::Assets(edition).game;const auto types=game.TypeTable().Types();
        for (const auto& asset:game.Types()) { //TYPE 순서와 실제 정의의 수치 사거리만 대조한다.
            CHECK(types[70+asset.block].range==static_cast<float>(asset.definition.Number("range").value_or(0)));++checked;
        }
        for (std::size_t type=0;type<70;++type) CHECK(types[type].range==0); //원문 전체 TYPE 초기화의 미지정 사거리다.
    }
    CHECK(checked==217);
}

// 원문의0초기화/마지막 속성 대입과 음수 float 비트를 유지한다.
TEST_CASE(RiftTypeRange_DefaultDuplicateOrderAndFloatBits) {
    const auto empty=RiftTypeDefinition::Parse("typename empty {} A00:default:\"a.gif\"#0;");
    const auto changed=RiftTypeDefinition::Parse("typename rangeType { range=3.25; range=-7.5; } A00:default:\"a.gif\"#0;");
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { //양판본+44의 같은 수치 대입을 확인한다.
        const auto order=TypeLoadOrder(edition);std::vector<RiftTypeSource> sources; //실제 로더가 요구하는 전체 이름 순서를 보존한다.
        for (std::size_t index=0;index<order.size();++index) sources.push_back({order[index],index ? &empty:&changed}); //첫 자산만 명시 중복 사거리 입력이다.
        const RiftTypeTable table(edition,sources);CHECK(std::bit_cast<std::uint32_t>(table.Types()[70].range)==0xc0f00000U && table.Types()[71].range==0.0F);
    }
}
