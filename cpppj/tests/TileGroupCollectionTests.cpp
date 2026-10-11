// 실제 행별 묶음 기계어 관찰과 SID/해시→공통 목록→kind 1 분배 연결을 검사한다.
#include "ConstructionSupport.h"
#include "client/TileGroupCollection.h"
#include "client/DrawSort.h"
#include <cstring>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 기계어 관찰의 가상 포인터 토큰 기준과 전체 출력 폭이다. 호스트 주소로 해석하지 않는다.
constexpr std::uint32_t kHeaders=0x15000000,kMembers=0x15001000,kPool=0x15030000;
constexpr std::size_t kMembersSize=96*16,kHeadersSize=64*8;
// 완성 fixture의 16진 입력 슬롯만 바이트로 읽는다.
std::vector<std::uint8_t> Bytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 두 글자가 입력 바이트 하나다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
// 기대 목록은 fixture에서만 읽고 실제 복원 수집기의 전체 연속 배열/헤더를 대조한다.
void Replay(const std::string& edition) {
    static const auto rows=LoadFixture(NETSTORM_TILEGROUP_FIXTURE).rows;
    SidPool pool(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,24000,false);
    SquidHash hash;std::size_t replayed=0;
    // 각 입력의 완성 슬롯/희소 해시와 원본의 현재 선택 단계를 실제 C++ 계층에 공급한다.
    for (const auto& row:rows) if (row[0]==edition) {
        const auto config=Split(row[2],',');hash.Reset();hash.Select(std::stoi(config[6]));
        if (row[3]!="-") {
            // y/x의 선형 인덱스는 해시 입력 위치이며 기대 방문 범위를 계산하지 않는다.
            for (const auto& cell:Split(row[3],';')) {
                const auto values=Inputs(cell);hash.Cell(0,static_cast<int>(values[0]%256),static_cast<int>(values[0]/256))=static_cast<std::uint16_t>(values[1]);
            }
        }
        // 타입/state/next와 미사용 바이트까지 원본 슬롯의 완성 입력을 복사한다.
        for (const auto& slot:Split(row[4],';')) {
            const auto pair=Split(slot,'=');const auto raw=Bytes(pair[1]);const Sid sid{static_cast<std::uint16_t>(Number(pair[0]))};
            CHECK(raw.size()==pool.Layout().stride);std::copy(raw.begin(),raw.end(),InputRaw(pool,sid).begin());
        }
        const std::vector<std::uint8_t> inputPool(pool.Bytes().begin(),pool.Bytes().end());
        const std::vector<std::uint16_t> inputHash(hash.Entries(0).begin(),hash.Entries(0).end());
        TileGroupCollection collector(hash,pool,{std::stoi(config[5]),kPool,kHeaders});bool covered=false;
        const RenderWorldRect view{Float(Number(config[0])),Float(Number(config[1])),Float(Number(config[2])),Float(Number(config[3]))};
        const auto groups=collector.Collect(view,std::stoi(config[4]),covered);const auto entries=collector.Entries();
        std::array<std::uint8_t,kMembersSize> members;members.fill(0xa5);std::array<std::uint8_t,kHeadersSize> headers;headers.fill(0xa5);
        CHECK(entries.size_bytes()<=members.size());std::memcpy(members.data(),entries.data(),entries.size_bytes());
        // 실제 C++ span의 주소만 격리 포인터로 직렬화한다. 헤더 개수/경계/멤버 결과는 계산하지 않는다.
        for (std::size_t i=0;i<groups.size();++i) {
            const auto& group=groups[i];const auto offset=static_cast<std::uint32_t>(group.entries.data()-entries.data())*16;
            Put(headers,i*8,kMembers+offset);Put(headers,i*8+4,static_cast<std::uint32_t>(*group.count));
            CHECK(collector.Group(group.token).entries.data()==group.entries.data());
        }
        // 모든 실제 객체 토큰을 같은 풀의 슬롯 번호로 되돌릴 수 있어야 한다.
        for (const auto& entry:entries) CHECK(collector.Object(entry.token).value==(entry.token-kPool)/pool.Layout().stride);
        const bool same=groups.size()==std::stoul(row[5]) && covered==(row[6]=="1") && Hex(members)==row[7] && Hex(headers)==row[8];
        CHECK(same);CHECK(hash.Selected()==2 && row[9]=="2,4,64");CHECK(std::equal(inputPool.begin(),inputPool.end(),pool.Bytes().begin()));
        CHECK(std::equal(inputHash.begin(),inputHash.end(),hash.Entries(0).begin()));
        if (!same) { std::printf("TileGroup %s %s: headers=%zu covered=%d\n",edition.c_str(),row[1].c_str(),groups.size(),covered);break; }++replayed;
    }
    CHECK(replayed==353);
}
// 호스트 연결 검사용 raw 위치/깊이 입력을 실제 슬롯에 공급한다.
void Place(SidPool& pool,Sid sid,float x,float y,std::int8_t depth) {
    auto raw=InputRaw(pool,sid);Put(raw,14,Bits(x));Put(raw,18,Bits(y));Put(raw,pool.Edition()==OriginalEdition::Patch1078 ? 35 : 33,static_cast<std::uint8_t>(depth),1);
}
}
// 패치판의 전체 수집/CRT/해시 실제 관찰을 재생한다.
TEST_CASE(TileGroupCollection_PatchNative) { Replay("originals"); }
// CD판의 고정 행 상한/별도 슬롯 깊이 배치와 원본 수집 몸체를 재생한다.
TEST_CASE(TileGroupCollection_CdNative) { Replay("originalCD"); }
// 추가판 PE의 독립 관찰도 같은 복원 경로에서 대조한다.
TEST_CASE(TileGroupCollection_AdditionalNative) { Replay("original1037"); }

// 행 한계는 strict/per-row이며 정상 종료만 해시 선택과 배경 결과를 갱신한다.
TEST_CASE(TileGroupCollection_RowLimitsAndEditionRows) {
    SidPool patch(OriginalEdition::Patch1078,24000),cd(OriginalEdition::Cd1072,24000);SquidHash hash;
    TileGroupCollection collector(hash,patch);hash.Cell(0,2,2)=5;hash.Cell(0,3,2)=5;hash.Cell(0,2,3)=5;hash.Cell(0,3,3)=5;bool covered=true;
    CHECK(collector.Collect({2,2,3,3},3,covered).size()==2);CHECK(collector.Entries().size()==4);
    covered=true;CHECK(Throws([&] { collector.Collect({2,2,3,3},2,covered); }));CHECK(hash.Selected()==0 && covered);CHECK(collector.Entries().size()==2);
    hash.Reset();CHECK(collector.Collect({2,2,3,3},0,covered).empty());CHECK(hash.Selected()==2 && !covered);
    // 같은 49개 행은 패치판에서는 유효하고 CD판에서는 49번째 헤더 기록 뒤 오류다.
    for (int y=0;y<49;++y) hash.Cell(0,1,y)=5;
    CHECK(collector.Collect({0,0,2,48},2,covered).size()==49);
    TileGroupCollection cdCollector(hash,cd);CHECK(Throws([&] { cdCollector.Collect({0,0,2,48},2,covered); }));CHECK(cdCollector.Entries().size()==49);
    hash.Reset();TileGroupCollection shortScreen(hash,patch,{0});
    // 화면 높이 0의 패치 상한은 6이다. 5개는 성공하고 6개는 실패한다.
    for (int y=0;y<5;++y) hash.Cell(0,1,y)=5;
    CHECK(shortScreen.Collect({0,0,2,4},2,covered).size()==5);hash.Cell(0,1,5)=5;
    CHECK(Throws([&] { shortScreen.Collect({0,0,2,5},2,covered); }));
}

// 잘못된 풀 번호/토큰 범위를 거부하고 새 수집 때 이전 헤더를 지우는 계약을 확인한다.
TEST_CASE(TileGroupCollection_TokenContractsAndLifetime) {
    SidPool pool(OriginalEdition::Patch1078,24000);SquidHash hash;TileGroupCollection collector(hash,pool);bool covered=false;
    CHECK(Throws([&] { TileGroupCollection invalid(hash,pool,{-1}); }));
    CHECK(Throws([&] { TileGroupCollection invalid(hash,pool,{768,0,1}); }));
    CHECK(Throws([&] { TileGroupCollection invalid(hash,pool,{768,0xffff0000U}); }));
    CHECK(Throws([&] { collector.Object(0); }));CHECK(Throws([&] { collector.Object(51); }));CHECK(Throws([&] { collector.Group(0x80000001U); }));
    hash.Cell(0,2,2)=5;const auto first=collector.Collect({2,2,2,2},2,covered).front();CHECK(collector.Object(first.entries.front().token)==Sid{5});
    hash.Reset();CHECK(collector.Collect({2,2,2,2},2,covered).empty());CHECK(collector.Entries().empty());CHECK(Throws([&] { collector.Group(first.token); }));
    hash.Cell(0,2,2)=65535;CHECK(Throws([&] { collector.Collect({2,2,2,2},2,covered); }));
}

// 실제 raw SID/해시의 행 순서를 공통 CRT 정렬과 kind 1 내부 sprite 제출까지 연결한다.
TEST_CASE(TileGroupCollection_CommonSubmissionIntegration) {
    SidPool pool(OriginalEdition::Patch1078,24000);SquidHash hash;TileGroupCollection collector(hash,pool);
    Place(pool,{5},30,20,2);Place(pool,{6},10,15,100);Place(pool,{7},50,40,10);
    hash.Cell(0,2,2)=5;hash.Cell(0,3,2)=6;hash.Cell(0,2,3)=7;
    DrawCollectionHooks collection;DrawDispatchHooks dispatch;BindTileGroupCollection(collection,dispatch,collector);
    DrawCollectionRequest request;request.visible={2,2,3,3};request.groupRowLimit=3;
    std::array<RenderCollectionEntry,8> work;std::memset(work.data(),0xa5,sizeof(work));
    const auto result=CollectDrawEntries(request,work,collection);CHECK(result.count==2 && !result.backgroundCovered);CHECK(hash.Selected()==2);
    auto entries=std::span(work).first(2);SortDrawEntries(entries,OriginalEdition::Patch1078);
    std::vector<std::uint16_t> drawn;
    // 행 내부의 높은 깊이 100도 순서를 다시 정렬하지 않고 SID 5 뒤에 그린다.
    dispatch.sprite=[&](const auto& entry) { drawn.push_back(collector.Object(entry.token).value);CHECK(entry.kind==1 && entry.tail==0); };
    DispatchDrawEntries(entries,{},dispatch);CHECK(drawn==std::vector<std::uint16_t>({7,5,6}));CHECK(work[0].tail==0xa5 && work[1].tail==0xa5);
    drawn.clear();DispatchDrawEntries(entries,{true,false},dispatch);CHECK(drawn.empty());
    request.dirtyFlags=4;hash.Select(1);CHECK(CollectDrawEntries(request,work,collection).count==0);CHECK(hash.Selected()==1);
}
