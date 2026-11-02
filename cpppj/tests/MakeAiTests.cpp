// 실제 MakeAi/객체 이전·삭제의 독립 Native 바이트/호출 순서와 부분 실패를 검사한다.
#include "RawSceneSupport.h"
#include "o/RawMakeAi.h"
#include <array>
#include <bit>
#include <filesystem>
using namespace netstorm::o; // 기존 기본설정과 같은 실제 일반 부모를 사용한다.
using namespace netstorm::test::rawscene; // 고정 TSV의 숫자/바이트 해독만 공유한다.

namespace { // 실제 게임과 떨어진 현재 입력/trace 작성기다.
FixtureData MakeRows(std::string_view name) { return LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures"/name).string().c_str()); } // CWD와 무관한 최초 원문 기대값이다.
void AddMakeTrace(std::string& trace,std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 실제 경계 호출 순서를 보존한다.
}

TEST_CASE(MakeAi_Native770DefaultSettingsSpFtolAndOwnerZeroEnvelope) { //원문600BYTE 전체와 초기화/현재조회/두인자생성 순서를 대조한다.
    std::size_t count{};
    for (const auto& row:MakeRows("make-ai-parent-x86.tsv").rows) { // 제품 계산에서 기대 설정을 만들지 않는다.
        CHECK(row.size()==13);if (row.size()!=13) continue;std::array<std::uint8_t,600> bytes;bytes.fill(205);auto post=Number(row[2]);std::string trace,created;MissionAiSettingsState state{std::bit_cast<std::int32_t>(Number(row[8])),std::bit_cast<std::int32_t>(Number(row[9])),false};MissionAiSettingsHooks settings;settings.get=[](auto)->std::string { throw std::logic_error("일반MakeAi의미션override금지"); };RawMissionAiSettings initialize(OriginalEdition::Patch1078,state,settings);
        MakeAiReads reads;reads.battle=[&] { return Number(row[1]); };reads.missionPost=[&] { return post; };reads.playerKnown=[&] { return Number(row[3]); };reads.localOwner=[&] { return Number(row[4]); };reads.brain=[&](auto owner) { CHECK(owner==Number(row[4])+1);return Number(row[6]); };
        MakeAiHooks hooks;hooks.initialize=[&](auto raw) { AddMakeTrace(trace,"Init");initialize.Initialize(raw);if (row[7]!="-1") post=Number(row[7]); };hooks.stormPower=[&](auto owner) { AddMakeTrace(trace,"Sp:"+std::to_string(owner));return std::bit_cast<float>(Number(row[5])); };
        hooks.create=[&](auto owner,auto raw) { AddMakeTrace(trace,"Create:"+std::to_string(owner));created=Hex(raw);return 0x12345678U; };hooks.transferObjects=[&](auto owner) { AddMakeTrace(trace,"Transfer:"+std::to_string(owner)); };hooks.clearObjects=[&](auto mask) { AddMakeTrace(trace,"Clear:"+std::to_string(mask)); };
        MakeAi(Number(row[0]),bytes,reads,hooks);CHECK(Hex(bytes)==row[10] && (trace.empty()?"-":trace)==row[11] && (created.empty()?"-":created)==row[12]);++count;
    }
    CHECK(count==770);
}

TEST_CASE(MakeAi_Native2592ActualIteratorOwnerCountsRegionAndClearMask) { //현재 슬롯50BYTE/Player첫region/깊이와두카운터를원문몸체에서검증한다.
    std::size_t count{};
    for (const auto& row:MakeRows("make-ai-objects-x86.tsv").rows) { // 원문 입력의 각 현재 물리는 별개로 소유한다.
        CHECK(row.size()==14);if (row.size()!=14) continue;const auto local=Number(row[1]),kind=Number(row[6]);std::array<std::array<std::uint8_t,50>,6> slots{};std::array<std::uint8_t,1620> players{};std::array<std::uint32_t,256> localCounts{},builtCounts{};std::array<std::uint32_t,8> flags{};std::uint32_t depth=7;std::string trace;
        for (std::size_t index=0;index<slots.size();++index) { // Native에 명시한 같은 pointer 식별값/좌표/free 입력이다.
            Put(slots[index],0,0x10015000);slots[index][11]=1;Put(slots[index],14,std::bit_cast<std::uint32_t>(static_cast<float>(index)));Put(slots[index],18,std::bit_cast<std::uint32_t>(static_cast<float>(index)));
        }
        slots[2][10]=static_cast<std::uint8_t>(kind);slots[2][11]=static_cast<std::uint8_t>(Number(row[3])|Number(row[7]));slots[2][34]=static_cast<std::uint8_t>(Number(row[2]));flags[kind]=Number(row[4]);Put(players,local*180+0x78,41);Put(players,(local+1)*180+0x78,99);
        MakeAiObjectHooks h(depth);h.capacity=[] { return 6U; };h.localOwner=[&] { return local; };h.playerType=[] { return 4U; };h.bridgeType=[] { return 5U; };h.slot=[&](auto index)->std::span<const std::uint8_t> { return slots.at(index); };h.players=[&] { return std::span(players); };
        h.flags=[&](auto index) { const auto value=flags.at(slots.at(index)[10]);AddMakeTrace(trace,"Flags:"+std::to_string(index)+":"+std::to_string(value));return value; };
        h.region=[&](float x,float y) { CHECK(x==y && x==2.0F);const auto region=Number(row[5]) ? 41U : 42U;AddMakeTrace(trace,"Region:2:"+std::to_string(region));return region; };
        h.setOwner=[&](auto index,auto owner) { AddMakeTrace(trace,"Owner:"+std::to_string(index)+":"+std::to_string(owner)+":"+std::to_string(depth));slots.at(index)[34]=static_cast<std::uint8_t>(owner); };h.destroy=[&](auto index,auto value) { AddMakeTrace(trace,"Destroy:"+std::to_string(index)+":"+std::to_string(value)); };
        h.decrementLocal=[&](auto type) { AddMakeTrace(trace,"Local:"+std::to_string(type));--localCounts.at(type); };h.decrementBuilt=[&](auto type) { AddMakeTrace(trace,"Built:"+std::to_string(type));--builtCounts.at(type); };
        if (row[0]=="Transfer") TransferMakeAiObjects(local+1,h);else ClearMakeAiObjects(row[0]=="MakeClear" ? 0x2000U : Number(row[4]),h);
        CHECK(Hex(slots[2])==row[8] && Get(players,(local+1)*180+0x78)==Number(row[9]) && depth==Number(row[10]) && localCounts[kind]==Number(row[11]) && builtCounts[kind]==Number(row[12]) && (trace.empty()?"-":trace)==row[13]);++count;
    }
    CHECK(count==2592);
}

TEST_CASE(MakeAi_PartialFailureAndNextCandidateBeforeVirtualDeletion) { //실제effect실패를성공으로바꾸지않고원문쓰기/iterator의선행선택을유지한다.
    MakeAiReads reads;reads.battle=[] { return 0U; };MakeAi(1,{},reads,{});reads.battle=[] { return 1U; };std::array<std::uint8_t,600> settings{};MakeAiHooks hooks;hooks.initialize=[](auto raw) { raw[30]=77;throw std::runtime_error("명시초기화실패"); };CHECK(Throws([&] { MakeAi(1,settings,reads,hooks); }) && settings[30]==77);
    std::array<std::array<std::uint8_t,50>,5> slots{};slots[0][11]=slots[1][11]=1;std::uint32_t depth{};std::vector<std::uint32_t> destroyed;MakeAiObjectHooks objects(depth);objects.capacity=[] { return 5U; };objects.slot=[&](auto index)->std::span<const std::uint8_t> { return slots.at(index); };
    objects.flags=[&](auto index) { if (index==2) slots[3][11]=1;return 0x2000U; };objects.destroy=[&](auto index,auto value) { CHECK(!value);destroyed.push_back(index); };ClearMakeAiObjects(0x2000,objects);CHECK(destroyed==std::vector<std::uint32_t>({2,3,4})); //3번은선택뒤free가돼도다음본문에서새free관문을추가하지않는다.
    std::array<std::uint8_t,1620> players{};objects.capacity=[] { return 3U; };slots[2][34]=0;objects.localOwner=[] { return 1U; };objects.playerType=[] { return 4U; };objects.bridgeType=[] { return 5U; };objects.players=[&] { return std::span(players); };objects.region=[](auto,auto) { return 1U; };objects.flags=[](auto) { return 0x200000U; };std::size_t owners{};objects.setOwner=[&](auto,auto) { if (++owners==2) throw std::runtime_error("명시두번째Owner실패"); };CHECK(Throws([&] { TransferMakeAiObjects(2,objects); }) && owners==2 && depth==1); //원문깊이증가뒤예외는감소를합성하지않는다.
}
