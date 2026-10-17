// 원본 Carrier 부모의837개 독립 입력과 현재83VT 분배를 재생한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierRegular.h"
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace netstorm::test::rawscene;

namespace {
constexpr std::uint32_t kParentVt=0x20000800U,kGhostVt=0x20000900U,kOtherVt=0x20000a00U; // 원본 관찰의 명시 가상 효과 경계 표다.
constexpr std::array<std::uint16_t,6> kWatch{0,50,80,81,90,91}; // 원문 관측과 같은 물리50바이트 슬롯이다.
// 원문 DWORD의 unsigned 비트를 읽는다.
std::vector<std::uint32_t> CarrierNumbers(std::string_view text) {
    std::vector<std::uint32_t> row; // 입력이며 기대 반환/분기를 계산하지 않는다.
    // 모든 항목의 원래 상위 비트를 보존한다.
    for (const auto& field:Split(text,',')) row.push_back(Number(field));
    return row;
}
// Native와 같은 입력/큰 하위 효과만 공급한다. 부모의 분기·좌표·프레임은 제품 코드가 실행한다.
struct CarrierRegularScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 부모/내용물/미리보기의 동일 물리 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 원본과 같은 현재 flags 표다.
    std::vector<CarrierRegularType> details=std::vector<CarrierRegularType>(188); // 같은 번호의 프레임/casttime이다.
    std::vector<std::uint32_t> row; //16DWORD의 독립 입력이다.
    bool boss{}; //492가 읽는 현재 권한이다.
    std::uint16_t created{}; // 실제 create 경계가 발급한 다음 명시 번호다.
    std::string trace; // 원문과 같은 순서의 효과다.
    // 원본 몸체가 허용하는 물리 슬롯을 추가 상태 필터 없이 초기화한다.
    std::span<std::uint8_t> Raw(std::uint16_t sid) { const auto raw=pool.Slot(Sid{sid});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // unsigned 인자만 원문 형식으로 남긴다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 인자 순서와 전체 폭을 그대로 기록한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 매 입력마다 관측한 여섯 raw와 현재 TYPE를 독립 초기화한다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);CHECK(row.size()==16);trace.clear();created=0;boss=row[9]!=0;
        // 무관한 바이트는a5로 보존해 원문 필드 쓰기 폭을 검사한다.
        for (const auto sid:kWatch) std::fill(Raw(sid).begin(),Raw(sid).end(),std::uint8_t{0xa5});
        Put(Raw(0),18,102,2);auto parent=Raw(50);Put(parent,0,kParentVt);parent[10]=158;parent[34]=1;parent[35]=0;
        const std::array<std::array<std::uint32_t,2>,4> points{{{std::bit_cast<std::uint32_t>(10.5F),std::bit_cast<std::uint32_t>(20.25F)},{0x7fc01234U,std::bit_cast<std::uint32_t>(-10.0F)},{0x7f800000U,std::bit_cast<std::uint32_t>(255.9F)},{std::bit_cast<std::uint32_t>(-5.0F),0x7fc01234U}}}; // 좌표 상하한/NaN의 명시 입력이다.
        Put(parent,14,points[row[7]][0]);Put(parent,18,points[row[7]][1]);
        // 두 내용물의 종류/단어 입력이며 실제 조회는 find 경계다.
        for (std::uint16_t index=0;index<2;++index) { auto raw=Raw(static_cast<std::uint16_t>(80+index));raw[10]=3;Put(raw,18,100U+index,2);Put(raw,12,index==0 ? 0xa123U : 0x8001U,2); }
        // create가 바꿀 수 있는 새 TYPE 두 개도 별도로 초기화한다.
        for (std::size_t kind=100;kind<104;++kind) { details[kind]={static_cast<std::int32_t>(kind-93),std::bit_cast<std::int32_t>(row[6])};types[kind].flags2=row[kind==100 ? 4 : 5]; }
    }
    // 원본이 호출한 큰 경계에서만 명시 현재 상태를 바꾼다.
    CarrierRegularHooks Hooks() {
        CarrierRegularHooks hooks;
        hooks.find=[this](Sid parent,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) {
            Event("F",{parent.value,mask,kind,index});const auto found=mask==0x20U && (row[3]&1U) ? 80U : mask==1U && (row[3]&2U) ? 81U : 0U;
            if (row[13]==5) Raw(50)[35]=17;return Sid{static_cast<std::uint16_t>(found)};
        };
        hooks.create=[this](std::uint32_t kind,std::uint32_t flags) {
            const auto sid=static_cast<std::uint16_t>(90+created++);Event("C",{kind,flags,sid});auto raw=Raw(sid);Put(raw,0,kGhostVt);raw[10]=static_cast<std::uint8_t>(kind+(row[13]==1 ? 2U : 0U));raw[11]=4;
            if (row[13]==1) Raw(50)[35]=5;return Sid{sid};
        };
        hooks.setWord=[this](Sid sid,std::uint32_t value) { Event("W",{sid.value,value});Put(Raw(sid.value),12,value,2);if (row[13]==2) Raw(50)[34]=7; };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) {
            Event("O",{sid.value,owner});Raw(sid.value)[34]=static_cast<std::uint8_t>(owner);
            if (row[13]==3) { Put(Raw(50),14,std::bit_cast<std::uint32_t>(30.5F));Put(Raw(50),18,std::bit_cast<std::uint32_t>(40.75F));Put(Raw(sid.value),0,kOtherVt); }
        };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            Event(Get(Raw(sid.value),0)==kOtherVt ? "X" : "P",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags});Put(Raw(sid.value),14,std::bit_cast<std::uint32_t>(x));Put(Raw(sid.value),18,std::bit_cast<std::uint32_t>(y));if (row[13]==4) types[101].flags2=0;
        };
        hooks.allocateTimer=[this] { Event("A",{24});return row[8]!=0; };
        hooks.constructTimer=[this](Sid sid,float duration) { Event("T",{sid.value,std::bit_cast<std::uint32_t>(duration)}); };
        hooks.refresh=[this](Sid sid) { Event("R",{sid.value});if (row[13]==6) Raw(50)[40]=0x12; };
        hooks.needsEscape=[this](float x,float y) { Event("E",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});return row[10]!=0; };
        hooks.movementType=[this](std::uint32_t type) { Event("M",{type,0,0});return row[11]; };
        hooks.hasPath=[this](Sid sid,std::uint32_t type) { Event("H",{sid.value,type});return row[12]!=0; };
        hooks.escape=[this](Sid sid,float delay,std::uint32_t event) { Event("V",{sid.value,std::bit_cast<std::uint32_t>(delay),event}); };
        hooks.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { Event("D",{sid.value,event,count,std::bit_cast<std::uint32_t>(payload)});if (row[13]==7) Raw(50)[40]=0x33;return std::bit_cast<float>(row[14]); };
        hooks.falling=[this](Sid sid,std::uint32_t count) { Event("B",{sid.value,count});if (row[13]==7) Raw(50)[40]=0x33;return std::bit_cast<float>(row[15]); };
        return hooks;
    }
    // 여섯 물리50바이트를 원문 순서로 합친다.
    std::string Image() {
        std::string value;
        // 각 슬롯의 관측 바이트를 지정된 순서로 붙인다.
        for (const auto sid:kWatch) value+=Hex(Raw(sid));
        return value;
    }
};
}
// 전체 부모/좌표 제한/현재 raw와 반환은 두 정밀도 원본의53비트 관측에서만 읽는다.
TEST_CASE(CarrierRegular_NativeParentAndCurrentCallbacks) {
    CarrierRegularScene scene;std::size_t count{},diagnostics{}; // 같은 저장소를 독립 재초기화하고 처음8개 불일치만 표시한다.
    // 기존 기대값을 바꾸지 않고 새837입력의 모든 효과/바이트를 비교한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_REGULAR_FIXTURE).rows) {
        CHECK(fields.size()==7);if (fields.size()!=7) continue;scene.Prepare(CarrierNumbers(fields[0]));RawCarrierRegular logic(scene.pool,scene.types,scene.details,scene.boss,scene.Hooks());
        const auto result=logic.Handle(Sid{50},scene.row[0],scene.row[1],std::bit_cast<float>(scene.row[2]));
        if ((scene.trace.empty() ? "-" : scene.trace)!=fields[2] && diagnostics++<8) std::cout<<"Carrier 입력 "<<fields[0]<<"\n실제 "<<scene.trace<<"\n원본 "<<fields[2]<<'\n';
        CHECK(std::bit_cast<std::uint32_t>(result)==Number(fields[1]));CHECK((scene.trace.empty() ? "-" : scene.trace)==fields[2]);CHECK(scene.Image()==fields[3]);++count;
    }
    CHECK(count==837);
}
// 기본 몸체의 범위를TYPE/genus로 추정하지 않고 실제83VT의 보호PE 슬롯과 비교한다.
TEST_CASE(CarrierRegular_NativeVtablesAndDerivedFallback) {
    CarrierRegularScene scene;scene.Prepare({0,0,0,0,0,0,0,0,1,1,0,54,0,0,0x3e4ccccdU,0});RawCarrierRegular logic(scene.pool,scene.types,scene.details,scene.boss,scene.Hooks());std::size_t direct{},count{};std::uint32_t fallbacks{};
    const auto handler=logic.Handler([&](Sid sid,std::uint32_t event,std::uint32_t invocation,float payload) { CHECK(sid.value==50 && event==0 && invocation==0 && payload==1.0F);++fallbacks;return 3.0F; });
    // 각 줄은 보호PE의 실제+5c DWORD이며 제품의 분배표로 생성하지 않는다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_REGULAR_VTABLES).rows) {
        CHECK(fields.size()==2);Put(scene.Raw(50),0,Number(fields[0]));const bool expected=Number(fields[1])==0x42a750U;CHECK(logic.Handles(Sid{50})==expected);
        CHECK(handler(Sid{50},0,0,1.0F)==(expected ? 0.2F : 3.0F));if (expected) ++direct;++count;
    }
    CHECK(count==83 && direct==6 && fallbacks==77);
}
// 내용물 없음/만료는 기본 처리나 생성/표시 경계를 요구하지 않는다.
TEST_CASE(CarrierRegular_LazyBoundariesAndEdition) {
    CarrierRegularScene scene;scene.Prepare({0x491,0,0x3e4ccccdU,0,0,0,0,0,1,1,0,54,0,0,0,0});auto hooks=scene.Hooks();CarrierRegularHooks empty;empty.find=hooks.find;RawCarrierRegular logic(scene.pool,scene.types,scene.details,scene.boss,std::move(empty));
    CHECK(logic.Handle(Sid{50},0x491,0,0.2F)==0);CHECK(logic.Handle(Sid{50},0x491,100,0.2F)==0);
    bool rejected{};try { logic.Handle(Sid{50},0,0,0.2F); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    rejected=false;try { logic.Handle(Sid{50},0x5105,0,0.2F); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    SidPool cd(OriginalEdition::Cd1072,15000,true);rejected=false;try { RawCarrierRegular invalid(cd,scene.types,scene.details,scene.boss,{}); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
