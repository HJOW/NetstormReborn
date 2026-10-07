// 원본 공통 destroy의 순서·실제 공간 해제/반납과 다리 훅 내부의 중첩 삭제를 창 없이 검사한다.
#include "TestSupport.h"
#include "o/RawSquidDestroy.h"
#include "o/RawBridgeLifecycle.h"
#include "o/RawSquidFinder.h"
#include "o/SquidFactory.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 세 실제 PE·두 x87 정밀도에 대한 고정 행 수와 실제 준비 풀 크기다.
constexpr std::size_t kDestroyRows=1800,kBridgeRows=192;
constexpr std::uint32_t kCapacity=32768;
// 실제 Reset/Create가 확보하는 일곱 client 번호다. 추가 root는 실제 Take로 준비한다.
constexpr std::array<Sid,7> kIds{{{5},{6},{7},{8},{9},{10},{11}}};
// TSV/목록을 원문 순서대로 분리한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin=0;
    // 마지막 필드까지 보존한다.
    for (;;) {
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==std::string_view::npos ? end : end-begin));
        if (end==std::string_view::npos) return result;
        begin=end+1;
    }
}
// 고정 기계어 출력만 읽으므로 테스트 때 원본 PE/Python/Ghidra가 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_DESTROY_FIXTURE);
        if (!input) throw std::runtime_error("Missing destroy fixture");
        std::vector<std::vector<std::string>> result;std::string line;
        // 주석을 제외하고 모든 비교 행을 읽는다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();return rows;
}
// 원본 raw WORD/DWORD/float 비트를 little endian으로 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 지정 폭 밖의 이웃 필드는 보존한다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// Python zlib와 같은 전체 버퍼 Adler-32다. 슬롯은 바이트별 비교도 한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 표준 소수와 누적 상태다. 긴 풀의 중간 넘침을 피한다.
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;std::size_t block=0;
    // 4096바이트마다 나머지를 취한다.
    for (auto byte:bytes) { a+=byte;b+=a;if (++block==4096) { a%=prime;b%=prime;block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// 네 해시 단계의 모든 WORD를 원본 배열 순서대로 비교한다.
std::uint32_t HashAdler(const SquidHash& hash) {
    std::vector<std::uint8_t> bytes;
    // 0단계부터 3단계까지 나란히 직렬화한다.
    for (int level=0;level<4;++level) {
        // 호스트 ushort 엔디언을 체크섬에 넣지 않는다.
        for (auto value:hash.Entries(level)) { bytes.push_back(static_cast<std::uint8_t>(value));bytes.push_back(static_cast<std::uint8_t>(value>>8)); }
    }
    return Adler(bytes);
}
// client/server 삭제 기록 20개씩의 모든 DWORD를 비교한다.
std::uint32_t LogAdler(const SidPool& pool) {
    std::vector<std::uint8_t> bytes;
    // 원본 두 기록 배열의 순서다.
    for (bool client:{true,false}) {
        // 최신 항목부터 나란히 읽는다.
        for (const auto& record:pool.Deletions(client)) {
            // 구조체 패딩은 넣지 않는다.
            for (auto value:{record.sid,record.type}) {
                // 각 DWORD를 낮은 바이트부터 기록한다.
                for (int shift=0;shift<32;shift+=8) bytes.push_back(static_cast<std::uint8_t>(value>>shift));
            }
        }
    }
    return Adler(bytes);
}
// 외부 효과만 명시적으로 대체한 동일 입력이다. 실제 Unpop/반납은 C++ 구현을 호출한다.
struct Scene {
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    SquidUnpop unpop;
    RawSquidDestroy destroy;
    RawBridgeLifecycle bridge;
    RawSquidFinder finder;
    std::map<std::uint16_t,std::span<std::uint8_t>> slots;
    std::vector<Sid> active;
    std::vector<std::string> events;
    Sid root{},selected{};
    bool bridgeMode{};
    int scenario{};
    SquidDestroyHooks hooks;
    BridgeLifecycleHooks bridgeHooks;
    // Reset/Create 뒤 덮어쓰는 입력은 vtable 기본값과 풀 목록 외에 생성자 효과가 없다.
    Scene(OriginalEdition edition,bool server,int role,int inputCase,bool isBridge)
        :pool(edition,kCapacity,server),types(edition==OriginalEdition::Patch1078 ? 188 : 171),unpop(pool,hash,spots),destroy(pool,unpop,types),
         bridge(pool,types),finder(pool,hash,types),bridgeMode(isBridge),scenario(inputCase) {
        // 처음 일곱 슬롯은 실제 client 할당의 동일 경로다.
        for (Sid expected:kIds) { CHECK(pool.Allocate(2)==expected); }
        root=role==0 ? kIds[0] : Sid{static_cast<std::uint16_t>(role==1 ? pool.Layout().serverFirst+9 : pool.Layout().predictableFirst+1)};
        if (role) { SquidFactory factory(pool,types);CHECK(factory.Take(74,root)==root); }
        active.push_back(root);
        // 추가 root 뒤 기존 일곱 번호를 중복 없이 둔다.
        for (Sid sid:kIds) if (sid!=root) active.push_back(sid);
        selected=inputCase%2 ? root : Sid{};
        // fixture의 합성 raw/타입/등록 입력만 옮긴다. 삭제 결과는 계산하지 않는다.
        for (Sid sid:active) {
            auto raw=pool.AllocatedBytes(sid);slots.emplace(sid.value,raw);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            const auto it=std::find(kIds.begin(),kIds.end(),sid);
            int type=sid==root ? 74 : 74+static_cast<int>(it-kIds.begin());
            int state=sid==root ? (inputCase==1 ? 4 : inputCase==2 ? 2 : 0) : 0;
            if (!isBridge && inputCase>=3 && inputCase<=6 && (sid==kIds[1] || (inputCase!=3 && sid==kIds[2]))) {
                type=inputCase==3 ? 20 : 75;state=inputCase==3 ? 0 : 8;
            }
            if (isBridge) { if (sid==kIds[1] || sid==kIds[4]) type=75;if (sid==kIds[2]) state=2; }
            const bool patch=edition==OriginalEdition::Patch1078;
            Put(raw,0,isBridge && sid==root ? (patch ? 0x5034c8u : 0x501ab0u) : (patch ? 0x501dd0u : 0x5058d8u));
            raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(state);
            Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
            types[type].flags1=0;types[type].flags2=isBridge && sid==root ? 4U : 0U;types[type].footX=types[type].footY=1;
            if (isBridge && sid==kIds[5]) types[type].flags2=0x10000;
            if (!(state&12) && type>=70) { auto& head=hash.Bucket(sid==root ? 0 : 1,20.75f,21.9f);Put(raw,4,head,2);head=sid.value; }
        }
        if (!isBridge && inputCase>=3 && inputCase<=6) {
            Put(slots.at(root.value),6,kIds[1].value,2);
            Put(slots.at(kIds[1].value),4,inputCase==3 ? 0 : kIds[2].value,2);
            if (inputCase!=3) Put(slots.at(kIds[2].value),4,0,2);
        }
        if (isBridge) {
            types[74].flags2=4;
            Put(slots.at(6),12,7,2);Put(slots.at(6),8,8,2);Put(slots.at(9),12,6,2);Put(slots.at(9),8,8,2);
        }
        // 같은 풀을 쓰는 효과 처리기를 연결한다. 다리 삭제는 실제 공통 destroy로 재진입한다.
        hooks={ [this](const SquidDestroyEvent& event) { Emit(event); },[this] { return selected; } };
        bridgeHooks=MakeBridgeLifecycleHooks(finder,[this](const BridgeLifecycleEvent& event) { EmitBridge(event); });
    }
    // P/O/A/B는 효과 직전 state까지 관찰한다. 같은 객체 재삭제는 dead 때문에 사건을 추가하지 않는다.
    void HookEvent(char tag,Sid sid,std::uint32_t flags) {
        events.push_back(std::string(1,tag)+':'+std::to_string(sid.value)+':'+std::to_string(flags)+':'+std::to_string(pool.Slot(sid)[11]));
    }
    // 원본 가상 release/destroy·선택·전파·공통 훅의 명시적 대체다.
    void Emit(const SquidDestroyEvent& event) {
        const auto id=std::to_string(event.sid.value),flags=std::to_string(event.flags);
        switch (event.effect) {
        case SquidDestroyEffect::PreDestroy:
            HookEvent('P',event.sid,event.flags);
            if (bridgeMode && event.sid==root) bridge.PreDestroy(root,event.flags,75,bridgeHooks);
            else {
                destroy.CompletePreDestroy();
                if (event.sid==root && scenario==6) Put(slots.at(root.value),6,7,2);
                if (event.sid==root && scenario==7) slots.at(root.value)[11]|=4;
            }
            break;
        case SquidDestroyEffect::PostDestroy:
            HookEvent('O',event.sid,event.flags);
            if (bridgeMode && event.sid==root) bridge.PostDestroy(root,event.flags,bridgeHooks);
            else destroy.CompletePostDestroy();
            break;
        case SquidDestroyEffect::ReleaseDependent:
            events.push_back("R:"+id+':'+std::to_string(event.parent.value));Put(slots.at(event.sid.value),4,0,2);
            if (scenario==5 && event.sid==kIds[1]) slots.at(event.sid.value)[11]=1;
            break;
        case SquidDestroyEffect::DestroyDependent:
            events.push_back("C:"+id+':'+flags);slots.at(event.sid.value)[11]|=2;break;
        case SquidDestroyEffect::Transmit:events.push_back("T:"+id+':'+flags);break;
        case SquidDestroyEffect::ClearSelection:events.push_back("S:"+std::to_string(selected.value));selected={};break;
        }
    }
    // 일반 finder의 커서를 유지한 채 링크는 실제로 해제/반납한다. 파편/소리/낙하는 사건 대체다.
    void EmitBridge(const BridgeLifecycleEvent& event) {
        const auto id=std::to_string(event.sid.value);
        switch (event.effect) {
        case BridgeLifecycleEffect::DestroyLink:destroy.Destroy(event.sid,event.flags,hooks);break;
        case BridgeLifecycleEffect::BasePreDestroy:HookEvent('A',event.sid,event.flags);destroy.CompletePreDestroy();break;
        case BridgeLifecycleEffect::BasePostDestroy:HookEvent('B',event.sid,event.flags);destroy.CompletePostDestroy();break;
        case BridgeLifecycleEffect::NotifyRemoval:events.push_back("N:"+id);break;
        case BridgeLifecycleEffect::FallSound:
            events.push_back("W:"+std::to_string(std::bit_cast<std::uint32_t>(event.x))+':'+std::to_string(std::bit_cast<std::uint32_t>(event.y)));break;
        case BridgeLifecycleEffect::FallWalker:events.push_back("F:"+id);slots.at(event.sid.value)[11]|=2;break;
        }
    }
    // 관찰 순서를 바꾸지 않고 기계어 외부 효과 기록과 비교한다.
    std::string Events() const {
        std::string result;
        // 빈 목록은 원본 fixture와 같은 - 표식이다.
        for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }
        return result.empty() ? "-" : result;
    }
    // 전체 풀·목록·공간·기록·선택·훅 깊이를 독립 기계어 출력에 대조한다.
    void Check(const std::vector<std::string>& row) const {
        CHECK(Events()==row[7]);
        // 대상과 주변 일곱 슬롯은 모든 바이트를 직접 비교한다.
        for (const auto& entry:Split(row[8],';')) {
            const auto fields=Split(entry,':');const auto raw=pool.Slot({static_cast<std::uint16_t>(std::stoul(fields[0]))});
            CHECK(fields[1].size()==raw.size()*2);
            // raw vtable 값도 정규화된 원본 기록값과 같다.
            for (std::size_t i=0;i<raw.size();++i) CHECK(raw[i]==std::stoul(fields[1].substr(i*2,2),nullptr,16));
        }
        const std::array<std::uint32_t,13> actual{pool.FreeCount(),pool.PredictableCursor(),pool.FirstFree(true).value,pool.FirstFree(false).value,
            pool.Tail(true).value,pool.Tail(false).value,Adler(pool.Bytes()),HashAdler(hash),Adler(spots),LogAdler(pool),selected.value,
            destroy.PreDepth(),destroy.PostDepth()};
        const auto expected=Split(row[9],',');CHECK(expected.size()==actual.size());
        // 실제 반납이 건드린 풀 꼬리와 최신 삭제 기록도 빠짐없이 비교한다.
        for (std::size_t i=0;i<actual.size();++i) CHECK(actual[i]==std::stoul(expected[i]));
    }
};
// 새 보호 경로는 원본 assert나 무한 루프를 실행하지 않는다.
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
// 모든 저장 입력을 재생하되 기대값은 원본 기계어 fixture만 사용한다.
void Replay(std::string_view kind,std::size_t expectedCount) {
    std::size_t count=0;
    // 10.37은 CD와 같은 raw 배치이며 실제 별도 PE 출력도 모두 대조한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=kind) continue;
        CHECK(row.size()==10);++count;
        Scene scene(row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row[3]=="1",std::stoi(row[4]),std::stoi(row[6]),kind=="Bridge");
        scene.destroy.Destroy(scene.root,static_cast<std::uint32_t>(std::stoul(row[5])),scene.hooks);scene.Check(row);
    }
    CHECK(count==expectedCount);
}
}

// 공통 자산 삭제·종속 변경·목록/전체 공간 결과의 기계어 기대값을 재생한다.
TEST_CASE(RawDestroy_OriginalMachineOrderSpacePoolAndDependentEffects) { Replay("Destroy",kDestroyRows); }
// 같은 finder를 유지하면서 실제 링크 해제/반납을 수행한 다리 삭제를 검사한다.
TEST_CASE(RawDestroy_ActualBridgeFinderAndNestedRelease) { Replay("Bridge",kBridgeRows); }

// dead를 가장 먼저 표시하는 순서가 중첩/반복 반납을 막는지 확인한다.
TEST_CASE(RawDestroy_DeadPreventsReentryAndRepeatedRelease) {
    Scene scene(OriginalEdition::Patch1078,true,0,0,false);
    const auto emit=scene.hooks.emit;
    // Pre 중 같은 root 재삭제는 dead 검사에서 끝나고 외부 효과/깊이를 늘리지 않는다.
    scene.hooks.emit=[&](const SquidDestroyEvent& event) { if (event.effect==SquidDestroyEffect::PreDestroy) scene.destroy.Destroy(scene.root,0,scene.hooks);emit(event); };
    scene.destroy.Destroy(scene.root,0,scene.hooks);const auto before=Adler(scene.pool.Bytes()),log=LogAdler(scene.pool);const auto count=scene.events.size();
    scene.destroy.Destroy(scene.root,0,{});
    CHECK(Adler(scene.pool.Bytes())==before && LogAdler(scene.pool)==log && scene.events.size()==count);
    CHECK(scene.destroy.PreDepth()==0 && scene.destroy.PostDepth()==0);
}

// 훅 깊이의 불균형은 해당 효과 뒤에 멈추며 부분 효과를 되돌린다고 주장하지 않는다.
TEST_CASE(RawDestroy_RejectsMissingCompletionAtItsEffectBoundary) {
    Scene pre(OriginalEdition::Patch1078,true,0,0,false);
    // 완료되지 않은 pre 뒤에는 Unpop/반납을 실행하지 않는다. 이미 수행한 dead/훅은 되돌리지 않는다.
    pre.hooks.emit=[](const SquidDestroyEvent&) {};
    CHECK(Throws([&] { pre.destroy.Destroy(pre.root,0,pre.hooks); }));
    CHECK(pre.pool.Slot(pre.root)[11]==2 && pre.destroy.PreDepth()==1 && pre.pool.Deletions(true)[0].sid==0);
    Scene post(OriginalEdition::Cd1072,true,0,0,false);
    // post 완료 누락은 실제 공간 해제 뒤 검출되고 SID는 반납하지 않는다.
    post.hooks.emit=[&](const SquidDestroyEvent& e) { if (e.effect==SquidDestroyEffect::PreDestroy) post.destroy.CompletePreDestroy(); };
    CHECK(Throws([&] { post.destroy.Destroy(post.root,0,post.hooks); }));
    CHECK(post.pool.Slot(post.root)[11]==6 && post.destroy.PostDepth()==1 && post.pool.Deletions(true)[0].sid==0);
}

// 원본 assert에 해당하는 미지원 입력은 dead 표시 전에 거부한다.
TEST_CASE(RawDestroy_ValidatesAuthorityPoolAndRootBeforeMutation) {
    Scene scene(OriginalEdition::Patch1078,false,1,0,false);const auto before=Adler(scene.pool.Bytes());
    CHECK(Throws([&] { scene.destroy.Destroy(scene.root,0,scene.hooks); }));
    CHECK(Throws([&] { scene.destroy.Destroy(scene.root,8,{}); }));
    CHECK(Throws([&] { scene.destroy.Destroy({0},8,scene.hooks); }));
    CHECK(Throws([&] { scene.destroy.Destroy({32768},8,scene.hooks); }));
    CHECK(Adler(scene.pool.Bytes())==before);
    SidPool other(OriginalEdition::Patch1078,kCapacity);
    CHECK(Throws([&] { RawSquidDestroy invalid(other,scene.unpop,scene.types); }));
    // free/contained/form root는 공통 자산 destroy 어댑터의 지원 범위 밖이다.
    for (auto state:{1,8}) { scene.slots.at(scene.root.value)[11]=static_cast<std::uint8_t>(state);CHECK(Throws([&] { scene.destroy.Destroy(scene.root,8,scene.hooks); }));CHECK(scene.pool.Slot(scene.root)[11]==state); }
    scene.slots.at(scene.root.value)[11]=0;scene.slots.at(scene.root.value)[10]=20;
    CHECK(Throws([&] { scene.destroy.Destroy(scene.root,8,scene.hooks); }));CHECK(scene.pool.Slot(scene.root)[11]==0);
}

// 종속 상태/체인 손상은 root의 공간 해제·반납 전 검출한다.
TEST_CASE(RawDestroy_RejectsInvalidOrCyclicDependentAfterPre) {
    // 종속 current의 유효 상태와 next 순환을 각각 확인한다. root dead 이후의 보호다.
    for (int scenario:{0,1,2}) {
        Scene scene(OriginalEdition::Patch1078,true,0,4,false);
        if (scenario==0) scene.slots.at(6)[11]=1;
        if (scenario==1) scene.slots.at(6)[11]=0;
        if (scenario==2) { Put(scene.slots.at(6),4,6,2); }
        CHECK(Throws([&] { scene.destroy.Destroy(scene.root,0,scene.hooks); }));
        CHECK(scene.pool.Slot(scene.root)[11]==2 && scene.pool.Deletions(true)[0].sid==0);
    }
}
