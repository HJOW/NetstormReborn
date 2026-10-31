// 게임 실행 없이 최초 PE 출력과 실제 SP 정책/Client 장부의 연결을 대조한다.
#include "RawSceneSupport.h"
#include "client/ClientCoreDataGlobals.h"
#include "o/RawForeignRitualReward.h"
#include "o/RawFortMoneyLoad.h"
#include <cmath>

// 원시 입력 파싱과 독립 Native 기대값 비교만 공유한다.
namespace {
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// 전체 장부의 미작성 이웃 BYTE까지 비교하는 hex다.
std::string ForeignHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 원시 BYTE 순서를 유지하는 출력이다.
    for (auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // NUL/padding도 생략하지 않는다.
    return result;
}
// 실제 저장 callback이 없는 Core helper에서는 메모리 전체를 비교하고 TYPE/assert 효과를 따로 비교한다.
std::string ForeignEffects(const std::vector<std::string>& effects) {
    std::string result; // Native와 같은 세미콜론 구분 출력이다.
    for (const auto& effect:effects) { if (!result.empty()) result+=';';result+=effect; } // 원문 효과 순서를 보존한다.
    return result;
}
}

// 정상/비정상 비트·마스크 shift·owner32비트 감김·별도 장부 이웃을 독립 PE144관측과 대조한다.
TEST_CASE(ForeignRitualReward_CoreReplaysIndependentNative144) {
    const auto rows=LoadFixture(NETSTORM_FOREIGN_RITUAL_FIXTURE).rows;std::size_t count{}; // 최초 자료의 기술 모드만 대조한다.
    for (const auto& row:rows) { // 양 x87 모드의 Core 결과는 동일하게 검사한다.
        const auto input=Split(row.at(0),',');if (Number(input.at(1))!=0) continue;
        const auto owner=Number(input.at(2)),seed=Number(input.at(8));const auto bit=static_cast<std::int32_t>(std::stol(input.at(7)));
        std::array<std::uint8_t,256> records{};std::vector<std::string> effects; // 장부 앞뒤와 padding까지 독립 Native와 같은 입력이다.
        for (std::size_t byte=0;byte<records.size();++byte) records[byte]=static_cast<std::uint8_t>(seed+byte*17); // 출력 기대값을 계산하지 않는다.
        Put(records,0x24,Number(input.at(4)));Put(records,0x2c,0x12200000U); // Native의 인접 known DWORD와 명시 Player 가상 주소 입력도 같은 창에 준비한다.
        GrantForeignRitualType({records,64},owner,85,[&](auto type) { effects.push_back("T:"+std::to_string(type));return bit; },
            [&](auto line) { effects.push_back("A:"+std::to_string(line)); });
        std::vector<std::string> nativeEffects; // DWORD 실제 쓰기는 전체256BYTE 비교가 검증한다.
        for (const auto& effect:Split(row.at(2),';')) if (effect.starts_with("T:") || effect.starts_with("A:")) nativeEffects.push_back(effect); // 진단 복귀도 포함한다.
        CHECK(ForeignHex(records)==row.at(1));CHECK(ForeignEffects(effects)==ForeignEffects(nativeEffects));++count;
    }
    CHECK(count==144);
}

// 실제 분산 SP getter/setter를 통해 signed32/64·음수·분수·NaN 및 Editor 지정 정책을 대조한다.
TEST_CASE(ForeignRitualReward_SpReplaysNative53Bit180WithActualStoreAndPolicy) {
    const auto rows=LoadFixture(NETSTORM_FOREIGN_RITUAL_FIXTURE).rows;std::size_t count{},extendedAudit{}; // 확장 정밀도는 원시 감사로 구별한다.
    for (const auto& row:rows) { // 원본 기본 0x27f 관측만 현재 제품의 계산과 대조한다.
        const auto input=Split(row.at(0),',');if (Number(input.at(1))!=1) continue;
        if (Number(input.at(0))!=0x27f) { ++extendedAudit;continue; }
        const auto owner=Number(input.at(2)),local=Number(input.at(3)),known=Number(input.at(4)),editor=Number(input.at(5)),initial=Number(input.at(6));
        GameRandom random;ScrambledSpStore store(random,[] { return 7U; });store.Initialize(); // 같은 게임 SP 저장소와 난수를 사용한다.
        for (std::uint32_t player=0;player<9;++player) store.Set(player,initial); // Native 경계가 돌려주는 원시 float 입력을 모든 owner에 준비한다.
        const PlayerSpSetReads reads{[&] { return known; },[&] { return local; },[&] { return editor; }}; // 실제 정책 helper의 현재 전역 입력이다.
        std::uint32_t gets{},sets{},refresh{};std::vector<std::string> actual; // 부모 호출 수와 실제 로컬 표시만 관측한다.
        GrantForeignRitualSp(owner,[&](auto selected) { ++gets;CHECK(selected==owner);return ReadPlayerSpAmount(selected,store,reads); },
            [&](auto selected,auto amount) { ++sets;CHECK(std::bit_cast<std::uint32_t>(selected)==owner);SetPlayerSpAmount(selected,amount,store,reads,[&] { ++refresh;actual.push_back("U"); }); });
        auto expected=initial;std::uint32_t expectedRefresh{};bool written{}; // 저장 기대값은 Native S 이벤트에서만 읽는다.
        for (const auto& event:Split(row.at(2),';')) { // 원문 getter 호출과 저장/표시를 파싱한다.
            if (event.starts_with("S:")) { const auto fields=Split(event,':');CHECK(Number(fields.at(1))==owner);expected=Number(fields.at(2));written=true; }
            else if (event=="U") ++expectedRefresh;
        }
        CHECK(gets==1 && sets==1 && refresh==expectedRefresh);CHECK(written==(owner>0 && owner<9));
        for (std::uint32_t player=0;player<9;++player) CHECK(store.Get(player)==(player==owner?expected:initial)); // 이웃 지갑은 바뀌지 않아야 한다.
        ++count;
    }
    CHECK(count==180 && extendedAudit==180);
}

// 비활성/비로컬0은 lazy 저장소를 읽지 않으며 활성 로컬은 비교 뒤 현재 owner를 다시 조회한다.
TEST_CASE(ForeignRitualReward_GetterPreservesLazyAndCurrentOwnerReads) {
    GameRandom random;ScrambledSpStore store(random,[] { return 7U; });std::uint32_t localReads{}; // 같은 저장소의 최초 초기화 여부를 관측한다.
    CHECK(ReadPlayerSpAmount(0,store,{[] { return 0U; },{}, {}})==0.0F && !store.Initialized());
    CHECK(ReadPlayerSpAmount(0xffffffffU,store,{[] { return 1U; },[] { return 2U; },{}})==0.0F && !store.Initialized());
    store.Set(8,std::bit_cast<std::uint32_t>(123.75F));
    const auto balance=ReadPlayerSpAmount(0,store,{[] { return 1U; },[&] { return localReads++?8U:0U; },{}});
    CHECK(balance==123.75F && localReads==2);
}

// 진단 뒤 현재 DWORD를 쓰며 이웃 필드를 보존하고 Client의 기존 장부를 그대로 사용한다.
TEST_CASE(ForeignRitualReward_CoreDiagnosticAndClientLedgerAreCurrent) {
    netstorm::client::ClientCoreDataGlobals globals;const auto* physical=globals.Records().data();std::uint32_t diagnoses{}; // Session 교체와 별도인 Client 수명의 실제 장부다.
    GrantForeignRitualType({globals.Records(),0},8,85,[](auto) { return 32; },[&](auto line) { CHECK(line==0x15a);++diagnoses;Put(globals.Records(),8*16+10,0x80000000U); });
    CHECK(diagnoses==1 && Get(globals.Records(),8*16+10)==0x80000001U && globals.Records().data()==physical);
    CHECK(Get(globals.Records(),8*16)==0 && globals.Records()[8*16+9]==0 && globals.Records()[8*16+14]==0);
    GrantForeignRitualType({{},0},0xffffffffU,85,[](auto) { return -1; },{}); // 비트 없는 TYPE은 실제 장부나 진단을 요구하지 않는다.
    std::array<std::uint8_t,13> shortCore{};bool rejected{}; // 단일 DWORD가 물리 끝을 넘으면 부분 BYTE store를 남기지 않는다.
    try { GrantForeignRitualType({shortCore,0},0,85,[](auto) { return 0; },{}); }catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && ForeignHex(shortCore)==std::string(26,'0'));
}
