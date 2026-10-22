// 보호PE의45c090 전체 반환을 실제 생산 처리기의 Form 삭제와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawWindAviaryProcess.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

// 정상 슬롯/주소 끝/bit31 별칭/진단 재진입과 사용하지 않는 flags를 모두 확인한다.
TEST_CASE(WindAviaryDestroy_IndependentNative880) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/wind-aviary-destroy-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 동결된 입력과 전체80BYTE/네 슬롯을 그대로 읽는다.
        CHECK(row.size()==4);if (row.size()!=4) continue;std::vector<std::uint32_t> a;
        for (const auto& value:Split(row[0],',')) a.push_back(static_cast<std::uint32_t>(std::stoull(value))); // 부호 비트를 포함한 DWORD다.
        CHECK(a.size()==7);if (a.size()!=7) continue;SidPool pool(OriginalEdition::Patch1078,24000);WindAviaryProcessFields fields;fields.bytes.fill(0xa5);Put(fields.bytes,48,a[1]);
        const auto writable=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(raw.data()),50); }; // 예약1과 free 입력을 원문대로 보존한다.
        constexpr std::array<std::uint16_t,4> ids{1,60,127,23999}; // 원문의 첫/정상/중간/마지막 주소 순서다.
        for (const auto id:ids) { auto raw=writable(Sid{id});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(a[3]);raw[11]=static_cast<std::uint8_t>(a[2]);raw[34]=2; } // 네 슬롯의 명시50BYTE다.
        bool fort{},friendly{},server=true,boss=a[0]!=0,checking{};std::uint32_t editor{},processType=59,flyerType=a[4],aviaryType=139;double now{};std::string effects;WindAviaryHooks hooks;
        const auto event=[&](std::string name,std::initializer_list<std::uint32_t> values) { if (!effects.empty()) effects+=';';effects+=name;for (const auto value:values) effects+=':'+std::to_string(value); }; // 효과의 현재 인수와 호출 순서다.
        hooks.assertion=[&](std::uint32_t line) {
            event("A",{line});if (a[6]==1) { Put(fields.bytes,48,0);writable(Sid{60})[11]=2; }if (a[6]==2) Put(writable(Sid{60}),0,0x12200100);if (a[6]==3) boss=false;
        }; // 원문 진단 경계가 바꾼 현재 상태를 이후 코드가 어떻게 읽는지 대조한다.
        hooks.destroy=[&](Sid sid,std::uint32_t flags) { event(Get(pool.Slot(sid),0)==0x12200100U ? "K2" : "K",{sid.value,flags});writable(sid)[11]|=3; };
        RawWindAviaryProcess logic(pool,WindAviaryGlobals{fort,friendly,server,boss,editor,checking,now,processType,flyerType,aviaryType},std::move(hooks));logic.FormDestroy(fields);
        std::string raws;for (const auto id:ids) raws+=Hex(pool.Slot(Sid{id})); // 경계 효과 이후의 네 전체 슬롯이다.
        if ((effects.empty() ? "-" : effects)!=row[1] || Hex(fields.bytes)!=row[2] || raws!=row[3]) { std::cout<<"Wind Form삭제 원문행"<<count<<":"<<row[0]<<"/실제"<<effects<<"/원본"<<row[1]<<'\n';CHECK(false); }++count;
    }
    CHECK(count==880);
}
