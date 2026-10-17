// 독립 PE 원문으로 캐리어 낙하/다리 판정/사제 해방의 캡처 시점을 검증한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierFalling.h"
#include <bit>
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kVt=0x50f210U,kChangedVt=0x20000800U; // 원문 격리 가상 표와 교체 표다.
// 원문 DWORD 입력을 부호 변환 없이 읽는다.
std::vector<std::uint32_t> FallingNumbers(std::string_view text) {
    std::vector<std::uint32_t> result;
    // 원문의 상위 비트를 그대로 보존한다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// 큰 가상 효과만 명시하며 부모의 좌표/분기/캡처 계산은 제품 몸체가 수행한다.
struct CarrierFallingScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 같은 50바이트 물리 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 genus 입력이다.
    std::vector<RiftTypeFrames> frames; // 현재 다리 코드 입력이다.
    std::vector<std::uint16_t> surface=std::vector<std::uint16_t>(65536); // 고정 hash0 입력이다.
    std::vector<std::uint8_t> players=std::vector<std::uint8_t>(128*0xb4); // 원문의 이전 소유자 기록이다.
    bool boss{},checking{}; // 콜백이 변경할 수 있는 실제 권한/진단 전역이다.
    std::uint32_t priestType{}; // 현재 사제 종류다.
    std::int32_t mapLimit{}; // 현재 signed 지도 제한이다.
    std::vector<std::uint32_t> row; // 독립 원문 19DWORD다.
    std::string trace; // 현재 효과 순서다.
    // 원문은 할당 필터 없이 물리 raw를 읽는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 가상 표 변경 뒤 현재 효과와 원래 효과를 구별한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> args) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 인자 폭/순서를 그대로 기록한다.
        for (const auto value:args) trace+=':'+std::to_string(value);
    }
    // 입력 자료만 설치하며 기대 좌표/반환/분기는 계산하지 않는다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);CHECK(row.size()==19);boss=row[4]!=0;checking=row[5]!=0;priestType=row[18];mapLimit=std::bit_cast<std::int32_t>(row[11]);trace.clear();
        // 부모/포획 Form/새 사제의 실제 관측 전 raw다.
        for (const auto sid:{Sid{50},Sid{51},Sid{52}}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,kVt);raw[10]=158;raw[11]=0;Put(raw,14,row[2]);Put(raw,18,row[3]); }
        Put(Raw(Sid{51}),12,row[8],2);frames.clear();
        // TYPE 번호와 원문의 8코드를 구성한다. 제품 프레임 조회 결과는 넣지 않는다.
        for (std::size_t kind=0;kind<188;++kind) {
            std::vector<FrameCode> codes;
            if (kind==70 || kind==71) {
                const Sid sid{static_cast<std::uint16_t>(kind-10)};Raw(sid)[10]=static_cast<std::uint8_t>(kind);Put(Raw(sid),36,row[13]);types[kind].flags2=row[12]^(kind==71 ? 4U : 0U);
                // 같은 고정 코드 패턴 입력을 제공한다.
                for (std::uint8_t index=0;index<8;++index) codes.push_back({65,0,index,static_cast<std::uint8_t>(row[14]^(index*17U)^(kind==71 ? 32U : 0U))});
            }
            frames.emplace_back(std::move(codes));
        }
        // 조회 좌표와 독립인 hash0 입력 패턴이다.
        for (std::size_t index=0;index<surface.size();++index) surface[index]=static_cast<std::uint16_t>(row[16] ? (index%7==0 ? 0 : 60+index%2) : row[15] ? 60 : 0);
        // 원문의 모든 이전 소유자 후보에 같은 명시 위치 입력을 둔다.
        for (std::size_t owner=0;owner<128;++owner) { Put(players,owner*0xb4+0xa8,row[9]);Put(players,owner*0xb4+0xac,row[10]); }
        if (row[0]==3) { Put(Raw(Sid{50}),0,row[2]);Put(Raw(Sid{50}),4,row[3]); }
    }
    // 기존 검색/생성/가상 부작용만 원문과 같은 경계로 공급한다.
    CarrierFallingHooks Hooks() {
        CarrierFallingHooks hooks;
        hooks.find=[this](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { Event("M",{sid.value,mask,kind,index});if (row[17]==2) boss=false;return Sid{static_cast<std::uint16_t>(row[6])}; };
        hooks.findKind=[this](Sid sid,std::uint32_t kind) { Event("K",{sid.value,kind,0,0});if (row[17]==3) { priestType=159;Put(Raw(Sid{51}),12,0x2342,2); }return Sid{static_cast<std::uint16_t>(row[7])}; };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { Event("C",{type,flags});if (row[17]==4) Put(Raw(Sid{51}),12,0x3443,2);return Sid{52}; };
        hooks.setWord=[this](Sid sid,std::uint32_t word) { Event("W",{sid.value,word});Put(Raw(sid),12,word,2);if (row[17]==5) Put(Raw(sid),0,kChangedVt); };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) { Event(Get(Raw(sid),0)==kChangedVt ? "o" : "O",{sid.value,owner});Raw(sid)[34]=static_cast<std::uint8_t>(owner);if (row[17]==6) { Raw(Sid{51})[12]=4;Put(Raw(sid),0,kChangedVt); } };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event(Get(Raw(sid),0)==kChangedVt ? "d" : "D",{sid.value,flags});Raw(sid)[11]|=2; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("U",{sid.value,flags});Raw(sid)[11]|=4;if (row[17]==1) { Put(Raw(sid),0,kChangedVt);Put(Raw(sid),14,std::bit_cast<std::uint32_t>(42.5F));Put(Raw(sid),18,std::bit_cast<std::uint32_t>(43.5F)); } };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event(Get(Raw(sid),0)==kChangedVt ? "p" : "P",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags});Put(Raw(sid),14,std::bit_cast<std::uint32_t>(x));Put(Raw(sid),18,std::bit_cast<std::uint32_t>(y));Raw(sid)[11]&=static_cast<std::uint8_t>(~4U);if (row[17]==9) { Put(Raw(sid),0,kChangedVt);Put(Raw(Sid{51}),0,kChangedVt); } };
        hooks.fallWalker=[this](Sid sid) { Event(Get(Raw(sid),0)==kChangedVt ? "f" : "F",{sid.value});if (row[17]==10) { Put(Raw(Sid{51}),0,kChangedVt);Put(Raw(Sid{50}),0,kChangedVt); }return true; };
        hooks.assertion=[this](std::uint32_t line) { Event("A",{line});if (row[17]==7) boss=true;if (row[17]==8) { std::fill(players.begin(),players.end(),std::uint8_t{});Put(Raw(Sid{52}),0,kChangedVt); } };
        return hooks;
    }
};
}
// 모든 기대값은 독립 PE 관측에서만 읽는다.
TEST_CASE(CarrierFalling_NativeGroundFallReleaseAndPoint) {
    CarrierFallingScene scene;std::size_t count{},diagnostics{}; // 실제 원문 개수와 제한된 불일치 출력 수다.
    // 53비트 관측을 재생하며 64비트 관측도 원문에 별도 보존한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_FALLING_FIXTURE).rows) {
        CHECK(fields.size()==11);if (fields.size()!=11) continue;scene.Prepare(FallingNumbers(fields[0]));RawCarrierFalling logic(scene.pool,scene.types,scene.frames,scene.surface,scene.players,scene.boss,scene.checking,scene.priestType,scene.mapLimit,scene.Hooks());std::uint32_t result{};
        if (scene.row[0]==0) result=logic.NeedsEscape(std::bit_cast<float>(scene.row[2]),std::bit_cast<float>(scene.row[3]));
        else if (scene.row[0]==1) result=std::bit_cast<std::uint32_t>(logic.Fall(Sid{50},scene.row[1]));
        else if (scene.row[0]==2) logic.ReleasePriest(Sid{50});
        else result=logic.ValidPoint(std::bit_cast<float>(scene.row[2]),std::bit_cast<float>(scene.row[3]));
        const auto raws=Hex(scene.Raw(Sid{50}))+","+Hex(scene.Raw(Sid{51}))+","+Hex(scene.Raw(Sid{52}));const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if ((result!=Number(fields[1]) || trace!=fields[2] || raws!=fields[3] || scene.boss!=(Number(fields[4])!=0) || scene.priestType!=Number(fields[5])) && diagnostics++<6) std::cout<<"캐리어 입력 "<<fields[0]<<"\n실제 "<<result<<' '<<trace<<' '<<raws<<"\n원본 "<<fields[1]<<' '<<fields[2]<<' '<<fields[3]<<'\n';
        CHECK(result==Number(fields[1]));CHECK(trace==fields[2]);CHECK(raws==fields[3]);CHECK(scene.boss==(Number(fields[4])!=0));CHECK(scene.priestType==Number(fields[5]));++count;
    }
    CHECK(count==9884);
    std::size_t tables{}; // 같은 실제 수명 분배를 선택한 원본 가상 표 수다.
    // 사제 보호막의 공통 수명과 사제/캐리어의 서로 다른 낙하 슬롯을 원문으로 확인한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_FALLING_VTABLES).rows) {
        CHECK(fields.size()==8);if (fields.size()!=8) continue;CHECK(Number(fields[1])==0x4af780U);CHECK(Number(fields[4])==0x4ad4a0U);CHECK(Number(fields[5])==0x4afe50U);CHECK(Number(fields[6])==0x4b02d0U);
        if (Number(fields[0])==0x507770U) { CHECK(Number(fields[2])==0x4b0950U && Number(fields[3])==0x4b0840U && Number(fields[7])==0x4437f0U); }
        else if (Number(fields[0])==0x50f210U) CHECK(Number(fields[7])==0x494e80U);
        else { CHECK(Number(fields[2])==(Number(fields[0])==0x5040c0U ? 0x42ae80U : 0x426890U));CHECK(Number(fields[3])==0x44b9e0U && Number(fields[7])==0x4287d0U); }++tables;
    }
    CHECK(tables==8);
}
// 선택된 하위가 실제 필요해지는 시점에 미연결을 거부한다.
TEST_CASE(CarrierFalling_LazyEffectsAndPhysicalFrameBounds) {
    CarrierFallingScene scene;scene.Prepare({0,101,0x41a40000U,0x41aa0000U,0,0,51,51,0x1281,0x41f40000U,0x41fc0000U,256,4,0,32,1,0,0,158});RawCarrierFalling logic(scene.pool,scene.types,scene.frames,scene.surface,scene.players,scene.boss,scene.checking,scene.priestType,scene.mapLimit,{});
    CHECK(logic.Fall(Sid{50},101)==1.0F);logic.ReleasePriest(Sid{50});CHECK(logic.NeedsEscape(-2,1)==0);CHECK(&logic.Pool()==&scene.pool);
    scene.boss=true;bool rejected{};try { static_cast<void>(logic.Fall(Sid{50},101)); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    Put(scene.Raw(Sid{60}),36,8);rejected=false;try { static_cast<void>(logic.NeedsEscape(20.5F,21.25F)); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}
