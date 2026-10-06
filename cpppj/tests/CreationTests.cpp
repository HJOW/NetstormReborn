// 창 없이 실제 두 PE의 생성/Take/가상 초기화 결과와 생성자 표를 대조한다.
#include "TestSupport.h"
#include "o/SquidFactory.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// 저장된 TSV의 입력/기대값 열을 읽는다.
std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> result; std::istringstream stream(line); std::string field;
    // 각 열을 순서대로 분리한다.
    while (std::getline(stream,field,'\t')) result.push_back(field);
    return result;
}
// 풀 전체의 Adler-32다. 대상 슬롯은 별도로 모든 바이트를 직접 비교한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // Adler의 소수와 블록 경계다.
    constexpr std::uint64_t prime=65521; std::uint64_t a=1,b=0; std::size_t block=0;
    // 중간값 넘침을 피하도록 4096바이트씩 나머지를 취한다.
    for (const auto byte:bytes) { a+=byte; b+=a; if (++block==4096) { a%=prime; b%=prime; block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// raw vtable 기록값을 little endian으로 쓴다. 호스트 함수 주소로 사용하지 않는다.
void PutVtable(std::span<std::uint8_t> bytes,std::uint32_t value) {
    // 포인터의 네 바이트를 직접 놓는다.
    for (std::size_t i=0;i<4;++i) bytes[i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 보호 경로의 실패가 예외로 보고되는지만 관찰한다.
template<class F> bool Throws(F&& operation) {
    try { operation(); } catch (const std::exception&) { return true; } return false;
}
// 판본과 일치하는 생성자 표를 최소 타입 입력에 연결한다.
std::vector<RiftTypeRecord> Types(OriginalEdition edition) {
    std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    // 모든 타입 번호의 실제 생성자 메타데이터를 채운다.
    for (std::size_t i=0;i<types.size();++i) types[i].constructorAddress=TypeConstructorAddress(edition,i);
    return types;
}
}

// 전체 생성자 주소 표는 제한된 원본 대입 구간과 모든 타입 번호에서 같다.
TEST_CASE(TypeConstructors_X86Table_MatchesBothEditions) {
    std::ifstream input(NETSTORM_CONSTRUCTOR_FIXTURE); CHECK(static_cast<bool>(input));
    std::string line; int rows=0,patch=0,cd=0;
    // 0 값도 생략하지 않고 base fallback 번호를 대조한다.
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;
        const auto row=Fields(line); CHECK(row.size()==3);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto address=TypeConstructorAddress(edition,std::stoul(row[1]));
        CHECK(address==std::stoul(row[2])); ++rows;
        if (address) { if (edition==OriginalEdition::Patch1078) ++patch; else ++cd; }
    }
    CHECK(rows==359 && patch==122 && cd==105);
    CHECK(Throws([] { TypeConstructorAddress(OriginalEdition::Cd1072,171); }));
}

// 대상 raw 슬롯은 hex의 모든 바이트, 풀은 카운터/목록/Adler를 기계어 기대값과 대조한다.
TEST_CASE(SquidFactory_X86Fixture_ReplaysBaseCreationAndVoidTake) {
    std::ifstream input(NETSTORM_CREATION_FIXTURE); CHECK(static_cast<bool>(input));
    std::unique_ptr<SidPool> pool; std::unique_ptr<SquidFactory> factory;
    std::vector<RiftTypeRecord> types; std::string line; bool weak=false; int rows=0,begins=0;
    // Type/Fill/Abstract는 합성 입력이며 원본 명령 호출 수에는 넣지 않는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty() || line[0]=='#') continue;
        const auto row=Fields(line);
        if (row[0]=="Begin") {
            CHECK(row.size()==5); const auto edition=row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
            pool=std::make_unique<SidPool>(edition,static_cast<std::uint32_t>(std::stoul(row[2])),row[3]=="1");
            types=Types(edition); weak=row[4]=="1"; factory=std::make_unique<SquidFactory>(*pool,types,weak); ++begins;
        } else if (row[0]=="Type") {
            CHECK(row.size()==7); auto& type=types.at(std::stoul(row[1]));
            type.flags1=static_cast<std::uint32_t>(std::stoul(row[2])); type.flags2=static_cast<std::uint32_t>(std::stoul(row[3]));
            type.maxHitPoints=static_cast<std::int32_t>(std::stol(row[4])); type.zOrder=static_cast<std::int32_t>(std::stol(row[5]));
            type.constructorAddress=static_cast<std::uint32_t>(std::stoul(row[6]));
            factory=std::make_unique<SquidFactory>(*pool,types,weak);
        } else if (row[0]=="Fill") {
            CHECK(row.size()==6); auto bytes=pool->AllocatedBytes(Sid{static_cast<std::uint16_t>(std::stoul(row[1]))});
            const auto seed=std::stoul(row[2]);
            // Python에 기록한 입력 바이트만 복사하고 기대값을 계산하지 않는다.
            for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(seed+i*37);
            bytes[11]=static_cast<std::uint8_t>(std::stoul(row[3])); bytes[10]=static_cast<std::uint8_t>(std::stoul(row[4]));
            PutVtable(bytes,static_cast<std::uint32_t>(std::stoul(row[5])));
        } else if (row[0]=="Abstract") {
            CHECK(row.size()==3); auto bytes=pool->AllocatedBytes(Sid{static_cast<std::uint16_t>(std::stoul(row[1]))});
            bytes[pool->Edition()==OriginalEdition::Patch1078 ? 40 : 35]=static_cast<std::uint8_t>(std::stoul(row[2]));
        } else {
            CHECK(row.size()==13); const auto type=static_cast<std::uint32_t>(std::stoul(row[2]));
            const auto arg=static_cast<std::uint32_t>(std::stoul(row[3])); Sid result{static_cast<std::uint16_t>(arg)};
            if (row[1]=="Create") result=factory->Create(type,arg);
            else if (row[1]=="Take") result=factory->Take(type,result);
            else if (row[1]=="PostCreate") factory->PostCreate(result);
            else if (row[1]=="PostTake") factory->PostTake(result);
            else throw std::runtime_error("생성 fixture 명령 오류");
            const std::array<std::uint32_t,8> actual{result.value,pool->FreeCount(),pool->PredictableCursor(),
                pool->FirstFree(true).value,pool->Tail(true).value,pool->FirstFree(false).value,pool->Tail(false).value,Adler(pool->Bytes())};
            // 반환 번호와 기존 목록/카운터를 각 열에 대조한다.
            for (std::size_t i=0;i<actual.size();++i) CHECK(actual[i]==std::stoul(row[i+4]));
            const auto bytes=pool->Slot(result); CHECK(row[12].size()==bytes.size()*2);
            // vtable·상태·owner·HP·깊이뿐 아니라 보존해야 할 모든 raw 바이트를 직접 비교한다.
            for (std::size_t i=0;i<bytes.size();++i) CHECK(bytes[i]==std::stoul(row[12].substr(i*2,2),nullptr,16));
            ++rows;
        }
    }
    CHECK(begins==4); CHECK(rows==964);
}

// 미복원 파생/잘못된 타입/클라이언트 권한/패치 금지 타입을 슬롯 할당 전에 거부한다.
TEST_CASE(SquidFactory_CreateGuards_LeavePoolUnchanged) {
    SidPool pool(OriginalEdition::Patch1078,32768,false); auto types=Types(pool.Edition());
    SquidFactory factory(pool,types); const auto before=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { factory.Create(1,2); })); CHECK(Throws([&] { factory.Create(188,2); }));
    CHECK(Throws([&] { factory.Create(82,2); })); CHECK(Throws([&] { factory.Create(162,2); }));
    CHECK(Throws([&] { factory.Create(74,0); }));
    CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
    CHECK(pool.FreeCount()==32763 && pool.PredictableCursor()==1);
}

// non-void·타입 불일치·파생 vtable·contained 수신은 변경 전에 거부한다. 정상 Take는 카운터를 조정하지 않는다.
TEST_CASE(SquidFactory_TakeGuards_PreservePayloadAndPoolAccounting) {
    SidPool pool(OriginalEdition::Cd1072,32768); auto types=Types(pool.Edition()); SquidFactory factory(pool,types);
    const auto sid=factory.Create(74); const auto free=pool.FreeCount();
    auto bytes=pool.AllocatedBytes(sid); bytes[11]=0;
    auto before=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { factory.Take(74,sid); })); CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
    bytes[11]=4; before=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { factory.Take(80,sid); })); CHECK(Throws([&] { factory.Take(74,Sid{5999}); }));
    CHECK(Throws([&] { factory.Take(74,Sid{32768}); })); CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
    PutVtable(bytes,123); before=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { factory.Take(74,sid); })); CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
    PutVtable(bytes,0x5058d8); bytes[11]=12; before=std::vector(pool.Bytes().begin(),pool.Bytes().end());
    CHECK(Throws([&] { factory.Take(74,sid); })); CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
    bytes[11]=6; bytes[26]=0xa1; bytes[27]=0xb2; bytes[32]=9;
    CHECK(factory.Take(74,sid)==sid && bytes[11]==4 && bytes[26]==0xa1 && bytes[27]==0xb2 && bytes[32]==9);
    CHECK(pool.FreeCount()==free && pool.FirstFree(false).value==6001);
    CHECK(factory.Take(74,Sid{14000}).value==14000); CHECK(pool.FreeCount()==free);
}

// .type의 HP/깊이 읽기와 실제 constructor=0 선택이 새 SID 초기화에 연결되는지 검사한다.
TEST_CASE(SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata) {
    // 두 판본의 HP 저장 폭 차이도 실제 타입 로더를 통과한 입력에서 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto order=TypeLoadOrder(edition); std::vector<RiftTypeDefinition> definitions; definitions.reserve(order.size());
        std::vector<RiftTypeSource> sources;
        // base 74에는 상수 이름+오프셋, base 80에는 숫자 깊이를 넣는다.
        for (std::size_t i=0;i<order.size();++i) {
            const auto body=i==4 ? "{ maxHitPoints=65536; zorder=\"zoBRIDGE_CONNECTOR - 3\"; }" :
                (i==10 ? "{ zorder=511; }" : "{}");
            definitions.push_back(RiftTypeDefinition::Parse("typename "+std::string(order[i])+" "+body+" A00:default:\"a.gif\"#0;"));
        }
        // 소유 배열을 완성한 뒤 정의 주소를 연결한다.
        for (std::size_t i=0;i<order.size();++i) sources.push_back({order[i],&definitions[i]});
        const RiftTypeTable table(edition,sources); CHECK(table.Types()[74].maxHitPoints==65536 && table.Types()[74].zOrder==-13);
        CHECK(table.Types()[80].zOrder==511 && table.Types()[82].constructorAddress!=0 && table.Types()[74].constructorAddress==0);
        SidPool pool(edition,32768); SquidFactory factory(pool,table.Types()); const auto sid=factory.Create(74);
        const auto bytes=pool.Slot(sid); const bool patch=edition==OriginalEdition::Patch1078;
        CHECK(bytes[patch ? 35 : 33]==243 && bytes[26]==0 && bytes[27]==0);
        CHECK(bytes[28]==(patch ? 1 : 0)); CHECK(Throws([&] { factory.Create(82); }));
    }
}
