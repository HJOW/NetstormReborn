// 원본 전체 회피1807입력의 시도/깊이/시각/현재raw를 독립 재생한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierEscape.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 마지막현재시각은64비트이며 나머지 입력도 축소하지 않고 읽는다.
std::vector<std::uint64_t> EscapeNumbers(std::string_view text) { std::vector<std::uint64_t> result;/* 원문 순서대로 각 입력을 읽는다. */ for (const auto& part:Split(text,',')) result.push_back(std::stoull(part));return result; }
// 같은 명시BFS 배열만 제공하며 기대 시각이나 최적 후보는 계산하지 않는다.
void EscapeRoute(RawPathRuntime& output,std::uint32_t mode,std::uint32_t which,std::uint32_t mask) {
    if (!(mask&(1U<<(which-60)))) return;constexpr std::array<std::array<float,2>,4> lengths{{{2,4},{2,2},{4,2},{.125F,.25F}}}; // 관측기에 공급한 명시 두 후보의 경로다.
    const std::array<RawPathPoint,2> points{{{0,0},{lengths[mode][which-60],0}}};const std::array<std::int32_t,2> directions{0,0};output.SetPath(points,directions);
}
}
// 원문 전체의 네방향·표면후보·진단·새경로·삭제후무접근을대조한다.
TEST_CASE(CarrierEscape_NativeDepthDirectionsCurrentStateAndCandidates) {
    SidPool pool(OriginalEdition::Patch1078,24000);std::size_t count{},diagnostics{}; // 대조 수와 제한한 불일치 진단 수다.
    // 원본 기대값은 읽기만 하며 각 행을 같은 물리 입력으로 실행한다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIER_ESCAPE_FIXTURE).rows) {
        CHECK(row.size()==9);if (row.size()!=9) continue;const auto c=EscapeNumbers(row[0]);CHECK(c.size()==20);if (c.size()!=20) continue;
        const auto d=[&](std::size_t i) { return static_cast<std::uint32_t>(c[i]); }; // 실제 DWORD 입력 필드다.
        auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),0x10000);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});SquidHash hash;std::vector<RiftTypeRecord> types(188);
        const std::array<std::vector<std::uint16_t>,4> scenes{{{},{60},{60,61},{61,60}}};const auto& scene=scenes[d(10)]; // 같은 실제bucket의 원래 체인 입력이다.
        // 부모와 후보는 관측기와 같은 raw 바이트로 준비한다.
        for (const auto sid:{50U,60U,61U}) { auto p=raw.subspan(sid*50,50);Put(p,10,sid==50 ? d(6) : sid==60 ? 70 : 71,1);Put(p,11,0,1);Put(p,40,0,1);Put(p,33,1,1);Put(p,14,sid==50 ? d(16) : std::bit_cast<std::uint32_t>(sid==60 ? 22.25F : 22.75F));Put(p,18,sid==50 ? d(17) : std::bit_cast<std::uint32_t>(22.5F));Put(p,4,0,2); }
        // 같은bucket에서원래목록순서를사용하며마지막next는0이다.
        for (std::size_t i=0;i<scene.size();++i) Put(raw,scene[i]*50+4,i+1<scene.size() ? scene[i+1] : 0,2);
        if (!scene.empty()) hash.Bucket(1,22.25F,22.5F)=scene.front();types[70].footX=types[70].footY=types[71].footX=types[71].footY=1;types[70].flags1=(d(11)&1U) ? 0x800U : 0;types[71].flags1=(d(11)&2U) ? 0x800U : 0;
        // 현재 바뀔 수 있는 네 부모 타입의 genus 입력이다.
        for (const auto kind:{133,137,135,158}) types[kind].flags2=d(5);
        std::vector<std::uint8_t> spots(65536,static_cast<std::uint8_t>(d(7) ? 16 : 0));double now=std::bit_cast<double>(c[19]);bool checking=d(4)!=0;CarrierEscapeState state;const std::array<std::uint32_t,4> cardinal{137,156,135,187};
        auto old=std::make_unique<RawPathRuntime>(OriginalEdition::Patch1078);old->SetMoveAwayDepth(std::bit_cast<std::int32_t>(d(2)));RawPathRuntime fresh(OriginalEdition::Patch1078);fresh.SetMoveAwayDepth(12345);std::uint32_t starts{},active{};bool deleted{};std::string events;
        // 실제효과와관측한탐색영역을동일DWORD순서로기록한다.
        const auto event=[&](char name,std::initializer_list<std::uint32_t> args) { if (!events.empty()) events+=';';events+=name;for (const auto value:args) events+=':'+std::to_string(value); };
        CarrierEscapeHooks hooks;
        hooks.path=[&](Sid sid) { event('P',{sid.value});if (d(15)==1) { Put(raw,50*50+14,std::bit_cast<std::uint32_t>(25.25F));Put(raw,50*50+18,std::bit_cast<std::uint32_t>(26.25F));old->SetMoveAwayDepth(1); }return d(3) ? old.get() : nullptr; };
        hooks.random=[&](std::int32_t low,std::int32_t high) { event('R',{static_cast<std::uint32_t>(low),static_cast<std::uint32_t>(high)});if (d(15)==2) { Put(raw,50*50+14,std::bit_cast<std::uint32_t>(1.0F));Put(raw,50*50+18,std::bit_cast<std::uint32_t>(2.0F)); }return static_cast<std::int32_t>(d(9)); };
        hooks.start=[&](Sid sid,RawPathPoint point)->RawPathRuntime* { ++starts;event('S',{sid.value,std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y)});if (d(15)==3) { Put(raw,50*50+14,std::bit_cast<std::uint32_t>(42.25F));Put(raw,50*50+18,std::bit_cast<std::uint32_t>(43.5F));checking=false; }if (d(3) && old) { old.reset();deleted=true; }return d(8) && starts==d(8) ? &fresh : nullptr; };
        hooks.scan=[&](const SquidSearchArea& area) { event('F',{static_cast<std::uint32_t>(area.left),static_cast<std::uint32_t>(area.top),static_cast<std::uint32_t>(area.right),static_cast<std::uint32_t>(area.bottom)}); };
        hooks.blocker=[&](RawPathPoint point) {
            event('O',{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y)});active=0;
            // 현재 물리 후보의 조회 좌표만 대조해 명시 경계 입력을 고른다.
            for (const auto sid:{60U,61U}) if (Get(raw,sid*50+14)==std::bit_cast<std::uint32_t>(point.x) && Get(raw,sid*50+18)==std::bit_cast<std::uint32_t>(point.y)) { active=sid;break; }
            if (!active) throw std::logic_error("회피 검사 후보 좌표 입력 불일치");if (d(15)==4) { Put(raw,50*50+10,137,1);Put(raw,active*50+18,std::bit_cast<std::uint32_t>(23.5F)); }return Sid{static_cast<std::uint16_t>((d(12)&(1U<<(active-60))) ? 70 : 0)};
        };
        hooks.search=[&](RawPathRuntime& output,Sid sid,RawPathPoint source,RawPathPoint destination,std::uint32_t flags) {
            CHECK(sid==Sid{50});event('B',{std::bit_cast<std::uint32_t>(source.x),std::bit_cast<std::uint32_t>(source.y),std::bit_cast<std::uint32_t>(destination.x),std::bit_cast<std::uint32_t>(destination.y),flags});CHECK(output.Count()==0);EscapeRoute(output,d(14),active,d(13));
            if (d(15)==5) { now=1000.25;Put(raw,50*50+10,135,1); }if (d(15)==6 || d(15)==8) { Put(raw,active*50+14,std::bit_cast<std::uint32_t>(d(15)==6 ? 23.0F : 0.0F));Put(raw,active*50+18,std::bit_cast<std::uint32_t>(23.0F)); }
        };
        hooks.movement=[&](Sid sid) { return std::array<float,2>{raw[sid.value*50+10]==135 ? 3.25F : std::bit_cast<float>(d(18)),90}; };
        hooks.valid=[](RawPathPoint p) { return p.x>0 && p.x<256 && p.y>0 && p.y<256; };
        hooks.assertion=[&](std::uint32_t line) { event('A',{line});if (d(15)==7 || d(15)==9) old->SetMoveAwayDepth(0);if (d(15)==7) checking=false; };
        RawCarrierEscape escape(pool,hash,types,spots,now,checking,cardinal,state,std::move(hooks));CHECK(&escape.Pool()==&pool);const auto result=escape.Run(Sid{50},std::bit_cast<float>(d(0)),d(1));
        const auto current=std::to_string(std::bit_cast<std::uint32_t>(fresh.MoveAwayDepth()))+','+std::to_string(deleted)+','+std::to_string(checking)+','+std::to_string(std::bit_cast<std::uint64_t>(now));const auto raws=Hex(raw.subspan(50*50,50))+','+Hex(raw.subspan(60*50,50))+','+Hex(raw.subspan(61*50,50));
        if ((std::to_string(result)!=row[1] || events!=row[2] || current!=row[3] || raws!=row[4]) && diagnostics++<5) std::cout<<"회피입력 "<<row[0]<<" 효과 "<<events<<" / "<<row[2]<<" 상태 "<<current<<" / "<<row[3]<<'\n';
        CHECK(std::to_string(result)==row[1] && events==row[2] && current==row[3] && raws==row[4]);CHECK(std::equal(row.begin()+1,row.begin()+5,row.begin()+5));++count;
    }
    CHECK(count==1807);
}
// 단위거리의비행체조기반환은미선택경계를요구하지않고실제시작누락/CD혼합은거부한다.
TEST_CASE(CarrierEscape_EditionAndLateBoundaries) {
    SidPool pool(OriginalEdition::Patch1078,24000);SquidHash hash;std::vector<RiftTypeRecord> types(188);std::vector<std::uint8_t> spots(65536);double now{};bool checking{};CarrierEscapeState state;const std::array<std::uint32_t,4> cardinal{137,156,135,187};
    auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),pool.Bytes().size());Put(raw,50*50+10,133,1);Put(raw,50*50+14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,50*50+18,std::bit_cast<std::uint32_t>(21.0F));types[133].flags2=0x20000;
    CarrierEscapeHooks hooks;hooks.path=[](Sid)->RawPathRuntime* { return nullptr; };RawCarrierEscape escape(pool,hash,types,spots,now,checking,cardinal,state,hooks);CHECK(escape.Run(Sid{50},1,0)==1000);types[133].flags2=0x10000;CHECK(Throws([&] { escape.Run(Sid{50},1,0); }));
    CHECK(Throws([&] { SidPool cd(OriginalEdition::Cd1072,24000);RawCarrierEscape bad(cd,hash,types,spots,now,checking,cardinal,state,{}); }));
}
