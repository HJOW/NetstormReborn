// 실제 공통 pre/postDestroy의 선택·목록·통계·비용을 저장된 독립 기계어 결과와 비교한다.
#include "TestSupport.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/RawGraph.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <memory>
#include <limits>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 실제 Reset/Create와 같은 풀 크기와 처음 일곱 client 번호다.
constexpr std::uint32_t kCapacity=32768;
constexpr std::array<Sid,7> kIds{{{5},{6},{7},{8},{9},{10},{11}}};
// 원문 TSV/콤마 항목을 순서대로 분리한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin=0;
    // 마지막 빈 항목도 보존한다.
    for (;;) {
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==std::string_view::npos ? end : end-begin));
        if (end==std::string_view::npos) return result;
        begin=end+1;
    }
}
// 기계어 출력만 읽으므로 일반 CTest에는 원본 PE/Python이 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_DESTROYGRAPH_FIXTURE);
        if (!input) throw std::runtime_error("삭제 장부 fixture 없음");
        std::vector<std::vector<std::string>> result;std::string line;
        // 주석은 실행 입력이 아니다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();return rows;
}
// 지정 폭의 little endian raw 필드를 쓰고 이웃 필드는 유지한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 호스트 정렬/엔디언을 입력에 섞지 않는다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// Python zlib와 같은 전체 버퍼 Adler-32다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 표준 소수와 중간 넘침을 막는 누적 폭이다.
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;std::size_t block=0;
    // 큰 풀도 4096바이트마다 나머지를 취한다.
    for (auto byte:bytes) { a+=byte;b+=a;if (++block==4096) { a%=prime;b%=prime;block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// DWORD를 물리 메모리와 같은 낮은 바이트 순서로 직렬화한다.
void Append(std::vector<std::uint8_t>& bytes,std::uint32_t value,std::size_t width=4) {
    // 구조체 패딩을 넣지 않는다.
    for (std::size_t i=0;i<width;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
// 원본 네 단계 해시의 모든 WORD를 비교한다.
std::uint32_t HashAdler(const SquidHash& hash) {
    std::vector<std::uint8_t> bytes;
    // 각 단계의 y/x 배열 순서다.
    for (int level=0;level<4;++level) { for (auto value:hash.Entries(level)) Append(bytes,value,2); }
    return Adler(bytes);
}
// client/server 최근 삭제 기록의 모든 DWORD를 비교한다.
std::uint32_t LogAdler(const SidPool& pool) {
    std::vector<std::uint8_t> bytes;
    // 두 기록 배열은 최신 항목부터 저장된다.
    for (bool client:{true,false}) { for (const auto& record:pool.Deletions(client)) { Append(bytes,record.sid);Append(bytes,record.type); } }
    return Adler(bytes);
}
// 원본 결과를 계산하지 않는 중복/미사용 꼬리 포함 목록 입력이다.
void ListInput(SquidPostPopList& list,int pattern,std::uint32_t base) {
    // 여섯 패턴의 활성 항목 수다.
    constexpr std::array<std::uint32_t,6> counts{0,1,5,6,4,3};list.count=counts.at(pattern);list.entries.resize(6);
    // 각 목록의 sentinel을 구별한다.
    for (std::size_t i=0;i<6;++i) list.entries[i]=base+static_cast<std::uint32_t>(i);
    if (pattern==1) list.entries[0]=5;
    if (pattern==3) list.entries[0]=list.entries[2]=list.entries[5]=5;
    if (pattern==4) std::fill_n(list.entries.begin(),4,5U);
    if (pattern==5) list.entries[1]=5;
}

// 발자국/프레임/상태/해시 단계의 원본 입력을 읽는다.
std::vector<std::vector<int>> Nodes(std::string_view text) {
    std::vector<std::vector<int>> result;if (text=="-") return result;
    // 각 객체/spot의 필드 순서를 유지한다.
    for (const auto& entry:Split(text,';')) {
        std::vector<int> node;for (const auto& value:Split(entry,',')) node.push_back(std::stoi(value));result.push_back(std::move(node));
    }
    return result;
}
// little endian hex에서 세 signed WORD를 패딩 없이 해독한다.
std::array<GraphRecord,Graph::kTableSize> Records(std::string_view hex) {
    CHECK(hex.size()==Graph::kTableSize*12);std::array<GraphRecord,Graph::kTableSize> result{};
    // sentinel/reserved를 포함한 모든 항목을 읽는다.
    for (std::size_t i=0;i<result.size();++i) {
        std::array<std::int16_t,3> words{};
        // 두 hex byte의 부호 비트를 보존한다.
        for (std::size_t j=0;j<3;++j) {
            const auto pos=i*12+j*4;const auto low=std::stoul(std::string(hex.substr(pos,2)),nullptr,16),high=std::stoul(std::string(hex.substr(pos+2,2)),nullptr,16);
            words[j]=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(low|(high<<8)));
        }
        result[i]={words[0],words[1],words[2]};
    }
    return result;
}
// 구조체 패딩을 비교하지 않고 원본 세 WORD만 대조한다.
bool SameRecord(const GraphRecord& a,const GraphRecord& b) { return a.surfaces==b.surfaces && a.inUse==b.inUse && a.reserved==b.reserved; }
// 원본 Graph의 두 사용 WORD와 reserved·스택 꼬리를 모두 관찰한다.
std::array<std::uint32_t,2> GraphHashes(const RawGraph& graph) {
    std::vector<std::uint8_t> records,stack;
    // 부호 있는 WORD도 원본 비트 그대로 비교한다.
    for (auto r:graph.Records()) { Append(records,static_cast<std::uint16_t>(r.surfaces),2);Append(records,static_cast<std::uint16_t>(r.inUse),2);Append(records,static_cast<std::uint16_t>(r.reserved),2); }
    // 활성 flood 범위 밖의 오래된 값도 포함한다.
    for (auto value:graph.FloodStack()) Append(stack,value);
    return {Adler(records),Adler(stack)};
}
struct Scene {
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::vector<std::vector<FrameCode>> frames;
    std::unique_ptr<RawGraph> graph;
    SquidUnpop unpop;
    RawSquidDestroy destroy;
    SquidPostPopState bookkeeping;
    SquidDeletionState state;
    std::unique_ptr<SquidDestroyLifecycle> lifecycle;
    std::vector<std::string> events;
    std::uint32_t flags{};
    // config/객체/spot/표는 생성기의 명시적인 입력이며 기대값을 계산하지 않는다.
    Scene(OriginalEdition edition,const std::vector<std::string>& input,const std::vector<std::vector<int>>& nodes,
        const std::vector<std::vector<int>>& initialSpots,std::string_view records)
        :pool(edition,kCapacity,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size()),unpop(pool,hash,spots),destroy(pool,unpop,types) {
        CHECK(input.size()==25 && nodes.size()==7);const bool patch=edition==OriginalEdition::Patch1078;
        // 실제 Reset/Create와 같은 첫 일곱 client 슬롯만 할당한다.
        for (Sid sid:kIds) CHECK(pool.Allocate(2)==sid);
        // root 번호/다른 객체의 타입·프레임 코드를 독립적으로 입력한다.
        for (std::size_t i=0;i<nodes.size();++i) {
            const auto& n=nodes[i];CHECK(n.size()==13);const auto number=i ? 74+i : std::stoul(input[0]);auto& type=types[number];
            type.flags1=static_cast<std::uint32_t>(n[4]);type.flags2=static_cast<std::uint32_t>(n[5]);type.footX=n[2];type.footY=n[3];type.cost=1.25f;
            frames[number]={{static_cast<std::uint8_t>(n[6]),static_cast<std::uint8_t>(n[7]),1,0},{'A','P',2,0}};
            auto raw=pool.AllocatedBytes(kIds[i]);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            Put(raw,0,patch ? 0x501dd0U : 0x5058d8U);Put(raw,8,23,2);raw[10]=static_cast<std::uint8_t>(number);raw[11]=static_cast<std::uint8_t>(n[9]);
            Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(n[0])));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(n[1])));
            raw[patch ? 30 : 28]=static_cast<std::uint8_t>(n[11]);raw[patch ? 33 : 31]=static_cast<std::uint8_t>(n[12]);raw[patch ? 34 : 32]=1;
            Put(raw,patch ? 36 : 34,static_cast<std::uint32_t>(n[8]),patch ? 4 : 1);raw[patch ? 40 : 35]=static_cast<std::uint8_t>(n[10]);
            if (!(n[9]&4)) { auto& head=hash.Bucket(n[12],static_cast<float>(n[0]),static_cast<float>(n[1]));Put(raw,4,head,2);head=kIds[i].value; }
        }
        const auto number=static_cast<std::size_t>(std::stoul(input[0]));types[number].group=std::stoi(input[4]);
        if (number!=162) {
            auto& replacement=types[162];replacement.flags1=TypeFlag1::kSurface;replacement.flags2=static_cast<std::uint32_t>(std::stoul(input[24]));
            replacement.footX=replacement.footY=std::stoi(input[23]);replacement.cost=1.25f;frames[162]={{'A','P',1,0},{'A','P',2,0}};
        }
        // interior 바이트는 해시/타입 입력과 별개다.
        for (const auto& spot:initialSpots) { CHECK(spot.size()==3);spots[static_cast<std::size_t>(spot[1]*256+spot[0])]=static_cast<std::uint8_t>(spot[2]); }
        flags=static_cast<std::uint32_t>(std::stoul(input[8]));state.selected=input[9]=="1" ? Sid{5} : Sid{};
        bookkeeping.localOwner=static_cast<std::uint8_t>(std::stoi(input[6]));bookkeeping.suppressed=input[10]=="1";
        state.unitLostSuppressed1=input[11]=="1";state.unitLostSuppressed2=input[12]=="1";state.silentType=static_cast<std::uint32_t>(std::stoul(input[13]));
        const auto pattern=std::stoi(input[14]);bookkeeping.totalCost=std::stoi(input[15]);bookkeeping.productionDirty=static_cast<std::uint32_t>(std::stoul(input[16]));
        bookkeeping.depth=3;ListInput(bookkeeping.providers,pattern,0x60000000);ListInput(bookkeeping.factories,pattern,0x61000000);
        // 원본 Player 0의 미사용 목록까지 같은 입력을 넣는다.
        for (std::size_t i=0;i<9;++i) ListInput(bookkeeping.ownerFactories[i],pattern,0x70000000+static_cast<std::uint32_t>(i)*100);
        const auto seed=static_cast<std::uint32_t>(std::stoul(input[18]));
        // 통계 표 전체의 비대상 타입 보존과 DWORD 감김도 비교한다.
        for (std::size_t i=0;i<256;++i) {
            bookkeeping.localCounts[i]=seed+static_cast<std::uint32_t>(i)*2654435761U;
            bookkeeping.localSecondaryCounts[i]=bookkeeping.localCounts[i]+17U;bookkeeping.globalCounts[i]=bookkeeping.localCounts[i]+34U;
        }
        state.placementX=-7.25f;state.placementY=8.5f;state.lostX=90.25f;state.lostY=-91.5f;
        bookkeeping.graphsEnabled=input[19]=="1";state.rebuildGraph=input[20]=="1";state.regionMask=static_cast<std::uint32_t>(std::stoul(input[21]));
        std::array<std::uint32_t,Graph::kFloodSize> stack{};const auto graphSeed=static_cast<std::uint32_t>(std::stoul(input[22]));
        // 전체 스택의 오래된 DWORD를 native 입력과 맞춘다.
        for (std::size_t i=0;i<stack.size();++i) stack[i]=graphSeed+static_cast<std::uint32_t>(i);
        graph=std::make_unique<RawGraph>(pool,hash,spots,types,frames,Records(records),stack);
        lifecycle=std::make_unique<SquidDestroyLifecycle>(pool,types,bookkeeping,state,destroy,SquidDeletionHooks{
            [this](const SquidDeletionEvent& event) { Emit(event); },
            [this](const SquidDestroyEvent& event) {
                if (event.effect!=SquidDestroyEffect::Transmit) throw std::logic_error("장면에 없는 종속 가상 효과");
                events.push_back("T:"+std::to_string(event.sid.value)+':'+std::to_string(event.flags));
            }},graph.get());
    }
    // 실제 장부 함수가 호출한 외부 UI/보상/소리만 기계어 대체 기록과 같은 형식으로 둔다.
    void Emit(const SquidDeletionEvent& event) {
        const auto id=std::to_string(event.sid.value);
        switch (event.effect) {
        case SquidDeletionEffect::ClearSelection:events.push_back("S:"+std::to_string(state.selected.value));break;
        case SquidDeletionEffect::Refund:events.push_back("Q:"+id+':'+std::to_string(event.recipient));break;
        case SquidDeletionEffect::UnitLostSound:events.push_back("L:"+id);break;
        }
    }
    // P/O 함수 진입 때 raw state를 별도로 관찰한다.
    void Entry(char tag,Sid sid,std::uint32_t value) { events.push_back(std::string(1,tag)+':'+std::to_string(sid.value)+':'+std::to_string(value)+':'+std::to_string(pool.Slot(sid)[11])); }
    // 직접 전/후 훅의 깊이 underflow와 통합 삭제의 균형 깊이를 서로 구별한다.
    void Run(std::string_view kind) {
        if (kind=="Pre") { Entry('P',{5},flags);lifecycle->PreDestroy({5},flags); }
        else if (kind=="Post") { Entry('O',{5},flags);lifecycle->PostDestroy({5},flags); }
        else {
            auto hooks=lifecycle->Hooks();const auto emit=hooks.emit;
            // 전체 삭제 안에서도 공통 훅 몸체 자체를 호출한다.
            hooks.emit=[&,emit](const SquidDestroyEvent& event) {
                if (event.effect==SquidDestroyEffect::PreDestroy) Entry('P',event.sid,event.flags);
                if (event.effect==SquidDestroyEffect::PostDestroy) Entry('O',event.sid,event.flags);
                emit(event);
            };
            destroy.Destroy({5},flags,hooks);
        }
    }
    // 모든 목록의 inactive DWORD와 누적 생산 표를 포함한 장부 결과다.
    std::vector<std::int64_t> Bookkeeping() const {
        std::vector<std::int64_t> values{bookkeeping.totalCost,bookkeeping.productionDirty,bookkeeping.depth};std::vector<std::uint8_t> data;
        // 서로 다른 세 타입 표를 정해진 순서로 해시한다.
        for (const auto* table:{&bookkeeping.localCounts,&bookkeeping.localSecondaryCounts,&bookkeeping.globalCounts}) {
            std::vector<std::uint8_t> bytes;for (auto value:*table) Append(bytes,value);values.push_back(Adler(bytes));
        }
        // provider/factory 다음에 Player 0..8 전체 목록을 관찰한다.
        for (const auto* list:{&bookkeeping.providers,&bookkeeping.factories}) { values.push_back(list->count);for (auto value:list->entries) Append(data,value); }
        for (const auto& list:bookkeeping.ownerFactories) { values.push_back(list.count);for (auto value:list.entries) Append(data,value); }
        values.push_back(Adler(data));return values;
    }
    // 외부 효과와 raw 슬롯 바이트·전체 풀/공간·장부·선택 좌표를 모두 대조한다.
    void Check(const std::vector<std::string>& row) const {
        std::string text;for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }CHECK((text.empty() ? "-" : text)==row[4]);
        // root와 주변 일곱 슬롯은 checksum 대신 직접 바이트도 확인한다.
        for (const auto& entry:Split(row[5],';')) {
            const auto fields=Split(entry,':');const auto raw=pool.Slot({static_cast<std::uint16_t>(std::stoul(fields[0]))});CHECK(fields[1].size()==raw.size()*2);
            for (std::size_t i=0;i<raw.size();++i) CHECK(raw[i]==std::stoul(fields[1].substr(i*2,2),nullptr,16));
        }
        const std::array<std::uint32_t,13> actual{pool.FreeCount(),pool.PredictableCursor(),pool.FirstFree(true).value,pool.FirstFree(false).value,
            pool.Tail(true).value,pool.Tail(false).value,Adler(pool.Bytes()),HashAdler(hash),Adler(spots),LogAdler(pool),state.selected.value,destroy.PreDepth(),destroy.PostDepth()};
        const auto summary=Split(row[6],',');CHECK(summary.size()==actual.size());
        // 실제 공간 해제/풀 반납/깊이 출력이다.
        for (std::size_t i=0;i<actual.size();++i) CHECK(actual[i]==std::stoul(summary[i]));
        const auto expected=Split(row[7],',');const auto book=Bookkeeping();CHECK(expected.size()==book.size());
        for (std::size_t i=0;i<book.size();++i) CHECK(book[i]==std::stoll(expected[i]));
        const auto context=Split(row[8],',');const std::array<std::uint32_t,5> coordinates{state.placementType,std::bit_cast<std::uint32_t>(state.placementX),
            std::bit_cast<std::uint32_t>(state.placementY),std::bit_cast<std::uint32_t>(state.lostX),std::bit_cast<std::uint32_t>(state.lostY)};
        CHECK(context.size()==coordinates.size());for (std::size_t i=0;i<coordinates.size();++i) CHECK(coordinates[i]==std::stoul(context[i]));
        CHECK(bookkeeping.pendingPlacement==(state.placementType!=0));
        const auto expectedRecords=Records(row[12]);CHECK(std::equal(expectedRecords.begin(),expectedRecords.end(),graph->Records().begin(),SameRecord));
        CHECK(GraphHashes(*graph)[1]==std::stoul(row[13]));
    }
};

// 보호 예외는 원본 assert를 실행하지 않고 C++ 입력 경계만 확인한다.
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
// fixture 전체 상태를 한 장면으로 재생한다.
std::unique_ptr<Scene> Make(const std::vector<std::string>& row) {
    CHECK(row.size()==14);return std::make_unique<Scene>(row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,
        Split(row[3],','),Nodes(row[9]),Nodes(row[10]),row[11]);
}
// 첫 patch/x87 묶음에서 지정된 사례만 꺼내 보호 검사를 구성한다.
const std::vector<std::string>& Case(std::string_view kind,int index) {
    const auto seed=std::to_string(index*17+10);
    // fixture의 명시적 초기 스택 seed를 선택 키로만 사용한다.
    for (const auto& row:Fixture()) if (row[0]==kind && row[1]=="originals" && row[2]=="639" && Split(row[3],',')[22]==seed) return row;
    throw std::runtime_error("삭제 Graph 입력 없음");
}
// 모든 실제 PE/정밀도 결과를 기대값으로 삼는다.
void Replay(std::string_view kind) {
    std::size_t count=0;
    // 비교 실패는 원본 입력 행까지 추적할 수 있게 남긴다.
    for (const auto& row:Fixture()) {
        if (row[0]!=kind) continue;auto scene=Make(row);
        try {
            const auto failures=netstorm::test::FailureCount();scene->Run(kind);scene->Check(row);
            if (netstorm::test::FailureCount()!=failures) throw std::runtime_error("저장 원본 출력과 불일치");
        } catch (const std::exception& error) {
            throw std::runtime_error(row[0]+"/"+row[1]+"/"+row[2]+"/"+row[3]+": "+error.what());
        }
        ++count;
    }
    CHECK(count==576);
}
}

// 원본 pre의 표면 분할/Free·특수 H 치환·선택/장부 순서를 비교한다.
TEST_CASE(DestroyGraph_ActualPreDetachFreeAndSpecialFrame) { Replay("Pre"); }
// 발자국 경계·네 해시 단계·중복 머리·inactive 표면과 순차 Add를 비교한다.
TEST_CASE(DestroyGraph_ActualPostFootprintFinderAndSequentialAdd) { Replay("Post"); }
// actual destroy→pre Graph/장부→Unpop→post Graph/비용→Release 전체를 비교한다.
TEST_CASE(DestroyGraph_ActualDestroyUnpopGraphAndRelease) { Replay("Destroy"); }

// Free는 raw root 번호만 해제하며 noGraph보다 먼저 실행하고 나머지 메모리를 보존한다.
TEST_CASE(DestroyGraph_FreeGuardsAndUntouchedPayload) {
    auto scene=Make(Case("Pre",13));const auto pool=Adler(scene->pool.Bytes()),hash=HashAdler(scene->hash),spots=Adler(scene->spots);
    const auto stack=GraphHashes(*scene->graph)[1];const auto root=scene->pool.Slot({5});const auto graphByte=root[30];
    const auto before=std::vector<GraphRecord>(scene->graph->Records().begin(),scene->graph->Records().end());
    scene->graph->Free(graphByte);const auto after=scene->graph->Records();
    CHECK(after[graphByte].surfaces==0 && after[graphByte].inUse==0 && after[graphByte].reserved==before[graphByte].reserved);
    CHECK(SameRecord(after[1],before[1]) && pool==Adler(scene->pool.Bytes()) && hash==HashAdler(scene->hash) && spots==Adler(scene->spots) && stack==GraphHashes(*scene->graph)[1]);
    const auto unchanged=GraphHashes(*scene->graph);scene->graph->Free(254);CHECK(GraphHashes(*scene->graph)==unchanged);
    // 원본 assert 번호는 호스트 표/스택을 건드리지 않고 거부한다.
    for (std::uint8_t invalid:std::array<std::uint8_t,4>{251,252,253,255}) CHECK(Throws([&] { scene->graph->Free(invalid); }));
    CHECK(GraphHashes(*scene->graph)==unchanged);
}

// H 치환 타입의 유효성은 필요한 분기에만 검사하며 실패하면 선택/장부/Graph를 보존한다.
TEST_CASE(DestroyGraph_SpecialReplacementValidationBeforeWrites) {
    auto scene=Make(Case("Pre",8));scene->state.specialSurfaceReplacementType=255;
    const auto book=scene->Bookkeeping();const auto graph=GraphHashes(*scene->graph);const auto raw=Adler(scene->pool.Bytes());
    CHECK(Throws([&] { scene->lifecycle->PreDestroy({5},scene->flags); }));
    CHECK(book==scene->Bookkeeping() && graph==GraphHashes(*scene->graph) && raw==Adler(scene->pool.Bytes()) && scene->state.selected==Sid{5} && scene->destroy.PreDepth()==0);
    auto skipped=Make(Case("Pre",9));skipped->state.specialSurfaceReplacementType=255;skipped->Run("Pre");skipped->Check(Case("Pre",9));
}

// 주변 후보 프레임 오류는 실제 풀/표/스택·비용·깊이를 반영하기 전에 거부한다.
TEST_CASE(DestroyGraph_PostRejectsInvalidCandidateBeforeAnyWrites) {
    auto scene=Make(Case("Post",14));Put(scene->pool.AllocatedBytes({7}),36,999);
    const auto book=scene->Bookkeeping();const auto graph=GraphHashes(*scene->graph);const auto raw=Adler(scene->pool.Bytes());
    CHECK(Throws([&] { scene->lifecycle->PostDestroy({5},scene->flags); }));
    CHECK(book==scene->Bookkeeping() && graph==GraphHashes(*scene->graph) && raw==Adler(scene->pool.Bytes()) && scene->destroy.PostDepth()==0);
}

// 동일 크기라도 풀·해시·spot·타입이 다른 Graph를 공통 삭제에 연결하지 않는다.
TEST_CASE(DestroyGraph_RejectsMismatchedRawSpaceAndTypes) {
    auto scene=Make(Case("Pre",0));SquidHash otherHash;std::vector<std::uint8_t> otherSpots(65536);
    RawGraph wrongHash(scene->pool,otherHash,scene->spots,scene->types,scene->frames);
    RawGraph wrongSpots(scene->pool,scene->hash,otherSpots,scene->types,scene->frames);
    auto otherTypes=scene->types;otherTypes[74].footX=2;RawGraph wrongTypes(scene->pool,scene->hash,scene->spots,otherTypes,scene->frames);
    // 생성 시 연결 오류를 거부하며 기존 장면은 변경하지 않는다.
    for (auto* wrong:{&wrongHash,&wrongSpots,&wrongTypes}) CHECK(Throws([&] {
        SquidDestroyLifecycle invalid(scene->pool,scene->types,scene->bookkeeping,scene->state,scene->destroy,{},wrong);
    }));
    CHECK(scene->destroy.PreDepth()==0 && scene->destroy.PostDepth()==0);
}
