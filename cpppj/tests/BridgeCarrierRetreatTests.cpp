//실제 finder/Graph/대피 선택의 독립 원문258행을 같은 물리 입력으로 재생한다.
#include "TestSupport.h"
#include "o/RawBridgeCarriers.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <string>

namespace { //이 장면은 원문 입력만 구성하고 기대 이동점/사건은 동결 TSV에서 읽는다.
using namespace netstorm::o;
//원문의 field 폭으로만 little endian 입력을 설치한다.
void RetreatPut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) { for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8)); } //지정 필드 바이트만 쓴다.
//원문과 C++ 결과의 raw 네 슬롯을 바이트별 비교하기 위한 읽기다.
std::uint32_t RetreatRead(std::span<const std::uint8_t> raw,std::size_t offset) { std::uint32_t value{};for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value; } //낮은 바이트의 원래 위치를 보존한다.
//검사 입력 소유자는 non-const이며 기존 할당/반납 정책을 거치지 않는 명시 raw fixture다.
std::span<std::uint8_t> RetreatSlot(SidPool& pool,std::uint16_t sid) { const auto bytes=pool.Bytes();return std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size()).subspan(static_cast<std::size_t>(sid)*50U,50U); }
//TSV와 원문 입력 열의 순서를 보존한다.
std::vector<std::string> RetreatFields(const std::string& value,char separator) { std::stringstream stream(value);std::vector<std::string> fields;std::string part;while (std::getline(stream,part,separator)) fields.push_back(part);return fields; } //모든 명시 열을 생략 없이 읽는다.
//바이트 원문의 두자리 hex를 비교용 벡터로만 해석한다.
std::vector<std::uint8_t> RetreatHex(const std::string& text) { std::vector<std::uint8_t> bytes;for (std::size_t i=0;i<text.size();i+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i,2),nullptr,16)));return bytes; } //기대 의미값을 계산하지 않는다.
}

//두 독립 x87 정밀도가 같았던 전체 행의 실제 후보 순서/이동 요청/거부/50바이트를 검사한다.
TEST_CASE(BridgeCarrierRetreat_IndependentNative258AndNestedFinders) {
    std::ifstream file(NETSTORM_BRIDGE_CARRIER_RETREAT_FIXTURE);CHECK(bool(file));std::string line;std::size_t rows{};
    while (std::getline(file,line)) { //원문을 새 생산 몸체의 결과로 다시 작성하거나 행을 생략하지 않는다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto columns=RetreatFields(line,'\t');const auto fields=RetreatFields(columns[0],',');std::vector<std::uint32_t> c;for (const auto& value:fields) c.push_back(static_cast<std::uint32_t>(std::stoul(value))); //명시 입력만 정수로 읽는다.
        CHECK(columns.size()==3 && c.size()==13);SidPool pool(OriginalEdition::Patch1078,24000);SquidHash hash;std::vector<RiftTypeRecord> types(188);
        const std::array<std::array<float,4>,6> positions{{{22,21,20,23},{21,21,20,22},{21,22,19,20},{22,21,18,21},{20.125F,21,20,22.125F},{20,21,22,23}}}; //원문 생성기와 같은 명시 위치 입력이며 선택점은 계산하지 않는다.
        const auto points=positions[c[4]];std::array<std::uint16_t,4> ids{50,60,61,62};if (c[10]) std::swap(ids[2],ids[3]);
        for (const auto id:ids) { //동일 타입/상태/좌표/바이트를 설치한 뒤 같은 순서로 해시 머리에 삽입한다.
            auto raw=RetreatSlot(pool,id);std::fill(raw.begin(),raw.end(),std::uint8_t{});RetreatPut(raw,0,0x11018000);
            const auto kind=id==50 ? 82U : id==60 ? c[2] : id==61 ? 70U : 71U;raw[10]=static_cast<std::uint8_t>(kind);types[kind].footX=types[kind].footY=1;
            types[kind].flags1=id==60 ? 0U : id==61 ? c[6] : 0x800U;types[kind].flags2=id==60 ? 0x10000U : id==62 ? 2U : 4U;
            const float x=id==50 ? 20.0F : id==60 ? std::bit_cast<float>(c[11]) : id==61 ? points[0] : points[2],y=id==50 ? 21.0F : id==60 ? std::bit_cast<float>(c[12]) : id==61 ? points[1] : points[3];RetreatPut(raw,14,std::bit_cast<std::uint32_t>(x));RetreatPut(raw,18,std::bit_cast<std::uint32_t>(y));raw[30]=static_cast<std::uint8_t>(id==60 ? 254U : id==61 ? c[7] : 1U);RetreatPut(raw,12,id==50 ? 40U : id==61 ? c[5]*8U : 0U,2);
            auto& head=hash.Bucket(id==60 ? 1 : 0,x,y);RetreatPut(raw,4,head,2);head=id;
        }
        const bool boss=c[1]!=0,checking=c[9]!=0;const std::uint32_t priest=158,rainWalker=156;const std::int32_t avoid=7;std::string events;
        const auto event=[&](std::string text) { if (!events.empty()) events+=';';events+=text; }; //원문이 기록한 외부 계약의 호출 순서만 잇는다.
        BridgeCarrierHooks hooks;
        hooks.graphFor=[&](Sid sid) { const auto raw=pool.Slot(sid);if (types[raw[10]].flags1&0x800U) return std::uint32_t(raw[30]);const auto x=static_cast<int>(static_cast<double>(std::bit_cast<float>(RetreatRead(raw,14)))+static_cast<double>(0.9999F)),y=static_cast<int>(static_cast<double>(std::bit_cast<float>(RetreatRead(raw,18)))+static_cast<double>(0.9999F));const auto head=hash.Cell(0,x,y);if (head && !(types[pool.Slot(Sid{head})[10]].flags1&0x800U)) event("A:1849");return head ? std::uint32_t(pool.Slot(Sid{head})[30]) : 254U; }; //이전 Graph 모듈과 같은 명시 해시 입력을 공급하며 기대 선택을 계산하지 않는다.
        hooks.movementType=[&](std::uint32_t type) { event("T:"+std::to_string(type)+":0:0");return 46U; };
        hooks.movementForm=[&](Sid sid,std::uint32_t type) { event("P:"+std::to_string(sid.value)+":"+std::to_string(type)+":0:0");return Sid{static_cast<std::uint16_t>(c[3] ? 900 : 0)}; };
        hooks.start=[&](Sid sid,RawPathPoint point) { event("M:"+std::to_string(sid.value)+":"+std::to_string(std::bit_cast<std::uint32_t>(point.x))+":"+std::to_string(std::bit_cast<std::uint32_t>(point.y))+":0:0:0:0:0");return c[8]!=0; };
        hooks.valid=[](RawPathPoint point) { return point.x>0 && point.y>0 && point.x<256 && point.y<256; };
        hooks.assertion=[&](std::uint32_t at) { event("A:"+std::to_string(at)); };
        hooks.wontWalkOffEdge=[](std::uint32_t type) { return type==158 || type==156 || type==137 || type==135 || type==187; };
        hooks.scan=[&](SquidSearchArea area) { event("F:"+std::to_string(static_cast<std::uint32_t>(area.left))+":"+std::to_string(static_cast<std::uint32_t>(area.top))+":"+std::to_string(static_cast<std::uint32_t>(area.right))+":"+std::to_string(static_cast<std::uint32_t>(area.bottom))); };
        RawBridgeCarriers carriers(pool,hash,types,boss,checking,priest,rainWalker,avoid,std::move(hooks));if (c[0]) carriers.Check(Sid{50});else carriers.Retreat(Sid{60});
        bool same=events==(columns[1]=="-" ? "" : columns[1]);const auto wanted=RetreatFields(columns[2],',');const std::array<std::uint16_t,4> order{50,60,61,62};
        for (std::size_t index=0;index<4;++index) { const auto bytes=RetreatHex(wanted[index]);same=same && std::equal(bytes.begin(),bytes.end(),pool.Slot(Sid{order[index]}).begin()); } //원문이 관측한 네 전체 슬롯도 바이트로 보존 비교한다.
        CHECK(same);if (!same) { std::fprintf(stderr,"다리 대피 원문 행%zu %s events=%s / %s\n",rows,columns[0].c_str(),events.c_str(),columns[1].c_str());break; }++rows;
    }
    CHECK(rows==258);
}
