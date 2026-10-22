// 보호41e100의 독립 원문과 현재 목록/기한/전체 물리 슬롯을 비교한다.
#include "RawSceneSupport.h"
#include "o/RawBombEffectTimer.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

// 효과 결과를 계산하지 않고 명시 입력·콜백 변형만 원본 실행기와 동일하게 공급한다.
TEST_CASE(BombEffectTimer_IndependentNative4416) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-effect-timer-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 마비/추상/중복/비활성/만료·NaN 입력과 콜백 변형을 모두 소비한다.
        CHECK(row.size()==4);if (row.size()!=4) continue;std::vector<std::int64_t> a;
        for (const auto& part:Split(row[0],',')) a.push_back(std::stoll(part)); // signed 기한 입력과 DWORD 상위 비트를 함께 보존한다.
        CHECK(a.size()==8);if (a.size()!=8) continue;
        SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);double now=100.25;
        types[133].flags2=static_cast<std::uint32_t>(a[3]);types[136].flags2=static_cast<std::uint32_t>(a[3])^0x200000U;
        const auto writable=[&](Sid sid) { const auto bytes=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(bytes.data()),50); }; // 비활성 원문도 주변 바이트와 함께 설치한다.
        for (const Sid sid:{Sid{1},Sid{60},Sid{127}}) { // 원본과 같은 세 슬롯의 명시 초기 입력이다.
            auto raw=writable(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(a[4]);raw[11]=static_cast<std::uint8_t>(a[1]);raw[40]=static_cast<std::uint8_t>(a[2]);
        }
        std::vector<std::uint32_t> items;if (a[7]==1) items={60};else if (a[7]==2) items={60,127,60};else if (a[7]==3) items={60,1,127};
        BombEffectTimerFields fields;std::string effects;bool didU{},didR{},didG{};
        const auto event=[&](std::string tag,Sid sid,std::optional<std::uint32_t> value={}) { if (!effects.empty()) effects+=';';effects+=tag+":"+std::to_string(sid.value);if (value) effects+=':'+std::to_string(*value); }; // 외부 호출의 순서/현재 SID/flags만 기록한다.
        BombEffectTimerHooks hooks;
        hooks.unpop=[&](Sid target,std::uint32_t flags) { event("U",target,flags);if (!didU) { didU=true;if (a[6]==3) fields.members[0]=127; } };
        hooks.repop=[&](Sid target,std::uint32_t flags) { event("R",target,flags);if (!didR) { didR=true;if (a[6]==1) fields.endTime=1000.25;if (a[6]==2) { fields.flags=3;writable(target)[40]=48; } } };
        hooks.refresh=[&](Sid target) { event("G",target);if (!didG) { didG=true;if (a[6]==4) writable(target)[10]=136;if (a[6]==5) writable(target)[11]|=2; } };
        hooks.fence=[&](Sid target) { event("F",target); };
        RawBombEffectTimer logic(pool,types,now,std::move(hooks));fields=logic.Construct(a[5]==2 ? 0 : static_cast<std::int32_t>(a[5]),items,static_cast<std::uint32_t>(a[0]));if (a[5]==2) fields.endTime=std::bit_cast<double>(std::uint64_t{0x7ff8000000000000ULL});
        logic.Frame(fields,[&] { if (!effects.empty()) effects+=';';effects+="K:0"; });
        if (effects.empty()) effects="-";std::string packed=std::to_string(std::bit_cast<std::uint64_t>(fields.endTime))+','+std::to_string(fields.flags),slots;
        for (const auto member:fields.members) packed+=','+std::to_string(member); // 현재 목록을 원문과 같은 순서로만 직렬화한다.
        for (const Sid sid:{Sid{1},Sid{60},Sid{127}}) slots+=Hex(pool.Slot(sid)); // 방문하지 않은 주변 바이트도 원문과 비교한다.
        if (effects!=row[1] || packed!=row[2] || slots!=row[3]) { std::cout<<"상태 효과 타이머 원문행"<<count<<":"<<row[0]<<" 실제"<<effects<<"/기한·목록"<<packed<<'\n';CHECK(false); }++count;
    }
    CHECK(count==4416);std::cout<<"상태 효과 타이머 원문"<<count<<"행/전체 슬롯·현재 기한/효과 대조\n";
}
