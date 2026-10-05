// 실제 두 판본의 버킷 주소·초기화·해시 단계·점유 genus 기대값을 새 C++와 비교한다.
#include "TestSupport.h"
#include "o/SquidHash.h"
#include "o/Squid.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// x86에 입력한 단정도 비트를 그대로 C++ 입력으로 복원한다.
float Float(const std::string& value) { return std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(value))); }
// 원본/분석 도구 없이 Git에 저장한 기계어 결과를 한 번 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_HASH_FIXTURE);
        if (!input) throw std::runtime_error("Missing hash x86 fixture");
        std::vector<std::vector<std::string>> result;
        std::string line;
        // 한국어 설명 줄을 제외하고 입력/결과 필드를 분리한다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (line.empty() || line.front()=='#') continue;
            std::istringstream stream(line); std::vector<std::string> row; std::string field;
            // TSV에는 빈 필드가 없다. 결과 주소는 버킷 배열의 short 인덱스다.
            while (std::getline(stream,field,'\t')) row.push_back(field);
            result.push_back(std::move(row));
        }
        return result;
    }();
    return rows;
}
// 원본에서 정상 범위가 아닌 좌표/단계를 새 코드가 오류로 알리는지 확인한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}
}

// 실제 네 초기화와 반환 포인터 807개를 같은 배열의 인덱스와 비교한다.
TEST_CASE(Hash_X86InitializationAndBucketAddresses) {
    SquidHash hash; std::size_t initialized=0,at=0,integer=0,floating=0;
    // 네이티브 주소 결과와 상태 필드를 직접 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]=="Init") {
            hash.Select(std::stoi(row[1])); hash.At(0,0)=7; hash.Reset();
            CHECK(hash.Selected()==std::stoi(row[2]));
            CHECK(SquidHash::kScales[static_cast<std::size_t>(hash.Selected())]==std::stoi(row[3]));
            CHECK(SquidHash::Side(hash.Selected())==std::stoi(row[4]));
            // 원본이 전체 배열을 지운 결과와 비교한다.
            for (int level=0;level<4;++level) CHECK(std::all_of(hash.Entries(level).begin(),hash.Entries(level).end(),[](auto id) { return id==0; }));
            ++initialized;
        } else if (row[0]=="At" || row[0]=="IntBucket" || row[0]=="FloatBucket") {
            const int level=std::stoi(row[1]); const auto expected=std::stoul(row[4]);
            const auto* begin=hash.Entries(level).data();
            if (row[0]=="At") {
                hash.Select(level); CHECK(static_cast<std::size_t>(&hash.At(std::stoi(row[2]),std::stoi(row[3]))-begin)==expected); ++at;
            } else if (row[0]=="IntBucket") {
                CHECK(static_cast<std::size_t>(&hash.Cell(level,std::stoi(row[2]),std::stoi(row[3]))-begin)==expected); ++integer;
            } else {
                CHECK(static_cast<std::size_t>(&hash.Bucket(level,Float(row[2]),Float(row[3]))-begin)==expected); ++floating;
            }
        }
    }
    CHECK(initialized==4 && at==100 && integer==100 && floating==607);
}
// SHP 헤더의 실제 크기와 genus가 원본이 저장한 단계 바이트를 재현한다.
TEST_CASE(Hash_X86ObjectLevelFromFrameSize) {
    std::size_t count=0;
    // 2/4 경계의 바로 앞/뒤와 섬/다리를 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Level") continue;
        const auto value=SquidHash::ObjectLevel(static_cast<std::uint32_t>(std::stoul(row[1])),Float(row[2]),Float(row[3]));
        CHECK(value==std::stoi(row[4]) && value==std::stoi(row[5])); ++count;
    }
    CHECK(count==605);
}
// 원본 두 함수의 발자국/지붕/소수 기준점 보정과 전체 uint 반환을 비교한다.
TEST_CASE(Squid_X86EffectiveGenusAndRoofInterior) {
    std::size_t count=0,changed=0;
    // 파생 점유 비트만 확인하며 실제 spot 등록 함수는 실행하지 않는다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Genus") continue;
        const auto flags=static_cast<std::uint32_t>(std::stoul(row[1]));
        const auto value=Squid::EffectiveGenus(flags,Float(row[4]),Float(row[5]),std::stoi(row[2]),std::stoi(row[3]),std::stoi(row[6]),std::stoi(row[7]));
        CHECK(value==std::stoul(row[8]) && value==std::stoul(row[9])); changed+=value!=flags; ++count;
    }
    CHECK(count==3216 && changed>0);
}
// 단계별 머리 배열의 독립성과 초기화가 선택 단계/할당 주소를 유지하는지 확인한다.
TEST_CASE(Hash_IndependentLevelsAndCompleteReset) {
    SquidHash hash(3);
    // 각 단계의 시작/마지막 버킷을 다른 값으로 채운다.
    for (int level=0;level<4;++level) {
        const int last=SquidHash::Side(level)-1; hash.Cell(level,0,0)=static_cast<std::uint16_t>(level+1);
        hash.Cell(level,last,last)=static_cast<std::uint16_t>(level+11);
    }
    const auto& read=hash; CHECK(read.At(0,0)==4 && read.Bucket(2,255.9f,255.9f)==13);
    const auto* address=hash.Entries(0).data(); hash.Reset(); CHECK(hash.Selected()==3 && address==hash.Entries(0).data());
    // 다른 단계도 모두 지워졌으며 공간 체인으로 추정한 번호가 남지 않는다.
    for (int level=0;level<4;++level) CHECK(std::all_of(hash.Entries(level).begin(),hash.Entries(level).end(),[](auto id) { return id==0; }));
}
// 해시와 spot의 서로 다른 좌표 규칙 및 x87 중간 덧셈의 정밀도 경계를 확인한다.
TEST_CASE(Hash_PlainTruncationAndSpotBiasAreDistinct) {
    CHECK(SquidHash::BucketIndex(0,20.25f,20.25f)==20*256+20);
    CHECK(Squid::EffectiveGenus(TypeFlag2::kVortex,20.25f,20.25f,3,3,20,20)==(TypeFlag2::kVortex|8));
    CHECK(Squid::EffectiveGenus(TypeFlag2::kVortex,20.0001f,20.0001f,3,3,19,19)==(TypeFlag2::kVortex|8));
}
// 원본이 정의하지 않은 배열 밖 입력을 메모리 주소 계산으로 진행하지 않는다.
TEST_CASE(Hash_RejectsInvalidLevelsCoordinatesAndSizes) {
    SquidHash hash;
    CHECK(Throws([&] { hash.Select(4); })); CHECK(hash.Selected()==0);
    CHECK(Throws([&] { hash.Cell(3,16,0); })); CHECK(Throws([&] { hash.At(-1,0); }));
    CHECK(Throws([&] { hash.Bucket(0,256,0); }));
    CHECK(Throws([&] { hash.Bucket(0,std::numeric_limits<float>::quiet_NaN(),0); }));
    CHECK(Throws([&] { SquidHash::ObjectLevel(0,-1,1); }));
    CHECK(Throws([&] { Squid::EffectiveGenus(0,0,0,0,1,0,0); }));
    CHECK(Throws([&] { Squid::EffectiveGenus(0,0,0,1,1,-1,0); }));
}
