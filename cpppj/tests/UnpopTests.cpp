// 창 없이 raw 공간 해제·수신·반납과 공통 firstPop을 실제 원본 기대값과 대조한다.
#include "TestSupport.h"
#include "o/SquidFactory.h"
#include "o/SquidUnpop.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// fixture의 각 입력/기대값 열을 원문 순서대로 분리한다.
std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> result; std::istringstream input(line); std::string field;
    // TSV 열 사이의 구분자만 제거한다.
    while (std::getline(input,field,'\t')) result.push_back(field);
    return result;
}
// Python zlib와 같은 Adler-32. raw 슬롯은 별도로 모든 바이트를 비교한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // Adler 소수와 블록 크기다. 긴 원본 풀의 중간 누적 넘침을 피한다.
    constexpr std::uint64_t prime=65521; std::uint64_t a=1,b=0; std::size_t block=0;
    // 입력 순서대로 누적하고 4096바이트마다 축소한다.
    for (auto byte:bytes) { a+=byte; b+=a; if (++block==4096) { a%=prime; b%=prime; block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// raw 정수/float 비트를 원본 little endian 폭으로 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 폭 밖의 CD 이웃 필드는 보존한다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 삭제 기록의 두 영역과 공간 해시의 네 영역을 padding 없이 직렬화한다.
std::vector<std::uint8_t> ExtraBytes(const SidPool* pool,const SquidHash* hash) {
    std::vector<std::uint8_t> bytes;
    if (pool) {
        // 원본 client/server 순서로 각각 20개 기록을 읽는다.
        for (bool client:{true,false}) {
            // 레코드마다 SID와 type의 32비트 값을 낮은 바이트부터 놓는다.
            for (const auto& record:pool->Deletions(client)) {
                // 구조체의 호스트 패딩을 결과에 포함하지 않는다.
                for (auto value:{record.sid,record.type}) {
                    // 각 원본 DWORD의 네 바이트를 직접 기록한다.
                    for (int shift=0;shift<32;shift+=8) bytes.push_back(static_cast<std::uint8_t>(value>>shift));
                }
            }
        }
    }
    if (hash) {
        // 가장 큰 0단계부터 3단계까지 모든 ushort 머리를 비교한다.
        for (int level=0;level<4;++level) {
            // 각 머리의 두 바이트를 원본 배열 순서로 놓는다.
            for (auto value:hash->Entries(level)) { bytes.push_back(static_cast<std::uint8_t>(value)); bytes.push_back(static_cast<std::uint8_t>(value>>8)); }
        }
    }
    return bytes;
}
// 판본별 실제 주소 표와 유효한 최소 발자국을 준비한다.
std::vector<RiftTypeRecord> Types(OriginalEdition edition) {
    std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    // 고정 주소 표는 독립 fixture에서 별도로 검증한다.
    for (std::size_t i=0;i<types.size();++i) { types[i].constructorAddress=TypeConstructorAddress(edition,i); types[i].footX=types[i].footY=1; }
    return types;
}
// 보호 경로의 예외만 관찰하여 변경 전후 상태를 별도로 검사한다.
template<class F> bool Throws(F&& operation) {
    try { operation(); } catch (const std::exception&) { return true; } return false;
}
}

// 대상 슬롯은 바이트별 대조, 풀/로그/전체 네 해시/spot은 독립 체크섬과 대조한다.
TEST_CASE(SquidUnpop_X86Fixture_ReplaysRawUnpopTakeReleaseAndFirstPop) {
    std::ifstream input(NETSTORM_UNPOP_FIXTURE); CHECK(static_cast<bool>(input));
    std::unique_ptr<SidPool> pool; std::unique_ptr<SquidHash> hash; std::unique_ptr<SquidUnpop> unpop;
    std::unique_ptr<SquidFactory> factory; std::vector<RiftTypeRecord> types; std::vector<std::uint8_t> spots;
    std::string line; int begins=0,steps=0;
    // Type/Fill/Head/Space는 실제 Pop을 대신하는 명시적 합성 입력이다.
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;
        const auto row=Fields(line);
        if (row[0]=="Begin") {
            CHECK(row.size()==5); factory.reset(); unpop.reset();
            const auto edition=row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
            pool=std::make_unique<SidPool>(edition,static_cast<std::uint32_t>(std::stoul(row[2])),row[3]=="1");
            hash=std::make_unique<SquidHash>(); spots.assign(kWorldCells*kWorldCells,0); types=Types(edition);
            unpop=std::make_unique<SquidUnpop>(*pool,*hash,spots); factory=std::make_unique<SquidFactory>(*pool,types,false,unpop.get()); ++begins;
        } else if (row[0]=="Type") {
            CHECK(row.size()==9); auto& type=types.at(std::stoul(row[1]));
            type.flags1=static_cast<std::uint32_t>(std::stoul(row[2])); type.flags2=static_cast<std::uint32_t>(std::stoul(row[3]));
            type.maxHitPoints=static_cast<std::int32_t>(std::stol(row[4])); type.zOrder=static_cast<std::int32_t>(std::stol(row[5]));
            type.constructorAddress=static_cast<std::uint32_t>(std::stoul(row[6])); type.footX=std::stoi(row[7]); type.footY=std::stoi(row[8]);
            factory=std::make_unique<SquidFactory>(*pool,types,false,unpop.get());
        } else if (row[0]=="Space") {
            hash->Reset(); std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(std::stoul(row[1])));
        } else if (row[0]=="Head") {
            CHECK(row.size()==5);
            hash->Bucket(std::stoi(row[1]),std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[2]))),
                std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[3]))))=static_cast<std::uint16_t>(std::stoul(row[4]));
        } else if (row[0]=="Fill") {
            CHECK(row.size()==11); auto bytes=pool->AllocatedBytes(Sid{static_cast<std::uint16_t>(std::stoul(row[1]))});
            const auto seed=std::stoul(row[2]); const bool patch=pool->Edition()==OriginalEdition::Patch1078;
            // 오염된 입력 payload를 놓고 원본 합성 입력에 지정한 필드만 덮는다.
            for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(seed+i*37);
            bytes[11]=static_cast<std::uint8_t>(std::stoul(row[3])); bytes[10]=static_cast<std::uint8_t>(std::stoul(row[4]));
            Put(bytes,0,static_cast<std::uint32_t>(std::stoul(row[5]))); Put(bytes,4,static_cast<std::uint32_t>(std::stoul(row[6])),2);
            Put(bytes,14,static_cast<std::uint32_t>(std::stoul(row[7]))); Put(bytes,18,static_cast<std::uint32_t>(std::stoul(row[8])));
            bytes[patch ? 40 : 35]=static_cast<std::uint8_t>(std::stoul(row[9])); bytes[patch ? 33 : 31]=static_cast<std::uint8_t>(std::stoul(row[10]));
            Put(bytes,patch ? 36 : 34,0,patch ? 4 : 1);
        } else if (row[0]=="Next") {
            CHECK(row.size()==3); Put(pool->AllocatedBytes(Sid{static_cast<std::uint16_t>(std::stoul(row[1]))}),4,
                static_cast<std::uint32_t>(std::stoul(row[2])),2);
        } else {
            CHECK(row.size()==17); const auto type=static_cast<std::uint32_t>(std::stoul(row[2]));
            const auto arg=static_cast<std::uint32_t>(std::stoul(row[3])); const Sid sid{static_cast<std::uint16_t>(std::stoul(row[4]))};
            std::uint32_t result=sid.value;
            if (row[1]=="Create") result=factory->Create(type,arg).value;
            else if (row[1]=="Take") result=factory->Take(type,Sid{static_cast<std::uint16_t>(arg)}).value;
            else if (row[1]=="Unpop") unpop->Unpop(sid,types.at(type),arg);
            else if (row[1]=="FirstPop") result=factory->FirstPopFlags(sid);
            else if (row[1]=="Release") pool->Release(sid);
            else throw std::runtime_error("Unpop fixture 명령 오류");
            const std::array<std::uint32_t,11> actual{result,pool->FreeCount(),pool->PredictableCursor(),
                pool->FirstFree(true).value,pool->Tail(true).value,pool->FirstFree(false).value,pool->Tail(false).value,
                Adler(pool->Bytes()),Adler(ExtraBytes(pool.get(),nullptr)),Adler(ExtraBytes(nullptr,hash.get())),Adler(spots)};
            // 원본 전체 공간 배열과 풀/삭제 기록 결과를 각 열에 대조한다.
            for (std::size_t i=0;i<actual.size();++i) CHECK(actual[i]==std::stoul(row[i+5]));
            const auto bytes=pool->Slot(sid); CHECK(row[16].size()==bytes.size()*2);
            // 대상 자신의 next·좌표/level·HP·섬 번호를 포함해 모든 바이트를 직접 비교한다.
            for (std::size_t i=0;i<bytes.size();++i) CHECK(bytes[i]==std::stoul(row[16].substr(i*2,2),nullptr,16));
            ++steps;
        }
    }
    CHECK(begins==18 && steps==3618);
}

// vtable 메타데이터는 두 PE의 실제 메서드 주소 선택과 각각 같은 지원 여부다.
TEST_CASE(SquidUnpop_VtableMetadata_MatchesActualCommonDisplayPaths) {
    std::ifstream input(NETSTORM_UNPOP_VTABLE_FIXTURE); CHECK(static_cast<bool>(input)); std::string line; int rows=0;
    // 표시 override를 base로 대체하지 않고 지정한 88/8c 경로별로 거부한다.
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;
        const auto row=Fields(line); CHECK(row.size()==6);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto vtable=static_cast<std::uint32_t>(std::stoul(row[1])); const auto mask=std::stoul(row[5]);
        CHECK(SquidUnpop::SupportsDisplay(edition,vtable,0)==((mask&1)!=0));
        CHECK(SquidUnpop::SupportsDisplay(edition,vtable,0x2000)==((mask&2)!=0)); ++rows;
    }
    CHECK(rows==151); CHECK(!SquidUnpop::SupportsDisplay(OriginalEdition::Patch1078,0,0));
}

// 손상된 검색 경로/미복원 효과는 상태·spot·머리·카운터를 바꾸기 전에 거부한다.
TEST_CASE(SquidUnpop_Guards_PreserveRawPoolAndSpaceBeforeUnsupportedEffects) {
    SidPool pool(OriginalEdition::Patch1078,32768); auto types=Types(pool.Edition()); SquidFactory factory(pool,types);
    SquidHash hash; std::vector<std::uint8_t> spots(kWorldCells*kWorldCells,0xff); SquidUnpop unpop(pool,hash,spots);
    const auto sid=factory.Create(82); auto bytes=pool.AllocatedBytes(sid); bytes[11]=0;
    Put(bytes,14,std::bit_cast<std::uint32_t>(20.f)); Put(bytes,18,std::bit_cast<std::uint32_t>(20.f));
    const auto before=std::vector(pool.Bytes().begin(),pool.Bytes().end()); const auto originalSpots=spots; const auto heads=ExtraBytes(nullptr,&hash);
    CHECK(Throws([&] { unpop.Unpop(sid,types[82]); }));
    CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()) && spots==originalSpots && ExtraBytes(nullptr,&hash)==heads);
    hash.Bucket(1,20,20)=sid.value;
    Put(bytes,0,0x12345678); auto invalidVtable=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { unpop.Unpop(sid,types[82]); }));
    CHECK(std::equal(invalidVtable.begin(),invalidVtable.end(),pool.Bytes().begin()));
    std::copy_n(before.begin()+static_cast<std::ptrdiff_t>(sid.value*pool.Layout().stride),4,bytes.begin());
    bytes[11]=8; const auto contained=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { unpop.Unpop(sid,types[82]); }));
    CHECK(std::equal(contained.begin(),contained.end(),pool.Bytes().begin())); bytes[11]=0;
    auto unsupported=types[82]; unsupported.flags2=TypeFlag2::kBridge;
    CHECK(Throws([&] { unpop.Unpop(sid,unsupported); }));
    Put(bytes,4,sid.value,2); auto corrupt=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { unpop.Unpop(sid,types[82]); })); CHECK(std::equal(corrupt.begin(),corrupt.end(),pool.Bytes().begin()));
    Put(bytes,4,0,2); Put(bytes,14,0x7fc00000); corrupt=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { unpop.Unpop(sid,types[82]); })); CHECK(std::equal(corrupt.begin(),corrupt.end(),pool.Bytes().begin()));
    CHECK(spots==originalSpots && hash.Bucket(1,20,20)==sid.value);
    SidPool other(pool.Edition(),32768); CHECK(Throws([&] { SquidFactory mixed(other,types,false,&unpop); }));
}

// CD에서 없는 체인은 assert 없이 void로 전환한다. 실제 삭제 효과 없이 슬롯을 반납하는 범위다.
TEST_CASE(SquidUnpop_CdMissingChain_AndTakePreservePoolAccounting) {
    SidPool pool(OriginalEdition::Cd1072,32768); auto types=Types(pool.Edition()); SquidHash hash;
    std::vector<std::uint8_t> spots(kWorldCells*kWorldCells); SquidUnpop unpop(pool,hash,spots); SquidFactory factory(pool,types,false,&unpop);
    const auto sid=factory.Create(82); auto bytes=pool.AllocatedBytes(sid); bytes[11]=2;
    Put(bytes,14,std::bit_cast<std::uint32_t>(30.f)); Put(bytes,18,std::bit_cast<std::uint32_t>(45.f));
    const auto free=pool.FreeCount(); const auto first=pool.FirstFree(false);
    CHECK(factory.Take(82,sid)==sid && bytes[11]==4); CHECK(pool.FreeCount()==free && pool.FirstFree(false)==first);
    pool.Release(sid); CHECK(pool.FreeCount()==free+1 && pool.Deletions(false)[0].sid==sid.value);
}
