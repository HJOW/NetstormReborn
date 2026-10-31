// PE에서 정적으로 확인한 Tutorial 소멸의 효과 경계/직접 쓰기/기본 Mission 예외 정리를 검사한다.
#include "TestSupport.h"
#include "o/RawTutorialMissionRelease.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace { // 새 Native 실행이나 fixture 합성 없이 원문 직접 store/호출 계약만 명시 입력에 적용한다.
struct ReleaseFields { // 실제 서로 다른 여섯 DWORD와 소멸이 건드리지 않는 인접 상태를 보유한다.
    std::int32_t denySalvage{-7}; // 음수도 bool로 축소하지 않고 원문 전체 DWORD 0쓰기와 비교한다.
    std::int32_t denyAscend{9}; // 두 번째 금지 DWORD의 초기 입력이다.
    std::uint32_t missionNumber{0x87654321U}; // 높은 비트가 있는 첫 번호 입력이다.
    std::uint32_t tutorialNumber{0xfedcba98U}; // 별개 두 번째 번호 입력이다.
    std::int32_t stage{-65}; // signed 단계 입력이며 BYTE 부분 쓰기와 구별한다.
    std::uint32_t stageDirty{0x80000004U}; // 비제로 표식의 모든 비트를 확인한다.
    std::array<std::uint32_t,4> unrelated{3,5,7,11}; // 시계·World·missionPost 등 외부 상태를 대신하는 인접 감시 입력이다.
    TutorialMissionReleaseState State() { return {denySalvage,denyAscend,missionNumber,tutorialNumber,stage,stageDirty}; } // 같은 필드 주소를 helper에 빌린다.
    std::array<std::uint32_t,6> Words() const { return {std::bit_cast<std::uint32_t>(denySalvage),std::bit_cast<std::uint32_t>(denyAscend),missionNumber,tutorialNumber,std::bit_cast<std::uint32_t>(stage),stageDirty}; } // 관측 시점의 전체 DWORD를 순서대로 읽는다.
};
template<class Function> std::string ReleaseFailure(Function function) { // 본문 또는 기본 Mission에서 던진 실제 예외를 관측하며 성공으로 바꾸지 않는다.
    try { function(); }catch (const std::exception& error) { return error.what(); }return {};
}
}

TEST_CASE(TutorialMissionRelease_CurrentFieldsAndCallbacksFollowOriginalStores) { // 효과가 같은 현재 필드를 바꾸면 뒤 원문 store/조회에 반영되며 최종 복구 사본이 없어야 한다.
    ReleaseFields fields;const auto initial=fields.Words();const auto unrelated=fields.unrelated;std::vector<std::string> calls;
    TutorialMissionReleaseHooks hooks;hooks.setAllowAll=[&](auto enabled) { CHECK(enabled==1 && fields.Words()==initial);calls.emplace_back("allow");fields.denySalvage=47;fields.stageDirty=0xffffffffU; };
    hooks.removeAllBrains=[&] { CHECK((fields.Words()==std::array<std::uint32_t,6>{}));calls.emplace_back("brains");fields.missionNumber=71; };
    hooks.releaseBaseMission=[&] { CHECK(fields.missionNumber==71 && fields.denySalvage==0 && fields.denyAscend==0 && fields.tutorialNumber==0 && fields.stage==0 && fields.stageDirty==0);calls.emplace_back("base");fields.denyAscend=-19; };
    ReleaseTutorialMission(fields.State(),hooks);
    CHECK((calls==std::vector<std::string>{"allow","brains","base"}));CHECK(fields.missionNumber==71 && fields.denyAscend==-19 && fields.unrelated==unrelated);
}

TEST_CASE(TutorialMissionRelease_AllowFailureRunsBaseBeforeAnyDirectZeroStore) { // 004c323c 호출 실패는 여섯 직접 store에 도달하지 않고 004fb940의 기본 정리만 실행한다.
    ReleaseFields fields;const auto initial=fields.Words();std::vector<std::string> calls;TutorialMissionReleaseHooks hooks;
    hooks.setAllowAll=[&](auto enabled) { CHECK(enabled==1);calls.emplace_back("allow");fields.stageDirty=23;throw std::runtime_error("allow interrupted"); };
    hooks.removeAllBrains=[&] { calls.emplace_back("unexpected brains"); };
    hooks.releaseBaseMission=[&] { calls.emplace_back("base");auto expected=initial;expected[5]=23;CHECK(fields.Words()==expected);fields.denyAscend=29; };
    CHECK(ReleaseFailure([&] { ReleaseTutorialMission(fields.State(),hooks); })=="allow interrupted");
    CHECK((calls==std::vector<std::string>{"allow","base"}));CHECK(fields.denySalvage==-7 && fields.denyAscend==29 && fields.missionNumber==initial[2] && fields.tutorialNumber==initial[3] && fields.stage==-65 && fields.stageDirty==23);
}

TEST_CASE(TutorialMissionRelease_BrainFailureKeepsDirectZeroesAndCurrentPartialWrites) { // 004c3268 실패 전 직접 0쓰기와 AI 하위의 부분 변경을 base 정리와 호출자에게 보존한다.
    ReleaseFields fields;std::vector<std::string> calls;TutorialMissionReleaseHooks hooks;
    hooks.setAllowAll=[&](auto enabled) { CHECK(enabled==1);calls.emplace_back("allow"); };
    hooks.removeAllBrains=[&] { CHECK((fields.Words()==std::array<std::uint32_t,6>{}));calls.emplace_back("brains");fields.stage=-31;throw std::runtime_error("brains interrupted"); };
    hooks.releaseBaseMission=[&] { CHECK(fields.denySalvage==0 && fields.denyAscend==0 && fields.missionNumber==0 && fields.tutorialNumber==0 && fields.stage==-31 && fields.stageDirty==0);calls.emplace_back("base");fields.stageDirty=37; };
    CHECK(ReleaseFailure([&] { ReleaseTutorialMission(fields.State(),hooks); })=="brains interrupted");
    CHECK((calls==std::vector<std::string>{"allow","brains","base"}));CHECK(fields.stage==-31 && fields.stageDirty==37 && fields.denySalvage==0 && fields.denyAscend==0);
}

TEST_CASE(TutorialMissionRelease_BaseFailureIsNotRetried) { // 정상 꼬리의 기본 Mission 실패는 해제 재시도로 바꾸지 않고 한 번만 전파한다.
    ReleaseFields fields;std::size_t allow{},brains{},base{};TutorialMissionReleaseHooks hooks;
    hooks.setAllowAll=[&](auto enabled) { CHECK(enabled==1);++allow; };hooks.removeAllBrains=[&] { ++brains; };
    hooks.releaseBaseMission=[&] { ++base;CHECK((fields.Words()==std::array<std::uint32_t,6>{}));throw std::runtime_error("base interrupted"); };
    CHECK(ReleaseFailure([&] { ReleaseTutorialMission(fields.State(),hooks); })=="base interrupted");CHECK(allow==1 && brains==1 && base==1);
}
