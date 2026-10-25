// 실제 양판본 부모12관측과 같은 Pool·기록·실패점 및 Client 대여 경계를 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientStateStorageCleanup.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawSquidPoolReset.h"
#include "o/RawAiCarrierActions.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { //창·장치를 만들지 않고 실제 보호 자료와 같은 물리 저장소를 사용한다.
using namespace netstorm::o;
using namespace netstorm::client;
//최초 TSV의 빈 칸과 열 순서를 유지한다.
std::vector<std::string> PoolResetFields(std::string_view line) {
    std::vector<std::string> result;std::size_t first{};
    for (;;) { const auto end=line.find('\t',first);result.emplace_back(line.substr(first,end==line.npos?line.npos:end-first));if (end==line.npos) return result;first=end+1; } //마지막 열까지 실제 구분자로 읽는다.
}
//Python zlib와 같은 전체 raw 바이트 Adler-32다.
std::uint32_t PoolResetAdler(std::span<const std::uint8_t> bytes) {
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;std::size_t block{}; //표준 소수와 중간 누적 폭이다.
    for (const auto byte:bytes) { a+=byte;b+=a;if (++block==4096) { a%=prime;b%=prime;block=0; } } //긴 Pool도 중간 넘침 없이 같은 순서로 누적한다.
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
//호스트 패딩을 읽지 않고 실제 SID/type 두 DWORD를 순서대로 직렬화한다.
std::vector<std::uint8_t> PoolResetRecords(const SidPool& pool,bool client) {
    std::vector<std::uint8_t> result;
    for (const auto& record:pool.Deletions(client)) for (const auto value:{record.sid,record.type}) for (std::size_t byte=0;byte<4;++byte) result.push_back(static_cast<std::uint8_t>(value>>(byte*8))); //각20쌍의 낮은BYTE부터 읽는다.
    return result;
}
//최초 Native 출력과 비교할 현재 바이트의 hex 표현이다.
std::string PoolResetHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result; //Native의 소문자16진 표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; //원시 순서와0을 생략하지 않는다.
}
//원본과 같은 실제 할당·반납으로 양쪽20개 삭제 기록을 채운다.
void PoolResetFillHistory(SidPool& pool) {
    for (const auto flags:{2U,0U}) { //client와 other의 실제 서로 다른 번호 영역을 사용한다.
        std::vector<Sid> allocated;
        for (std::uint32_t index=0;index<20;++index) { const auto sid=pool.Allocate(flags);auto raw=pool.AllocatedBytes(sid);for (std::size_t byte=0;byte<raw.size();++byte) raw[byte]=static_cast<std::uint8_t>(83+index+byte*37);raw[10]=static_cast<std::uint8_t>(83+index);raw[11]=4;allocated.push_back(sid); } //파생 생성자의 명시 입력이며 출력 계산이 아니다.
        for (const auto sid:allocated) pool.Release(sid); //반납 순서로 실제20쌍 이동과 FIFO를 실행한다.
    }
}
//실제 하위 Reset의 기존 주요 기록과 부모의 삭제 기록을 같은 열로 관측한다.
std::array<std::uint32_t,9> PoolResetSnapshot(const SidPool& pool) {
    auto logs=PoolResetRecords(pool,true);const auto other=PoolResetRecords(pool,false);logs.insert(logs.end(),other.begin(),other.end());
    return {0,pool.FreeCount(),pool.PredictableCursor(),pool.FirstFree(true).value,pool.Tail(true).value,pool.FirstFree(false).value,pool.Tail(false).value,PoolResetAdler(pool.Bytes()),PoolResetAdler(logs)};
}
//실패한 물리/대여 입력의 실제 예외만 관측한다.
template<class Function> bool PoolResetThrows(Function&& function) { try { function();return false; }catch (const std::exception&) { return true; } }
//같은 보호 경로의 Client를 장치 없이 만든다.
ClientOptions PoolResetOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; }
}

//원문 부모·하위 Pool·80DWORD·Y/X 모두 실제 CPU로 먼저 고정한12사례를 재생한다.
TEST_CASE(SquidPoolReset_ReplaysBothIndependentNativeParents12) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/squid-pool-reset-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{};
    while (std::getline(file,line)) { //각 판본의0/음수 인자와 두 x87 정밀도를 전부 읽는다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=PoolResetFields(line);CHECK(row.size()==21);if (row.size()!=21) continue;
        SidPool pool(row[0]=="originals"?OriginalEdition::Patch1078:OriginalEdition::Cd1072,32768,true);PoolResetFillHistory(pool);CHECK(PoolResetHex(PoolResetRecords(pool,true))==row[3] && PoolResetHex(PoolResetRecords(pool,false))==row[4]);RawPathPoint invalid{std::bit_cast<float>(0x3f123456U),std::bit_cast<float>(0xc0123456U)};
        const auto argument=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(std::stoull(row[1])));const auto* address=pool.Bytes().data();ResetSquidWorldPool(pool,invalid,argument);const auto snapshot=PoolResetSnapshot(pool);
        for (std::size_t column=0;column<snapshot.size();++column) CHECK(snapshot[column]==std::stoull(row[column+5])); //전체 raw Pool·카운터·목록·삭제 기록은 Native 기대에서만 읽는다.
        CHECK(pool.Bytes().data()==address && PoolResetHex(PoolResetRecords(pool,true))==row[14] && PoolResetHex(PoolResetRecords(pool,false))==row[15] && std::bit_cast<std::uint32_t>(invalid.x)==std::stoull(row[16]) && std::bit_cast<std::uint32_t>(invalid.y)==std::stoull(row[17]));CHECK(row[18]=="wrapper,pool,history" && std::count(row[19].begin(),row[19].end(),';')==81 && std::stoul(row[20])>0); //82store는 Native CPU 감사이며 C++ store 계측으로 가장하지 않는다.
        ++rows;
    }
    CHECK(rows==12);
}

//기존 하위 Reset의 기록 보존과 새 부모의 기록 삭제·재확보를 구별한다.
TEST_CASE(SquidPoolReset_LowerPreservesHistoryParentClearsAndReallocates) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { //두 판본의 실제 경계와 같은 API 계약이다.
        SidPool pool(edition,32768,true);PoolResetFillHistory(pool);const auto client=PoolResetRecords(pool,true),other=PoolResetRecords(pool,false);pool.Reset();CHECK(PoolResetRecords(pool,true)==client && PoolResetRecords(pool,false)==other);
        RawPathPoint invalid{11.25F,-7.5F};ResetSquidWorldPool(pool,invalid,40000);CHECK(pool.Capacity()==40000 && pool.FreeCount()==39995 && pool.PredictableCursor()==1 && invalid.x==-10000 && invalid.y==-10000 && std::all_of(pool.Deletions(true).begin(),pool.Deletions(true).end(),[](const auto& r) { return r.sid==0 && r.type==0; }) && std::all_of(pool.Deletions(false).begin(),pool.Deletions(false).end(),[](const auto& r) { return r.sid==0 && r.type==0; }));
    }
}

//하위 실패 뒤 꼬리를 앞당기지 않고, 기록 정리만 실행할 때는 다른 Pool 필드를 보존한다.
TEST_CASE(SquidPoolReset_FailureAndHistoryOnlyKeepOtherState) {
    SidPool pool(OriginalEdition::Patch1078,32768,true);PoolResetFillHistory(pool);RawPathPoint invalid{11.25F,-7.5F};const auto before=PoolResetSnapshot(pool);const auto* address=pool.Bytes().data();CHECK(PoolResetThrows([&] { ResetSquidWorldPool(pool,invalid,5); }) && PoolResetSnapshot(pool)==before && pool.Bytes().data()==address && invalid.x==11.25F && invalid.y==-7.5F);
    pool.ClearDeletionHistory();auto after=PoolResetSnapshot(pool);CHECK(std::equal(before.begin(),before.begin()+8,after.begin()) && invalid.x==11.25F && invalid.y==-7.5F && pool.Bytes().data()==address); //기록80DWORD 외의 현재 Pool·좌표는 그대로다.
}

//같은 물리의 대여 중/다른 Client 권한에서는 첫 Native store 전에 거부한다.
TEST_CASE(SquidPoolReset_ClientRejectsBorrowedAndForeignBeforeMutation) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client(PoolResetOptions());auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());PoolResetFillHistory(storage->pool);storage->boardTargetState.invalid={13.25F,-9.75F};storage->GameAux()->LostX()=42.5F;storage->GameAux()->LostY()=-71.25F;const auto before=PoolResetSnapshot(storage->pool);
    { auto use=storage->AcquirePrepared();CHECK(PoolResetThrows([&] { ResetClientSquidPool(client,*storage); }) && PoolResetSnapshot(storage->pool)==before && storage->boardTargetState.invalid.x==13.25F); } //실제 World 물리 대여를 반납한 뒤에만 계속한다.
    auto foreign=MakeRawSurfaceStorage(assets.game);CHECK(PoolResetThrows([&] { ResetClientSquidPool(client,*foreign); }) && PoolResetSnapshot(storage->pool)==before);ResetClientSquidPool(client,*storage);CHECK(storage->boardTargetState.invalid.x==-10000 && storage->boardTargetState.invalid.y==-10000 && storage->GameAux()->LostX()==42.5F && storage->GameAux()->LostY()==-71.25F && storage->pool.FreeCount()==32763);
}

//기존 Board/AI getter가 진단 callback 뒤 같은 장기 실패점을 다시 읽는다.
TEST_CASE(SquidPoolReset_SharedBoardAndDaisPointRequeryAfterAssertion) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);auto& state=storage->boardTargetState;const auto* address=&state.invalid;std::uint32_t assertions{};RawBoardTarget board(state,[&](auto line) { CHECK(line==0x262);++assertions;state.invalid={13.25F,-9.75F}; });const auto first=board.Nearest({0,0,4,4},{7,3},0);CHECK(first.x==13.25F && first.y==-9.75F && assertions==1);
    const std::uint32_t checking=1;const auto point=DaisApproachPoint(storage->pool,storage->types,checking,state.invalid,[&](auto) { ++assertions;state.invalid={31.5F,-17.25F}; },Sid{});CHECK(point.x==31.5F && point.y==-17.25F && assertions==2);
    { auto use=storage->AcquirePrepared();CHECK(&storage->boardTargetState.invalid==address && state.invalid.x==31.5F); }CHECK(state.invalid.x==31.5F);ResetSquidWorldPool(storage->pool,state.invalid);const auto after=board.Nearest({0,0,4,4},{7,3},0);CHECK(after.x==13.25F && after.y==-9.75F && &state.invalid==address); //새 Runtime/Prepare 없이 진단에서 바꾼 현재값을 다시 읽는다.
}
