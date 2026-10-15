// 실제 신전 Pre 원본102입력의 현재 Player/세 객체/물리 목록/호출 순서를 전수 대조한다.
#include "RawSceneSupport.h"
#include "o/RawVortexPreDestroy.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,3> kVortexPreIds{50,60,61}; // 원본 관찰의 부모와 두 outpost SID 입력이다.
// 실제 Pool과 서로 다른 등록 목록을 같은 신전 Pre 객체에 공급한다.
struct VortexScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 원본의 고정 free-list 영역을 실제 확보하며 선택 대상은50/60/61이다.
    std::array<std::uint8_t,1620> players{}; // 현재9개 실제 Player 원문이다.
    SquidPostPopList temples,residences,outposts; // 압축 뒤 물리 꼬리를 보존할 서로 다른 세 목록이다.
    std::uint32_t variant{},mutation{},loading{}; // 실제 외부 현재 입력이며 callback 변경도 공유한다.
    std::string trace; // 실제로 선택된 외부 하위만 순서대로 기록한다.
    std::unique_ptr<RawVortexPreDestroy> body; // 위 자료보다 먼저 해제할 실제 제품 몸체다.
    // 판단 결과를 계산하지 않고 native와 같은 raw 입력을 실제 할당 슬롯에 설치한다.
    VortexScene(std::uint32_t input,std::uint32_t change):variant(input),mutation(change),loading((input&256U) ? 1U : 0U) {
        // 마지막 검사 대상61까지 실제 클라이언트 슬롯을 차례로 확보한다.
        for (;;) { const auto sid=pool.Allocate(2);if (!sid.value || sid.value>61) throw std::logic_error("신전 검사 실제 SID 준비 오류");if (sid.value==61) break; }
        players.fill(0xa5);const auto owner=(variant>>2)&15U,match=(variant>>6)&3U;
        // 네 신전 참조를 owner0까지 독립 입력으로 준비한다.
        for (std::uint32_t player=0;player<9;++player) { Put(players,player*180+0x7c,(match&1U) ? 50U : 99U);Put(players,player*180+0x80,99);Put(players,player*180+0x84,(match&2U) ? 50U : 99U);Put(players,player*180+0x88,99); }
        // 각 실제 할당 슬롯에 native와 같은 전체50BYTE를 복사한다.
        for (const auto sid:kVortexPreIds) {
            std::array<std::uint8_t,50> bytes{};bytes[10]=sid==50 ? 124 : 126;bytes[11]=sid==50 ? 2 : 0;bytes[34]=static_cast<std::uint8_t>(sid==61 ? (owner+1)%9 : owner);bytes[40]=sid==50 ? std::array<std::uint8_t,4>{0,1,8,9}[variant&3U] : 0;
            Put(bytes,14,std::bit_cast<std::uint32_t>(static_cast<float>(sid)+.25F));Put(bytes,18,std::bit_cast<std::uint32_t>(static_cast<float>(sid)+.5F));std::copy(bytes.begin(),bytes.end(),pool.AllocatedBytes(Sid{sid}).begin());
        }
        temples.entries={50,60,50,61};temples.count=4;residences.entries={62,50,63,50};residences.count=4;outposts.entries={60,61};outposts.count=2;
        body=std::make_unique<RawVortexPreDestroy>(pool,players,temples,residences,outposts,loading,Hooks());
    }
    // 각 호출 시점의 인자 비트를 문자열로 기록한다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 원본이 외부로 선택한 작은 경계의 현재 반환/변경 입력만 공급한다.
    VortexPreDestroyHooks Hooks() {
        VortexPreDestroyHooks h;
        h.islandNumber=[this](float x,float y) { Event("Island,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y)));return 2; };
        h.islandExists=[this](std::int32_t island) { Event("Exists,"+std::to_string(island));return !(variant&512U); };
        h.islandTemple=[this](std::int32_t island) { Event("Temple,"+std::to_string(island));return (variant&1024U) ? 99U : 50U; };
        h.clearIslandTemple=[this](std::int32_t island) { Event("Clear,"+std::to_string(island)); };
        h.islandOwner=[this](std::int32_t island,std::uint32_t owner) { Event("Owner,"+std::to_string(island)+','+std::to_string(owner)); };
        h.regionOwnership=[this](float x,float y) { Event("Region,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))); };
        h.damageablePre=[this](Sid sid,std::uint32_t flags) { Event("Base,"+std::to_string(flags));auto raw=pool.AllocatedBytes(sid);if (mutation==1) raw[34]=2;if (mutation==2) raw[40]=9;if (mutation==3) loading=1; };
        h.refreshProduction=[this] { Event("Refresh");if (mutation==4) pool.AllocatedBytes(Sid{50})[34]=8; };
        h.assertion=[this](std::uint32_t line) { Event("Assert,"+std::to_string(line)); };return h;
    }
    // 출력은 실제 비교 대상 세 슬롯의 모든150BYTE다.
    std::string Objects() const { std::string result;for (const auto sid:kVortexPreIds) result+=Hex(pool.Slot(Sid{sid}));return result; }
    // 논리 count와 사용하지 않는 물리 꼬리까지 원문 순서로 직렬화한다.
    std::string Lists() const {
        std::array<std::uint8_t,48> bytes{};Put(bytes,0,temples.count);Put(bytes,4,residences.count);std::size_t offset=8;
        // 각 목록은 같은 초기 물리 capacity를 그대로 유지한다.
        for (const auto* list:{&temples,&residences,&outposts}) for (const auto value:list->entries) { Put(bytes,offset,value);offset+=4; }return Hex(bytes);
    }
};
}
// 원본 전체 출력을 비교하여 owner/extra/로딩 변경 뒤 현재 재읽기를 검증한다.
TEST_CASE(VortexPreDestroy_Native102) {
    const auto fixture=LoadFixture(NETSTORM_VORTEX_PRE_FIXTURE);CHECK(fixture.rows.size()==103 && fixture.rows.front()[0]=="input");
    // 헤더 이외의102개를 모두 새로운 실제 입력 저장소에서 재생한다.
    for (std::size_t index=1;index<fixture.rows.size();++index) {
        const auto& row=fixture.rows[index];const auto input=Split(row[0],',');CHECK(row.size()==6 && input.size()==3);VortexScene scene(Number(input[0]),Number(input[1]));scene.body->PreDestroy(Sid{50},Number(input[2]));
        if (Hex(scene.players)!=row[1] || scene.Objects()!=row[2] || scene.Lists()!=row[3] || scene.trace!=row[5]) std::fprintf(stderr,"신전 Pre 불일치 %s\n실제%s\n기대%s\n",row[0].c_str(),scene.trace.c_str(),row[5].c_str());
        CHECK(Hex(scene.players)==row[1]);CHECK(scene.Objects()==row[2]);CHECK(scene.Lists()==row[3]);CHECK(scene.loading==Number(row[4]));CHECK(scene.trace==row[5]);
    }
}
// 필수 Damageable이 없으면 목록/지역의 앞선 실제 효과 뒤 원본 Player 정리를 실행하지 않는다.
TEST_CASE(VortexPreDestroy_RequiredDamageableAndStorage) {
    VortexScene scene(196,0);const auto before=scene.players;auto hooks=scene.Hooks();hooks.damageablePre={};
    RawVortexPreDestroy missing(scene.pool,scene.players,scene.temples,scene.residences,scene.outposts,scene.loading,std::move(hooks));
    CHECK(Throws([&] { missing.PreDestroy(Sid{50},0x400000); }));CHECK(scene.temples.count==2 && scene.residences.count==2 && scene.players==before);
    CHECK(Throws([&] { RawVortexPreDestroy bad(scene.pool,std::span(scene.players).first(180),scene.temples,scene.residences,scene.outposts,scene.loading,scene.Hooks()); }));
}
