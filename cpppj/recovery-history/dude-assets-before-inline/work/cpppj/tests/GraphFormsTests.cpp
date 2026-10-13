// 실제 전체풀 재구성에서 내부 Form을 생략하는 양판본 raw 관찰을 대조한다.
#include "TestSupport.h"
#include "o/RawGraph.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace netstorm;
namespace {
// TSV의 각 열 및 슬롯 구분자를 원래 입력 순서대로 읽는다.
std::vector<std::string> SplitForms(const std::string& text,char delimiter) {
    std::istringstream input(text);std::string item;std::vector<std::string> result;
    // 빈 열은 fixture 오류로 남기며 구분자로만 분리한다.
    while(std::getline(input,item,delimiter))result.push_back(item);
    return result;
}
// 모든 raw 바이트를 두 자리 hex에서 읽어 padding을 포함해 대조한다.
std::vector<std::uint8_t> FormBytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 메모리 순서대로 각 바이트를 읽는다.
    for(std::size_t index=0;index<hex.size();index+=2)
        result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(index,2),nullptr,16)));
    return result;
}
// 그래프 레코드의 세 signed WORD를 원본 little endian 순서로 읽는다.
std::array<o::GraphRecord,o::Graph::kTableSize> FormRecords(const std::string& hex) {
    const auto bytes=FormBytes(hex);std::array<o::GraphRecord,o::Graph::kTableSize> records{};
    // 미사용 reserved와 sentinel도 각각 읽는다.
    for(std::size_t index=0;index<records.size();++index) {
        std::array<std::int16_t,3> words{};
        // 각 WORD의 부호를 호스트 캐스트에 의존하지 않고 보존한다.
        for(std::size_t field=0;field<3;++field) {
            const auto offset=index*6+field*2;
            words[field]=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(bytes.at(offset)|(static_cast<std::uint16_t>(bytes.at(offset+1))<<8)));
        }
        records[index]={words[0],words[1],words[2]};
    }
    return records;
}
// 레코드와 flood DWORD를 엔디언/padding과 관계없이 원본 바이트로 직렬화한다.
void AppendFormBytes(std::vector<std::uint8_t>& bytes,std::uint32_t value,std::size_t width) {
    // 필요한 필드 폭의 low byte부터 추가한다.
    for(std::size_t index=0;index<width;++index)bytes.push_back(static_cast<std::uint8_t>(value>>(index*8)));
}
}

// 새로운64개 고유 입력의 전체 슬롯/255레코드/4096스택 DWORD를 실제 PE 결과와 비교한다.
TEST_CASE(GraphForms_ActualNativeFullPoolSkipsNonSpatialForms) {
    std::ifstream input(NETSTORM_GRAPHFORMS_FIXTURE);CHECK(input.good());std::string line;int count=0;
    // 각 행은 초기 입력과 실제 PE 정상 반환을 함께 보존한다.
    while(std::getline(input,line)) {
        if(line.empty()||line.front()=='#')continue;
        const auto row=SplitForms(line,'\t');CHECK(row.size()==11);
        const auto edition=row.at(0)=="originals"?o::OriginalEdition::Patch1078:o::OriginalEdition::Cd1072;
        o::SidPool pool(edition,32768,false);o::SquidHash hash;
        const auto expectedIds=SplitForms(row.at(4),',');const auto before=SplitForms(row.at(5),';');const auto after=SplitForms(row.at(6),';');
        std::vector<o::Sid> ids;
        // 실제 SID client 할당 순서와 초기 전체 raw 슬롯을 준비한다.
        for(std::size_t index=0;index<expectedIds.size();++index) {
            const auto id=pool.Allocate(2);ids.push_back(id);CHECK(id.value==std::stoul(expectedIds.at(index)));
            auto slot=pool.AllocatedBytes(id);const auto bytes=FormBytes(before.at(index));CHECK(bytes.size()==slot.size());
            std::copy(bytes.begin(),bytes.end(),slot.begin());
        }
        hash.Bucket(0,20,20)=ids.front().value;
        std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells);
        std::vector<o::RiftTypeRecord> types(edition==o::OriginalEdition::Patch1078?188U:171U);
        types[74].flags1=o::TypeFlag1::kSurface;types[74].flags2=2;types[74].footX=types[74].footY=1;
        std::vector<std::vector<o::FrameCode>> frames(types.size());frames[74]={{65,80,1,0},{65,80,2,0}};
        std::array<std::uint32_t,o::Graph::kFloodSize> stack{};const auto seed=static_cast<std::uint32_t>(std::stoul(row.at(9)));
        // 이전 flood 미사용 DWORD까지 동일 seed 입력을 넣는다.
        for(std::size_t index=0;index<stack.size();++index)stack[index]=seed+static_cast<std::uint32_t>(index);
        o::RawGraph graph(pool,hash,spots,types,frames,FormRecords(row.at(7)),stack);
        graph.Rebuild(std::stoul(row.at(1))!=0);
        // 내부 Form 여섯 개의 NaN/stale 필드를 포함한 전체 슬롯도 보존돼야 한다.
        for(std::size_t index=0;index<ids.size();++index) {
            const auto bytes=FormBytes(after.at(index));const auto actual=pool.Slot(ids[index]);
            CHECK(std::equal(bytes.begin(),bytes.end(),actual.begin(),actual.end()));
        }
        std::vector<std::uint8_t> table,flood;
        // sentinel/미사용 reserved를 포함한255 레코드 전체를 비교한다.
        for(const auto& record:graph.Records()) {
            AppendFormBytes(table,static_cast<std::uint16_t>(record.surfaces),2);
            AppendFormBytes(table,static_cast<std::uint16_t>(record.inUse),2);
            AppendFormBytes(table,static_cast<std::uint16_t>(record.reserved),2);
        }
        // 쓰이지 않은 flood 스택까지 전체 바이트를 비교한다.
        for(const auto value:graph.FloodStack())AppendFormBytes(flood,value,4);
        CHECK(table==FormBytes(row.at(8)));CHECK(flood==FormBytes(row.at(10)));
        CHECK(hash.Bucket(0,20,20)==ids.front().value);
        CHECK(std::all_of(spots.begin(),spots.end(),[](auto value){return value==0;}));
        ++count;
    }
    CHECK(count==64);
}

// 공간 머리에 내부 Form이 등장하는 손상 입력은 기존처럼 쓰기 전에 거부해야 한다.
TEST_CASE(GraphForms_HashFormRejectsBeforeGraphOrPoolWrites) {
    // 양판본 모두 같은 손상 머리의 검증 순서를 유지한다.
    for(const auto edition:{o::OriginalEdition::Patch1078,o::OriginalEdition::Cd1072}) {
        o::SidPool pool(edition,32768,false);const auto id=pool.Allocate(2);auto slot=pool.AllocatedBytes(id);
        slot[10]=46;slot[11]=4;o::SquidHash hash;hash.Bucket(0,20,20)=id.value;
        std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells);
        std::vector<o::RiftTypeRecord> types(edition==o::OriginalEdition::Patch1078?188U:171U);
        std::vector<std::vector<o::FrameCode>> frames(types.size());o::RawGraph graph(pool,hash,spots,types,frames);
        const std::vector<std::uint8_t> before(pool.Bytes().begin(),pool.Bytes().end());
        bool rejected=false;
        try{graph.Rebuild(false);}catch(const std::exception&){rejected=true;}
        CHECK(rejected);CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin(),pool.Bytes().end()));
        CHECK(hash.Bucket(0,20,20)==id.value);CHECK(graph.Records()[0].inUse==0);
    }
}
