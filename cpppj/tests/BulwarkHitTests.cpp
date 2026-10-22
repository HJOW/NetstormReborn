// 보호 PE의 독립 불워크 관문492입력과 효과/두 슬롯/전체 반환을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBulwarkHit.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

// 비행 속성/공격자0/생존 필터 부재와 일반 피해의 인수·반환 폭을 확인한다.
TEST_CASE(BulwarkHit_IndependentNative492) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bulwark-hit-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 독립 원문이 준 입력과 기대 출력을 그대로 사용한다.
        CHECK(row.size()==4);if (row.size()!=4) continue;std::vector<std::uint32_t> a;
        for (const auto& part:Split(row[0],',')) a.push_back(static_cast<std::uint32_t>(std::stoull(part))); // 부호 비트도 DWORD로 보존한다.
        CHECK(a.size()==8);if (a.size()!=8) continue;
        SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);const Sid victim{50},source{60};
        const auto writable=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(raw.data()),50); }; // free/dead 입력도 원문 그대로 설치한다.
        for (const auto sid:{victim,source}) { // 주변 바이트를 포함하는 원래50바이트 입력이다.
            auto raw=writable(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,sid==victim ? 0x50ea08U : 0x12200000U);raw[10]=static_cast<std::uint8_t>(sid==victim ? 79 : a[1]);raw[34]=sid==victim ? 2 : 1;
            if (sid==victim) Put(raw,26,0x12345678U);else { raw[11]=static_cast<std::uint8_t>(a[3]);raw[40]=static_cast<std::uint8_t>(a[4]); }
        }
        types[a[1]].flags2=a[2];std::string effects;
        const auto result=RawBulwarkHit(pool,types,victim,std::bit_cast<std::int32_t>(a[5]),a[6],Sid{static_cast<std::uint16_t>(a[0])},[&](Sid target,std::int32_t amount,std::uint32_t owner,Sid attacker) {
            effects="H:"+std::to_string(target.value)+":"+std::to_string(std::bit_cast<std::uint32_t>(amount))+":"+std::to_string(owner)+":"+std::to_string(attacker.value);
            Put(writable(target),26,0x12345678U-std::bit_cast<std::uint32_t>(amount));writable(target)[34]=static_cast<std::uint8_t>(owner);if (attacker.value) writable(attacker)[10]=70;return a[7];
        }); // 이미 별도 원문으로 검증한 일반 피해의 명시 경계만 동일 부수효과를 공급한다.
        const auto raws=Hex(pool.Slot(victim))+Hex(pool.Slot(source));
        if ((effects.empty() ? "-" : effects)!=row[1] || raws!=row[2] || result!=Number(row[3])) { std::cout<<"불워크 원문행"<<count<<":"<<row[0]<<" 실제"<<effects<<"/반환"<<result<<'\n';CHECK(false); }++count;
    }
    CHECK(count==492);
}
