// 실제 두 PE의 건물 등록/해제와 모든 옆면 후보 raw 상태를 독립 fixture에 대조한다.
#include "TestSupport.h"
#include "o/RawBuildingSpace.h"
#include "o/SquidFactory.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// native 입력과 동일한 전체 풀 크기, 바이트 직접 비교 범위다.
constexpr std::uint32_t kCapacity=32768,kPrefix=192;
// TSV 필드를 변형 없이 분리한다.
std::vector<std::string> Fields(const std::string& text) {
    std::istringstream stream(text);std::vector<std::string> fields;std::string field;
    // raw hex와 부호를 바꾸지 않고 탭만 제거한다.
    while (std::getline(stream,field,'\t')) fields.push_back(field);
    return fields;
}
// 전체 지도/풀 체크섬은 앞 슬롯 바이트 직접 비교와 함께 사용한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // Python zlib와 같은 소수와 블록 길이다.
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;std::size_t count=0;
    // long 풀에서도 누적 넘침 없이 같은 byte 순서로 계산한다.
    for (const auto value:bytes) { a+=value;b+=a;if (++count==4096) { a%=prime;b%=prime;count=0; } }
    return static_cast<std::uint32_t>((b%prime)<<16|a%prime);
}
// 네 해시 머리 배열을 원본 little endian 순서로 직렬화한다.
std::vector<std::uint8_t> Heads(const SquidHash& hash) {
    std::vector<std::uint8_t> result;
    // 각 단계의 모든 WORD를 순회한다.
    for (int level=0;level<4;++level) {
        // 호스트 endian과 독립적인 low/high BYTE다.
        for (const auto value:hash.Entries(level)) { result.push_back(static_cast<std::uint8_t>(value));result.push_back(static_cast<std::uint8_t>(value>>8)); }
    }
    return result;
}
// 실제 native 슬롯 입력을 현재 풀에 지정하며 할당 흐름 검증으로 세지 않는다.
void Seed(SidPool& pool,Sid sid,const std::string& hex) {
    CHECK(hex.size()==pool.Layout().stride*2);
    auto* bytes=const_cast<std::uint8_t*>(pool.Slot(sid).data());
    // 모든 슬롯 바이트를 그대로 주입한다. free 후보도 의도적인 원본 입력이다.
    for (std::size_t i=0;i<pool.Layout().stride;++i) bytes[i]=static_cast<std::uint8_t>(std::stoul(hex.substr(i*2,2),nullptr,16));
}
// 호스트 보호 경계의 예외를 확인한다.
template<class F> bool Throws(F&& function) { try { function(); } catch (const std::exception&) { return true; } return false; }
}

// 일반 Pop/Unpop의 실제 순서를 실행하고 free·non-surface·묻힘·겹침과 높은 BYTE 보존을 대조한다.
TEST_CASE(BuildingSpace_X86Fixture_ReplaysWholeBuildingPopAndUnpop) {
    std::ifstream input(NETSTORM_BUILDINGSPACE_FIXTURE);CHECK(static_cast<bool>(input));
    std::unique_ptr<SidPool> pool;std::unique_ptr<SquidHash> hash;std::unique_ptr<SquidFactory> factory;
    std::unique_ptr<RawBuildingSpace> building;std::unique_ptr<SquidPop> pop;std::unique_ptr<SquidUnpop> unpop;
    std::vector<RiftTypeRecord> types;std::vector<std::uint8_t> spots;BuildingSpaceState state{82,81,false};
    std::string line;int begins=0,steps=0;
    // fixture 입력을 받은 뒤에만 각 actual 원본 결과와 비교한다.
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;
        const auto row=Fields(line);
        if (row[0]=="Begin") {
            factory.reset();pop.reset();unpop.reset();building.reset();
            const auto edition=row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
            pool=std::make_unique<SidPool>(edition,kCapacity,false);hash=std::make_unique<SquidHash>();spots.assign(65536,0);
            types.assign(edition==OriginalEdition::Patch1078 ? 188 : 171,RiftTypeRecord{});
            // base Create 입력의 maxHP/zOrder/foot 초기값이다. 파생 constructor는 등록하지 않는다.
            for (auto& type:types) { type.maxHitPoints=91;type.zOrder=7;type.footX=type.footY=1; }
            types[74].flags2=static_cast<std::uint32_t>(std::stoul(row[3]));types[74].footX=std::stoi(row[4]);types[74].footY=std::stoi(row[5]);
            types[81].flags1=std::stoi(row[6]) ? TypeFlag1::kSurface : 0;types[81].flags2=4;
            types[82].flags1=TypeFlag1::kSurface;types[82].flags2=2;
            factory=std::make_unique<SquidFactory>(*pool,types);building=std::make_unique<RawBuildingSpace>(*pool,*hash,types,state);
            pop=std::make_unique<SquidPop>(*pool,*hash,spots);unpop=std::make_unique<SquidUnpop>(*pool,*hash,spots);
            pop->SetBuildingSpace(building.get());unpop->SetBuildingSpace(building.get());++begins;
        } else if (row[0]=="Building") {
            CHECK(factory->Create(74,2).value==std::stoul(row[1]));Seed(*pool,Sid{static_cast<std::uint16_t>(std::stoul(row[1]))},row[2]);
        } else if (row[0]=="Candidate") {
            const Sid sid{static_cast<std::uint16_t>(std::stoul(row[1]))};Seed(*pool,sid,row[4]);hash->Cell(0,std::stoi(row[2]),std::stoi(row[3]))=sid.value;
        } else if (row[0]=="Spot") {
            spots[static_cast<std::size_t>(std::stoi(row[2])*256+std::stoi(row[1]))]=static_cast<std::uint8_t>(std::stoul(row[3]));
        } else if (row[0]=="Step") {
            const Sid sid{static_cast<std::uint16_t>(std::stoul(row[2]))};int result=0;
            if (row[1]=="Pop") result=static_cast<int>(pop->Pop(sid,types[74],4,4,30,45,8));
            else unpop->Unpop(sid,types[74],8);
            CHECK(result==std::stoi(row[3]));CHECK(Adler(pool->Bytes())==std::stoul(row[4]));CHECK(Adler(Heads(*hash))==std::stoul(row[5]));CHECK(Adler(spots)==std::stoul(row[6]));
            const auto actual=pool->Bytes().first(kPrefix*pool->Layout().stride);CHECK(row[7].size()==actual.size()*2);
            std::size_t differences=0;
            // 명시한 모든 후보와 그 외 앞192슬롯을 바이트별로 확인한다.
            for (std::size_t i=0;i<actual.size();++i) {
                const auto expected=std::stoul(row[7].substr(i*2,2),nullptr,16);
                CHECK(actual[i]==expected);
                // 처음 네 차이는 입력/호출/번호/필드 위치까지 남겨 독립 관찰과 다른 준비 입력을 찾는다.
                if (actual[i]!=expected && ++differences<=4)
                    std::printf("건물 입력 %d %s SID%zu +%zu C++=%u native=%lu\n",begins,row[1].c_str(),i/pool->Layout().stride,i%pool->Layout().stride,actual[i],expected);
            }
            ++steps;
        } else throw std::logic_error("건물 공간 fixture 행 오류");
    }
    CHECK(begins==128);CHECK(steps>=240);
}

// 기본 구성은 여전히 미연결 건물 변경을 막고 같은 공간의 연결만 허용한다.
TEST_CASE(BuildingSpace_BindingAndNativeDiagnostic_AreEditionSpecific) {
    // actual 구동에 사용하는 두 판본을 동일한 타입 번호 입력으로 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false),other(edition,kCapacity,false);SquidHash hash,otherHash;
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[74].flags2=0x4000;types[74].footX=3;types[74].footY=3;types[81].flags1=0x800;
        std::vector<std::uint8_t> spots(65536);SquidFactory factory(pool,types);const auto sid=factory.Create(74,2);
        SquidPop pop(pool,hash,spots);SquidUnpop unpop(pool,hash,spots);BuildingSpaceState state{82,81,true};
        RawBuildingSpace building(pool,hash,types,state),mismatch(other,otherHash,types,state);
        CHECK(Throws([&] { pop.Pop(sid,types[74],3,3,30,45); }));CHECK(Throws([&] { pop.SetBuildingSpace(&mismatch); }));CHECK(Throws([&] { unpop.SetBuildingSpace(&mismatch); }));
        pop.SetBuildingSpace(&building);unpop.SetBuildingSpace(&building);
        auto bytes=pool.AllocatedBytes(sid);bytes[10]=81;bytes[12]=0x7e;bytes[edition==OriginalEdition::Patch1078 ? 40 : 35]=0;
        CHECK(Throws([&] { building.NotifySurface(sid); })==(edition==OriginalEdition::Patch1078));
        state.debug=false;building.NotifySurface(sid);CHECK(bytes[12]==0x7e);
        CHECK(!building.Surface(-1,0).value);CHECK(!building.Surface(256,0).value);CHECK(!building.HasIsland(-0.25f,2.5f));
    }
}
