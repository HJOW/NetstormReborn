// 독립 원본의기도명령/수신/진행과콜백뒤현재값을비교한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPrayer.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

// 원문이계산한호출순서/전체슬롯/반환비트/최종권한을그대로대조한다.
TEST_CASE(PriestPrayer_IndependentNative1126) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/priest-prayer-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 명령/수신/605와모든명시관문·변형을소비한다.
        CHECK(row.size()==5);if (row.size()!=5) continue;const auto parts=Split(row[0],',');CHECK(parts.size()==15);if (parts.size()!=15) continue;const auto mode=parts[0];std::vector<std::int64_t> a;
        for (std::size_t index=1;index<parts.size();++index) a.push_back(std::stoll(parts[index])); // DWORD상위비트와음수방향/기도횟수를보존한다.
        SidPool pool(OriginalEdition::Patch1078,24000);const Sid actor{60};const auto slot=pool.Slot(actor);auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(slot.data()),50);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=158;raw[11]=static_cast<std::uint8_t>(a[0]);Put(raw,36,7);Put(raw,42,static_cast<std::uint32_t>(a[12]));Put(raw,46,0xaabbccdd);
        std::vector<PriestPrayerType> types(188);types[158].defaultFrame=7;types[159].defaultFrame=9;types[113].prayTime=types[168].prayTime=static_cast<std::int32_t>(a[9]);bool boss=a[3]!=0;std::string effects;const auto parameter=(static_cast<std::uint32_t>(a[12])<<16)|60U;
        const auto event=[&](std::string value) { if (!effects.empty()) effects+=';';effects+=value; }; // 효과순서만기록하며실제판정은제품이수행한다.
        PriestPrayerHooks hooks;
        hooks.hasEvent=[&](Sid,std::uint32_t code) { event("E:"+std::to_string(code));return a[1]!=0; };
        hooks.direction=[&](Sid sid) { event("D:"+std::to_string(sid.value));return static_cast<std::int32_t>(a[2]); };
        hooks.movementType=[&](std::uint32_t kind) { event("M:"+std::to_string(kind));return 54U; };
        hooks.movement=[&](Sid sid,std::uint32_t type,std::uint32_t ordinal,bool remove) { event("L:"+std::to_string(sid.value)+':'+std::to_string(type)+':'+std::to_string(ordinal)+':'+std::to_string(remove ? 1 : 0));if (a[13]==2) Put(raw,42,113);return a[5] ? Sid{77} : Sid{}; };
        hooks.request=[&](std::int32_t target,std::span<const std::uint8_t> packet) { event("P:"+std::to_string(target)+':'+Hex(packet));if (a[13]==3) { boss=false;raw[10]=159; } };
        hooks.broadcast=[&](std::span<const std::uint8_t> packet) { event("B:"+Hex(packet));if (a[13]==3) { boss=false;raw[10]=159; } };
        hooks.reserveShared=[&] { event("A:40");return a[4]!=0; };hooks.addShared=[&](Sid sid,std::uint32_t code,float payload) { event("R:"+std::to_string(code)+':'+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload))); };
        hooks.ritual=[&](Sid sid,std::uint32_t stage) { event("S:"+std::to_string(sid.value)+':'+std::to_string(stage)); };
        hooks.immobile=[&](Sid sid) { event("I:"+std::to_string(sid.value));if (a[13]==4) { boss=false;Put(raw,42,113); }return a[6]!=0; };
        hooks.containedType=[&](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t ordinal) { event("H:"+std::to_string(sid.value)+':'+std::to_string(mask)+':'+std::to_string(kind)+':'+std::to_string(ordinal));if (a[13]==5) boss=false;return a[7] ? 168U : 0U; };
        hooks.discard=[&](Sid sid) { event("Q:"+std::to_string(sid.value));if (a[13]==6) { Put(raw,42,113);raw[10]=159; } };
        hooks.create=[&](Sid sid,std::uint32_t kind,std::uint32_t argument,std::uint32_t flags,std::uint32_t attachFlags,std::uint32_t quality) { event("C:"+std::to_string(sid.value)+':'+std::to_string(kind)+':'+std::to_string(argument)+':'+std::to_string(flags)+':'+std::to_string(attachFlags)+':'+std::to_string(quality)); };
        hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) { event("F:"+std::to_string(sid.value)+':'+std::to_string(frame)+':'+std::to_string(flags));Put(raw,36,std::bit_cast<std::uint32_t>(frame)); };
        hooks.panic=[&](Sid sid) { event("V:"+std::to_string(sid.value)); };hooks.parent=[&](Sid sid,std::uint32_t code,std::uint32_t value) { event("X:"+std::to_string(sid.value)+':'+std::to_string(code)+':'+std::to_string(value));return 0x12345678U; };
        RawPriestPrayer logic(pool,types,boss,std::move(hooks));std::uint32_t result{};
        if (mode=="C") result=logic.Command(actor,static_cast<std::uint32_t>(a[11]),parameter);
        else if (mode=="R") { const std::array<std::uint8_t,8> packet{1,0x6d,8,0,60,0,static_cast<std::uint8_t>(a[12]),static_cast<std::uint8_t>(a[12]>>8)};logic.Receive(packet); }
        else result=std::bit_cast<std::uint32_t>(logic.Tick(actor,static_cast<std::uint32_t>(a[8]),std::bit_cast<float>(static_cast<std::uint32_t>(a[10]))));
        if (effects.empty()) effects="-";if (effects!=row[1] || Hex(raw)!=row[2] || std::to_string(result)!=row[3] || std::to_string(boss ? 1 : 0)!=row[4]) { std::cout<<"사제기도원문행"<<count<<':'<<row[0]<<" 실제"<<effects<<"/반환"<<result<<'\n';CHECK(false); }++count;
    }
    CHECK(count==1126);
}
