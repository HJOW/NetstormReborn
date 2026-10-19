// 독립 원본1420입력의 네 주문 자격/효과·전체 슬롯·공유 난수를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBombStrike.h"
#include <filesystem>
#include <iostream>

using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint32_t,4> kStrikeTables{0x502908,0x5029d0,0x502aa8,0x502b70}; // 독립 실행기가 보호PE에서 확인한 실제 표다.
constexpr std::uint32_t kStrikeTarget=0x501dd0,kStrikeAlt=0x20000800; // 현재 피해/삭제 가상 경계를 구별하는 원문 토큰이다.
constexpr std::array<std::uint16_t,5> kStrikeSlots{0,50,51,60,70}; // 전체 바이트를 동결한 물리 슬롯이다.
// 제품 계산과 구별되는 독립 raw/TYPE·큰 효과 입력 장면이다.
struct StrikeScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 현재 부모/대상/QA/생성물의 같은 저장소다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 자격/발자국 속성이다.
    GameRandom random; // 제품이 직접 소비하는 실제 게임 난수다.
    BombStrikeState state; // 명시 효과가 바꿀 수 있는 현재 권한/종류다.
    std::vector<std::uint32_t> input; // 원문의20DWORD 입력이다.
    std::string trace; // 큰 효과의 순서와 전체DWORD 인자다.
    std::uint32_t projects{}; // 투영 경계가 매번 다른 결과를 주는 독립 입력 카운터다.
    // 물리 입력과 선언한 재진입 효과만 쓴다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 부호 있는 화면 좌표도 원문DWORD로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 실제 인자 순서와 전체 비트를 유지한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 독립 원문 입력을 같은 원본 폭으로 설치한다.
    void Prepare(std::string_view text) {
        input.clear();trace.clear();state=BombStrikeState{};projects=0;
        // signed 입력도 하위DWORD 비트로 읽는다.
        for (const auto& value:Split(text,',')) input.push_back(Number(value));
        CHECK(input.size()==20);if (input.size()!=20) throw std::invalid_argument("직접 피해 주문 원문 입력 개수");
        std::fill(types.begin(),types.end(),RiftTypeRecord{});
        // 원문의 전체 관측 슬롯을 매 입력 초기화한다.
        for (const auto sid:kStrikeSlots) std::fill(Raw(Sid{sid}).begin(),Raw(Sid{sid}).end(),std::uint8_t{});
        auto parent=Raw(Sid{50});Put(parent,0,kStrikeTables[input[1]]);parent[10]=168;parent[34]=static_cast<std::uint8_t>(input[9]);Put(parent,12,input[7],2);Put(parent,14,input[14]);Put(parent,18,input[15]);
        // 기본 두 후보는 QA 생존 조회도 같은 raw를 읽게 한다.
        for (const auto sid:{51,60}) { auto raw=Raw(Sid{static_cast<std::uint16_t>(sid)});Put(raw,0,kStrikeTarget);raw[10]=70; }
        const Sid target{static_cast<std::uint16_t>(input[8])};auto raw=Raw(target);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kStrikeTarget);raw[10]=static_cast<std::uint8_t>(input[2]);raw[11]=static_cast<std::uint8_t>(input[6]);raw[40]=static_cast<std::uint8_t>(input[5]);Put(raw,14,input[16]);Put(raw,18,input[17]);Put(raw,26,input[10]);Put(Raw(Sid{70}),0,kStrikeTarget);
        auto& type=types[input[2]];type.flags1=input[4];type.flags2=input[3];type.footX=std::bit_cast<std::int32_t>(input[18]);type.footY=std::bit_cast<std::int32_t>(input[19]);types[168].footX=1;types[168].footY=3;
        state.boss=input[11];random.SetState(input[12]);
    }
    // 큰 실제 하위의 명시 입력/재진입만 공급하며 제품 분기를 계산하지 않는다.
    BombStrikeHooks Hooks() {
        BombStrikeHooks hooks;
        // 생성은 현재 대상/부모를 바꿀 수 있으며 중심 캡처 순서를 구별한다.
        hooks.create=[this](std::uint32_t kind,std::uint32_t flags) {
            Event("create",{kind,flags});const Sid target{static_cast<std::uint16_t>(input[8])};
            if (input[13]==1) Raw(target)[10]=138;
            if (input[13]==2) { Put(Raw(target),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(target),18,std::bit_cast<std::uint32_t>(201.0F)); }
            if (input[13]==3) { Raw(Sid{50})[34]=3;Put(Raw(target),0,kStrikeAlt); }
            return Sid{70};
        };
        // 원문은 생성 뒤 현재 가상90을 요청한다.
        hooks.pop=[this](Sid created,std::uint32_t table,float x,float y,std::uint32_t flags) {
            CHECK(created.value==70 && table==kStrikeTarget);Event("pop",{created.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags});const Sid target{static_cast<std::uint16_t>(input[8])};
            if (input[13]==4) { Put(Raw(target),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(target),18,std::bit_cast<std::uint32_t>(201.0F)); }
            if (input[13]==5) { Raw(target)[10]=138;state.boss=0; }
            if (input[13]==6) { Raw(target)[10]=70;Put(Raw(target),0,kStrikeAlt);Raw(Sid{50})[34]=4; }
        };
        // 음향 뒤dead/QA/owner를 바꾸는 명시 재진입이다.
        hooks.sound=[this](float x,float y) {
            Event("sound",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),0});
            if (input[13]==7) Raw(Sid{static_cast<std::uint16_t>(input[8])})[11]=2;
            if (input[13]==8) { Put(Raw(Sid{50}),12,0,2);Raw(Sid{50})[34]=6; }
        };
        // 실제 피해몸체 대신 선언한HP 감김/변경만 입력으로 준다.
        hooks.damage=[this](Sid target,std::uint32_t table,std::int32_t amount,std::uint8_t owner,Sid parent) {
            CHECK(parent.value==50 && target.value==input[8] && (table==kStrikeTarget || table==kStrikeAlt));Event(table==kStrikeTarget ? "damage" : "damage-alt",{target.value,std::bit_cast<std::uint32_t>(amount),owner,parent.value});Put(Raw(target),26,Get(Raw(target),26)-std::bit_cast<std::uint32_t>(amount));
            if (input[13]==9) Put(Raw(target),26,1000000);
            if (input[13]==10) { Raw(parent)[34]=7;Put(Raw(target),0,kStrikeAlt); }
            if (input[13]==11) { Raw(target)[11]=2;Put(Raw(target),26,0); }
            if (input[13]==12) Put(Raw(parent),12,target.value,2);
        };
        // 특별 대상 삭제의 현재 표와 정확한flags를 기록한다.
        hooks.destroy=[this](Sid target,std::uint32_t table,std::uint32_t flags) { CHECK(table==kStrikeTarget || table==kStrikeAlt);Event(table==kStrikeTarget ? "destroy" : "destroy-alt",{target.value,flags}); };
        // QA 조회와 이미 통과한dead 관문을 빔 뒤 다시 검사하지 않는지 확인한다.
        hooks.beam=[this](Sid source,Sid target) { Event("beam",{source.value,target.value});if (input[13]==13) { Raw(target)[11]=2;Put(Raw(Sid{50}),12,target.value,2); } };
        // 두 호출에 다른 정수점을 주고 대상 위치를 바꾸어 캡처/출력 선택을 구별한다.
        hooks.project=[this](float x,float y) {
            Event("project",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});++projects;
            if (input[13]==15) { const Sid target{static_cast<std::uint16_t>(input[8])};Put(Raw(target),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(target),18,std::bit_cast<std::uint32_t>(201.0F)); }
            return std::array<std::int32_t,2>{17+2*static_cast<std::int32_t>(projects),-23-2*static_cast<std::int32_t>(projects)};
        };
        // 원문의 첫 점y=0과 둘째 투영·수명5를 전체DWORD로 관측한다.
        hooks.column=[this](Sid target,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float life) {
            Event("column",{target.value,std::bit_cast<std::uint32_t>(from[0]),std::bit_cast<std::uint32_t>(from[1]),std::bit_cast<std::uint32_t>(to[0]),std::bit_cast<std::uint32_t>(to[1]),std::bit_cast<std::uint32_t>(life)});
            if (input[13]==14) { Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(200.0F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(201.0F)); }
        };
        return hooks;
    }
    // 원문과 같은 물리 슬롯 순서로 전체 바이트를 반환한다.
    std::string Raws() {
        std::string text;
        // 선언 경계가 쓰지 않은 필드까지 빠짐없이 대조한다.
        for (const auto sid:kStrikeSlots) { if (!text.empty()) text+=',';text+=Hex(pool.Slot(Sid{sid})); }
        return text;
    }
};
}

// 실제 네 몸체를 두 정밀도의 독립 기대값과 대조한다.
TEST_CASE(BombStrike_Native1420Inputs) {
    StrikeScene scene;RawBombStrike strike(scene.pool,scene.types,scene.random,scene.state,scene.Hooks());std::size_t rows{},diagnostics{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-strike-x86.tsv").string();
    // 원문 기대값은 제품 계산으로 재생성하지 않고 보존한 두 정밀도와 대조한다.
    for (const auto& fields:LoadFixture(path.c_str()).rows) {
        CHECK(fields.size()==11);if (fields.size()!=11) continue;scene.Prepare(fields[0]);
        const Sid parent{50},target{static_cast<std::uint16_t>(scene.input[8])};std::uint32_t result{};
        if (scene.input[0]==0) result=strike.MayTarget(parent,target);
        else if (scene.input[0]==1) strike.Apply(parent,target);
        else { result=strike.MayTarget(parent,target);if (result) strike.Apply(parent,target); }
        const std::array<std::string,5> actual{std::to_string(result),scene.trace.empty() ? "-" : scene.trace,scene.Raws(),std::to_string(scene.random.State()),std::to_string(scene.state.boss)};
        // 두 원본 정밀도 각각에 반환/효과/전체 슬롯/난수/권한이 같아야 한다.
        for (std::size_t i=0;i<actual.size();++i) {
            if ((actual[i]!=fields[i+1] || actual[i]!=fields[i+6]) && diagnostics++<3) std::cerr<<"직접 피해 주문 행 "<<rows<<" 열 "<<i<<" 입력 "<<fields[0]<<"\n실제 "<<actual[i]<<"\n원문 "<<fields[i+1]<<'\n';
            CHECK(actual[i]==fields[i+1]);CHECK(actual[i]==fields[i+6]);
        }
        ++rows;
    }
    CHECK(rows==1420);
}

// 미도달 효과·실제 하위 누락·초기dead 관문의 계열 차이를 확인한다.
TEST_CASE(BombStrike_LazyEffectsAndScope) {
    StrikeScene scene;scene.Prepare("1,0,70,2097152,16,0,0,60,60,1,1000,1,0,0,1112211456,1112670208,1112932352,1112801280,1,3");RawBombStrike missing(scene.pool,scene.types,scene.random,scene.state,{});
    missing.Apply(Sid{50},Sid{60});CHECK(scene.random.State()==0); // QA와 같은 대상은 큰 효과를 요구하지 않는다.
    Put(scene.Raw(Sid{50}),12,51,2);bool rejected{};
    try { missing.Apply(Sid{50},Sid{60}); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);Put(scene.Raw(Sid{50}),0,kStrikeTables[1]);scene.Raw(Sid{60})[11]=2;missing.Apply(Sid{50},Sid{60});CHECK(scene.random.State()==0);
    Put(scene.Raw(Sid{50}),0,kStrikeTables[3]);scene.random.SetState(1);rejected=false;
    try { missing.Apply(Sid{50},Sid{60}); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && scene.random.State()!=0); // 마지막 계열은dead 대상에도 난수/생성을 수행하므로 단순 조기 반환하면 안 된다.
    Put(scene.Raw(Sid{50}),0,0x502370);CHECK(!missing.Handles(Sid{50}));rejected=false;
    try { static_cast<void>(missing.MayTarget(Sid{50},Sid{60})); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected);
}
