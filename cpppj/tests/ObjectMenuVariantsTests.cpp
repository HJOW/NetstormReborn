// 원문 파생+a8의1024개 입력과 재진입 효과 순서를 같은 공개 작성기로 대조한다.
#include "RawSceneSupport.h"
#include "client/RawObjectMenuVariants.h"
#include <iostream>
using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 기대값은 fixture에서만 읽고 현재 슬롯/모드와 외부 반환을 명시 입력으로 공급한다.
struct VariantScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 실제50BYTE 슬롯 폭이다.
    std::vector<std::int64_t> input; // 원문11개 입력과 변경되는 모드다.
    std::string trace; // 실제 C++ 외부 호출 순서다.
    // 독립 검사의 현재 슬롯만 바꾸며 제품의 생존 판정을 합성하지 않는다.
    std::span<std::uint8_t> Raw() { auto raw=pool.Slot(Sid{50});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // C 문자열의 원본 바이트를 비교 가능한 hex로 남긴다.
    static std::string Encoded(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
    // 외부 효과 하나를 전체 순서에 추가한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // native 관측기와 같은 명시 callback 변경만 적용한다.
    void Change(std::string_view where) {
        if (input[10]==1 && where=="translate") Raw()[34]=2;
        if (input[10]==2 && where=="count") Raw()[10]=122;
        if (input[10]==3 && where=="ai") Raw()[34]=2;
        if (input[10]==4 && where=="process") input[3]|=1;
        if (input[10]==5 && where=="translate") Put(Raw(),26,0);
    }
    // 입력을 해석하고 별도 메뉴 기대 목록 없이 실제 슬롯을 만든다.
    explicit VariantScene(std::string_view values) {
        for (const auto& field:Split(values,',')) input.push_back(std::stoll(field)); // 각 원문 필드의 부호를 유지한다.
        CHECK(input.size()==11);Put(Raw(),10,static_cast<std::uint32_t>(input[2]),1);Put(Raw(),34,static_cast<std::uint32_t>(input[0]),1);Put(Raw(),26,static_cast<std::uint32_t>(input[4]));Put(Raw(),12,static_cast<std::uint32_t>(input[8]),2);
    }
    // 실제 메뉴 효과만 기록하며 반환은 원문에 공급한 독립 입력이다.
    ObjectMenuVariantHooks Hooks() {
        ObjectMenuVariantHooks hooks;
        hooks.global=[this](std::uint32_t address)->std::uint32_t { if (const auto code=ObjectMenuVariantCommandValue(address)) return *code;switch (address) { case 0x594fa4:return static_cast<std::uint32_t>(input[3])&4U;case 0x5c85a4:return static_cast<std::uint32_t>(input[3])&1U;case 0x55a480:return static_cast<std::uint32_t>(input[3])&2U;case 0x540c70:return static_cast<std::uint32_t>(input[1]);default:throw std::logic_error("명시 파생 메뉴 다른 전역"); } };
        hooks.localOwner=[this] { Add("L");return static_cast<std::uint32_t>(input[1]); };
        hooks.maximumHp=[this](Sid sid) { CHECK(sid==Sid{50});Add("H,50");return static_cast<std::int32_t>(input[5]); };
        hooks.eventZero=[this](Sid sid) { CHECK(sid==Sid{50});Add("P,50,0");Change("process");return input[6]!=0; };
        hooks.hasBrain=[this](Sid sid) { CHECK(sid==Sid{50});return Raw()[34]!=0 && input[9]!=0; };
        hooks.aiMenu=[this](auto owner) { Add("AI,"+std::to_string(owner));Change("ai"); };
        hooks.translate=[this](auto text) { Add("T,"+Encoded(text));Change("translate");return std::string(text); };
        hooks.format=[this](std::string_view pattern,std::int32_t value) { Add("F,"+Encoded(pattern)+','+std::to_string(std::bit_cast<std::uint32_t>(value)));std::string text(pattern);const auto at=text.find("%d");if (at==text.npos) throw std::logic_error("고정%d 파생 메뉴 서식 없음");text.replace(at,2,std::to_string(value));return text; };
        hooks.helpWord=[] { return std::string("About"); };
        hooks.count=[this](Sid sid) { CHECK(sid==Sid{50});Add("C,50");Change("count");return static_cast<std::uint32_t>(input[7]); };
        hooks.item=[this](auto label,auto command,auto parameter,auto include,auto enabled) { Add("I,"+Encoded(label)+','+std::to_string(command)+','+std::to_string(parameter)+','+std::to_string(include)+','+std::to_string(enabled)+",0"); };
        hooks.header=[this](auto text) { Add("A,"+Encoded(text)); };hooks.separator=[this] { Add("X"); };
        hooks.object=[this](Sid sid) { CHECK(sid==Sid{50});Add("B,50"); };hooks.damageable=[this](Sid sid) { CHECK(sid==Sid{50});Add("D,50"); };
        hooks.salvage=[this](Sid sid,auto type,auto count) { CHECK(sid==Sid{50});Add("S,50,"+std::to_string(type)+','+std::to_string(count)); };return hooks;
    }
};
}
// 네이티브 여덟 본체의 전체 외부 효과·인자·순서를 독립 자료로 대조한다.
TEST_CASE(ObjectMenuVariants_Native1024) {
    const auto rows=LoadFixture(NETSTORM_OBJECT_MENU_VARIANTS_FIXTURE).rows;CHECK(rows.size()==1024);
    for (const auto& row:rows) { // 각 행은 새 현재 상태에서 같은 공개 본체를 실행한다.
        CHECK(row.size()==3);VariantScene scene(row[1]);RawObjectMenuVariants body(scene.pool,scene.Hooks());body.Open(Sid{50},static_cast<std::uint32_t>(std::stoul(row[0],nullptr,16)));
        CHECK(scene.trace==row[2]);if (scene.trace!=row[2]) { std::cout<<row[0]<<' '<<row[1]<<" expected="<<row[2]<<" actual="<<scene.trace<<'\n';return; }
    }
}
