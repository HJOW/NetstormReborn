// 실제 SID 풀과 공통 postPop/Pop을 연결한 독립 x86 기대값·쓰기 전 보호를 검증한다.
#include "TestSupport.h"
#include "o/RawGraph.h"
#include "o/SquidFactory.h"
#include "o/SquidPostPop.h"
#include "o/SquidPop.h"
#include "o/SquidDisplay.h"
#include "client/Renderer.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace netstorm;
namespace {
// TSV 열과 hex 슬롯 목록은 구분자를 보존하여 읽는다.
std::vector<std::string> Split(const std::string& text,char delimiter) {
    std::istringstream input(text); std::vector<std::string> result; std::string item;
    // 빈 입력도 한 필드로 읽어 잘못된 fixture를 감지한다.
    while (std::getline(input,item,delimiter)) result.push_back(item);
    return result;
}
// 부호 있는 입력 행을 읽으며 -는 빈 목록이다.
std::vector<std::vector<int>> Rows(const std::string& text) {
    std::vector<std::vector<int>> result; if (text=="-") return result;
    // 각 행은 원본 입력 순서를 유지한다.
    for (auto row:Split(text,';')) {
        std::replace(row.begin(),row.end(),',',' '); std::istringstream input(row); int value; std::vector<int> values;
        // 모든 정수 필드를 읽는다.
        while (input>>value) values.push_back(value);
        result.push_back(std::move(values));
    }
    return result;
}
// 실제 raw hex의 모든 byte를 읽는다.
std::vector<std::uint8_t> Bytes(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    // 두 자리씩 little endian 메모리 순서로 읽는다.
    for (std::size_t p=0;p<hex.size();p+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(p,2),nullptr,16)));
    return bytes;
}
// 호스트 엔디언과 관계없이 지정 폭을 기록한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // low byte부터 원본 필드 폭만 쓴다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 전체 표 checksum을 위해 원본 WORD/DWORD를 직렬화한다.
void Append(std::vector<std::uint8_t>& bytes,std::uint32_t value,std::size_t width) {
    // 타입 padding과 호스트 포인터는 포함하지 않는다.
    for (std::size_t i=0;i<width;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
// Python의 zlib와 같은 Adler-32이며 byte별 비교와 구별한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 원본 checksum 자체가 아니라 fixture 대조 도구의 모듈러다.
    constexpr std::uint32_t prime=65521; std::uint32_t a=1,b=0;
    // 전체 버퍼를 순서대로 누적한다.
    for (auto byte:bytes) { a=(a+byte)%prime; b=(b+a)%prime; }
    return (b<<16)|a;
}
// 기존 표의 원본 WORD 세 개를 signed로 해석한다.
std::array<o::GraphRecord,o::Graph::kTableSize> Records(const std::string& hex) {
    const auto bytes=Bytes(hex); std::array<o::GraphRecord,o::Graph::kTableSize> records{};
    // 각 레코드의 미사용 reserved WORD도 입력한다.
    for (std::size_t i=0;i<records.size();++i) {
        std::array<std::int16_t,3> words{};
        // 두 바이트의 부호 확장을 명시한다.
        for (std::size_t j=0;j<3;++j) {
            const auto p=i*6+j*2;
            words[j]=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(bytes[p]|(static_cast<std::uint16_t>(bytes[p+1])<<8)));
        }
        records[i]={words[0],words[1],words[2]};
    }
    return records;
}
// 원본 스택의 활성 범위 밖 오래된 DWORD를 모두 읽는다.
std::array<std::uint32_t,o::Graph::kFloodSize> Stack(const std::string& hex) {
    const auto bytes=Bytes(hex); std::array<std::uint32_t,o::Graph::kFloodSize> stack{};
    // little endian DWORD로 조합한다.
    for (std::size_t i=0;i<stack.size();++i)
        // 모든 바이트를 포함한다.
        for (std::size_t j=0;j<4;++j) stack[i]|=static_cast<std::uint32_t>(bytes[i*4+j])<<(8*j);
    return stack;
}
// 실제 Renderer의 dirty 순서·플래그·합침을 공유한다.
struct Sink final:o::SquidDisplaySink {
    client::DirtyRegions dirty{640,480};
    // 전체 표시 억제 상태도 원본 출력에 포함한다.
    bool Suppressed() const override { return dirty.FullRedraw(); }
    // 공통 가상 표시의 사각형을 실제 병합기에 전달한다.
    void Invalidate(o::SquidDisplayRect r,std::uint32_t flags) override { dirty.Add({r.left,r.top,r.right,r.bottom},flags); }
};
// 판본별 타입 배열을 원본 Create의 합성 입력과 맞춘다.
std::vector<o::RiftTypeRecord> Types(o::OriginalEdition edition) {
    std::vector<o::RiftTypeRecord> types(edition==o::OriginalEdition::Patch1078 ? 188 : 171);
    // 그래프 계산과 무관한 base 초기화 값도 명시한다.
    for (auto& type:types) { type.maxHitPoints=91; type.zOrder=7; type.footX=type.footY=1; type.cost=1.25f; }
    return types;
}
// 할당한 슬롯 전체를 fixture 입력으로 채운다. derived ctor/월드 초기화의 복원이 아니다.
void Node(o::SidPool& pool,o::Sid sid,std::uint8_t number,const std::vector<int>& n) {
    auto bytes=pool.AllocatedBytes(sid); std::fill(bytes.begin(),bytes.end(),std::uint8_t{});
    const bool patch=pool.Edition()==o::OriginalEdition::Patch1078;
    Put(bytes,0,patch ? 0x501dd0U : 0x5058d8U); Put(bytes,8,23,2); bytes[10]=number; bytes[11]=static_cast<std::uint8_t>(n[9]);
    Put(bytes,14,std::bit_cast<std::uint32_t>(static_cast<float>(n[0]))); Put(bytes,18,std::bit_cast<std::uint32_t>(static_cast<float>(n[1])));
    bytes[patch ? 30 : 28]=static_cast<std::uint8_t>(n[11]); bytes[patch ? 33 : 31]=static_cast<std::uint8_t>(n[12]);
    bytes[patch ? 34 : 32]=1; Put(bytes,patch ? 36 : 34,static_cast<std::uint32_t>(n[8]),patch ? 4U : 1U);
    bytes[patch ? 40 : 35]=static_cast<std::uint8_t>(n[10]);
}
// 성공/예외를 확인하는 새 호스트 보호 경로다.
template<class F> bool Throws(F operation) { try { operation(); } catch (const std::exception&) { return true; } return false; }
// 전체 표와 전체 스택의 stale 영역을 검사한다.
std::array<std::uint32_t,2> GraphHashes(const o::RawGraph& graph) {
    std::vector<std::uint8_t> records,stack;
    // reserved WORD도 포함한다.
    for (auto r:graph.Records()) { Append(records,static_cast<std::uint16_t>(r.surfaces),2); Append(records,static_cast<std::uint16_t>(r.inUse),2); Append(records,static_cast<std::uint16_t>(r.reserved),2); }
    // 모든 DWORD를 보존하여 비교한다.
    for (auto value:graph.FloodStack()) Append(stack,value,4);
    return {Adler(records),Adler(stack)};
}
}

// 공통 입력/전체 출력 대조를 공유하되 기대값은 각 실제 x86 도구가 독립적으로 기록한다.
static void ReplayRawGraph(const char* path,int expectedBegins,int expectedCalls,bool regionFixture) {
    std::ifstream input(path); CHECK(input.good()); std::string line; int lineNumber=0,begins=0,calls=0;
    std::unique_ptr<o::SidPool> pool; std::unique_ptr<o::SquidHash> hash; std::unique_ptr<o::RawGraph> graph;
    std::unique_ptr<o::SquidPostPop> post; std::unique_ptr<o::SquidPop> pop; std::unique_ptr<o::SquidDisplay> display; std::unique_ptr<Sink> sink;
    std::vector<o::Sid> ids; std::vector<o::RiftTypeRecord> types; std::vector<std::vector<o::FrameCode>> frames;
    std::vector<o::SquidDisplayShape> shapes; std::vector<std::uint8_t> spots; o::SquidPostPopState state;
    // Begin/Setup은 기계어 함수 검증 횟수에 포함하지 않는 명시적 준비 입력이다.
    while (std::getline(input,line)) {
        ++lineNumber; if (line.empty() || line[0]=='#') continue; const auto row=Split(line,'\t'); const auto before=test::FailureCount();
        if (row[0]=="Begin") {
            pop.reset(); post.reset(); graph.reset(); display.reset(); sink.reset();
            const auto edition=row[1]=="originals" ? o::OriginalEdition::Patch1078 : o::OriginalEdition::Cd1072;
            types=Types(edition); frames.assign(types.size(),{}); shapes.assign(types.size(),{}); spots.assign(o::kWorldCells*o::kWorldCells,0);
            pool=std::make_unique<o::SidPool>(edition,32768,false); hash=std::make_unique<o::SquidHash>();
            o::SquidFactory factory(*pool,types); ids.clear(); const auto expected=Rows(row[3]).at(0);
            // 각 실행 파일 배치의 실제 client SID 할당 순서도 확인한다.
            for (int i=0;i<7;++i) { ids.push_back(factory.Create(74,2)); CHECK(ids.back().value==expected[static_cast<std::size_t>(i)]); }
            ++begins;
        } else if (row[0]=="Setup") {
            pop.reset(); post.reset(); graph.reset(); display.reset(); sink=std::make_unique<Sink>(); hash->Reset(); std::fill(spots.begin(),spots.end(),std::uint8_t{});
            const auto nodes=Rows(row[1]); CHECK(nodes.size()==ids.size());
            // 지도는 발자국 전체가 아니라 실제 기준점 버킷/체인을 입력한다.
            for (std::size_t i=0;i<nodes.size();++i) {
                const auto& n=nodes[i]; auto& type=types[74+i]; type.flags1=static_cast<std::uint32_t>(n[4]); type.flags2=static_cast<std::uint32_t>(n[5]);
                type.footX=n[2]; type.footY=n[3]; frames[74+i]={{static_cast<std::uint8_t>(n[6]),static_cast<std::uint8_t>(n[7]),1,0},{65,80,2,0}};
                shapes[74+i]={2,true,{{11,17,12,16},{19,23,-3,5},{11,17,12,16},{19,23,-3,5}}};
                Node(*pool,ids[i],static_cast<std::uint8_t>(74+i),n);
                if (!(n[9]&4) && (regionFixture || n[12]==0)) {
                    const auto level=regionFixture ? n[12] : 0;
                    auto& head=hash->Bucket(level,static_cast<float>(n[0]),static_cast<float>(n[1]));
                    if (regionFixture) Put(pool->AllocatedBytes(ids[i]),4,head,2);
                    head=ids[i].value;
                }
            }
            // 내부 spot 후보를 별도로 준비한다.
            for (const auto& s:Rows(row[2])) spots[static_cast<std::size_t>(s[1]*o::kWorldCells+s[0])]=static_cast<std::uint8_t>(s[2]);
            state={}; state.totalCost=53; state.depth=5; state.localOwner=1; state.graphsEnabled=true;
            // 세 Totalmade 전체에 동일한 stale DWORD 입력을 넣는다.
            for (std::size_t i=0;i<256;++i) {
                const auto value=31337U+static_cast<std::uint32_t>(i)*2654435761U;
                state.localCounts[i]=value; state.localSecondaryCounts[i]=value+17; state.globalCounts[i]=value+34;
            }
            auto stack=regionFixture ? std::array<std::uint32_t,o::Graph::kFloodSize>{} : Stack(row[4]);
            // 새 fixture는 긴 stale 스택을 반복 저장하지 않고 입력 시드로 복원한다.
            if (regionFixture) for (std::size_t i=0;i<stack.size();++i) stack[i]=static_cast<std::uint32_t>(std::stoul(row[4])+i);
            graph=std::make_unique<o::RawGraph>(*pool,*hash,spots,types,frames,Records(row[3]),stack);
            post=std::make_unique<o::SquidPostPop>(*pool,types,state,graph.get());
            display=std::make_unique<o::SquidDisplay>(pool->Edition(),types,shapes,*sink,o::SquidDisplayView{0,0,65536,{0,0,640,480}});
            pop=std::make_unique<o::SquidPop>(*pool,*hash,spots,display.get(),post.get());
        } else {
            const auto args=Rows(row[1]);
            if (row[0]=="Pop") {
                // void 입력의 결과는 실제 x86 출력의 void 해제 여부로 읽는다. 패치 충돌은 부분 쓰기를 남긴다.
                const auto expectedRoot=Bytes(Split(row[3],';').front());
                const auto expected=(expectedRoot[11]&4) ? o::RawPopResult::Overlap : o::RawPopResult::Registered;
                const auto& a=args.at(0); CHECK(pop->Pop(ids[0],types[74],1,1,static_cast<float>(a[1]),static_cast<float>(a[2]),static_cast<std::uint32_t>(a[0]))==expected);
            } else if (row[0]=="PostPop") post->PostPop(ids[0],static_cast<std::uint32_t>(args.at(0)[0]));
            else if (row[0]=="Region") graph->InvalidateRegion(ids[0]);
            else if (row[0]=="Add") graph->Add(ids[0]);
            else if (row[0]=="Flood") CHECK(graph->Flood(ids[0],static_cast<std::uint8_t>(args.at(0)[0]))==std::stoul(row[2])); else CHECK(false);
            const auto raw=Split(row[3],';');
            // 7개 슬롯의 vtable·graph·좌표·state·payload 모든 바이트를 직접 비교한다.
            for (std::size_t i=0;i<ids.size();++i) {
                const auto expected=Bytes(raw[i]); const auto actual=pool->Slot(ids[i]); CHECK(std::equal(actual.begin(),actual.end(),expected.begin(),expected.end()));
            }
            const auto checksums=GraphHashes(*graph); CHECK(checksums[0]==std::stoul(row[4])); CHECK(checksums[1]==std::stoul(row[5]));
            std::vector<std::uint8_t> heads;
            // 4단계 해시 전체를 원본 배열 순서로 대조한다.
            for (int level=0;level<4;++level)
                // 모든 머리를 WORD로 직렬화한다.
                for (auto id:hash->Entries(level)) Append(heads,id,2);
            CHECK(Adler(heads)==std::stoul(row[6])); CHECK(Adler(spots)==std::stoul(row[7])); CHECK(Adler(pool->Bytes())==std::stoul(row[8]));
            CHECK(state.totalCost==std::stoll(row[9])); CHECK(state.depth==std::stoul(row[10]));
            const std::array<std::span<const std::uint32_t>,3> tables{state.localCounts,state.localSecondaryCounts,state.globalCounts};
            // 연산 대상 이외의 타입 통계도 비교한다.
            for (std::size_t i=0;i<3;++i) {
                std::vector<std::uint8_t> bytes;
                // 대상 타입 이외의 stale DWORD도 전체 checksum에 포함한다.
                for (auto v:tables[i]) Append(bytes,v,4);
                CHECK(Adler(bytes)==std::stoul(row[11+i]));
            }
            CHECK(sink->Suppressed()==(std::stoi(row[14])!=0)); const auto dirty=Rows(row[15]); CHECK(dirty.size()==sink->dirty.Entries().size());
            // dirty 항목의 좌표·플래그·순서를 모두 비교한다.
            for (std::size_t i=0;i<std::min(dirty.size(),sink->dirty.Entries().size());++i) {
                const auto& d=sink->dirty.Entries()[i]; const auto& e=dirty[i];
                CHECK(d.rect.left==e[0]); CHECK(d.rect.top==e[1]); CHECK(d.rect.right==e[2]); CHECK(d.rect.bottom==e[3]); CHECK(d.flags==static_cast<std::uint32_t>(e[4]));
            }
            ++calls;
        }
        if (test::FailureCount()!=before) { std::cerr<<"raw 그래프 fixture 행: "<<lineNumber<<'\n'; break; }
    }
    CHECK(begins==expectedBegins); CHECK(calls==expectedCalls);
}

// 기존 fixture를 그대로 재생하여 일반 표면/매몰 다리 경로의 회귀를 확인한다.
TEST_CASE(RawGraph_X86_AllocatedSidPostPopAndProjectedPop) {
    ReplayRawGraph(NETSTORM_RAWGRAPH_FIXTURE,4,1536,false);
}

// 세 실제 PE와 두 x87 정밀도의 일반 탐색/다리/섬 Pop 결과를 전체 슬롯/메모리와 대조한다.
TEST_CASE(RawGraph_RegionX86_ThreeBinariesAndNormalBridgeIslandPop) {
    ReplayRawGraph(NETSTORM_REGIONGRAPH_FIXTURE,6,1536,true);
}

// void 해제 전 옛 좌표로 시험하면 놓칠 그래프 소진/소수 좌표를 Pop 쓰기 전에 잡는다.
TEST_CASE(RawGraph_ProjectedPopRejectsExhaustionAndFractionalBeforeMutation) {
    const auto edition=o::OriginalEdition::Patch1078; auto types=Types(edition); types[74].flags1=o::TypeFlag1::kSurface;
    std::vector<std::vector<o::FrameCode>> frames(types.size()); frames[74]={{65,80,1,0}};
    o::SidPool pool(edition,32768,false); o::SquidFactory factory(pool,types); const auto sid=factory.Create(74,2);
    Node(pool,sid,74,{90,90,1,1,0x800,0,65,80,0,4,0,254,1});
    o::SquidHash hash; std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells); std::array<o::GraphRecord,o::Graph::kTableSize> records{};
    // 정상 251개가 모두 사용 중인 입력은 원본 전역 재구성 경로를 요구한다.
    for (std::size_t i=0;i<o::Graph::kCount;++i) records[i]={1,1,71};
    o::RawGraph graph(pool,hash,spots,types,frames,records); o::SquidPostPopState state; state.graphsEnabled=true;
    o::SquidPostPop post(pool,types,state,&graph); o::SquidPop pop(pool,hash,spots,nullptr,&post);
    const std::vector<std::uint8_t> raw(pool.Bytes().begin(),pool.Bytes().end()); const auto before=GraphHashes(graph);
    CHECK(Throws([&]{pop.Pop(sid,types[74],1,1,20,20);})); CHECK(Throws([&]{pop.Pop(sid,types[74],1,1,20.25f,20);}));
    CHECK(std::equal(raw.begin(),raw.end(),pool.Bytes().begin())); CHECK(GraphHashes(graph)==before); CHECK(state.depth==0); CHECK(state.totalCost==0);
    CHECK(hash.Bucket(1,20,20)==0); CHECK(std::all_of(spots.begin(),spots.end(),[](auto byte){return byte==0;}));
}

// 영역 후보를 감소시켜도 표가 소진되면 Pop의 좌표/spot/후처리까지 전부 쓰기 전에 거부한다.
TEST_CASE(RawGraph_RegionThenAddExhaustionRejectsBeforeAnyPopMutation) {
    const auto edition=o::OriginalEdition::Patch1078; auto types=Types(edition);
    types[74].flags1=types[75].flags1=o::TypeFlag1::kSurface; types[74].flags2=8;
    types[74].footX=types[74].footY=3; types[75].footX=3;
    std::vector<std::vector<o::FrameCode>> frames(types.size()); frames[74]=frames[75]={{65,80,1,0}};
    o::SidPool pool(edition,32768,false); o::SquidFactory factory(pool,types);
    const auto root=factory.Create(74,2),candidate=factory.Create(75,2);
    Node(pool,root,74,{90,90,3,3,0x800,8,65,80,0,4,0,254,1});
    Node(pool,candidate,75,{21,20,3,1,0x800,0,65,80,0,0,0,0,1});
    o::SquidHash hash; hash.Bucket(1,21,20)=candidate.value;
    std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells); spots[20*o::kWorldCells+21]=8;
    std::array<o::GraphRecord,o::Graph::kTableSize> records{};
    // 한 후보를 빼도 어느 그래프도 비지 않는 입력이다.
    for (std::size_t i=0;i<o::Graph::kCount;++i) records[i]={2,1,71};
    o::RawGraph graph(pool,hash,spots,types,frames,records); o::SquidPostPopState state; state.graphsEnabled=true;
    o::SquidPostPop post(pool,types,state,&graph); o::SquidPop pop(pool,hash,spots,nullptr,&post);
    const std::vector<std::uint8_t> raw(pool.Bytes().begin(),pool.Bytes().end()),beforeSpots=spots;
    const auto before=GraphHashes(graph); const auto oldHead=hash.Bucket(1,20,20);
    CHECK(Throws([&]{pop.Pop(root,types[74],1,1,20,20,0x201);}));
    CHECK(std::equal(raw.begin(),raw.end(),pool.Bytes().begin())); CHECK(spots==beforeSpots); CHECK(GraphHashes(graph)==before);
    CHECK(hash.Bucket(1,20,20)==oldHead); CHECK(state.depth==0); CHECK(state.totalCost==0);
}

// 프레임/지도 변경은 각 호출에서 다시 읽고 graph byte 외의 raw 필드는 유지한다.
TEST_CASE(RawGraph_RefreshesFramesMapAndRawMembershipBetweenCalls) {
    const auto edition=o::OriginalEdition::Cd1072; auto types=Types(edition); types[74].flags1=types[75].flags1=o::TypeFlag1::kSurface;
    types[74].flags2=types[75].flags2=o::TypeFlag2::kBridge;
    std::vector<std::vector<o::FrameCode>> frames(types.size()); frames[74]={{76,80,1,0},{65,80,2,0}}; frames[75]={{65,80,1,0}};
    o::SidPool pool(edition,32768,false); o::SquidFactory factory(pool,types); const auto a=factory.Create(74,2),b=factory.Create(75,2);
    Node(pool,a,74,{20,20,1,1,0x800,4,76,80,0,0,0,254,0}); Node(pool,b,75,{21,20,1,1,0x800,4,65,80,0,0,0,1,0});
    o::SquidHash hash; hash.Cell(0,20,20)=a.value; hash.Cell(0,21,20)=b.value; std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells);
    std::array<o::GraphRecord,o::Graph::kTableSize> records{}; records[1]={2,1,91}; o::RawGraph graph(pool,hash,spots,types,frames,records);
    graph.Add(a); CHECK(pool.Slot(a)[28]==0); CHECK(graph.Records()[0].surfaces==1);
    pool.AllocatedBytes(a)[34]=1; graph.Add(a); CHECK(pool.Slot(a)[28]==1); CHECK(graph.Records()[1].surfaces==3);
    pool.AllocatedBytes(a)[28]=254; hash.Cell(0,21,20)=0; graph.Add(a); CHECK(pool.Slot(a)[28]==2); CHECK(graph.Records()[2].surfaces==1);
    CHECK(pool.Slot(a)[34]==1); CHECK(pool.Slot(a)[11]==0); CHECK(pool.Slot(b)[28]==1); CHECK(graph.Records()[1].reserved==91);
}

// 서로 다른 풀/지도, 손상된 후보와 영역 체인 순환은 결과를 쓰기 전에 거부한다.
TEST_CASE(RawGraph_CorruptCandidatesAndMismatchedConnectionsRejectBeforeMutation) {
    const auto edition=o::OriginalEdition::Patch1078; auto types=Types(edition); types[74].flags1=o::TypeFlag1::kSurface;
    std::vector<std::vector<o::FrameCode>> frames(types.size()); frames[74]={{65,80,1,0}};
    o::SidPool pool(edition,32768,false),other(edition,32768,false); o::SquidFactory factory(pool,types); const auto sid=factory.Create(74,2);
    Node(pool,sid,74,{20,20,1,1,0x800,0,65,80,0,0,0,254,1}); o::SquidHash hash,otherHash;
    std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells),otherSpots(spots.size()); o::RawGraph graph(pool,hash,spots,types,frames);
    o::SquidPostPopState state; state.graphsEnabled=true; o::SquidPostPop post(pool,types,state,&graph);
    CHECK(Throws([&]{o::SquidPostPop bad(other,types,state,&graph);}));
    CHECK(Throws([&]{o::SquidPop bad(pool,otherHash,spots,nullptr,&post);})); CHECK(Throws([&]{o::SquidPop bad(pool,hash,otherSpots,nullptr,&post);}));
    const auto before=GraphHashes(graph); const std::vector<std::uint8_t> raw(pool.Bytes().begin(),pool.Bytes().end());
    hash.Cell(0,21,20)=30000; CHECK(Throws([&]{post.Activate(sid,1);})); CHECK(state.depth==0); CHECK(GraphHashes(graph)==before);
    CHECK(std::equal(raw.begin(),raw.end(),pool.Bytes().begin()));
    hash.Cell(0,21,20)=0; pool.AllocatedBytes(sid)[36]=99; CHECK(Throws([&]{graph.Add(sid);})); CHECK(GraphHashes(graph)==before);
    pool.AllocatedBytes(sid)[36]=0; types[74].flags2=8;
    CHECK(Throws([&]{o::SquidPostPop mismatch(pool,types,state,&graph);}));
    o::RawGraph regionGraph(pool,hash,spots,types,frames); o::SquidPostPop region(pool,types,state,&regionGraph);
    hash.Bucket(1,20,20)=sid.value; Put(pool.AllocatedBytes(sid),4,sid.value,2);
    CHECK(Throws([&]{region.Activate(sid,1);})); CHECK(state.depth==0); CHECK(pool.Slot(sid)[30]==254);
    Put(pool.AllocatedBytes(sid),4,0,2); region.Activate(sid,1);
    CHECK(state.depth==0); CHECK(pool.Slot(sid)[30]==0); CHECK(regionGraph.Records()[0].surfaces==1);
}
