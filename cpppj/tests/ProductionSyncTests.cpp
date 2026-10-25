#include "client/ClientGameplayGlobals.h" // 실제0054db14 DWORD의 동일 장기 owner를 빌린다.
// 독립 원본98입력의 생산 내용물 삭제/생성/중복·신전/조회 순서를 새 몸체와 대조한다.
#include "ConstructionSupport.h"
#include "o/RawProductionSync.h"
#include "o/RawProductionDeck.h"
using namespace netstorm::test::construction;
namespace {
// 입력 두 정수의 목록만 해석하며 기대 동작은 만들지 않는다.
std::vector<std::array<std::int32_t,2>> Pairs(const std::string& text) {
    std::vector<std::array<std::int32_t,2>> result;if (text=="-") return result;
    // 세미콜론으로 나뉜 원문kind:flags 또는kind:power를 읽는다.
    for (const auto& item:Split(text,';')) { const auto fields=Split(item,':');CHECK(fields.size()==2);result.push_back({std::stoi(fields[0]),std::stoi(fields[1])}); }return result;
}
// 메시지의 원본 UTF8/ASCII 바이트를 그대로 기록한다.
std::string TextBytes(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 원본 실행기에 공급한 동일 풀/TYPE/Deck/조회 입력이다.
struct SyncScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 같은 물리 ContentForm 범위를 사용한다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 명시 TYPEbit4 프로필이다.
    ContainedFinderState finder;std::array<std::uint8_t,kProductionDeckBytes> deck{}; // 실제 순회 정책과5바이트 Deck이다.
    std::uint32_t owner{1},full{},change{},getters{},query{};bool temple{}; // 원본 현재 전역과 조회 순서 입력이다.
    std::vector<std::uint32_t> queries;std::string trace;ProductionSyncHooks hooks; // 명시 조회 반환과 실제 효과 관측이다.
    // 실제 새 계산은 수행하지 않고 원본과 같은 경계 입력만 만든다.
    explicit SyncScene(const std::vector<std::string>& row) {
        const auto forms=Pairs(row[0]),products=Pairs(row[1]);const auto profile=Number(row[2]);temple=Number(row[3])!=0;full=Number(row[4]);change=Number(row[5]);queries=Inputs(row[6]);
        auto parent=InputRaw(pool,Sid{50});Put(parent,6,forms.empty() ? 0U : 100U,2);
        // 중복도 각각의 실제 raw Form과next를 공급한다.
        for (std::size_t i=0;i<forms.size();++i) { auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(100+i)});raw[10]=6;Put(raw,4,i+1<forms.size() ? static_cast<std::uint32_t>(101+i) : 0U,2);Put(raw,18,static_cast<std::uint32_t>(forms[i][0]),2);Put(raw,20,static_cast<std::uint32_t>(forms[i][1]),2); }
        // Native TYPE의 같은bit4만 바꾸며 다른 분기 결과는 공급하지 않는다.
        for (std::size_t kind=0;kind<types.size();++kind) types[kind].flags2=profile==1 || (!profile && kind==133) ? 4U : 0U;
        Put(deck,0x14c,static_cast<std::uint32_t>(products.size()));
        // Native 물리 순서와signed power를 보존한다.
        for (std::size_t i=0;i<products.size();++i) { deck[i*5]=static_cast<std::uint8_t>(products[i][0]);deck[i*5+2]=static_cast<std::uint8_t>(products[i][1]); }
        hooks.deck=[this] { Event("K");return std::span<const std::uint8_t>(deck); };
        hooks.technology=[this] { const auto parent=50U+(getters ? change : 0U);++getters;Event("T,"+std::to_string(parent));return parent; };
        hooks.temple=[this](auto current) { CHECK(current==owner);return temple; };
        hooks.cancelCursor=[this] { Event("X"); };hooks.destroy=[this](Sid sid,auto flags) { Event("D,"+std::to_string(sid.value)+","+std::to_string(flags)); };
        hooks.findKind=[this](Sid parent,auto mask,auto kind,auto index) { const auto value=queries[std::min<std::size_t>(query,queries.size()-1)];++query;Event("Q,"+std::to_string(parent.value)+","+std::to_string(mask)+","+std::to_string(kind)+","+std::to_string(index)+","+std::to_string(value));return value; };
        hooks.create=[this](Sid parent,auto kind,auto amount,auto flags,auto attach,auto quality) { Event("C,"+std::to_string(parent.value)+","+std::to_string(kind)+","+std::to_string(amount)+","+std::to_string(flags)+","+std::to_string(attach)+","+std::to_string(quality));return Sid{20000}; };
        hooks.assertion=[this](std::string_view text) { Event("A,"+TextBytes(text)); };
    }
    // 이벤트 사이의 구분자만 추가한다.
    void Event(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
};
}
// 모든 원본 행의 전체 호출 순서와 인자를 비교하며 기대값을 재생성하지 않는다.
TEST_CASE(ProductionSync_Native98) {
    std::size_t count{};
    // BYTE seen의255/256/257개 중복 입력도 같은 실제 검색기로 순회한다.
    for (const auto& row:LoadFixture(NETSTORM_PRODUCTION_SYNC_FIXTURE).rows) {
        CHECK(row.size()==8);SyncScene scene(row);RawProductionSync sync(scene.pool,scene.types,scene.finder,{scene.owner,scene.full},scene.hooks);const auto before=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());sync.Synchronize();
        if (scene.trace!=row[7]) std::printf("생산 동기화 원문%zu 불일치\n실제:%s\n원본:%s\n",count,scene.trace.c_str(),row[7].c_str());
        CHECK(scene.trace==row[7]);CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));++count;
    }
    CHECK(count==98);
}

// 실제 기술 부모 순회는 같은 Client DWORD를 매 동기화에서 읽고 Deck getter 효과 뒤 비영을 보존한다.
TEST_CASE(ProductionSync_SharedFullDeckReadsAfterDeckGetter) {
    netstorm::client::ClientGameplayGlobals globals;SyncScene scene({"71:8","-","2","0","0","0","0",""});std::uint32_t destroyed{},getterValue{0x80000000U};auto hooks=scene.hooks;const auto deck=hooks.deck;hooks.deck=[&] { globals.FullDeck()=getterValue;return deck(); };hooks.destroy=[&](Sid sid,auto flags) { CHECK(sid==Sid{100} && flags==0);++destroyed; };
    RawProductionSync sync(scene.pool,scene.types,scene.finder,{scene.owner,globals.FullDeck()},hooks);
    for (const std::uint32_t value:{0x80000000U,0xffffffffU,0U}) { // 부모는 같은 raw 내용물을 유지하므로 삭제 관문의 변화만 관측한다.
        getterValue=value;sync.Synchronize();CHECK(globals.FullDeck()==value && scene.full==0 && destroyed==(value ? 0U : 1U));
    }
}
