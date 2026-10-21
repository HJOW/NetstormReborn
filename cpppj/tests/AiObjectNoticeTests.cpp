// 실제 본체/목록의 복합 분기와 현재 Player 포인터를 검사한다. 중단 지시에 따라 작성·컴파일만 한다.
#include "o/RawAiObjectNotice.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 결과를 계산하지 않고 실제 등록 메모리에 명시 입력을 배치하는 검사 도구다.
struct NoticeInput {
    RawAiBrainMemory memory{0x40000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); }}; // 미초기화 메모리의 명시 입력이며 native heap 관측값이 아니다.
    SidPool pool{OriginalEdition::Patch1078,32768}; // 같은 실제 풀의 할당 슬롯을 사용한다.
    Sid sid{pool.Allocate(2)}; // free 목록을 우회하지 않고 확보한 통지 대상이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 분기 결합을 위한 명시 genus 입력이다.
    std::array<std::uint8_t,1620> players{}; // 매번 조회하는 실제 Player9 입력이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},outpost{126}; // 실제 본체 핸들과 현재 제외 타입이다.
    double now{16777216.25};bool disabled{}; // float 축소와 구분되는 현재 QWORD 및 모드다.
    std::vector<std::uint32_t> distances;std::vector<AiObjectLog> logs; // 하위 선택 순서/표시 인수의 관찰값이다.
    std::unique_ptr<RawAiObjectNotice> notice; // 같은 입력 저장소보다 먼저 해제한다.
    // 원본 생성자를 대신했다고 주장하지 않는 명시 본체/목록 입력을 준비한다.
    NoticeInput() {
        auto raw=memory.Bytes(token);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(players,180+0xb0,token);
        constexpr std::array<std::size_t,16> lists{0x284,0x2dc,0x2e8,0x2f4,0x300,0x30c,0x318,0x324,0x330,0x33c,0x348,0x354,0x360,0x36c,0x378,0x384}; // 원본 생성자의16개 목록 위치다.
        // 모든 목록에 실제 backing을 확보하되 inactive 꼬리는 a7로 유지한다.
        for (const auto at:lists) { Put(raw,at,memory.Allocate(16));Put(raw,at+4,4); }
        auto object=pool.AllocatedBytes(sid);object[10]=71;object[34]=1;Put(object,14,std::bit_cast<std::uint32_t>(26.5F));Put(object,18,std::bit_cast<std::uint32_t>(-12.5F));
        AiObjectNoticeHooks hooks;hooks.carrierTargets=[](Sid) { return std::array<std::uint32_t,2>{701,999}; };
        hooks.distanceSquared=[this](Sid first,std::uint32_t second) { CHECK(first==sid);distances.push_back(second);return distances.size()==2 ? 180.499 : 180.5; };
        hooks.log=[this](auto brain,const auto& log) { CHECK(brain==token);logs.push_back(log); };
        notice=std::make_unique<RawAiObjectNotice>(memory,pool,types,players,now,outpost,[this] { return disabled; },std::move(hooks));
    }
    // 본체의 현재 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) { return Get(memory.Bytes(token),at); }
    // 본체의 현재 DWORD만 덮어쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 실제 목록 포인터의 활성 입력을 준비하고 비활성 꼬리는 보존한다.
    void List(std::size_t at,std::initializer_list<std::uint32_t> values) { std::size_t index{};for (const auto value:values) Put(memory.Bytes(Field(at)),4*index++,value);Field(at+8,static_cast<std::uint32_t>(values.size())); } // 반복문은 전달 순서대로 입력 DWORD만 쓴다.
    // 활성 목록을 결과 관찰값으로 복사하며 판단을 대신하지 않는다.
    std::vector<std::uint32_t> List(std::size_t at) { std::vector<std::uint32_t> values;for (std::uint32_t index=0;index<Field(at+8);++index) values.push_back(Get(memory.Bytes(Field(at)),index*4));return values; } // 반복문은 현재 count만 읽는다.
    // 타이머의 실제 QWORD 비트를 double로 해석한다.
    double Time(std::size_t at) { return std::bit_cast<double>((std::uint64_t(Field(at+4))<<32)|Field(at)); }
};
// 비활성/signed owner/NULL은 본체를 선택하지 않고 다음 호출은 현재 Player 포인터를 읽는다.
TEST_CASE(AiObjectNotice_CurrentPlayerAndPhysicalContracts) {
    NoticeInput input;input.disabled=true;input.Field(0x6d4+71*4,7);Put(input.players,180+0xb0,0xdead0000);input.notice->Notice(1,input.sid,true);input.disabled=false;
    // signed 비양수와 NULL은 잘못된 포인터나 SID에 도달하지 않는다.
    for (const auto owner:{0U,0xffffffffU,0x80000000U,8U}) input.notice->Notice(owner,Sid{65535},true);
    CHECK(input.Field(0x6d4+71*4)==7 && input.logs.empty());CHECK(Throws([&] { input.notice->Notice(9,input.sid,true); }));CHECK(Throws([&] { input.notice->Notice(1,input.sid,true); }));
    Put(input.players,180+0xb0,input.token);input.notice->Notice(1,input.sid,true);CHECK(input.Field(0x6d4+71*4)==8 && &input.notice->Pool()==&input.pool);
    const auto small=input.memory.Allocate(16);Put(input.players,180+0xb0,small);CHECK(Throws([&] { input.notice->Notice(1,input.sid,true); }));input.memory.Free(small);CHECK(Throws([&] { input.notice->Notice(1,input.sid,true); }));
}
// 복합 genus의 실제 중복 정책/거리 전체 방문/타이머 정밀도를 독립 명시 결과와 대조한다.
TEST_CASE(AiObjectNotice_CombinedCreationKeepsDistinctListRules) {
    NoticeInput input;const auto sid=input.sid.value;input.types[71].flags2=0x4000000|0x400000|0x30000|0x800|0x4000|0x200;input.Field(0x294,101);input.List(0x300,{101,102,103});input.List(0x2dc,{sid,sid});input.List(0x2f4,{sid});input.List(0x30c,{101});input.List(0x2e8,{8,8,9});input.Field(0x2ec,3);input.Field(0x3b0,4);input.Field(0x3c8,0xfffffffe);input.Field(0x6d4+71*4,7);input.Field(0x228,std::bit_cast<std::uint32_t>(3.25F));input.Field(0x5b8,1);input.Field(0x5c0,1);
    input.notice->Notice(1,input.sid,true);
    CHECK(input.Field(0x6d4+71*4)==8 && input.Field(0x2a4)==sid && input.Field(0x2c8)==1 && input.Field(0x3c8)==1);
    CHECK(input.distances==std::vector<std::uint32_t>({101,101,102,103}));CHECK(input.List(0x360)==std::vector<std::uint32_t>({sid}));CHECK(input.List(0x2dc)==std::vector<std::uint32_t>({sid,sid,sid}));CHECK(input.List(0x2f4)==std::vector<std::uint32_t>({sid,sid}));CHECK(input.List(0x2e8)==std::vector<std::uint32_t>({9,sid}));CHECK(input.List(0x30c)==std::vector<std::uint32_t>({101,sid}));
    CHECK(input.Time(0x5a8)==16777219.5 && input.Time(0x5b0)==16777220.5 && input.Time(0x5b8)==0 && input.Time(0x5c0)==0);
    CHECK(input.logs.size()==1);if (!input.logs.empty()) CHECK(input.logs[0].kind==AiObjectLogKind::Built && input.logs[0].type==71 && input.logs[0].x==26 && input.logs[0].y==-12);
}
// 삭제의 중복 압축/가득 찬 제외 목록/시설 재예약과 signed 감김을 함께 검사한다.
TEST_CASE(AiObjectNotice_CombinedDeletionReschedulesAndKeepsInactiveTail) {
    NoticeInput input;const auto sid=input.sid.value;input.types[71].flags2=0x4000000|0x400000|0x30000|0x800|0x4200;input.Field(0x248,8);input.Field(0x294,sid);input.Field(0x2a4,sid);input.Field(0x3f8,sid);input.Field(0x3fc,sid);input.Field(0x3b0,3);
    input.List(0x330,{700,701,702});input.List(0x33c,{900});input.Field(0x340,1);input.List(0x384,{sid,21,sid});input.List(0x2dc,{sid,12,sid,13});input.List(0x360,{sid,51,sid});input.List(0x36c,{sid,22});input.List(0x378,{sid});input.List(0x300,{sid,40,41,sid});input.List(0x30c,{sid,40});input.List(0x318,{3,sid});input.List(0x2e8,{sid,sid,51});input.List(0x2f4,{sid});
    input.notice->Notice(1,input.sid,false);
    CHECK(input.Field(0x6d4+71*4)==0xffffffff && !input.Field(0x294) && !input.Field(0x2a4) && !input.Field(0x3f8) && !input.Field(0x3fc) && !input.Field(0x2c8));
    CHECK(input.List(0x330)==std::vector<std::uint32_t>({700,702}) && input.List(0x33c)==std::vector<std::uint32_t>({900}));CHECK(input.List(0x384)==std::vector<std::uint32_t>({21}) && input.Time(0x5e0)==16777246.25);
    CHECK(input.List(0x2dc)==std::vector<std::uint32_t>({12,13}) && Get(input.memory.Bytes(input.Field(0x2dc)),8)==sid);CHECK(input.List(0x360)==std::vector<std::uint32_t>({51}) && input.List(0x36c)==std::vector<std::uint32_t>({22}) && input.List(0x378).empty());CHECK(input.List(0x300)==std::vector<std::uint32_t>({40,41}) && input.List(0x30c)==std::vector<std::uint32_t>({40,41}) && input.List(0x318)==std::vector<std::uint32_t>({3}));CHECK(input.List(0x2e8)==std::vector<std::uint32_t>({51}) && input.List(0x2f4).empty());
    CHECK(input.Field(0x3e4)==6 && input.Field(0x3f0)==1 && !input.Field(0x3e0) && !input.Field(0x3f4) && !input.Field(0x400) && !input.Field(0x3ec));CHECK(input.logs.size()==2);if (input.logs.size()==2) CHECK(input.logs[0].kind==AiObjectLogKind::MadeGeyserIneligible && input.logs[1].kind==AiObjectLogKind::Built);
}
// 사제도 경로 정리는 수행하되 일반 수집자 Built 처리는 없고 현재 outpost 제외 번호를 존중한다.
TEST_CASE(AiObjectNotice_PriestAndCurrentOutpostBranches) {
    NoticeInput input;input.types[71].flags2=0x230000;input.Field(0x2a0,input.sid.value);input.Field(0x29c,input.sid.value);input.Field(0x248,8);input.notice->Notice(1,input.sid,false);CHECK(!input.Field(0x2a0) && !input.Field(0x29c) && input.logs.size()==1 && input.logs[0].kind==AiObjectLogKind::CannotExcludeGeyser);
    input.logs.clear();input.types[71].flags2=0x4000;input.outpost=71;input.Field(0x5c0,1);input.notice->Notice(1,input.sid,true);CHECK(input.List(0x300).empty() && input.List(0x30c).empty() && input.Time(0x5c0)==0 && input.logs.empty());
    input.outpost=126;input.notice->Notice(1,input.sid,true);CHECK(input.List(0x300)==std::vector<std::uint32_t>({input.sid.value}));
}
}
