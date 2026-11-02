// 원문 직접 writer의 현재 Player/본체와 보존 바이트를 실제 소유 메모리에서 확인한다.
#include "RawSceneSupport.h"
#include "o/RawAiCommandState.h"
#include "o/RawAiBrain.h"
#include <array>
#include <bit>
#include <filesystem>
using namespace netstorm::o; // 기존 실제 AI 본체 소유자와 새 직접 writer를 함께 사용한다.
using namespace netstorm::test::rawscene; // 원문 TSV 해독과 비정렬 DWORD 입력 작성만 공유한다.

namespace { // 검사 전용 본문/Player 입력은 실제 게임/MP와 구별한다.
constexpr std::array<std::size_t,7> kCommandOffsets{0x220,0x228,0x230,0x234,0x24c,0x2d8,0x600}; // 독립 Native fixture의 전체 출력 DWORD 순서다.
// 선택한 fixture를 실제 소스 위치로 읽어 CWD에 의존하지 않는다.
FixtureData CommandStateRows() { return LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/ai-command-state-x86.tsv").string().c_str()); }
}

// 1940행을 실제 같은 소유 본체에 기록하고 나머지 모든 바이트/Player 포인터를 확인한다.
TEST_CASE(AiCommandState_Native1940RowsAndWholeBodyPreservation) {
    std::size_t count{};
    for (const auto& row:CommandStateRows().rows) { // 기대값은 원문 관측이며 제품 출력에서 만들지 않는다.
        CHECK(row.size()==6);if (row.size()!=6) continue;std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory(0x4f000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); });std::array<std::uint32_t,9> tokens{};
        for (std::size_t owner=0;owner<9;++owner) { // 아홉 실제 본체는 owner0 보존과 selector0 재조회 대상을 포함한다.
            tokens[owner]=memory.Allocate(RawAiBrainConstructor::kBytes);auto raw=memory.Bytes(tokens[owner]);std::fill(raw.begin(),raw.end(),static_cast<std::uint8_t>(0xa0+owner));
            if (!owner || (Number(row[1])&(1U<<(owner-1)))) Put(players,owner*180+0xb0,tokens[owner]);
        }
        const auto packed=Number(row[2]),selected=packed&7U;bool flipped{};std::size_t bodyCalls{};
        AiCommandStateHooks hooks;hooks.players=[&] { return std::span<const std::uint8_t>(players); };hooks.body=[&](auto token) { ++bodyCalls;if (row[3]=="1" && !flipped) { Put(players,selected*180+0xb0,tokens[8]);flipped=true; }return memory.Bytes(token); };
        if (row[0]=="Money") CrankAiMoney(hooks);else SetAiParameter(packed,hooks);
        const auto owners=Split(row[4],'/');CHECK(owners.size()==9);
        for (std::size_t owner=0;owner<owners.size() && owner<9;++owner) { // 7DWORD 이외의 실제0xb08 바이트도 보존해야 한다.
            const auto fields=Split(owners[owner],',');CHECK(fields.size()==7);if (fields.size()!=7) continue;std::vector<std::uint8_t> expected(RawAiBrainConstructor::kBytes,static_cast<std::uint8_t>(0xa0+owner));
            for (std::size_t field=0;field<fields.size();++field) Put(expected,kCommandOffsets[field],Number(fields[field])); // 기대 변경은 원문 fixture에서 읽은 DWORD뿐이다.
            CHECK(std::equal(expected.begin(),expected.end(),memory.Bytes(tokens[owner]).begin()));
            const auto attached=!owner || (Number(row[1])&(1U<<(owner-1)));const auto expectedToken=flipped && owner==selected ? tokens[8] : attached ? tokens[owner] : 0U;CHECK(Get(players,owner*180+0xb0)==expectedToken);
        }
        CHECK(memory.Allocations().size()==9 && memory.Frees().empty());if (row[3]=="1") CHECK(flipped && bodyCalls==2);++count;
    }
    CHECK(count==1940);
}

// owner0/없는 brain/미지원 selector는 선택하지 않은 body 경계를 요구하지 않는다.
TEST_CASE(AiCommandState_LazyParentsCurrentPointersAndExplicitPhysicalFailures) {
    SetAiParameter(0,{});SetAiParameter(8,{});std::array<std::uint8_t,1620> players{};AiCommandStateHooks hooks;std::size_t reads{};hooks.players=[&] { ++reads;return std::span<const std::uint8_t>(players); };
    CrankAiMoney(hooks);CHECK(reads==8);SetAiParameter(1,hooks);CHECK(reads==9);Put(players,180+0xb0,0xdeadbeef);SetAiParameter(1|(255U<<4),hooks);CHECK(reads==10);
    CHECK(Throws([&] { SetAiParameter(1,hooks); }));std::array<std::uint8_t,8> shortBody{};hooks.body=[&](auto) { return std::span<std::uint8_t>(shortBody); };CHECK(Throws([&] { CrankAiMoney(hooks); }));
    hooks.players=[] { return std::span<const std::uint8_t>{}; };CHECK(Throws([&] { SetAiParameter(1,hooks); }));CHECK(Throws([&] { CrankAiMoney({}); }));
}

// 앞선 본체 해석의 후속 Player 변경을 다음 money 방문이 현재 포인터로 읽는다.
TEST_CASE(AiCommandState_MoneyVisitsCurrentLaterBrainAndStopsAtFailure) {
    std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory(0x4f000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x7b}); });const auto first=memory.Allocate(RawAiBrainConstructor::kBytes),second=memory.Allocate(RawAiBrainConstructor::kBytes);Put(players,180+0xb0,first);
    AiCommandStateHooks hooks;hooks.players=[&] { return std::span<const std::uint8_t>(players); };hooks.body=[&](auto token) { if (token==first) Put(players,2*180+0xb0,second);return memory.Bytes(token); };CrankAiMoney(hooks);CHECK(Get(memory.Bytes(first),0x2d8)==30000 && Get(memory.Bytes(second),0x2d8)==30000);
    Put(memory.Bytes(first),0x2d8,17);Put(memory.Bytes(second),0x2d8,19);hooks.body=[&](auto token) { if (token==second) throw std::runtime_error("명시 두 번째 현재 본체 실패");return memory.Bytes(token); };CHECK(Throws([&] { CrankAiMoney(hooks); }));CHECK(Get(memory.Bytes(first),0x2d8)==30000 && Get(memory.Bytes(second),0x2d8)==19);
}
