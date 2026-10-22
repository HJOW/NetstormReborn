// 기본PC53의 원주 좌표와 세 소환 함수의 생성필드/현재 인수 순서를 독립 원문에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawWindCircle.h"
#include "o/RawWindSpell.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

TEST_CASE(WindCircle_PointNative232) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/wind-circle-point-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 원본CRT 기본53비트의 두 float 저장을 비교한다.
        const auto a=Split(row[0],',');const auto point=WindCirclePoint(static_cast<float>(std::bit_cast<float>(Number(a[2]))),std::bit_cast<float>(Number(a[3])),static_cast<std::int32_t>(std::stol(a[0])),static_cast<std::int32_t>(std::stol(a[1])));
        const auto actual=std::to_string(std::bit_cast<std::uint32_t>(point[0]))+','+std::to_string(std::bit_cast<std::uint32_t>(point[1]));
        if (actual!=row[1]) { std::cout<<"Wind 원주 입력"<<row[0]<<" 실제"<<actual<<" 원문"<<row[1]<<'\n';CHECK(false); }++count;
    }
    CHECK(count==232);CHECK(Throws([] { WindCirclePoint(0,0,0,0); }));
}

TEST_CASE(WindSpell_SummonNative300) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/wind-spell-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 결과를 계산하지 않고 조회/힙/부착의 명시 변화만 공급한다.
        const auto words=Split(row[0],',');const auto variant=Number(words[0]),mutation=Number(words[4]);bool server=Number(words[1])!=0;double now=100;std::vector<std::int32_t> counts(188,static_cast<std::int32_t>(std::stol(words[3])));WindSpellState state;state.counts=counts;
        SidPool pool(OriginalEdition::Patch1078,24000);const auto slot=pool.Slot({50});auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(slot.data()),50);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,std::array<std::uint32_t,3>{0x5025d8U,0x502e68U,0x502f30U}.at(variant));Put(raw,12,60,2);
        std::string effects,images;const auto event=[&](std::string value) { if (!effects.empty()) effects+=';';effects+=value; }; // 경계 호출을 원문 순서로 기록한다.
        WindSpellHooks hooks;
        hooks.exists=[&](Sid parent,std::uint32_t type) { event("F:"+std::to_string(parent.value)+':'+std::to_string(type)+":0:0");if (mutation==1) Put(raw,12,61,2);return Number(words[2])!=0; };
        hooks.allocate=[&]()->std::optional<WindAviaryProcessFields> { event("N:80");WindAviaryProcessFields fields;fields.bytes.fill(0xa5);if (mutation==2) { server=false;state.processType=60;now=101.25; }return fields; };
        hooks.attach=[&](WindAviaryProcessFields fields,Sid parent,std::uint32_t type,std::uint32_t flags) { event("A:"+std::to_string(type)+':'+std::to_string(parent.value)+":0:"+std::to_string(flags));if (!images.empty()) images+=',';images+=Hex(fields.bytes);if (mutation==3) counts[171+variant]=1;if (mutation==4) { Put(raw,12,62,2);state.processType=60; } };
        RawWindSpell logic(pool,server,now,state,std::move(hooks));const auto result=logic.MayTarget({50});if (effects.empty()) effects="-";if (images.empty()) images="-";
        if (std::to_string(result ? 1 : 0)!=row[1] || effects!=row[2] || images!=row[3]) { std::cout<<"Wind 소환 입력"<<row[0]<<" 실제"<<effects<<'\n';CHECK(false); }++count;
    }
    CHECK(count==300);
}
