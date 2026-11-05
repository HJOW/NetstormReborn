// 공통 레벨/다이스 Count의 읽기 정의역을 명시 계약으로 검사한다. 새 CPU 관측이나 실제 의식 실행 검사가 아니다.
#include "TestSupport.h"
#include "o/RawSquidLevel.h"
#include "o/RawTempleRitual.h"
#include <algorithm>
#include <array>
#include <bit>
#include <memory>
#include <stdexcept>

using namespace netstorm::o;
namespace { // 검사 소유 raw 입력과 효과를 거부하는 최소 실제 모듈 연결을 공유한다.
std::span<std::uint8_t> CountInput(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free/예약 슬롯도 같은 물리 범위의 검사 입력으로만 수정한다.
void CountPut(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value,std::size_t width) { // 판본별 실제 필드 폭으로 입력을 쓴다.
    for (std::size_t byte=0;byte<width;++byte) raw[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 인접 CD extra와 stale 바이트를 보존한다.
}

template<class Function> bool CountRangeFailed(Function function) { try { function(); }catch (const std::out_of_range&) { return true; }return false; } // 일반 실패와 물리 TYPE/프레임 범위 오류를 구별한다.
struct ReadOnlyDaisCount { // 의식은 실행하지 않고 기존 Count의 실제 의존 모듈만 같은 풀에 구성한다.
    SidPool pool; // 두 판본의 실제 raw 슬롯 폭/예약 영역을 보유한다.
    SquidHash hash; // Count에서 변경하지 않아야 하는 실제 공간 저장소다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536,0xa5); // 변경 감지를 위한 전체 점유 지도 입력이다.
    std::vector<RiftTypeRecord> types; // 현재 TYPE 번호의 물리 범위를 정한다.
    std::vector<RiftTypeFrames> frames; // 호출마다 현재 TYPE로 조회하는 실제 프레임 코드 배열이다.
    std::vector<SquidDisplayShape> shapes; // Count가 조회하거나 변경하지 않는 모듈 생성 의존 자료다.
    SquidPostPopState bookkeeping; // 보상 모듈이 빌리는 같은 장부이며 Count는 갱신하지 않는다.
    SquidRewardState rewardState; // 같은 보상 상태의 최초 입력이다.
    GameRandom random{0x12345678}; // Count에서 난수 소비가 없어야 한다.
    std::size_t effects{}; // 금지된 의식/생성/음향/예약 장치 호출 횟수다.
    std::unique_ptr<ScrambledSpStore> store; // 패치 보상 생성자에 필요한 미초기화 저장소다.
    std::unique_ptr<RawSquidNeighbors> footprint; // 같은 실제 풀/해시/타입을 보상·의식 모듈에 제공한다.
    std::unique_ptr<SquidReward> reward; // 실제 보상 모듈이며 Count에서 경제 메서드를 호출하지 않는다.
    std::unique_ptr<RawPriestRitualActions> actions; // 모든 동작 장치가 실패하는 의식 의존 모듈이다.
    TempleRitualState state; // 읽기 전용 조회와 구별되는 의식 상태다.
    std::unique_ptr<RawTempleRitual> ritual; // 검사에서 호출하는 메서드는 Count뿐이다.

    [[noreturn]] void ForbiddenEffect() { ++effects;throw std::logic_error("Count 읽기 검사에서 의식/생성/음향/예약 효과 호출 금지"); } // 잘못된 부수 효과를 빈 성공으로 숨기지 않는다.
    explicit ReadOnlyDaisCount(OriginalEdition edition):pool(edition,24000),types(edition==OriginalEdition::Patch1078 ? 188U : 171U),frames(types.size(),RiftTypeFrames({})),shapes(types.size()) { // 기존 TempleRitual 검사와 같은 실제 생성자 요구 사항만 충족한다.
        store=std::make_unique<ScrambledSpStore>(random,[this]() -> std::uint32_t { ForbiddenEffect(); });
        footprint=std::make_unique<RawSquidNeighbors>(pool,hash,spots,types,frames);
        reward=std::make_unique<SquidReward>(pool,types,bookkeeping,rewardState,edition==OriginalEdition::Patch1078 ? store.get() : nullptr);
        PriestRitualActionHooks actionHooks;actionHooks.setFrame=[this](Sid,std::int32_t,std::uint32_t) { ForbiddenEffect(); };actionHooks.reserveShared=[this]() -> bool { ForbiddenEffect(); };actionHooks.addShared=[this](Sid,std::uint32_t,float) { ForbiddenEffect(); }; // 사제 프레임과604 예약은 모두 실행 금지다.
        actions=std::make_unique<RawPriestRitualActions>(pool,frames,std::move(actionHooks));TempleRitualHooks hooks;
        hooks.create=[this](std::uint32_t,std::uint32_t) -> Sid { ForbiddenEffect(); };hooks.setHitPoints=[this](Sid,std::int32_t) { ForbiddenEffect(); };hooks.setOwner=[this](Sid,std::uint32_t) { ForbiddenEffect(); }; // 생성/HP/소유자 쓰기를 모두 거부한다.
        hooks.pop=[this](Sid,float,float,std::uint32_t) { ForbiddenEffect(); };hooks.playAt=[this](float,float,std::string_view,std::uint32_t) { ForbiddenEffect(); };hooks.reserveShared=[this]() -> bool { ForbiddenEffect(); };hooks.addShared=[this](Sid,std::uint32_t,float) { ForbiddenEffect(); }; // 공간/소리/의식 예약을 모두 거부한다.
        ritual=std::make_unique<RawTempleRitual>(pool,hash,types,frames,shapes,spots,*footprint,*reward,*actions,state,std::move(hooks));
    }
    void Direction(std::uint32_t type,std::uint8_t side) { // 원문 Count에 공급할 코드 배열만 교체하며 게임 프레임 setter를 호출하지 않는다.
        std::vector<FrameCode> codes(180);
        for (std::size_t frame=0;frame<codes.size();++frame) codes[frame]={side,'P',static_cast<std::uint8_t>(frame+1),0}; // 각 방향을 모든 경계 프레임에서 독립적으로 시험한다.
        frames.at(type)=RiftTypeFrames(std::move(codes));
    }
};
struct CountDirectionRow { // 방향 우선과 프레임 경계의 기대값은 제품 분기 대신 수동 상수표로 정한다.
    std::uint8_t side; // Count가 읽는 원문 signed BYTE 방향 코드다.
    std::array<std::uint32_t,6> counts; // 프레임0/63/64/119/120/179의 직접 기대값이다.
};
constexpr std::array<std::uint32_t,6> kCountFrames{0,63,64,119,120,179}; // 세 개수 구간의 양 끝을 명시한다.
constexpr std::array<CountDirectionRow,5> kCountDirections{{ // N/M/L은 프레임 경계보다 우선하며 I/음수 방향은 경계를 따른다.
    {'I',{1,1,2,2,3,3}},{'N',{1,1,1,1,1,1}},{'M',{2,2,2,2,2,2}},{'L',{3,3,3,3,3,3}},{0x80,{1,1,2,2,3,3}}
}};
}

TEST_CASE(CurrentCountGetter_BaseLevelAllStatesAndReservedSlotsAreReadOnly) { // 전체 상태BYTE의 모든 조합에서 Has/Get은 원문 정의역을 유지한다.
    std::size_t rows{}; // 실제 실행한 명시 입력 행 수이며 원본 CPU fixture 수가 아니다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 판본의 예약/일반 슬롯을 각각 읽는다.
        SidPool pool(edition,24000);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);std::size_t diagnostics{};RawSquidLevel level(pool,types,[&](SquidLevelAssertion) { ++diagnostics;throw std::logic_error("읽기 getter는 setter 진단을 호출하지 않는다"); });
        const std::array<Sid,7> slots{{Sid{0},Sid{1},Sid{4},Sid{5},Sid{static_cast<std::uint16_t>(pool.Layout().serverFirst)},Sid{static_cast<std::uint16_t>(pool.Layout().predictableFirst)},Sid{23999}}};const auto free=pool.FreeCount(); // 첫 예약 영역/일반 client/server/예측 머리와 풀 끝을 포함한다.
        for (const auto sid:slots) { // 할당/반납을 실행하지 않고 물리 슬롯 입력만 바꾼다.
            auto raw=CountInput(pool,sid);raw[10]=74;std::vector<std::uint8_t> expected(pool.Bytes().begin(),pool.Bytes().end());const auto stateOffset=static_cast<std::size_t>(sid.value)*pool.Layout().stride+11;
            for (std::uint32_t value=0;value<8;++value) { // 원문의 상위3비트 기대값을 직접 입력한다.
                for (std::uint32_t flags=0;flags<32;++flags) { // free/dead/void/contained와전송중을 포함한 하위5비트 전체 조합이다.
                    raw[11]=static_cast<std::uint8_t>((value<<5)|flags);expected[stateOffset]=raw[11];CHECK(level.Has(sid));CHECK(level.Get(sid)==value);CHECK(std::equal(expected.begin(),expected.end(),pool.Bytes().begin()));++rows; // getter는 전체 풀의 어떤 BYTE도 쓰지 않아야 한다.
                }
            }
        }
        CHECK(diagnostics==0 && pool.FreeCount()==free);
    }
    CHECK(rows==3584);
}

TEST_CASE(CurrentCountGetter_DaisCurrentTypesDirectionsAndBoundariesHaveNoEffects) { // 비165 TYPE도 현재 코드 배열을 읽으며 상태 필터나 의식 효과를 추가하지 않는다.
    std::size_t rows{}; // 명시 행렬의 실제 Count 호출 수다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 프레임 DWORD/BYTE 폭을 두 판본에서 각각 검사한다.
        ReadOnlyDaisCount scene(edition);const Sid sid{50};auto raw=CountInput(scene.pool,sid);raw[35]=0xe7;const auto initialSpots=scene.spots;const auto initialRandom=scene.random.State();const auto free=scene.pool.FreeCount(); // CD 프레임 바로 뒤 stale BYTE는 해석하면 안 된다.
        for (const auto type:{74U,165U}) { // 실제 가상 호출의 현재 TYPE는 고정165와 독립적이다.
            raw[10]=static_cast<std::uint8_t>(type);
            for (const auto& row:kCountDirections) { // 분기 복제 계산 없이 수동 표의 기대값에 대조한다.
                scene.Direction(type,row.side);std::vector<std::uint8_t> expected(scene.pool.Bytes().begin(),scene.pool.Bytes().end());auto expectedRaw=std::span(expected).subspan(static_cast<std::size_t>(sid.value)*scene.pool.Layout().stride,scene.pool.Layout().stride);
                for (std::size_t index=0;index<kCountFrames.size();++index) { // 여섯 경계 프레임을 입력할 뿐 게임 프레임을 진행하지 않는다.
                    const bool patch=edition==OriginalEdition::Patch1078;CountPut(raw,patch ? 36U : 34U,kCountFrames[index],patch ? 4U : 1U);CountPut(expectedRaw,patch ? 36U : 34U,kCountFrames[index],patch ? 4U : 1U);
                    for (std::uint32_t flags=0;flags<16;++flags) { // free/dead/void/contained의 모든 조합을 그대로 읽는다.
                        raw[11]=static_cast<std::uint8_t>(0xe0U|flags);expectedRaw[11]=raw[11];CHECK(scene.ritual->Count(sid)==row.counts[index]);CHECK(std::equal(expected.begin(),expected.end(),scene.pool.Bytes().begin()));++rows; // 상태 레벨7과 무관한 Count와 전체 풀 불변을 함께 대조한다.
                    }
                }
            }
        }
        CHECK(scene.effects==0 && scene.random.State()==initialRandom && !scene.store->Initialized() && scene.spots==initialSpots && scene.pool.FreeCount()==free);
        for (int level=0;level<4;++level) CHECK(std::all_of(scene.hash.Entries(level).begin(),scene.hash.Entries(level).end(),[](std::uint16_t value) { return value==0; })); // 공간 해시도 초기 전체0을 유지해야 한다.
    }
    CHECK(rows==1920);
}

TEST_CASE(CurrentCountGetter_TypeAndFrameBoundsFailWithoutWritesOrEffects) { // 허용 정의역 확대가 물리 범위 밖 읽기까지 허용하지는 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 판본의 잘못된 TYPE/프레임을 같은 read-only 계약으로 검사한다.
        ReadOnlyDaisCount scene(edition);const Sid sid{50};auto raw=CountInput(scene.pool,sid);RawSquidLevel level(scene.pool,scene.types,[](SquidLevelAssertion) { throw std::logic_error("범위 검사에서 setter 진단 호출 금지"); });
        for (const auto type:{static_cast<std::uint32_t>(scene.types.size()),255U}) { // 첫 범위 밖 번호와 BYTE 최댓값을 구분한다.
            raw[10]=static_cast<std::uint8_t>(type);const auto expected=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());CHECK(CountRangeFailed([&] { level.Has(sid); }) && CountRangeFailed([&] { level.Get(sid); }) && CountRangeFailed([&] { scene.ritual->Count(sid); }));CHECK(std::equal(expected.begin(),expected.end(),scene.pool.Bytes().begin()));
        }
        scene.Direction(74,'I');raw[10]=74;const bool patch=edition==OriginalEdition::Patch1078;
        for (const auto frame:{180U,0xffffffffU}) { // CD는후자의하위BYTE255,패치는signed음수-1을소비하여기존프레임보호를유지한다.
            CountPut(raw,patch ? 36U : 34U,frame,patch ? 4U : 1U);const auto expected=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());CHECK(CountRangeFailed([&] { scene.ritual->Count(sid); }));CHECK(std::equal(expected.begin(),expected.end(),scene.pool.Bytes().begin()));
        }
        raw[10]=75;CountPut(raw,patch ? 36U : 34U,0,patch ? 4U : 1U);const auto expected=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());CHECK(CountRangeFailed([&] { scene.ritual->Count(sid); }));CHECK(std::equal(expected.begin(),expected.end(),scene.pool.Bytes().begin()) && scene.effects==0 && !scene.store->Initialized()); // 정상 TYPE라도 코드 배열이 비어 있으면 명시 범위 실패다.
    }
}

TEST_CASE(CurrentLevelSetter_PhysicalFreeAndReservedSlotsPreservePoolLists) { // 원문 base+84는 유효한 물리 슬롯이면 예약/반납 상태에도 상태 BYTE만 쓰고 자유 목록은 갱신하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 양판본의 같은 +b 쓰기와 서로 다른 풀 배치에서 확인한다.
        SidPool pool(edition,24000);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);RawSquidLevel level(pool,types,[](SquidLevelAssertion) { throw std::logic_error("정상 물리 레벨 쓰기의 예상 밖 진단"); });
        for (const auto number:{0U,1U,4U,5U,pool.Layout().serverFirst,pool.Layout().predictableFirst,pool.Capacity()-1}) { // 예약0/1/4·반납5·서버/예측 경계·마지막 슬롯의 실제 번호만 사용한다.
            const Sid sid{static_cast<std::uint16_t>(number)};auto raw=CountInput(pool,sid);raw[10]=82;raw[11]=1;auto expected=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());expected[static_cast<std::size_t>(number)*pool.Layout().stride+11]=0x61;const auto free=pool.FreeCount();const auto first=pool.FirstFree(false),client=pool.FirstFree(true),tail=pool.Tail(false);level.Set(sid,3);CHECK(pool.Slot(sid)[11]==0x61 && level.Get(sid)==3);CHECK(std::equal(expected.begin(),expected.end(),pool.Bytes().begin()) && pool.FreeCount()==free && pool.FirstFree(false)==first && pool.FirstFree(true)==client && pool.Tail(false)==tail);
        }
        const Sid last{static_cast<std::uint16_t>(pool.Capacity()-1)};CountInput(pool,last)[10]=255;CHECK(CountRangeFailed([&] { level.Set(last,3); }));CHECK(CountRangeFailed([&] { level.Set(Sid{static_cast<std::uint16_t>(pool.Capacity())},3); })); // 물리 TYPE/SID 범위는 원문 대응 배열의 호스트 경계로 유지한다.
    }
}

TEST_CASE(CurrentLevelSetter_DiagnosticReleaseKeepsCurrentByteAndWrittenPrefix) { // 범위 진단 뒤 반납된 같은 슬롯에 현재 하위 비트를 읽어 쓰며 Has/TYPE를 다시 검사하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 원문 두 진단 파일/행을 유지한 채 실제 Release 뒤 상태를 확인한다.
        SidPool pool(edition,24000);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);raw[10]=82;raw[11]=4;const auto free=pool.FreeCount();std::size_t calls{};std::vector<std::uint8_t> expected;
        RawSquidLevel level(pool,types,MakeSquidLevelAssertion(edition,[&](std::string_view expression,std::string_view file,std::uint32_t line) { CHECK(expression=="0 <= n && n < 8" && file==(edition==OriginalEdition::Patch1078 ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp") && line==(edition==OriginalEdition::Patch1078 ? 0x579U : 0x562U));++calls;pool.Release(sid);CHECK(pool.Slot(sid)[11]==3);CountInput(pool,sid)[10]=255;expected.assign(pool.Bytes().begin(),pool.Bytes().end());expected[static_cast<std::size_t>(sid.value)*pool.Layout().stride+11]=0xe3; })); // 실제 일반 반납의 free|dead 두 비트3을 원문대로 보존한다.
        level.Set(sid,0xffffffffU);CHECK(calls==1 && pool.FreeCount()==free+1 && pool.Slot(sid)[10]==255 && pool.Slot(sid)[11]==0xe3 && std::equal(expected.begin(),expected.end(),pool.Bytes().begin()));
    }
}
