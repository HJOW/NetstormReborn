// 독립 실제 x86의 비용/목록/통계·raw 상태·표시 결과를 재생한다.
#include "TestSupport.h"
#include "o/SquidPostPop.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"
#include "o/SquidFactory.h"
#include "o/SquidDisplay.h"
#include "client/Renderer.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace netstorm;
namespace {
// TSV의 열과 정수 비트 패턴을 손실 없이 읽는다.
std::vector<std::string> Fields(const std::string& line) {
    std::istringstream input(line); std::vector<std::string> result; std::string item;
    // 모든 필드를 원문 순서대로 유지한다.
    while (std::getline(input,item,'\t')) result.push_back(item);
    return result;
}
// 부호 있는 rect와 DWORD count 목록을 64비트로 읽는다.
std::vector<std::int64_t> Numbers(std::string text) {
    std::replace(text.begin(),text.end(),',',' '); std::replace(text.begin(),text.end(),';',' ');
    std::istringstream input(text); std::vector<std::int64_t> result; std::int64_t number;
    // -는 빈 배열이므로 실패 시 빈 목록을 돌려준다.
    while (input>>number) result.push_back(number);
    return result;
}
// raw little endian 쓰기는 원본 폭 밖을 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // CD frame/graph는 한 바이트다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 원본 슬롯의 모든 바이트를 비교할 hex다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    // 바이트 앞의 0도 보존한다.
    constexpr char digits[]="0123456789abcdef"; std::string result;
    // 모든 raw 필드를 포함한다.
    for (auto byte:bytes) { result.push_back(digits[byte>>4]); result.push_back(digits[byte&15]); }
    return result;
}
// 호스트 엔디언과 관계없이 전체 DWORD 표를 직렬화한다.
void Words(std::vector<std::uint8_t>& bytes,std::span<const std::uint32_t> words) {
    // 활성 영역 밖 stale DWORD도 checksum에 포함한다.
    for (auto value:words) {
        // 각 값의 low byte부터 쓰는 원본 메모리 순서다.
        for (std::size_t i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
}
// Python zlib와 동일한 Adler-32다. 체크섬은 전체 바이트 동일성 증명과 구별한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 표와 작은 목록 버퍼는 이 누적 폭을 넘지 않는다.
    constexpr std::uint32_t prime=65521; std::uint32_t a=1,b=0;
    // 순서대로 누적하고 매 바이트에서 모듈러를 적용한다.
    for (auto byte:bytes) { a=(a+byte)%prime; b=(b+a)%prime; }
    return (b<<16)|a;
}
// 두 판본의 generic base 입력이다. 원본 로딩 전체가 아니라 명시적인 fixture 준비다.
std::vector<o::RiftTypeRecord> Types(o::OriginalEdition edition) {
    std::vector<o::RiftTypeRecord> types(edition==o::OriginalEdition::Patch1078 ? 188 : 171);
    // base 초기화의 HP/깊이/발자국 입력을 지정한다.
    for (auto& t:types) { t.maxHitPoints=91; t.zOrder=7; t.footX=t.footY=1; }
    return types;
}
// 실제 Renderer와 같은 dirty 병합/100항목 full 상태를 공유한다.
struct Sink final:o::SquidDisplaySink {
    client::DirtyRegions dirty{640,480};
    // 전체 갱신이면 후속 표시는 생략한다.
    bool Suppressed() const override { return dirty.FullRedraw(); }
    // main/shadow 순서를 유지한다.
    void Invalidate(o::SquidDisplayRect r,std::uint32_t flags) override { dirty.Add({r.left,r.top,r.right,r.bottom},flags); }
};
// 새 보호 경로의 예외를 관찰한다.
template<class F> bool Throws(F operation) { try { operation(); } catch (const std::exception&) { return true; } return false; }
// 결과 열의 비용/알림/depth·전체 표/목록·raw 슬롯·dirty 모든 항목을 대조한다.
void CheckResult(const std::vector<std::string>& row,std::size_t start,const o::SquidPostPopState& state,
    const o::SidPool& pool,o::Sid sid,const Sink& sink) {
    CHECK(state.totalCost==std::stoll(row[start])); CHECK(state.productionDirty==std::stoul(row[start+1])); CHECK(state.depth==std::stoul(row[start+2]));
    const std::array<std::span<const std::uint32_t>,3> tables{state.localCounts,state.localSecondaryCounts,state.globalCounts};
    // 실제 세 Totalmade 배열 전체를 checksum으로 대조한다.
    for (std::size_t i=0;i<tables.size();++i) { std::vector<std::uint8_t> data; Words(data,tables[i]); CHECK(Adler(data)==std::stoul(row[start+3+i])); }
    const auto counts=Numbers(row[start+6]); CHECK(counts.size()==11);
    CHECK(state.providers.count==static_cast<std::uint32_t>(counts[0])); CHECK(state.factories.count==static_cast<std::uint32_t>(counts[1]));
    std::vector<std::uint8_t> data; Words(data,state.providers.entries); Words(data,state.factories.entries);
    // 중립 owner 0의 보존 메모리와 모든 소유자 목록을 포함한다.
    for (std::size_t i=0;i<state.ownerFactories.size();++i) { CHECK(state.ownerFactories[i].count==static_cast<std::uint32_t>(counts[i+2])); Words(data,state.ownerFactories[i].entries); }
    CHECK(Adler(data)==std::stoul(row[start+7])); CHECK(Hex(pool.Slot(sid))==row[start+8]);
    CHECK(sink.Suppressed()==(std::stoi(row[start+9])!=0)); const auto expected=Numbers(row[start+10]);
    CHECK(expected.size()==sink.dirty.Entries().size()*5);
    // 무시된 항목도 플래그/순서/좌표까지 확인한다.
    for (std::size_t i=0;i<std::min(expected.size()/5,sink.dirty.Entries().size());++i) {
        const auto n=i*5; const auto& e=sink.dirty.Entries()[i];
        CHECK(e.rect.left==expected[n]); CHECK(e.rect.top==expected[n+1]); CHECK(e.rect.right==expected[n+2]); CHECK(e.rect.bottom==expected[n+3]);
        CHECK(e.flags==static_cast<std::uint32_t>(expected[n+4]));
    }
}
}

// 실제 공통 후처리와 Pop/Unpop·재등록을 같은 상태/표시 객체에 연결한다.
TEST_CASE(SquidPostPop_X86_CostListsTotalsAndActiveLifecycle) {
    std::ifstream input(NETSTORM_POSTPOP_FIXTURE); CHECK(input.good());
    std::unique_ptr<o::SidPool> pool; std::unique_ptr<o::SquidHash> hash; std::unique_ptr<o::SquidFactory> factory;
    std::unique_ptr<o::SquidPostPop> post; std::unique_ptr<o::SquidDisplay> display;
    std::unique_ptr<o::SquidPop> pop; std::unique_ptr<o::SquidUnpop> unpop; std::unique_ptr<Sink> sink;
    o::SquidPostPopState state; std::vector<o::RiftTypeRecord> types; std::vector<o::SquidDisplayShape> shapes; std::vector<std::uint8_t> spots;
    o::Sid sid{}; std::uint32_t vtable=0; int number=74,begins=0,posts=0,pops=0,unpops=0,lineNumber=0; std::string line;
    // Begin/Type/State/Fill은 명시적인 준비 입력이며 원본 월드 로딩을 실행한 수치가 아니다.
    while (std::getline(input,line)) {
        ++lineNumber; if (line.empty() || line[0]=='#') continue; const auto row=Fields(line); const auto before=test::FailureCount();
        if (row[0]=="Begin") {
            pop.reset(); unpop.reset(); post.reset(); display.reset(); factory.reset(); sink.reset();
            const auto edition=row[1]=="originals" ? o::OriginalEdition::Patch1078 : o::OriginalEdition::Cd1072;
            types=Types(edition); shapes.assign(types.size(),{}); spots.assign(o::kWorldCells*o::kWorldCells,0);
            pool=std::make_unique<o::SidPool>(edition,32768,false); hash=std::make_unique<o::SquidHash>();
            factory=std::make_unique<o::SquidFactory>(*pool,types); sid=factory->Create(74,2); CHECK(sid.value==std::stoul(row[3]));
            const auto bytes=pool->Slot(sid); vtable=static_cast<std::uint32_t>(bytes[0])|(static_cast<std::uint32_t>(bytes[1])<<8)|
                (static_cast<std::uint32_t>(bytes[2])<<16)|(static_cast<std::uint32_t>(bytes[3])<<24); ++begins;
        } else if (row[0]=="Type") {
            number=std::stoi(row[1]); auto& type=types[static_cast<std::size_t>(number)];
            type.flags1=static_cast<std::uint32_t>(std::stoul(row[2])); type.flags2=static_cast<std::uint32_t>(std::stoul(row[3]));
            type.cost=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[4])));
            shapes[static_cast<std::size_t>(number)]={1,true,{{11,17,12,16},{19,23,-3,5}}};
        } else if (row[0]=="State") {
            pop.reset(); unpop.reset(); post.reset(); display.reset(); state={}; sink=std::make_unique<Sink>();
            const auto pc=std::stoul(row[1]),fc=std::stoul(row[2]),oc=std::stoul(row[3]),initial=std::stoul(row[4]);
            state.providers.entries.resize(pc); state.factories.entries.resize(fc); state.providers.count=static_cast<std::uint32_t>(std::min(initial,pc));
            state.factories.count=static_cast<std::uint32_t>(std::min(initial,fc));
            // 일반 목록의 active/inactive 영역도 원본 준비 바이트와 일치시킨다.
            for (auto* list:{&state.providers,&state.factories}) {
                // 첫 항목의 현재 SID는 provider duplicate와 factory duplicate 차이를 검사한다.
                for (std::size_t i=0;i<list->entries.size();++i) list->entries[i]=i==0 ? sid.value : 0x60000000U+static_cast<std::uint32_t>(i);
            }
            // 소유자 0도 원본 메모리 보존 검사에 포함한다.
            for (std::size_t owner=0;owner<state.ownerFactories.size();++owner) {
                auto& list=state.ownerFactories[owner]; list.entries.resize(oc); list.count=static_cast<std::uint32_t>(std::min(initial,oc));
                // 원본 fixture가 지정한 stale DWORD들이다.
                for (std::size_t i=0;i<oc;++i) list.entries[i]=0x70000000U+static_cast<std::uint32_t>(owner*100+i);
            }
            state.totalCost=static_cast<std::int32_t>(std::stoll(row[5])); state.productionDirty=static_cast<std::uint32_t>(std::stoul(row[6]));
            const auto seed=static_cast<std::uint32_t>(std::stoul(row[7]));
            // 세 통계 배열 전체를 원본과 같은 서로 다른 값으로 채운다.
            for (std::size_t i=0;i<256;++i) {
                const auto value=seed+static_cast<std::uint32_t>(i)*2654435761U;
                state.localCounts[i]=value; state.localSecondaryCounts[i]=value+17; state.globalCounts[i]=value+34;
            }
            state.localOwner=static_cast<std::uint8_t>(std::stoul(row[8])); state.graphsEnabled=std::stoi(row[9])!=0;
            state.suppressed=std::stoi(row[10])!=0; state.depth=static_cast<std::uint32_t>(std::stoul(row[11]));
            post=std::make_unique<o::SquidPostPop>(*pool,types,state);
            display=std::make_unique<o::SquidDisplay>(pool->Edition(),types,shapes,*sink,o::SquidDisplayView{0,0,65536,{0,0,640,480}});
            pop=std::make_unique<o::SquidPop>(*pool,*hash,spots,display.get(),post.get()); unpop=std::make_unique<o::SquidUnpop>(*pool,*hash,spots,display.get());
        } else if (row[0]=="Fill") {
            hash->Reset(); auto bytes=pool->AllocatedBytes(sid); std::fill(bytes.begin(),bytes.end(),std::uint8_t{});
            const bool patch=pool->Edition()==o::OriginalEdition::Patch1078;
            Put(bytes,0,vtable); bytes[10]=static_cast<std::uint8_t>(number); bytes[11]=4;
            Put(bytes,14,std::bit_cast<std::uint32_t>(20.0f)); Put(bytes,18,std::bit_cast<std::uint32_t>(20.0f));
            bytes[patch ? 34 : 32]=static_cast<std::uint8_t>(std::stoul(row[1])); bytes[patch ? 40 : 35]=static_cast<std::uint8_t>(std::stoul(row[2]));
            bytes[patch ? 30 : 28]=17;
        } else if (row[0]=="PostPop") {
            post->PostPop(sid,static_cast<std::uint32_t>(std::stoul(row[1]))); CheckResult(row,2,state,*pool,sid,*sink); ++posts;
        } else if (row[0]=="Pop") {
            const auto x=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[2]))),y=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[3])));
            CHECK(pop->Pop(sid,types[static_cast<std::size_t>(number)],1,1,x,y,static_cast<std::uint32_t>(std::stoul(row[1])))==o::RawPopResult::Registered);
            CheckResult(row,4,state,*pool,sid,*sink); ++pops;
        } else if (row[0]=="Unpop") {
            unpop->Unpop(sid,types[static_cast<std::size_t>(number)],static_cast<std::uint32_t>(std::stoul(row[1]))); CheckResult(row,2,state,*pool,sid,*sink); ++unpops;
        } else CHECK(false);
        if (test::FailureCount()!=before) { std::cerr<<"postPop fixture 행: "<<lineNumber<<'\n'; break; }
    }
    CHECK(begins==4); CHECK(posts==768); CHECK(pops==144); CHECK(unpops==72);
}

// 최초 Pop의 비용/통계는 재등록에서 자동 중복 증가하지 않으며 직접 flags 1은 다시 집계한다.
TEST_CASE(SquidPostPop_FirstRegistrationAndProviderDuplicate_PreserveOriginalCounters) {
    auto types=Types(o::OriginalEdition::Patch1078); types[74].flags1=0x10000000; types[74].flags2=0x10000; types[74].cost=3.75f;
    o::SidPool pool(o::OriginalEdition::Patch1078,32768,false); o::SquidFactory factory(pool,types); const auto sid=factory.Create(74,2);
    pool.AllocatedBytes(sid)[34]=1; o::SquidPostPopState state; state.localOwner=1; state.providers.entries.resize(1);
    o::SquidPostPop post(pool,types,state); o::SquidHash hash; std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells);
    o::SquidPop pop(pool,hash,spots,nullptr,&post); o::SquidUnpop unpop(pool,hash,spots);
    CHECK(pop.Pop(sid,types[74],1,1,20,20)==o::RawPopResult::Registered); CHECK(state.totalCost==3); CHECK(state.globalCounts[74]==1); CHECK(state.depth==0);
    unpop.Unpop(sid,types[74]); CHECK(pop.Pop(sid,types[74],1,1,30,30)==o::RawPopResult::Registered);
    CHECK(state.totalCost==3); CHECK(state.providers.count==1); CHECK(state.productionDirty==1);
    post.Activate(sid,1); CHECK(state.totalCost==6); CHECK(state.providers.count==1); CHECK(state.productionDirty==2);
    CHECK(state.localCounts[74]==2); CHECK(state.localSecondaryCounts[74]==2); CHECK(state.globalCounts[74]==2); CHECK(state.depth==0);
}

// 미복원 효과/비용/손상 목록은 Pop이 좌표나 체인을 쓰기 전에 거부한다.
TEST_CASE(SquidPostPop_UnsupportedEffectsAndCorruptList_RejectBeforeSpaceWrites) {
    auto types=Types(o::OriginalEdition::Patch1078); types[74].flags2=0x10000; types[74].flags1=0x10000000;
    o::SidPool pool(o::OriginalEdition::Patch1078,32768,false); o::SquidFactory factory(pool,types); const auto sid=factory.Create(74,2);
    pool.AllocatedBytes(sid)[34]=1; o::SquidPostPopState state; state.providers.entries.resize(1); state.providers.count=2;
    o::SquidPostPop post(pool,types,state); o::SquidHash hash; std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells);
    o::SquidPop pop(pool,hash,spots,nullptr,&post); const auto before=Hex(pool.Slot(sid));
    CHECK(Throws([&]{ pop.Pop(sid,types[74],1,1,20,20); })); CHECK(Hex(pool.Slot(sid))==before); CHECK(state.depth==0); CHECK(hash.Bucket(1,20,20)==0);
    state.providers.count=0; state.aiAttached[1]=true;
    CHECK(Throws([&]{ pop.Pop(sid,types[74],1,1,20,20); })); CHECK(Hex(pool.Slot(sid))==before); CHECK(state.totalCost==0);
    state.aiAttached[1]=false; state.pendingPlacement=true;
    CHECK(Throws([&]{ pop.Pop(sid,types[74],1,1,20,20); })); CHECK(Hex(pool.Slot(sid))==before);
    state.pendingPlacement=false; state.suppressed=true;
    CHECK(pop.Pop(sid,types[74],1,1,20,20)==o::RawPopResult::Registered); CHECK(state.globalCounts[74]==0); CHECK(state.depth==0);
    // 표면 그래프 생성은 거부하고 noGraph 리셋 경로만 허용한다.
    auto graphTypes=types; graphTypes[74].flags1=o::TypeFlag1::kSurface;
    o::SquidPostPopState graphState; graphState.graphsEnabled=true;
    o::SquidPostPop graphPost(pool,graphTypes,graphState);
    CHECK(Throws([&]{ graphPost.Validate(sid,1); })); graphPost.Validate(sid,0x2000001);
    // 비유한 비용은 통계/깊이/슬롯에 쓰기 전에 거부한다.
    graphTypes[74].cost=std::numeric_limits<float>::quiet_NaN();
    o::SquidPostPop invalidCost(pool,graphTypes,graphState); const auto registered=Hex(pool.Slot(sid));
    CHECK(Throws([&]{ invalidCost.Activate(sid,0x2000001); })); CHECK(graphState.depth==0); CHECK(Hex(pool.Slot(sid))==registered);
    o::SidPool other(o::OriginalEdition::Patch1078,32768,false); o::SquidPostPop wrong(other,types,state);
    CHECK(Throws([&]{ o::SquidPop invalid(pool,hash,spots,nullptr,&wrong); }));
}

// 실제 .type 속성을 float cost로 읽으며 누락/중복/문자열 비용의 원본 규칙을 유지한다.
TEST_CASE(SquidPostPop_TypeCost_LoadsFloatAndRejectsString) {
    const auto edition=o::OriginalEdition::Patch1078; const auto order=o::TypeLoadOrder(edition);
    std::vector<o::RiftTypeDefinition> definitions(order.size());
    definitions[0]=o::RiftTypeDefinition::Parse("typename test { cost=-0.5; cost=3.75; } A00:default:\"a.gif\"#0;");
    std::vector<o::RiftTypeSource> sources;
    // 원본 로드 순서를 유지하며 나머지 비용은 0 기본값으로 둔다.
    for (std::size_t i=0;i<order.size();++i) sources.push_back({order[i],&definitions[i]});
    const o::RiftTypeTable table(edition,sources); CHECK(table.Types()[70].cost==3.75f); CHECK(table.Types()[71].cost==0);
    definitions[0]=o::RiftTypeDefinition::Parse("typename test { cost=\"invalid\"; } A00:default:\"a.gif\"#0;");
    CHECK(Throws([&]{ o::RiftTypeTable invalid(edition,sources); }));
}
