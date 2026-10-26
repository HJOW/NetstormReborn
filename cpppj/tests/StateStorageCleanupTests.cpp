// 먼저 고정한 실제 PE 출력으로 마커·Zone의 폭/순서와 실제 Client 저장소 수명을 대조한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientStateStorageCleanup.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawStateZoneReset.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 원본 파일은 읽기만 하며 창·OS 음향·실제 원본 게임을 실행하지 않는다.
using namespace netstorm::o;
using namespace netstorm::client;
// 최초 TSV의 입력/관측 칼럼을 구분하며 기대를 다시 계산하지 않는다.
std::vector<std::string> StorageFields(std::string_view row) {
    std::vector<std::string> result;std::size_t first{};
    for (;;) { const auto end=row.find('\t',first);result.emplace_back(row.substr(first,end==row.npos?row.npos:end-first));if (end==row.npos) return result;first=end+1; } //마지막 칼럼까지 실제 탭 경계로 읽는다.
}
// 실제 버퍼를 같은 little endian DWORD 입력으로 바꾼다.
void StoragePut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //원문 필드 이외는 쓰지 않는다.
// CPU 출력과 비교할 실제 전체 바이트를 순서대로 직렬화한다.
std::string StorageHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2); //최초 관측의 소문자 hex 표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; //빈 칸·주소 정규화로 실제 바이트를 버리지 않는다.
}
// 명시한 가상 삭제 장치의 중단을 원문 정상 반환과 구별한다.
struct StorageStop {};
// 짧은 물리·잘못된 선택의 실제 호스트 예외만 관측한다.
template<class F> bool StorageThrows(F&& function) { try { function();return false; }catch (const std::exception&) { return true; } }
// 비활성 실제 Client를 구성하며 화면·장치를 만들지 않는다.
ClientOptions StorageOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; }
}

// signed 첫 관문·DWORD SID 산술·가상 복귀 뒤0·명시중단을 먼저 고정한86입력과 대조한다.
TEST_CASE(StateStorageCleanup_MetaMarkerIndependentNative86) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/meta-zone-marker-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{},stopped{};
    while (std::getline(file,line)) { //두 x87 환경의 최초 관측을 빠짐없이 읽는다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=StorageFields(line);CHECK(row.size()==11);if (row.size()!=11) continue;
        const auto seed=std::stoul(row[0]),index=std::stoul(row[1]);const auto status=static_cast<std::uint32_t>(std::stoull(row[2])),marker=static_cast<std::uint32_t>(std::stoull(row[3]));std::array<std::uint8_t,kMetaZoneCleanupBytes+16> bytes{};
        for (std::size_t byte=0;byte<bytes.size();++byte) bytes[byte]=static_cast<std::uint8_t>(seed+byte*17); //기대와 독립인 Native 초기 패턴이다.
        const auto offset=index*kMetaZoneCleanupStride;StoragePut(bytes,offset,status);StoragePut(bytes,offset+76,marker);std::string outcome="normal",calls;
        try { ClearMetaZoneMarker(bytes,static_cast<std::uint32_t>(index),{[&](auto sid,auto flags) {
            const auto address=std::uint32_t(0x12000000U+sid*50U);calls=std::to_string(address)+":"+std::to_string(flags); //실제 가상 this 주소의32비트 Host 경계이며 본문 선택/출력은 Native fixture에서만 읽는다.
            if (row[4]=="stop") throw StorageStop{};if (row[4]=="sid") StoragePut(bytes,offset+76,0x76543210);if (row[4]=="status") StoragePut(bytes,offset,0x89abcdef);
        }}); }catch (const StorageStop&) { outcome="stop";++stopped; }
        CHECK(outcome==row[6] && StorageHex(bytes)==row[7] && (calls.empty()?"-":calls)==row[8]);CHECK(std::stoul(row[10])>0);++rows;
    }
    CHECK(rows==86 && stopped==2);
}

// 상태0/음수와 SID0은 삭제를 요구하지 않고 선택된 실패에서 원래 필드가 남는다.
TEST_CASE(StateStorageCleanup_MetaMarkerDemandAndShortPhysicalKeepPrefix) {
    std::array<std::uint8_t,100> bytes{};StoragePut(bytes,0,0xffffffffU);StoragePut(bytes,76,0x10000U);ClearMetaZoneMarker(std::span(bytes).first(4),0,{});CHECK(bytes[78]==1); //생략되는+4c를 미리 조회하면 실패한다.
    StoragePut(bytes,0,1);StoragePut(bytes,76,0);ClearMetaZoneMarker(bytes,0,{});StoragePut(bytes,76,0x10000U);const auto before=bytes;CHECK(StorageThrows([&] { ClearMetaZoneMarker(bytes,0,{}); }) && bytes==before);
    CHECK(StorageThrows([&] { ClearMetaZoneMarker(std::span(bytes).first(79),0,{}); }) && bytes==before);CHECK(StorageThrows([&] { ClearMetaZoneMarker(bytes,51,{}); }) && bytes==before);
    std::uint32_t observed{};ClearMetaZoneMarker(bytes,0,{[&](auto sid,auto flags) { observed=sid;CHECK(flags==0);StoragePut(bytes,76,7); }});CHECK(observed==0x10000U && bytes[76]==0 && bytes[78]==0 && bytes[80]==before[80]);
}

// 원문9BYTE가 남기는 전체24BYTE와 두 store의 폭·순서는 최초 PE10관측에서만 읽는다.
TEST_CASE(StateStorageCleanup_ZoneIndependentNative10AndExactSixBytes) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/state-zone-reset-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{};
    while (std::getline(file,line)) { //모든 초기 패턴과 두 정밀도를 실제로 재생한다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=StorageFields(line);CHECK(row.size()==11);if (row.size()!=11) continue;std::array<std::uint8_t,24> bytes{};const auto seed=std::stoul(row[0]);
        for (std::size_t byte=0;byte<bytes.size();++byte) bytes[byte]=static_cast<std::uint8_t>(seed+byte*17); //CPU 실행 전과 같은 명시 입력이다.
        ResetStateZone(bytes);CHECK(row[6]=="normal" && StorageHex(bytes)==row[7] && row[8]=="-" && row[9]=="0:4:0;4:2:0");++rows;
    }
    CHECK(rows==10);
    for (std::size_t length=0;length<=8;++length) { //선행 전체 폭 검사나8BYTE0이 첫 DWORD prefix/인접WORD를 잃는지 구별한다.
        std::array<std::uint8_t,10> bytes;bytes.fill(0xa5);const bool rejected=StorageThrows([&] { ResetStateZone(std::span(bytes).first(length)); });CHECK(rejected==(length<6));const auto changed=length<4?0U:length<6?4U:6U;
        for (std::size_t byte=0;byte<bytes.size();++byte) CHECK(bytes[byte]==(byte<changed?0:0xa5)); //완료한 store와 나머지 canary를 별도로 검증한다.
    }
}

// 실제 세 DWORD와 Zone을 같은 owner에서만 지우며 다섯 State 블록·Meta 표·인접WORD는 보존한다.
TEST_CASE(StateStorageCleanup_ClientClockAndZoneShareActualFields) {
    ClientLocalStateGlobals owner;auto clock=owner.ClockSynchronization();CHECK(owner.SynchronizationConnection()==0 && owner.SynchronizationReplyMode()==0 && owner.GameTicks()==0 && owner.MetaFirstReset()==1);CHECK(&clock.gameTicks==&owner.GameTicks());
    clock.connection=0x80000000U;clock.replyMode=0xffffffffU;owner.GameTicks()=0x12345678;owner.ZoneWords()={0x76543210,0xabcd9876};owner.Blocks()[2][0]=0x89abcdef;auto meta=owner.MetaZones();meta.records[3]=0x71;
    ResetClientClockSynchronization(owner);CHECK(clock.connection==0 && clock.replyMode==0 && owner.GameTicks()==0 && owner.ZoneWords()[0]==0x76543210 && owner.Blocks()[2][0]==0x89abcdef && meta.records[3]==0x71 && meta.firstReset==1);
    ResetClientStateZone(owner);CHECK(owner.ZoneWords()[0]==0 && owner.ZoneWords()[1]==0xabcd0000 && owner.Blocks()[2][0]==0x89abcdef && meta.records[3]==0x71 && meta.firstReset==1);
}

// 첫 reset1은 표0/flag0을 수행하고 이후51개 빈 기록은 Runtime 없이 원문 관문으로 생략한다.
TEST_CASE(StateStorageCleanup_ActualClientMetaFirstAndRepeatedEmptyPath) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client(StorageOptions());auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());auto meta=client.LocalStateGlobals()->MetaZones();std::fill(meta.records.begin(),meta.records.end(),std::uint8_t{0xa5});ClearClientMetaZoneRecords(client,*storage);CHECK(meta.firstReset==0 && std::all_of(meta.records.begin(),meta.records.end(),[](auto b) { return b==0; }));ClearClientMetaZoneRecords(client,*storage);
    StoragePut(meta.records,2*kMetaZoneCleanupStride,1);StoragePut(meta.records,2*kMetaZoneCleanupStride+76,5);const auto before=std::vector<std::uint8_t>(meta.records.begin(),meta.records.end());CHECK(StorageThrows([&] { ClearClientMetaZoneRecords(client,*storage); }) && std::equal(before.begin(),before.end(),meta.records.begin()) && meta.firstReset==0); //선택된 실제 삭제를 빈 성공으로 바꾸지 않는다.
    auto foreign=MakeRawSurfaceStorage(assets.game);CHECK(StorageThrows([&] { ClearClientMetaZoneRecords(client,*foreign); }) && std::equal(before.begin(),before.end(),meta.records.begin()));
}

// 먼저 고정한 전체 버퍼의 Adler32이며 C++ 정리 결과로 기대값을 계산하지 않는다.
namespace { // 같은 검사 파일 안에서만 독립 CPU 관측의 버퍼 체크섬을 읽는다.
std::uint32_t FlatResetAdler(std::span<const std::uint8_t> bytes) {
    std::uint32_t first=1,second=0; //zlib과 같은 두 누적값이며 주소·호스트 정렬과 무관하다.
    for (const auto byte:bytes) { first=(first+byte)%65521U;second=(second+first)%65521U; } //입력 순서의 모든 바이트를 한 번씩 포함한다.
    return (second<<16)|first;
}
}

// 실제 PE16관측의 전체 체크섬과 인접32BYTE로 두 작은 몸체의 정확한 쓰기 범위를 확인한다.
TEST_CASE(StateFlatReset_Native16AndAdjacentBytes) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/state-flat-reset-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{};
    while (std::getline(file,line)) { //두 x87 상태와 네 비영 패턴의 최초 관측을 빠짐없이 재생한다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=StorageFields(line);CHECK(row.size()==9);if (row.size()!=9) continue;
        const auto length=static_cast<std::size_t>(std::stoul(row[3]));const auto seed=std::stoul(row[1]);std::vector<std::uint32_t> words((length+32)/4); //DWORD 객체를 먼저 만들고 그 표현 바이트만 준비한다.
        auto bytes=std::span(reinterpret_cast<std::uint8_t*>(words.data()),words.size()*4);
        for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(seed+index*17); //Native의 명시 초기 패턴이며 기대 출력은 fixture에서 읽는다.
        CHECK(FlatResetAdler(bytes)==std::stoul(row[4]));
        if (row[0]=="warnings") ResetStatePlayerNoMakeWarnings(std::span(words).subspan(4));
        else { CHECK(row[0]=="spots");ResetStateSpotMap(bytes.subspan(16)); }
        CHECK(FlatResetAdler(bytes)==std::stoul(row[5]));CHECK(StorageHex(bytes.first(16))+StorageHex(bytes.last(16))==row[6]);CHECK(std::stoul(row[7])==length/4 && std::stoul(row[8])>0);++rows;
    }
    CHECK(rows==16);
}

// 지원한 호스트 물리 크기 밖에서는 기존 버퍼를 보존하고 다른 reset까지 실행하지 않는다.
TEST_CASE(StateFlatReset_ShortPhysicalRejectsWithoutWriting) {
    std::array<std::uint32_t,256> warnings;warnings.fill(0xabcdef01U);const auto before=warnings;
    CHECK(StorageThrows([&] { ResetStatePlayerNoMakeWarnings(std::span(warnings).first(254)); }) && warnings==before);
    std::vector<std::uint8_t> spots(65537,0xb3);const auto saved=spots;
    CHECK(StorageThrows([&] { ResetStateSpotMap(std::span(spots).first(65535)); }) && spots==saved);
    ResetStatePlayerNoMakeWarnings(warnings);CHECK(std::all_of(warnings.begin(),warnings.begin()+255,[](auto value) { return value==0; }) && warnings[255]==before[255]);
    ResetStateSpotMap(spots);CHECK(std::all_of(spots.begin(),spots.begin()+65536,[](auto value) { return value==0; }) && spots.back()==0xb3);
}

// 실제 Client owner와 World 대여 반납을 요구하며 같은 경고 표를 SP 추적·Session 준비와 독립으로 유지한다.
TEST_CASE(StateStorageCleanup_PhysicalEffectOwnersAndBorrowedGuard) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client(StorageOptions());auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
    auto owner=client.LocalStateGlobals();auto& warnings=owner->PlayerNoMakeWarnings();CHECK(std::all_of(warnings.begin(),warnings.end(),[](auto value) { return value==0; }));warnings.fill(0x80000001U);const auto* address=warnings.data();owner->GameTicks()=123;owner->SourceType()=456;
    { auto use=storage->AcquirePrepared();CHECK(StorageThrows([&] { ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear4328a0); }) && warnings.front()==0x80000001U && warnings.back()==0x80000001U); }
    { auto use=client.GameplayGlobals()->AcquirePrepared();CHECK(warnings.data()==address && warnings.front()==0x80000001U); } //Gameplay 준비도 다른 실제 전역을 임의 초기화하지 않는다.
    auto foreign=MakeRawSurfaceStorage(assets.game);CHECK(StorageThrows([&] { ResetClientStateWorldStorage(client,*foreign,StateWorldCleanupEffect::Clear4328a0); }) && warnings.front()==0x80000001U);
    CHECK(StorageThrows([&] { ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::ReplaceUber); }) && warnings.front()==0x80000001U);
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear4328a0);CHECK(warnings.data()==address && std::all_of(warnings.begin(),warnings.end(),[](auto value) { return value==0; }) && owner->GameTicks()==123 && owner->SourceType()==456);
}

// 일곱 Effect는 같은 장기 물리의 선택한 표만 변경하고 Pool·다른 표의 보존 영역을 유지한다.
TEST_CASE(StateStorageCleanup_SevenPhysicalEffectsShareCurrentStorage) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client(StorageOptions());auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
    const auto poolBefore=std::vector<std::uint8_t>(storage->pool.Bytes().begin(),storage->pool.Bytes().end());auto& counts=storage->totalMade;counts.localCounts.fill(0x12345678U);counts.localSecondaryCounts.fill(0xabcdef01U);counts.globalCounts.fill(0x87654321U);storage->spots.assign(65536,0xb3);storage->hash.Select(2);
    for (int level=0;level<4;++level) storage->hash.Cell(level,0,0)=77; //각 해시 단계에 실제 비영 머리를 준비한다.
    storage->graph.records[0].reserved=std::bit_cast<std::int16_t>(std::uint16_t{0xabcd});storage->graph.records[252]={17,18,19};storage->graph.stack[5]=0xabcdef01U;storage->islands.records.fill(0xb3);storage->chunks.Mode()=0x80000000U;storage->chunks.At(0,0).ordinal=55;
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear4c2450);CHECK(counts.localCounts[0]==0 && counts.localSecondaryCounts[254]==0 && counts.globalCounts[254]==0 && counts.localCounts[255]==0x12345678U && counts.localSecondaryCounts[255]==0xabcdef01U && counts.globalCounts[255]==0x87654321U && storage->spots[0]==0xb3);
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear4b2b50);CHECK(storage->hash.Selected()==2);
    for (int level=0;level<4;++level) CHECK(storage->hash.Cell(level,0,0)==0); //표0의 정리로 다른 높이 단계가 남지 않아야 한다.
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear4ab940);CHECK(std::all_of(storage->spots.begin(),storage->spots.end(),[](auto value) { return value==0; }));
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Reset568b60);CHECK(storage->islands.records[0]==0 && storage->islands.records[1]==0 && storage->islands.records[18]==0xb3 && storage->islands.records[19]==0xb3);
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Reset54c8a0);CHECK(storage->chunks.Mode()==0x80000000U && storage->chunks.At(0,0).island==0x7f && storage->chunks.At(0,0).ordinal==0);
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::ResetGraph);CHECK(storage->graph.records[0].inUse==1 && static_cast<std::uint16_t>(storage->graph.records[0].reserved)==0xab01U && storage->graph.records[252].surfaces==17 && storage->graph.records[252].inUse==18 && storage->graph.records[252].reserved==19 && storage->graph.stack[5]==0xabcdef01U);
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear4328a0);CHECK(std::equal(poolBefore.begin(),poolBefore.end(),storage->pool.Bytes().begin()));
}
