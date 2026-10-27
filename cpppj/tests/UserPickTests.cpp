// 독립 원문 후보 순위/호출 순서를 대조하며 실제 공중3종 생성/생산 검사를 추가하지 않는다.
#include "RawSceneSupport.h"
#include "client/RawUserPick.h"
#include "o/RawSquidNeighbors.h"
using namespace netstorm::test::rawscene;
using namespace netstorm::client;

TEST_CASE(UserPick_ReplaysNative1092PrioritiesFiltersAndNearestFallback) { //원문fixture의 반환/가상hit 호출 순서/전체 슬롯 불변성을 대조한다.
    SidPool pool(OriginalEdition::Patch1078,24000,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::size_t count{}; //기존 풀의 서버 영역을 포함하고 순위 검사에서 사용하지 않는 지도도 실제 중심 처리기 계약을 만족한다.
    for (const auto& row:LoadFixture(NETSTORM_USER_PICK_FIXTURE).rows) { //기대값은 원문 결과이고 합성 raw 입력은 두 제품이 동일하다.
        CHECK(row.size()==4);const auto header=Split(row[0],','),objects=Split(row[1],';');std::vector<RiftTypeRecord> types(188);std::vector<std::uint32_t> ids;std::vector<bool> hits;std::string trace;std::size_t cursor{};
        for (const auto& encoded:objects) { //TYPE80..87의 합성 역할만 설치한다. 실제 세 공중 유닛의 생성자/VT는 실행하지 않는다.
            const auto item=Split(encoded,',');CHECK(item.size()==12);const auto sid=Number(item[0]),type=Number(item[1]);ids.push_back(sid);hits.push_back(Number(item[9])!=0);types[type].flags1=Number(item[2]);types[type].flags2=Number(item[3]);types[type].footX=static_cast<int>(Number(item[10]));types[type].footY=static_cast<int>(Number(item[11]));
            auto bytes=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);raw[34]=static_cast<std::uint8_t>(Number(item[4]));raw[35]=static_cast<std::uint8_t>(std::stoi(item[5]));raw[40]=static_cast<std::uint8_t>(Number(item[8]));Put(raw,14,Number(item[6]));Put(raw,18,Number(item[7]));
        }
        const std::vector<std::uint8_t> before(pool.Bytes().begin(),pool.Bytes().end());RawSquidNeighbors footprint(pool,hash,spots,types,frames);UserPickHooks hooks; //기존 원본4adea0 중심을 새 순위 몸체에 직접 연결한다.
        hooks.paused=[&] { trace="paused";return Number(header[0]); };hooks.localOwner=[&] { return Number(header[1]); };hooks.metamap=[&] { return Number(header[2]); };hooks.hideBuildings=[&] { return Number(header[3]); };
        hooks.project=[&](auto x,auto y) { CHECK(x==480 && y==330);trace+=";project";return std::array<float,2>{std::bit_cast<float>(Number(header[5])),std::bit_cast<float>(Number(header[6]))}; };
        hooks.begin=[&](auto point,int x,int y) { CHECK(std::bit_cast<std::uint32_t>(point[0])==Number(header[5]) && std::bit_cast<std::uint32_t>(point[1])==Number(header[6]));trace+=";begin:"+std::to_string(x)+','+std::to_string(y);cursor=0;return ids.front(); };
        hooks.next=[&] { return ++cursor<ids.size() ? ids[cursor] : 0U; };hooks.center=[&](Sid sid) { return footprint.Center(sid); };
        hooks.hit=[&](Sid sid,auto x,auto y) { CHECK(x==480 && y==330);trace+=";hit:"+std::to_string(sid.value);return hits[sid.value-50]; };
        CHECK(RawUserPick(pool,types,std::move(hooks)).Pick(480,330,Number(header[4])).value==Number(row[2]));CHECK(trace==row[3]);CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));++count;
    }
    CHECK(count==1092);
}

TEST_CASE(UserPick_OnlyReachedBoundariesAreRequired) { //정지/빈 검색에서 불필요한 하위를 호출하지 않고 실제 도달한 누락은 거부한다.
    SidPool pool(OriginalEdition::Patch1078,24000,true);std::vector<RiftTypeRecord> types(188);UserPickHooks hooks;hooks.paused=[] { return 1U; };
    CHECK(RawUserPick(pool,types,hooks).Pick(0,0).value==0);hooks.paused=[] { return 0U; };
    CHECK(Throws([&] { static_cast<void>(RawUserPick(pool,types,hooks).Pick(0,0)); }));
    hooks.project=[](auto,auto) { return std::array<float,2>{}; };hooks.metamap=[] { return 0U; };hooks.begin=[](auto,int,int) { return 24000U; };
    CHECK(RawUserPick(pool,types,hooks).Pick(0,0).value==0);hooks.begin=[](auto,int,int) { return 0U; };CHECK(RawUserPick(pool,types,hooks).Pick(0,0).value==0);
}
