// 실제 공통 pre/postDestroy의 선택·목록·통계·비용을 저장된 독립 기계어 결과와 비교한다.
#include "TestSupport.h"
#include "o/SquidDestroyLifecycle.h"
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
        std::ifstream input(NETSTORM_DESTROYLIFECYCLE_FIXTURE);
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
// 실제 raw 삭제 및 새 장부 훅을 같은 풀/상태에 연결한 재생 장면이다.
struct Scene {
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    SquidUnpop unpop;
    RawSquidDestroy destroy;
    SquidPostPopState bookkeeping;
    SquidDeletionState state;
    std::unique_ptr<SquidDestroyLifecycle> lifecycle;
    std::vector<std::string> events;
    std::uint32_t flags{};
    // 19개 값은 fixture 생성기의 합성 입력이다. 결과 fixture만 기대값으로 쓴다.
    Scene(OriginalEdition edition,const std::vector<std::string>& input)
        :pool(edition,kCapacity,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171),unpop(pool,hash,spots),destroy(pool,unpop,types) {
        CHECK(input.size()==19);const bool patch=edition==OriginalEdition::Patch1078;
        // 부모 실제 Create와 동일한 client 할당만 준비한다.
        for (Sid sid:kIds) CHECK(pool.Allocate(2)==sid);
        // 같은 위치의 root 0단계·주변 1단계 버킷 체인을 넣는다.
        for (std::size_t i=0;i<kIds.size();++i) {
            auto raw=pool.AllocatedBytes(kIds[i]);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            Put(raw,0,patch ? 0x501dd0U : 0x5058d8U);raw[10]=static_cast<std::uint8_t>(74+i);
            Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
            types[74+i].footX=types[74+i].footY=1;
            auto& head=hash.Bucket(i==0 ? 0 : 1,20.75f,21.9f);Put(raw,4,head,2);head=kIds[i].value;
        }
        const auto number=static_cast<std::size_t>(std::stoul(input[0]));auto& type=types[number];
        type.flags1=static_cast<std::uint32_t>(std::stoul(input[1]));type.flags2=static_cast<std::uint32_t>(std::stoul(input[2]));
        type.cost=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(input[3])));type.group=std::stoi(input[4]);type.footX=type.footY=1;
        auto root=pool.AllocatedBytes({5});root[10]=static_cast<std::uint8_t>(number);root[11]=input[17]=="1" ? 4 : 0;
        root[patch ? 34 : 32]=static_cast<std::uint8_t>(std::stoi(input[5]));root[patch ? 40 : 35]=static_cast<std::uint8_t>(std::stoi(input[7]));
        if (root[11]&4) hash.Bucket(0,20.75f,21.9f)=0;
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
        lifecycle=std::make_unique<SquidDestroyLifecycle>(pool,types,bookkeeping,state,destroy,SquidDeletionHooks{
            [this](const SquidDeletionEvent& event) { Emit(event); },
            [this](const SquidDestroyEvent& event) {
                if (event.effect!=SquidDestroyEffect::Transmit) throw std::logic_error("장면에 없는 종속 가상 효과");
                events.push_back("T:"+std::to_string(event.sid.value)+':'+std::to_string(event.flags));
            }});
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
    // 직접 훅 쌍의 깊이 underflow와 통합 삭제의 균형 깊이를 서로 구별한다.
    void Run(bool pair) {
        if (pair) { Entry('P',{5},flags);lifecycle->PreDestroy({5},flags);Entry('O',{5},flags);lifecycle->PostDestroy({5},flags); }
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
    }
};
// 명시적인 C++ 보호 예외를 원본 assert 실행 없이 확인한다.
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
// 같은 raw 배치인 CD/10.37도 서로 다른 PE의 출력 행을 모두 재생한다.
void Replay(std::string_view kind,std::size_t expectedCount) {
    std::size_t count=0;
    // 저장된 기대값을 코드에서 다시 계산하지 않는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=kind) continue;CHECK(row.size()==9);++count;
        Scene scene(row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,Split(row[3],','));scene.Run(kind=="Pair");scene.Check(row);
    }
    CHECK(count==expectedCount);
}
// 보호 검사용 정상 입력. 외부 보상/소리를 억제하고 이미 void인 자산이다.
std::vector<std::string> Basic() { return Split("74,0,0,1142292480,0,1,1,0,2097152,0,0,0,0,148,3,1000,0,1,123",','); }
}

// 선택 복구·모든 중복 제거·현재/누적 통계·억제와 판본별 비용을 독립 원본 출력으로 비교한다.
TEST_CASE(DestroyLifecycle_ActualCommonPrePostBookkeeping) { Replay("Pair",1152); }
// 실제 공통 destroy→장부 pre→Unpop→장부 post→SID Release를 한 수명으로 대조한다.
TEST_CASE(DestroyLifecycle_ActualDestroyUnpopAndReleaseIntegration) { Replay("Destroy",576); }

// 생성/삭제는 현재 수와 비용을 되돌리고 누적 made만 늘린 채 유지한다.
TEST_CASE(DestroyLifecycle_PostPopSharesCurrentCountsAndPreservesMade) {
    // 두 판본의 같은 장부를 생성 훅과 삭제 훅에 연결한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        auto args=Basic();args[1]="268435456";args[2]="512";args[14]="0";Scene scene(edition,args);
        const auto before=scene.bookkeeping;SquidPostPop post(scene.pool,scene.types,scene.bookkeeping);post.Activate({5},1);
        const auto made=scene.bookkeeping.localSecondaryCounts[74];scene.destroy.Destroy({5},scene.flags,scene.lifecycle->Hooks());
        CHECK(scene.bookkeeping.totalCost==before.totalCost && scene.bookkeeping.localCounts==before.localCounts && scene.bookkeeping.globalCounts==before.globalCounts);
        CHECK(scene.bookkeeping.localSecondaryCounts[74]==made && made==before.localSecondaryCounts[74]+1);
        CHECK(scene.bookkeeping.providers.count==0 && scene.bookkeeping.factories.count==0 && scene.bookkeeping.ownerFactories[1].count==0);
        CHECK(scene.bookkeeping.productionDirty==4 && scene.destroy.PreDepth()==0 && scene.destroy.PostDepth()==0);
    }
}

// Graph/AI/손상 목록/누락 효과는 장부 쓰기와 깊이 감소 전에 거부한다.
TEST_CASE(DestroyLifecycle_RejectsUnsupportedEffectsBeforeBookkeepingWrites) {
    auto args=Basic();args[1]="268437504";args[2]="512";Scene scene(OriginalEdition::Patch1078,args);
    const auto before=scene.Bookkeeping();const auto raw=Adler(scene.pool.Bytes());
    scene.bookkeeping.graphsEnabled=true;CHECK(Throws([&] { scene.lifecycle->PreDestroy({5},0); }));CHECK(scene.Bookkeeping()==before);
    CHECK(Throws([&] { scene.lifecycle->PostDestroy({5},0); }));CHECK(scene.Bookkeeping()==before);scene.bookkeeping.graphsEnabled=false;
    scene.bookkeeping.aiAttached[1]=true;CHECK(Throws([&] { scene.lifecycle->PreDestroy({5},scene.flags); }));CHECK(scene.Bookkeeping()==before);scene.bookkeeping.aiAttached[1]=false;
    scene.bookkeeping.providers.count=7;const auto corrupt=scene.Bookkeeping();CHECK(Throws([&] { scene.lifecycle->PreDestroy({5},scene.flags); }));CHECK(scene.Bookkeeping()==corrupt);scene.bookkeeping.providers.count=6;
    SquidDestroyLifecycle missing(scene.pool,scene.types,scene.bookkeeping,scene.state,scene.destroy,{});
    CHECK(Throws([&] { missing.PreDestroy({5},0); }));scene.state.selected={5};CHECK(Throws([&] { missing.PreDestroy({5},scene.flags); }));
    CHECK(Adler(scene.pool.Bytes())==raw && scene.destroy.PreDepth()==0 && scene.destroy.PostDepth()==0);
}

// 억제는 Pre의 통계만 생략하고 ordinary Post 비용은 남기는 원본 계약이다.
TEST_CASE(DestroyLifecycle_SuppressionAndCostBoundaryGuards) {
    auto args=Basic();args[10]="1";Scene scene(OriginalEdition::Cd1072,args);const auto before=scene.bookkeeping;
    scene.Run(true);CHECK(scene.bookkeeping.localCounts==before.localCounts && scene.bookkeeping.globalCounts==before.globalCounts);
    CHECK(scene.bookkeeping.totalCost==before.totalCost-600 && scene.destroy.PreDepth()==0xffffffffU && scene.destroy.PostDepth()==0xffffffffU);
    scene.types[74].cost=std::numeric_limits<float>::infinity();SquidDestroyLifecycle invalid(scene.pool,scene.types,scene.bookkeeping,scene.state,scene.destroy,{});
    const auto cost=scene.bookkeeping.totalCost;CHECK(Throws([&] { invalid.PostDestroy({5},0); }));CHECK(scene.bookkeeping.totalCost==cost && scene.destroy.PostDepth()==0xffffffffU);
}

// 다른 풀 연결과 raw root 밖 경로를 명시적으로 거부한다.
TEST_CASE(DestroyLifecycle_RejectsWrongPoolAndFormOrContained) {
    Scene scene(OriginalEdition::Patch1078,Basic());SidPool other(OriginalEdition::Patch1078,kCapacity);
    CHECK(Throws([&] { SquidDestroyLifecycle invalid(other,scene.types,scene.bookkeeping,scene.state,scene.destroy,{}); }));
    auto raw=scene.pool.AllocatedBytes({5});raw[10]=20;CHECK(Throws([&] { scene.lifecycle->PreDestroy({5},scene.flags); }));
    raw[10]=74;raw[11]|=8;CHECK(Throws([&] { scene.lifecycle->PostDestroy({5},0); }));CHECK(scene.destroy.PreDepth()==0 && scene.destroy.PostDepth()==0);
}
