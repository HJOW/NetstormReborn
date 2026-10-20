// 원본 옵션3305입력의 실제 float비트/현재TYPE/사용표/옵션바이트를 전수 대조한다.
#include "RawSceneSupport.h"
#include "o/RawMissionBattleOptions.h"
#include <set>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립 관측의16진 입력을 같은 순서의 바이트로 읽는다. 기대 출력은 이 함수로 계산하지 않는다.
std::vector<std::uint8_t> MissionOptionBytes(std::string_view text) {
    if (text.size()%2) throw std::invalid_argument("옵션 관측 입력의16진 길이 오류");std::vector<std::uint8_t> result;
    // 정확히 두 글자씩만 한 BYTE로 해석한다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));return result;
}
}
// 전체296개 최초 원문의 이웃/예약/현재 옵션·네 번째float와 실제 파생값을 비교한다. 제품 검사 실행은 후속 통합 단계로 이월한다.
TEST_CASE(MissionBattleOptions_ResetNative296) {
    const auto rows=LoadFixture(NETSTORM_MISSION_OPTION_RESET_FIXTURE).rows;CHECK(rows.size()==296);
    // 입력 저장소는 원본 전 관측 바이트이며 결과를 계산해 기대값으로 쓰지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==9);if (row.size()!=9) return;auto bytes=MissionOptionBytes(row[4]);CHECK(bytes.size()==103);auto raw=std::span<std::uint8_t>(bytes).subspan(8,87);
        std::vector<RiftTypeRecord> types(188);std::array<std::uint8_t,256> use{};std::array<float,4> table{14,22,30,std::bit_cast<float>(Number(row[3]))}; // 실제4float 입력과 같은 현재 타입/사용표다.
        std::uint32_t selected{},value{};float range{},squared{}; // 원본 두float/현재 비용 대상/보상값이다.
        RawMissionBattleOptions options(OriginalEdition::Patch1078,{raw,types,use,table,selected,range,squared,value});
        if (row[0]=="F") raw[4]=static_cast<std::uint8_t>(Number(row[2]));else ResetMissionBattleOptions(OriginalEdition::Patch1078,raw);
        if (row[0]=="P" || row[0]=="F") { options.UpdateRange();std::array<std::uint8_t,8> floats{};Put(floats,0,std::bit_cast<std::uint32_t>(range));Put(floats,4,std::bit_cast<std::uint32_t>(squared));CHECK(Hex(floats)==row[6]); }
        if (row[0]=="P") { CHECK(std::to_string(options.TypeCost())==row[7]);CHECK(std::to_string(options.CurrentValue())==row[8]); }
        CHECK(Hex(bytes)==row[5]);
    }
}
// 잘못된 판본/물리 크기는 쓰기 전에 거부하고4float의 범위 밖 선택도 명시 오류로 유지한다.
TEST_CASE(MissionBattleOptions_ResetRejectsBeforeWrite) {
    std::array<std::uint8_t,87> bytes;bytes.fill(0x5a);const auto original=bytes; // 거부 경로의 실제 쓰기 보존을 확인하는 입력이다.
    CHECK(Throws([&] { ResetMissionBattleOptions(OriginalEdition::Cd1072,bytes); }));CHECK(bytes==original);
    CHECK(Throws([&] { ResetMissionBattleOptions(OriginalEdition::Patch1078,std::span(bytes).first(86)); }));CHECK(bytes==original);
    CHECK(Throws([&] { ResetMissionBattleOptions(OriginalEdition::Patch1078,std::span<std::uint8_t>{}); }));CHECK(bytes==original);
}
namespace {
// 원본 관찰과 같은 입력 저장소를 만들되 결과 계산은 제품에 맡긴다.
struct BattleOptionFixture {
    std::array<std::uint8_t,87> options{}; // 동일한 현재 옵션87BYTE다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 실제 typed188TYPE다.
    std::array<std::uint8_t,264> use{}; // 현재256BYTE와 보존할 양쪽 이웃4BYTE다.
    std::array<float,3> table{}; // 현재3float의 원문 비트다.
    std::uint32_t selected{},value{0x12345678}; // 현재 대상TYPE와 파생값이다.
    float range{std::bit_cast<float>(0x418c0000U)},squared{std::bit_cast<float>(0x43992000U)}; // 현재 두 float 전역이다.
    // fixture 입력 수치만 공급하며 실제 원본 정상반환 출력을 참조하지 않는다.
    explicit BattleOptionFixture(const std::vector<std::string>& input) {
        const auto pattern=Number(input[5]);selected=Number(input[4]);
        // 원본과 같은87BYTE 입력을 전부 작성한다.
        for (std::size_t index=0;index<options.size();++index) options[index]=static_cast<std::uint8_t>(index*11+pattern);
        options[4]=static_cast<std::uint8_t>(Number(input[1]));options[0x10]=static_cast<std::uint8_t>(Number(input[2]));options[0xf]=static_cast<std::uint8_t>(Number(input[3]));
        // TYPE의 기존 비용 입력은 각각 다른 유한값이다.
        for (std::size_t index=0;index<types.size();++index) types[index].cost=100.25F+static_cast<float>(index);
        // 양쪽 이웃을 포함한 사용표 초기 바이트를 공급한다.
        for (std::size_t index=0;index<use.size();++index) use[index]=static_cast<std::uint8_t>(index*7+pattern);
        // NaN도 숫자 변환 없이 원문32비트로 보존한다.
        for (std::size_t index=0;index<table.size();++index) table[index]=std::bit_cast<float>(Number(input[index+6]));
    }
    // 모든 필드는 현재 원문을 빌리며 selected 변경도 다음 호출에 보인다.
    MissionBattleOptionState State() { return {options,types,std::span<std::uint8_t>(use).subspan(4,256),table,selected,range,squared,value}; }
    // 두 실제 float의 비트를 little endian 원문으로 읽는다.
    std::string RangeText() const { std::array<std::uint8_t,8> raw{};Put(raw,0,std::bit_cast<std::uint32_t>(range));Put(raw,4,std::bit_cast<std::uint32_t>(squared));return Hex(raw); }
    // 현재188TYPE 비용을 전부 읽어 선택 대상 밖의 보존도 대조한다.
    std::string CostText() const {
        std::array<std::uint8_t,188*4> raw{};
        // 타입 번호 순서대로 실제 typed cost필드만 읽는다.
        for (std::size_t index=0;index<types.size();++index) Put(raw,index*4,std::bit_cast<std::uint32_t>(types[index].cost));return Hex(raw);
    }
};
}
// x87 특수float와 실제0x67 부모의288조합을 독립 원본 출력과 비교한다.
TEST_CASE(MissionBattleOptions_Native3305) {
    std::set<std::string> unique;std::size_t count=0;
    // 행을 생략하지 않고 전체 원본 정상반환 기록을 대조한다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_BATTLE_OPTIONS_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==8 && unique.insert(row[0]).second);const auto input=Split(row[0],',');BattleOptionFixture fixture(input);RawMissionBattleOptions options(OriginalEdition::Patch1078,fixture.State());std::uint32_t result=0;
        if (input[0]=="Range") options.UpdateRange();
        else if (input[0]=="Cost") result=static_cast<std::uint32_t>(options.TypeCost());
        else if (input[0]=="Value") result=options.CurrentValue();
        else if (input[0]=="Clear") options.ClearTypes();
        else if (input[0]=="Apply") options.ApplyDerived();
        else CHECK(false);
        if (fixture.RangeText()!=row[2]) std::fprintf(stderr,"옵션 범위 불일치 입력=%s\n실제=%s\n기대=%s\n",row[0].c_str(),fixture.RangeText().c_str(),row[2].c_str());
        CHECK(std::to_string(result)==row[1]);CHECK(fixture.RangeText()==row[2]);CHECK(std::to_string(fixture.value)==row[3]);CHECK((input[0]=="Apply" ? fixture.CostText() : "-")==row[4]);CHECK((input[0]=="Clear" ? Hex(fixture.use) : "-")==row[5]);CHECK(Hex(fixture.options)==row[6]);++count;
    }
    CHECK(count==3305);
}
// 현재 옵션/선택TYPE/범위표의 변경을 생성자 사본이나 캐시가 가리지 못하게 한다.
TEST_CASE(MissionBattleOptions_CurrentStorageAndInvalidInputs) {
    BattleOptionFixture fixture(Split("Apply,0,0,0,1,17,1096810496,1102053376,1106247680",','));RawMissionBattleOptions options(OriginalEdition::Patch1078,fixture.State());options.ApplyDerived();CHECK(fixture.range==14.0F && fixture.types[1].cost==1000.0F && fixture.value==0);
    fixture.options[4]=1;fixture.options[0x10]=3;fixture.options[0xf]=5;fixture.selected=187;fixture.table[1]=18.0F;options.ApplyDerived();CHECK(fixture.range==18.0F && fixture.squared==324.0F && fixture.types[187].cost==5000.0F && fixture.types[1].cost==1000.0F && fixture.value==150);
    // Unicorn원본fixture와구분한현재CPU FLD/FSTP진단의두signaling NaN변환이다.
    for (const auto bits:std::array<std::uint32_t,2>{0x7f800001U,0xff800001U}) { fixture.table[1]=std::bit_cast<float>(bits);options.UpdateRange();CHECK(std::bit_cast<std::uint32_t>(fixture.range)==(bits|0x00400000U) && std::bit_cast<std::uint32_t>(fixture.squared)==(bits|0x00400000U)); }
    CHECK(Throws([&] { RawMissionBattleOptions wrong(OriginalEdition::Cd1072,fixture.State()); }));auto shortState=fixture.State();shortState.typeUse=shortState.typeUse.first(255);CHECK(Throws([&] { RawMissionBattleOptions wrong(OriginalEdition::Patch1078,shortState); }));
    fixture.options[4]=3;CHECK(Throws([&] { options.UpdateRange(); }));fixture.options[4]=0;fixture.options[0x10]=4;CHECK(Throws([&] { static_cast<void>(options.TypeCost()); }));fixture.options[0xf]=6;CHECK(Throws([&] { static_cast<void>(options.CurrentValue()); }));fixture.selected=188;CHECK(Throws([&] { options.SetSelectedTypeCost(1.0F); }));
}
