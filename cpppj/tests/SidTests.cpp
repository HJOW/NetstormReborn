// 창/원본 프로세스 없이 저장된 SID 기계어 결과와 풀 수명 경계를 검사한다.
#include "TestSupport.h"
#include "o/SidPool.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>

using namespace netstorm::o;
namespace {
// fixture의 빈 필드를 포함하여 TSV 행을 분리한다.
std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> result; std::istringstream stream(line); std::string field;
    // 실제 파일의 각 열을 입력값 그대로 읽는다.
    while (std::getline(stream,field,'\t')) result.push_back(field);
    return result;
}
// 원본 풀 전체와 삭제 기록 전체를 Python zlib의 Adler-32 결과에 대조한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // Adler의 표준 소수와 초기 상태다. 중간 누적은 64비트로 넘침을 피한다.
    constexpr std::uint64_t prime=65521;
    std::uint64_t a=1,b=0; std::size_t block=0;
    // 4096바이트마다 나머지를 취하여 긴 풀에서도 중간값이 커지지 않게 한다.
    for (const auto byte:bytes) {
        a+=byte; b+=a;
        if (++block==4096) { a%=prime; b%=prime; block=0; }
    }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// 원본 두 삭제 기록 배열의 little endian 바이트를 비교한다.
std::uint32_t LogAdler(const SidPool& pool) {
    std::vector<std::uint8_t> bytes;
    // 원본 client/server 기록 순서대로 각각 20항목을 직렬화한다.
    for (const bool client:{true,false}) {
        // 해당 영역의 최신 삭제 기록부터 순회한다.
        for (const auto& record:pool.Deletions(client)) {
            // 두 32비트 필드를 호스트 패딩/엔디언과 무관하게 쓴다.
            for (const auto value:{record.sid,record.type}) {
                // 각 필드의 낮은 바이트부터 순회한다.
                for (int shift=0;shift<32;shift+=8) bytes.push_back(static_cast<std::uint8_t>(value>>shift));
            }
        }
    }
    return Adler(bytes);
}
// 원본의 주요 카운터·목록 머리/꼬리와 전체 버퍼 결과를 같은 열 순서로 만든다.
std::array<std::uint32_t,9> Snapshot(const SidPool& pool,std::uint32_t result) {
    return {result,pool.FreeCount(),pool.PredictableCursor(),pool.FirstFree(true).value,pool.Tail(true).value,
        pool.FirstFree(false).value,pool.Tail(false).value,Adler(pool.Bytes()),LogAdler(pool)};
}
// 실패한 입력이 풀을 손상시키는지 검사할 때 예외만 관찰한다.
template<class F> bool Throws(F&& function) {
    try { function(); } catch (const std::exception&) { return true; }
    return false;
}
}

// 독립 원본 기계어의 저장 결과를 읽어 두 판본의 모든 전이를 대조한다.
TEST_CASE(SidPool_X86Fixture_ReplaysBothEditions) {
    std::ifstream input(NETSTORM_SID_FIXTURE); CHECK(static_cast<bool>(input));
    std::unique_ptr<SidPool> pool; std::string line; int steps=0,begins=0;
    // 입력 Fill은 합성 파생 생성자 입력이며 기계어 사례 수에는 넣지 않는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty() || line.front()=='#') continue;
        const auto row=Fields(line);
        if (row[0]=="Begin") {
            CHECK(row.size()==4);
            pool=std::make_unique<SidPool>(row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,
                static_cast<std::uint32_t>(std::stoul(row[2])),row[3]=="1");
            ++begins;
        } else if (row[0]=="Fill") {
            CHECK(row.size()==5); CHECK(pool!=nullptr);
            auto bytes=pool->AllocatedBytes(Sid{static_cast<std::uint16_t>(std::stoul(row[1]))});
            const auto seed=std::stoul(row[2]);
            // 기대값을 만들지 않고 Python이 기록한 입력 바이트만 옮긴다.
            for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(seed+i*37);
            bytes[10]=static_cast<std::uint8_t>(std::stoul(row[4]));
            bytes[11]=static_cast<std::uint8_t>(std::stoul(row[3]));
        } else {
            CHECK(row.size()==12); CHECK(pool!=nullptr);
            const auto arg=static_cast<std::uint32_t>(std::stoul(row[2])); std::uint32_t result=0;
            if (row[1]=="Reset") pool->Reset();
            else if (row[1]=="Allocate") result=pool->Allocate(arg).value;
            else if (row[1]=="Release") pool->Release(Sid{static_cast<std::uint16_t>(arg)});
            else if (row[1]=="Rebuild") pool->RebuildServer();
            else throw std::runtime_error("SID fixture 명령 오류");
            const auto actual=Snapshot(*pool,result);
            // 머리/꼬리·카운터뿐 아니라 전체 풀과 삭제 기록의 변화를 검사한다.
            for (std::size_t i=0;i<actual.size();++i) CHECK(actual[i]==std::stoul(row[i+3]));
            ++steps;
        }
    }
    CHECK(begins==16); CHECK(steps==3168);
}

// 마지막 free 슬롯을 예약한 뒤 반납된 번호가 FIFO 꼬리로 들어가는지 검사한다.
TEST_CASE(SidPool_OrdinaryExhaustion_PreservesReservedTailAndFifo) {
    SidPool pool(OriginalEdition::Cd1072,14300);
    const auto initial=pool.FreeCount();
    // 원본 CD의 일반 클라이언트 영역 5..5998까지만 할당할 수 있다.
    for (std::uint16_t sid=5;sid<5999;++sid) CHECK(pool.Allocate(2).value==sid);
    CHECK(pool.FirstFree(true).value==5999 && pool.Tail(true).value==5999);
    const auto before=Adler(pool.Bytes());
    CHECK(Throws([&] { pool.Allocate(2); })); CHECK(Adler(pool.Bytes())==before);
    pool.AllocatedBytes(Sid{8})[10]=77;
    pool.Release(Sid{8});
    CHECK(pool.Tail(true).value==8 && pool.Slot(Sid{8})[10]==77 && pool.Slot(Sid{8})[11]==3);
    // 이전 예약 꼬리가 다음 번호가 되고 방금 반납한 번호는 새 예약 꼬리다.
    CHECK(pool.Allocate(2).value==5999); CHECK(pool.FirstFree(true).value==8);
    CHECK(Throws([&] { pool.Allocate(2); }));
    CHECK(pool.FreeCount()==initial-5994);
}

// 예측 번호는 커서로만 진행하며 bit 1이 client bit보다 우선한다. 마지막 서버 번호는 남긴다.
TEST_CASE(SidPool_PredictableBoundaries_KeepEditionAndServerRules) {
    SidPool server(OriginalEdition::Patch1078,23005);
    const auto a=server.Allocate(3),b=server.Allocate(1);
    CHECK(a.value==23002 && b.value==23003);
    server.Release(a); CHECK(server.Slot(a)[11]==3);
    CHECK(Throws([&] { server.Allocate(1); }));
    CHECK(server.PredictableCursor()==3);
    SidPool client(OriginalEdition::Cd1072,14003,false);
    const auto first=client.Allocate(1); CHECK(first.value==14001);
    const auto count=client.FreeCount();
    client.Release(first); CHECK(client.Slot(first)[11]==1 && client.FreeCount()==count);
    CHECK(client.Allocate(1).value==14002);
    CHECK(Throws([&] { client.Allocate(1); }));
    CHECK(Throws([&] { client.Allocate(0); }));
}

// raw 서버 목록 재구성은 freeCount 재계산이 아니라 기존 값에 free 서버 슬롯 수를 더한다.
TEST_CASE(SidPool_RebuildServer_PreservesAdditiveCounterAndOrder) {
    SidPool pool(OriginalEdition::Cd1072,14300);
    const auto a=pool.Allocate(),b=pool.Allocate(); CHECK(a.value==6000 && b.value==6001);
    pool.Release(a); const auto count=pool.FreeCount();
    pool.RebuildServer(); CHECK(pool.FirstFree(false).value==6000 && pool.Tail(false).value==13999);
    CHECK(pool.FreeCount()==count+7999); CHECK(pool.Allocate().value==6000);
    CHECK(pool.Allocate().value==6002);
}

// 파생 생성자의 가짜 payload가 반납/재초기화 뒤 남지 않는지와 삭제 기록의 별도 수명을 검사한다.
TEST_CASE(SidPool_Release_WipesPayloadAndRetainsDeletionHistory) {
    SidPool pool(OriginalEdition::Patch1078,23300);
    const auto sid=pool.Allocate(); auto bytes=pool.AllocatedBytes(sid);
    std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xab}); bytes[10]=123; bytes[11]=6;
    pool.Release(sid);
    const auto freed=pool.Slot(sid);
    // 반납 후 타입/상태를 제외한 50바이트 전체가 지워진다.
    for (std::size_t i=0;i<freed.size();++i) CHECK(freed[i]==(i==10 ? 123 : (i==11 ? 3 : 0)));
    CHECK(pool.Deletions(false)[0].sid==sid.value && pool.Deletions(false)[0].type==123);
    pool.Reset(); CHECK(pool.Deletions(false)[0].type==123); CHECK(pool.Slot(sid)[10]==0);
}

// 오류 입력과 원본 SID/임시 월드 번호의 암묵 혼용을 막는다.
TEST_CASE(SidPool_InvalidInput_RejectsBeforeMutation) {
    static_assert(!std::is_convertible_v<std::uint32_t,Sid>);
    CHECK(Throws([] { SidPool pool(OriginalEdition::Patch1078,23002); }));
    CHECK(Throws([] { SidPool pool(OriginalEdition::Cd1072,65536); }));
    SidPool pool(OriginalEdition::Cd1072,14300); const auto before=Adler(pool.Bytes());
    CHECK(Throws([&] { pool.Release(Sid{5}); }));
    CHECK(Throws([&] { pool.AllocatedBytes(Sid{0}); }));
    CHECK(Throws([&] { pool.Slot(Sid{14300}); }));
    CHECK(Adler(pool.Bytes())==before);
    const auto sid=pool.Allocate(2); pool.AllocatedBytes(sid)[11]=0;
    const auto active=Adler(pool.Bytes());
    CHECK(Throws([&] { pool.Release(sid); })); CHECK(Adler(pool.Bytes())==active);
}
