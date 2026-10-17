// 전체 원본 Halt에서 독립 관측한 대상 사망 분기를 같은 입력/호출 순서로 재생한다.
#include "RawSceneSupport.h"
#include "o/RawPathRetarget.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원본 DWORD 입력과 관측 상태를 축소 없이 읽는다.
std::vector<std::uint32_t> RetargetNumbers(std::string_view text) {
    std::vector<std::uint32_t> result;
    // 현재 행의 모든 DWORD를 그대로 보존한다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// Runtime의 현재 부모는 SID API로 축소하지 않고 실제 DWORD 폭으로 입력한다.
std::span<std::uint8_t> RetargetBytes(RawPathRuntime& runtime) { const auto bytes=runtime.Bytes();return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
// 큰 내용물/선택/새 이동은 native 도구와 동일한 명시 효과 경계다.
std::pair<std::string,std::string> ReplayRetarget(SidPool& pool,const std::vector<std::uint32_t>& c) {
    auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),0x10000);std::fill(raw.begin(),raw.end(),std::uint8_t{0x2a});
    // 부모 가상 주소는 비교 대상인 원래 raw 입력이며 실제 함수 포인터가 아니다.
    for (const auto sid:{50U,51U,52U}) { Put(raw,sid*50,0x12200000);Put(raw,sid*50+10,133,1);Put(raw,sid*50+11,0,1);Put(raw,sid*50+40,0,1); }
    Put(raw,c[0]*50+11,c[2],1);Put(raw,c[1]*50+11,c[3],1);
    auto runtime=std::make_unique<RawPathRuntime>(OriginalEdition::Patch1078);runtime->SetAttachment(Sid{5},Sid{50});runtime->SetTargets(c[0],c[1]);Put(RetargetBytes(*runtime),8,c[4]);
    std::uint32_t nuggetType=c[11],finds{};std::vector<std::string> events;std::array<std::uint32_t,3> snapshot{};bool deleted{}; // 현재 전역/조회 수/삭제 전 마지막 필드 관측이다.
    // 실제 효과의 인자를 원본 로그와 같은 문자열로 기록한다.
    const auto event=[&](char name,std::initializer_list<std::uint32_t> args) { std::string row(1,name);for (const auto arg:args) row+=':'+std::to_string(arg);events.push_back(std::move(row)); };
    // 첫 조회/선택/이동의 외부 효과는 결과를 계산하지 않고 입력 모드만 적용한다.
    const auto mutate=[&](char phase) {
        const auto mode=c[10];auto bytes=RetargetBytes(*runtime);
        if (phase=='F' && finds==1) {
            if (mode==1) Put(bytes,0x186cc,0);if (mode==2) Put(bytes,0x186cc,62);if (mode==3) Put(bytes,0x186d0,0);
            if (mode==4) { Put(bytes,8,52);nuggetType=155; }
            if (mode==8) Put(raw,c[0]*50+11,0,1);if (mode==9) Put(raw,c[1]*50+11,0,1);
            if (mode==10) nuggetType=155;if (mode==11) Put(bytes,0x186d0,62);
        }
        if (phase=='C' && mode==5) { Put(bytes,0x186cc,62);Put(bytes,0x186d0,0); }
        if (phase=='Y' && mode==6) { Put(bytes,0x186d0,62);Put(bytes,0x186cc,0); }
        if (phase=='G' && mode==7) { Put(bytes,0x186cc,64);Put(bytes,0x186d0,65); }
    };
    PathRetargetHooks hooks;
    hooks.find=[&](Sid parent,std::uint32_t mask,std::uint32_t type,std::uint32_t index) { ++finds;event('F',{parent.value,mask,type,index});mutate('F');return Sid{static_cast<std::uint16_t>(finds==1 ? c[5] : c[6])}; };
    hooks.chooseReturn=[&](Sid parent) { event('C',{parent.value});mutate('C');return Sid{static_cast<std::uint16_t>(c[7])}; };
    hooks.chooseGeyser=[&](Sid parent) { event('Y',{parent.value});mutate('Y');return Sid{static_cast<std::uint16_t>(c[8])}; };
    // 원본의 go 경계와 같이 호출 중 Runtime을 실제 해제한다. Update는 이후 이를 읽으면 안 된다.
    const auto go=[&](char name,Sid parent,Sid target) {
        event(name,{parent.value,target.value,runtime->TargetA(),runtime->TargetB(),Get(runtime->Bytes(),8)});mutate('G');snapshot={runtime->TargetA(),runtime->TargetB(),Get(runtime->Bytes(),8)};
        if (c[9]) { deleted=true;runtime.reset(); }
    };
    hooks.goReturn=[&](Sid parent,Sid target) { go('R',parent,target); };hooks.goGeyser=[&](Sid parent,Sid target) { go('G',parent,target); };
    RawPathRetarget retarget(pool,nuggetType,std::move(hooks));CHECK(&retarget.Pool()==&pool);retarget.Update(*runtime,Sid{50});
    if (!deleted) snapshot={runtime->TargetA(),runtime->TargetB(),Get(runtime->Bytes(),8)};
    std::string effects; // 효과가 없는 행도 원문의 '-'와 동일하게 기록한다.
    for (const auto& item:events) { if (!effects.empty()) effects+=';';effects+=item; }if (effects.empty()) effects="-";
    const auto state=std::to_string(deleted)+','+std::to_string(snapshot[0])+','+std::to_string(snapshot[1])+','+std::to_string(snapshot[2])+','+std::to_string(nuggetType)+','+std::to_string(finds)+','+std::to_string(Adler(raw));return {effects,state};
}
}
// 두 대상/두 부모/현재 필드 변경/두 선택의 실패와 삭제 후 무접근을 원본4801행과 대조한다.
TEST_CASE(PathRetarget_NativeEffectsCurrentParentsAndDeletion) {
    SidPool pool(OriginalEdition::Patch1078,24000);std::size_t count{},diagnostics{}; // 행 수와 제한된 불일치 진단 수다.
    // 원본의53/64비트 정밀도 결과는 같고 C++는 같은 정수 분기를 한 번 실행한다.
    for (const auto& fields:LoadFixture(NETSTORM_PATH_RETARGET_FIXTURE).rows) {
        CHECK(fields.size()==5);if (fields.size()!=5) continue;const auto input=RetargetNumbers(fields[0]);CHECK(input.size()==12);if (input.size()!=12) continue;
        const auto [effects,state]=ReplayRetarget(pool,input);
        if ((effects!=fields[1] || state!=fields[2]) && diagnostics++<4) std::cout<<"대상 재확인 "<<fields[0]<<" 효과 "<<effects<<" / "<<fields[1]<<" 상태 "<<state<<" / "<<fields[2]<<'\n';
        CHECK(effects==fields[1] && state==fields[2]);CHECK(fields[1]==fields[3] && fields[2]==fields[4]);++count;
    }
    CHECK(count==4801);
}
// 선택하지 않은 경계는 요구하지 않으며 실제 죽은 대상의 미연결은 거부한다.
TEST_CASE(PathRetarget_LazyBoundariesAndEdition) {
    SidPool pool(OriginalEdition::Patch1078,24000);std::uint32_t nuggetType=154;RawPathRetarget retarget(pool,nuggetType,{});RawPathRuntime runtime(OriginalEdition::Patch1078);
    runtime.SetTargets(60,61);Put(RetargetBytes(runtime),8,0xffffffff);auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),pool.Bytes().size());Put(raw,60*50+11,1,1);Put(raw,61*50+11,1,1);
    CHECK(!Throws([&] { retarget.Update(runtime,Sid{50}); }));Put(raw,61*50+11,2,1);CHECK(Throws([&] { retarget.Update(runtime,Sid{50}); }));
    CHECK(Throws([&] { RawPathRuntime cd(OriginalEdition::Cd1072);retarget.Update(cd,Sid{50}); }));CHECK(Throws([&] { SidPool cd(OriginalEdition::Cd1072,24000);RawPathRetarget bad(cd,nuggetType,{}); }));
}
