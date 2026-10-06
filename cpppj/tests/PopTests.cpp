// 실제 원본 x86의 raw 등록/해제/반납 기대값을 재생한다. GUI/원본 파일이 필요하지 않다.
#include "TestSupport.h"
#include "o/SquidPop.h"
#include "o/SquidFactory.h"
#include "o/SquidUnpop.h"
#include "o/SquidSpatial.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// TSV 열을 원문 순서로 읽는다.
std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> result; std::istringstream input(line); std::string field;
    // 구분자만 제거하고 raw hex/숫자를 보존한다.
    while (std::getline(input,field,'\t')) result.push_back(field);
    return result;
}
// Python zlib와 같은 전체 버퍼 Adler-32다. 대상 슬롯은 별도로 바이트별 대조한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 긴 raw 풀의 누적 넘침을 피하도록 블록마다 소수로 축소한다.
    constexpr std::uint64_t prime=65521; std::uint64_t a=1,b=0; std::size_t block=0;
    // 원본 바이트 순서대로 누적한다.
    for (auto byte:bytes) { a+=byte; b+=a; if (++block==4096) { a%=prime; b%=prime; block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// 정렬되지 않은 raw 정수를 지정한 폭으로 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // CD의 좁은 frame/상태 필드 옆을 덮지 않는다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 전체 네 해시 배열을 원본 ushort 바이트 순서로 직렬화한다.
std::vector<std::uint8_t> Heads(const SquidHash& hash) {
    std::vector<std::uint8_t> result;
    // 0단계부터 모든 머리를 포함한다.
    for (int level=0;level<4;++level) {
        // 호스트 메모리 배치를 fixture의 little endian 값과 혼용하지 않는다.
        for (auto head:hash.Entries(level)) { result.push_back(static_cast<std::uint8_t>(head)); result.push_back(static_cast<std::uint8_t>(head>>8)); }
    }
    return result;
}
// 합성 타입 입력이다. 원본 파일 파서나 파생 생성자 전체를 대신했다고 해석하지 않는다.
std::vector<RiftTypeRecord> Types(OriginalEdition edition) {
    std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    // base fallback 생성에 유효한 최소 발자국만 지정한다.
    for (auto& type:types) { type.footX=type.footY=1; type.maxHitPoints=91; type.zOrder=7; }
    return types;
}
// 보호 경로의 예외만 관찰한다.
template<class F> bool Throws(F&& operation) {
    try { operation(); } catch (const std::exception&) { return true; } return false;
}
}

// 실제 Create→Pop/재등록→Unpop→반납의 슬롯 전체와 풀/머리/spot 체크섬을 대조한다.
TEST_CASE(SquidPop_X86Fixture_ReplaysActualRawRegistrationLifecycle) {
    std::ifstream input(NETSTORM_POP_FIXTURE); CHECK(static_cast<bool>(input));
    std::unique_ptr<SidPool> pool; std::unique_ptr<SquidHash> hash;
    std::unique_ptr<SquidFactory> factory; std::unique_ptr<SquidPop> pop; std::unique_ptr<SquidUnpop> unpop;
    std::vector<RiftTypeRecord> types; std::vector<std::uint8_t> spots;
    float fw=1,fh=1; std::string line; int begins=0,steps=0,lineNumber=0;
    // Claim/Fill/Seed는 원본 월드 로딩이 아닌 fixture의 명시적인 준비 입력이다.
    while (std::getline(input,line)) {
        ++lineNumber;
        if (line.empty() || line[0]=='#') continue;
        const auto row=Fields(line);
        if (row[0]=="Begin") {
            CHECK(row.size()==4); factory.reset(); pop.reset(); unpop.reset();
            const auto edition=row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
            pool=std::make_unique<SidPool>(edition,static_cast<std::uint32_t>(std::stoul(row[2])),false);
            hash=std::make_unique<SquidHash>(); spots.assign(kWorldCells*kWorldCells,0); types=Types(edition);
            factory=std::make_unique<SquidFactory>(*pool,types);
            pop=std::make_unique<SquidPop>(*pool,*hash,spots); unpop=std::make_unique<SquidUnpop>(*pool,*hash,spots); ++begins;
        } else if (row[0]=="Type") {
            CHECK(row.size()==8); auto& type=types.at(std::stoul(row[1]));
            type.flags1=static_cast<std::uint32_t>(std::stoul(row[2])); type.flags2=static_cast<std::uint32_t>(std::stoul(row[3]));
            type.footX=std::stoi(row[4]); type.footY=std::stoi(row[5]);
            fw=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[6])));
            fh=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[7])));
            factory=std::make_unique<SquidFactory>(*pool,types);
        } else if (row[0]=="Claim") {
            // Take로 free 슬롯을 확보한 뒤 Fill이 모든 payload를 덮는다. 목록/카운터는 보존한다.
            factory->Take(74,Sid{static_cast<std::uint16_t>(std::stoul(row[1]))});
        } else if (row[0]=="Fill") {
            CHECK(row.size()==7); const Sid sid{static_cast<std::uint16_t>(std::stoul(row[1]))}; auto bytes=pool->AllocatedBytes(sid);
            const auto seed=std::stoul(row[2]); const bool patch=pool->Edition()==OriginalEdition::Patch1078;
            // x86 입력과 같은 오염 바이트를 슬롯 전체에 놓는다.
            for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(seed+i*37);
            bytes[10]=static_cast<std::uint8_t>(std::stoul(row[4])); bytes[11]=static_cast<std::uint8_t>(std::stoul(row[3]));
            Put(bytes,0,static_cast<std::uint32_t>(std::stoul(row[5]))); Put(bytes,4,0,2); Put(bytes,8,23,2);
            Put(bytes,14,std::bit_cast<std::uint32_t>(20.0f)); Put(bytes,18,std::bit_cast<std::uint32_t>(20.0f));
            bytes[patch ? 40 : 35]=static_cast<std::uint8_t>(std::stoul(row[6]));
            bytes[patch ? 33 : 31]=3; Put(bytes,patch ? 36 : 34,0,patch ? 4 : 1);
        } else if (row[0]=="Seed") {
            spots.at(static_cast<std::size_t>(std::stoi(row[2])*kWorldCells+std::stoi(row[1])))=static_cast<std::uint8_t>(std::stoul(row[3]));
        } else if (row[0]=="Clear") {
            hash->Reset(); std::fill(spots.begin(),spots.end(),std::uint8_t{});
        } else if (row[0]=="Step") {
            const auto failuresBefore=netstorm::test::FailureCount();
            CHECK(row.size()==11); const Sid sid{static_cast<std::uint16_t>(std::stoul(row[2]))};
            const auto arg=static_cast<std::uint32_t>(std::stoul(row[3])); std::uint32_t result=0;
            if (row[1]=="Create") result=factory->Create(arg,2).value;
            else if (row[1]=="Release") pool->Release(sid);
            else if (row[1]=="Unpop") unpop->Unpop(sid,types.at(pool->Slot(sid)[10]),arg);
            else if (row[1]=="Pop") result=static_cast<std::uint32_t>(pop->Pop(sid,types.at(pool->Slot(sid)[10]),fw,fh,
                std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[4]))),
                std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[5]))),arg));
            else CHECK(false);
            CHECK(result==std::stoul(row[6])); CHECK(Adler(pool->Bytes())==std::stoul(row[7]));
            CHECK(Adler(Heads(*hash))==std::stoul(row[8])); CHECK(Adler(spots)==std::stoul(row[9]));
            const auto bytes=pool->Slot(sid); CHECK(row[10].size()==bytes.size()*2);
            // raw 오염/HP/섬 번호/next/프레임/좌표를 모두 원본 바이트와 비교한다.
            for (std::size_t i=0;i<bytes.size();++i) CHECK(bytes[i]==std::stoul(row[10].substr(i*2,2),nullptr,16));
            if (netstorm::test::FailureCount()!=failuresBefore)
                throw std::runtime_error("raw Pop fixture 줄 "+std::to_string(lineNumber)+" "+row[1]+" SID "+row[2]);
            ++steps;
        } else CHECK(false);
    }
    CHECK(begins==8); CHECK(steps==4600);
}

// vtable 지원 여부는 PE 주소 표의 firstPop/postPop과 두 표시 경로로 제한한다.
TEST_CASE(SquidPop_Vtables_RejectsUnrestoredPostPopOverrides) {
    std::ifstream input(NETSTORM_POP_VTABLE_FIXTURE); CHECK(static_cast<bool>(input)); std::string line; int rows=0;
    // 메타데이터는 기계어 호출 수와 별도로 검사한다.
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;
        const auto fields=Fields(line); CHECK(fields.size()==5);
        const auto edition=fields[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto vtable=static_cast<std::uint32_t>(std::stoul(fields[1])); const bool supported=fields[4]=="1";
        CHECK(SquidPop::Supports(edition,vtable,0)==supported); CHECK(SquidPop::Supports(edition,vtable,0x2000)==supported); ++rows;
    }
    CHECK(rows==151);
}

// 미복원 효과와 잘못된 상태는 raw 공간 쓰기 전에 실패해야 한다.
TEST_CASE(SquidPop_Guards_PreservesRawPoolAndMap) {
    SidPool pool(OriginalEdition::Patch1078,32768,false); auto types=Types(pool.Edition()); SquidFactory factory(pool,types);
    SquidHash hash; std::vector<std::uint8_t> spots(kWorldCells*kWorldCells); SquidPop pop(pool,hash,spots);
    const auto sid=factory.Create(74,2); const auto before=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());
    types[74].flags2=0x4000;
    CHECK(Throws([&]{ pop.Pop(sid,types[74],1,1,30,40); }));
    types[74].flags2=0x11; types[74].footX=40;
    CHECK(Throws([&]{ pop.Pop(sid,types[74],1,1,30,40); }));
    CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
    CHECK(std::all_of(spots.begin(),spots.end(),[](auto v){return v==0;}));
    CHECK(hash.Bucket(1,30,40)==0);
}

// CD 디컴파일의 정수 캐스트 표현과 달리 양 판본은 0<x<1의 실제 float 좌표를 보존한다.
TEST_CASE(SquidSpatial_Coordinates_PreservesFractionalBoundaryInBothEditions) {
    // 각 판본은 spot 쓰기가 없는 일반 객체로 좌표 보정만 관찰한다.
    for (const auto edition:{SpatialEdition::Patch1078,SpatialEdition::CD1072}) {
        SquidSpatial spatial(edition); SpatialObject object; object.id=7; spatial.Add(object);
        CHECK(spatial.Pop(7,0.25f,20.5f).result==SpatialResult::Registered);
        CHECK(spatial.Object(7).x==0.25f); CHECK(spatial.Object(7).y==20.5f);
    }
}
