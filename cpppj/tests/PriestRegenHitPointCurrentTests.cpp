// 원문 회복 HP 부모와 같은 UI TYPE reader만 검사한다. 게임/포획/희생/공격/MP는 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawPriestRegenHitPoints.h"
#include "client/CurrentTypeHitPoints.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

TEST_CASE(PriestRegenHitPointCurrent_Native864NumberHpAndRepeatedGetterOrder) { // 양판본의 Getter/번호/HP trace와 Setter 인자를 모두 원문과 대조한다.
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_REGEN_HP_CURRENT_FIXTURE).rows) { // 원문 입력과 독립 출력만 읽는다.
        CHECK(row.size()==6);const auto input=Split(row[1],',');CHECK(input.size()==7);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const bool patch=edition==OriginalEdition::Patch1078;std::int32_t hp=std::stoi(input[0]);if (!patch) hp=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(hp));std::uint32_t number=74;
        const std::array<std::int32_t,3> actor{std::stoi(input[1]),std::stoi(input[2]),std::stoi(input[3])};const std::array<std::int32_t,2> type{std::stoi(input[4]),std::stoi(input[5])};const auto profile=Number(input[6]);std::size_t actorCalls{},typeCalls{};std::string trace;
        const auto emit=[&](std::string event) { if (!trace.empty()) trace+=';';trace+=event; }; // 실제 원문 장치/메모리 읽기 순서다.
        const auto mutate=[&](bool isActor,std::size_t count) { // Native의 명시 자식 복귀 변경 입력을 그대로 공급한다.
            if (!profile) return;
            if (profile==1) { hp=isActor ? 90-static_cast<std::int32_t>(count)*17 : -13+static_cast<std::int32_t>(count)*40;number=static_cast<std::uint32_t>((isActor ? 80 : 90)+count); }
            else { hp=isActor ? (patch ? 0x7ffffff0+static_cast<std::int32_t>(count) : 32767-static_cast<std::int32_t>(count)*100) : (patch ? -2147483647-1+static_cast<std::int32_t>(count) : -32768+static_cast<std::int32_t>(count));number=static_cast<std::uint32_t>((isActor ? 110 : 120)+count); }
            if (!patch) hp=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(hp));
        };
        PriestRegenHitPointHooks hooks;hooks.current=[&] { emit("H"+std::to_string(hp));return hp; };hooks.priestType=[&] { emit("N"+std::to_string(number));return number; };
        hooks.maximum=[&] { CHECK(actorCalls<actor.size());const auto value=actor[actorCalls++];emit("A"+std::to_string(value));mutate(true,actorCalls);return value; };
        hooks.typeMaximum=[&](auto selected) { CHECK(typeCalls<type.size());const auto value=type[typeCalls++];emit("T"+std::to_string(selected)+':'+std::to_string(value));mutate(false,typeCalls);return value; };
        const auto value=ReadPriestRegenHitPoints(edition,hooks);const auto actual=value ? std::to_string(*value) : "-";
        const bool same=actual==row[2] && trace==row[3] && hp==std::stoi(row[4]) && number==Number(row[5]);if (!same) { std::printf("%s %s\nactual %s %s\nnative %s %s\n",row[0].c_str(),row[1].c_str(),actual.c_str(),trace.c_str(),row[2].c_str(),row[3].c_str());CHECK(same);return; }++rows;
    }
    CHECK(rows==864);
}

TEST_CASE(CurrentTypeHitPointReader_Native1158CurrentGlobalsAndSavedPhysicalType) { // UI 번호 API의 범위 안에 있는 기존 독립 HP 관측을 그대로 대조한다.
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_UI_HP_CURRENT_FIXTURE).rows) { // 초기 음수 getNum2행은 직접 getter1160검사에 보존한다.
        CHECK(row.size()==6);const auto input=Split(row[1],',');CHECK(input.size()==9);if (std::stoi(input[7])) continue;
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const auto count=edition==OriginalEdition::Patch1078 ? 188U : 171U;const auto number=Number(input[0]);std::vector<RiftTypeRecord> physical(190);auto& selected=physical[1+number];selected.maxHitPoints=std::stoi(input[1]);selected.flags1=Number(input[2]);
        std::uint32_t checking=Number(input[3]),priest=Number(input[5]),dais=Number(input[6]);bool quarter=Number(input[4])!=0;std::int32_t shift{};const auto profile=Number(input[8]);std::string diagnostics;
        auto reader=MakeCurrentTypeHitPoints(edition,[&] { return std::span<const RiftTypeRecord>(physical).subspan(static_cast<std::size_t>(1+shift),count); },{checking,quarter,priest,dais},[&](auto expression,auto file,auto line) {
            if (!diagnostics.empty()) diagnostics+=';';diagnostics+=std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line);
            if (profile) { checking=0;quarter=true;selected.maxHitPoints=std::array<std::int32_t,3>{-13,-37,41}[profile-1];shift=std::array<std::int32_t,3>{0,1,-1}[profile-1];priest=static_cast<std::uint32_t>(74-shift); }
        });
        CHECK(reader(number)==std::stoi(row[2]) && (diagnostics.empty() ? "-" : diagnostics)==row[3]);++rows;
    }
    CHECK(rows==1158);
}

TEST_CASE(PriestRegenHitPointCurrent_UnselectedDevicesAndMissingReadPrefix) { // 회복 없는 분기는 TYPE장치를 요구하지 않고 실제 실패 위치를 보존한다.
    PriestRegenHitPointHooks hooks;std::string trace;hooks.current=[&] { trace+='H';return 50; };hooks.maximum=[&] { trace+='A';return 40; };
    CHECK(!ReadPriestRegenHitPoints(OriginalEdition::Patch1078,hooks) && trace=="HA");trace.clear();hooks.maximum=[&] { trace+='A';return 100; };CHECK(Throws([&] { ReadPriestRegenHitPoints(OriginalEdition::Patch1078,hooks); }) && trace=="HA");
    std::uint32_t priest=74,dais=165;auto absent=MakeCurrentTypeHitPoints(OriginalEdition::Patch1078,{},{false,false,priest,dais},{});CHECK(Throws([&] { absent(74); }));
}
