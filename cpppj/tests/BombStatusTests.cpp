// 새 보호 PE 원문의 전체 슬롯·대상 자격·가상 효과·송신 observer를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBombStatus.h"
#include "o/RawBombTimerPacket.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

TEST_CASE(BombStatus_IndependentNative) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-status-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 같은 명시 슬롯/전역에서 실제 제품 몸체만 실행한다.
        const auto words=Split(row[0],',');const auto mode=words[0];std::vector<std::uint32_t> a;
        for (std::size_t i=1;i<words.size();++i) a.push_back(Number(words[i])); // 상위 상태 비트도 그대로 보존한다.
        SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);std::array<std::uint32_t,81> alliances{};alliances[11]=a[5];
        const auto raw=[&](std::uint16_t sid) { const auto slot=pool.Slot({sid});return std::span<std::uint8_t>(const_cast<std::uint8_t*>(slot.data()),50); }; // 원본처럼 free 슬롯도 물리 상태 입력을 받는다.
        for (const auto sid:std::array<std::uint16_t,2>{50,60}) { auto bytes=raw(sid);std::fill(bytes.begin(),bytes.end(),std::uint8_t{});Put(bytes,0,0x12200000);bytes[34]=sid==50 ? 1 : 2; } // 두 owner와 가상 효과 표 입력이다.
        Put(raw(50),12,a[4] ? 60 : 0,2);raw(60)[10]=static_cast<std::uint8_t>(a[0]);raw(60)[11]=static_cast<std::uint8_t>(a[2]);raw(60)[40]=static_cast<std::uint8_t>(a[3]);
        for (auto& type:types) type.flags2=a[1];types[136].flags2^=0x200000U; // 현재TYPE 변경으로 다시 조회하는 genus도 원문과 같다.
        BombStatusState state;state.alliances=alliances;std::string effects,result="-";BombStatusHooks hooks;
        const auto event=[&](std::string value) { if (!effects.empty()) effects+=';';effects+=value; }; // 순서 있는 큰 경계만 기록한다.
        const auto mutation=[&] { if (a[6]==1) raw(60)[10]=136;if (a[6]==2) state.flare=107;if (a[6]==3) raw(60)[11]=7; }; // 선택 효과 뒤 현재 TYPE/전역·상태를 바꾸는 명시 입력이다.
        hooks.refresh=[&](Sid sid) { event("R:"+std::to_string(sid.value));mutation(); };
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { event("U:"+std::to_string(sid.value)+':'+std::to_string(flags));mutation(); };
        hooks.repop=[&](Sid sid,std::uint32_t flags) { event("P:"+std::to_string(sid.value)+':'+std::to_string(flags));mutation(); };
        hooks.fence=[&](Sid sid) { event("F:"+std::to_string(sid.value));if (a[6]==1) state.flare=107; };
        hooks.flare=[&](Sid sid,std::int32_t n,std::uint32_t eventId,std::uint32_t type,std::uint32_t direction) { event("L:"+std::to_string(sid.value)+':'+std::to_string(n)+':'+std::to_string(eventId)+':'+std::to_string(type)+':'+std::to_string(direction)); };
        std::array<std::uint8_t,32> packet;packet.fill(0xa5);
        if (mode=="H") { Put(packet,16,a[0],2);FinalizeBombTimerPacket(packet,a[1]!=0); }
        else {
            const auto table=(mode=="I" || mode=="i") ? 0x502510U : 0x502ff8U;Put(raw(50),0,table);RawBombStatus logic(pool,types,state,std::move(hooks));
            if (mode=="i" || mode=="p") result=logic.MayTarget({50},{60}) ? "1" : "0";else logic.Apply({50},{60});Put(raw(50),0,0x12200000); // 원문 실행기의 가상 호출 표만 비교용으로 정규화한다.
        }
        const auto expected=(mode=="i" || mode=="p") ? (Number(row[1]) ? "1" : "0") : row[1];if (effects.empty()) effects="-";
        if (result!=expected || effects!=row[2] || Hex(raw(50))+Hex(raw(60))!=row[3] || Hex(packet)!=row[4]) { std::cout<<"상태 주문 원문행"<<count<<':'<<row[0]<<" 실제"<<result<<'/'<<effects<<'\n';CHECK(false); }++count;
    }
    CHECK(count==612);std::cout<<"상태 주문 원문"<<count<<"행 대조\n";
}
