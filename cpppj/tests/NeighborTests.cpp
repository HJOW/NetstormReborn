// flag 0 첫 이웃과 실제 탐색을 사용하는 끝 칸 변환을 원본 기계어 기대값으로 검사한다.
#include "TestSupport.h"
#include "o/RawBridgeEvents.h"
#include "o/RawSquidNeighbors.h"
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include "o/SquidPop.h"
#include "o/SquidDestroyLifecycle.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 기계어 실행기의 입력 번호·가상 표 기록값이다. 호스트 함수 주소로 해석하지 않는다.
constexpr Sid kSource{50},kBorn{60};
constexpr std::uint32_t kBridgeType=82,kVtable=0x11010000,kCapacity=24000;
// 마지막 빈 필드도 보존한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start=0;
    // 구분자가 없을 때 남은 마지막 필드까지 추가한다.
    for (;;) {
        const auto end=text.find(separator,start);
        result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));
        if (end==std::string_view::npos) return result;
        start=end+1;
    }
}
// fixture 숫자 필드를 DWORD로 읽는다.
std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoul(value)); }
// little endian raw 필드를 쓰되 주변 바이트는 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 원본 필드의 폭만큼 낮은 바이트부터 쓴다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 원본 전체 슬롯의 hex 관찰을 바이트마다 비교한다.
bool SameBytes(std::span<const std::uint8_t> bytes,std::string_view hex) {
    if (bytes.size()*2!=hex.size()) return false;
    // 해시 충돌 대신 각 바이트를 직접 확인한다.
    for (std::size_t i=0;i<bytes.size();++i)
        if (bytes[i]!=std::stoul(std::string(hex.substr(i*2,2)),nullptr,16)) return false;
    return true;
}
// 입력된 합성 코드 표를 원본 fixture의 세 변형과 같게 만든다. 판단 결과는 계산하지 않는다.
RiftTypeFrames Frames(int variant) {
    std::vector<FrameCode> codes;
    const std::string letters="JKALMNOJKLBJPPCD";
    // variant별 hard 비트는 bridgeevent 도구와 같은 입력이다.
    for (std::size_t i=0;i<letters.size();++i) {
        const bool hard=variant==1 ? i==0 || i==1 || i==7 || i==8 || i==11 : variant==2 && (i==0 || i==8);
        const auto flags=static_cast<std::uint8_t>(hard ? 0x40 : letters[i]=='L' || letters[i]=='M' ? 0x20 : 0);
        codes.push_back({static_cast<std::uint8_t>(letters[i]),'P',static_cast<std::uint8_t>(i+1),flags});
    }
    return RiftTypeFrames(std::move(codes));
}
// 원본 PE/Python 없이 저장된 독립 기계어 출력만 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_NEIGHBOR_FIXTURE);
        if (!input) throw std::runtime_error("첫 이웃 fixture 없음");
        std::vector<std::vector<std::string>> result;std::string line;
        // 주석을 제외하고 탭으로 입력/관찰 칸을 나눈다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();return rows;
}
// 명시적인 보호 경로를 확인한다.
template<class F> bool Throws(F operation) { try { operation(); } catch (const std::exception&) { return true; } return false; }
struct Scene {
    SidPool pool;
    SquidHash hash;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::vector<RiftTypeRecord> types;
    std::vector<RiftTypeFrames> frames;
    std::unique_ptr<RawSquidNeighbors> neighbors;
    BridgeEventState state;
    std::vector<std::string> events;
    // 동일한 풀 번호 5~74를 확보하며 원본 타입/프레임 입력은 행별로 준비한다.
    explicit Scene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({})) {
        // 예측/서버 목록과 무관한 client 번호를 입력 슬롯으로 확보한다.
        for (std::uint16_t id=5;id<=74;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 각 입력의 raw 슬롯·타입·실제 네 단계 체인을 준비한다.
    void Prepare(std::string_view nodes,std::string_view initialSpots,int variant) {
        hash.Reset();std::fill(spots.begin(),spots.end(),std::uint8_t{});events.clear();
        // 두 관찰 슬롯의 stale 바이트도 제거한다.
        for (Sid sid:{kSource,kBorn}) { auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        // 각 node는 분석 도구에 넣은 12개 정수 필드다.
        for (const auto& entry:Split(nodes,';')) {
            const auto n=Split(entry,',');CHECK(n.size()==12);
            const Sid sid{static_cast<std::uint16_t>(Number(n[0]))};const auto typ=Number(n[1]);
            auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(typ);raw[11]=static_cast<std::uint8_t>(Number(n[2]));
            const bool patch=pool.Edition()==OriginalEdition::Patch1078;
            raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(n[3]));
            Put(raw,14,Number(n[4]));Put(raw,18,Number(n[5]));
            Put(raw,patch ? 36 : 34,Number(n[10]),patch ? 4 : 1);
            auto& type=types.at(typ);type.footX=std::stoi(n[6]);type.footY=std::stoi(n[7]);type.flags1=Number(n[8]);type.flags2=Number(n[9]);
            frames.at(typ)=Frames(variant);
            auto& head=hash.Bucket(std::stoi(n[11]),std::bit_cast<float>(Number(n[4])),std::bit_cast<float>(Number(n[5])));
            Put(raw,4,head,2);head=sid.value;
        }
        if (initialSpots!="-") {
            // 기준점/발자국의 내부 비트 입력만 적용한다.
            for (const auto& entry:Split(initialSpots,';')) {
                const auto n=Split(entry,',');spots.at(static_cast<std::size_t>(std::stoi(n[1])*256+std::stoi(n[0])))=static_cast<std::uint8_t>(Number(n[2]));
            }
        }
        neighbors=std::make_unique<RawSquidNeighbors>(pool,hash,spots,types,frames);
    }
    // 실제 첫 이웃과 프레임만 사용하며 후속 외부 효과는 원본 실행기의 대체와 같은 경계로 기록한다.
    BridgeEventHooks Hooks() {
        BridgeEventHooks hooks;
        hooks.frames=[this](std::uint32_t type) -> const RiftTypeFrames& { return neighbors->Frames(type); };
        hooks.firstNeighbor=[this](Sid sid) {
            const auto raw=pool.Slot(sid);const auto offset=pool.Edition()==OriginalEdition::Patch1078 ? 36 : 34;
            events.push_back("F:"+std::to_string(sid.value)+':'+std::to_string(raw[offset])+":0");
            const auto first=neighbors->First(sid);events.push_back("I:"+std::to_string(first.value));return first;
        };
        hooks.notifySurface=[this](Sid sid) { events.push_back("N:"+std::to_string(sid.value)); };
        hooks.create=[this](std::uint32_t type) {
            events.push_back("C:"+std::to_string(type)+":0");auto raw=pool.AllocatedBytes(kBorn);
            Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=0;return kBorn;
        };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("D:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.setOwner=[this](Sid sid,std::uint8_t owner) { events.push_back("O:"+std::to_string(sid.value)+':'+std::to_string(owner)); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+
                std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags));
        };
        return hooks;
    }
};
// 각 실제 PE의 첫 이웃과 실제 탐색을 넣은 이벤트/슬롯 전체를 재생한다.
void Replay(const char* edition) {
    Scene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::size_t firstCount=0,eventCount=0;
    // 이전 행의 효과를 다음 행에 섞지 않는다.
    for (const auto& row:Fixture()) {
        if (row[1]!=edition) continue;
        if (row[0]=="First") {
            CHECK(row.size()==6);scene.Prepare(row[2],row[3],std::stoi(row[4]));
            CHECK(scene.neighbors->First(kSource).value==Number(row[5]));++firstCount;
        } else {
            CHECK(row.size()==25);scene.Prepare(row[19],row[20],std::stoi(row[6]));
            scene.state.authority=row[3]=="1";scene.state.debugKeep=row[4]=="1";scene.state.bridgeType=Number(row[5]);
            auto raw=scene.pool.AllocatedBytes(kSource);const bool patch=scene.pool.Edition()==OriginalEdition::Patch1078;
            Put(raw,8,Number(row[11]),2);Put(raw,12,Number(row[10]),2);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(Number(row[12]));
            raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[13]));
            RawBridgeEvents bridge(scene.pool,scene.state,scene.Hooks());
            const float result=bridge.Handle(kSource,Number(row[15]),0,std::bit_cast<float>(Number(row[16])));
            std::string events;
            // 모든 원본 효과의 순서와 인자를 한 문자열로 비교한다.
            for (const auto& event:scene.events) { if (!events.empty()) events+=';';events+=event; }
            const bool same=std::to_string(std::bit_cast<std::uint32_t>(result))==row[21] && (events.empty() ? "-" : events)==row[22] &&
                SameBytes(scene.pool.Slot(kSource),row[23]) && SameBytes(scene.pool.Slot(kBorn),row[24]);
            CHECK(same);
            if (!same) {
                std::printf("  %s 입력 %zu: 프레임 %s / 기대 %s / 실제 %s\n",edition,eventCount,row[7].c_str(),row[22].c_str(),events.c_str());
                break;
            }
            ++eventCount;
        }
    }
    CHECK(firstCount==288 && eventCount==288);
}
}
// 세 PE는 같은 시나리오라도 각 실행 파일에서 직접 얻은 관찰을 사용한다.
TEST_CASE(neighbor_patch_x86_first_and_bridge_event) { Replay("originals"); }
// CD 판본의 바이트 프레임/일반 해시 가상 필터 경로도 검사한다.
TEST_CASE(neighbor_cd_x86_first_and_bridge_event) { Replay("originalCD"); }
// 추가 10.37은 별도 PE 입력 SHA를 가진 관찰이다.
TEST_CASE(neighbor_1037_x86_first_and_bridge_event) { Replay("original1037"); }

// 앞선 첫 결과 뒤의 손상된 체인/다른 풀 슬롯을 미리 읽지 않고 조회 전체는 읽기 전용이다.
TEST_CASE(neighbor_first_stops_before_unvisited_damage_and_does_not_mutate) {
    Scene scene(OriginalEdition::Patch1078);
    const std::string nodes="50,82,0,0,1101004800,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0";
    scene.Prepare(nodes,"-",0);
    auto candidate=scene.pool.AllocatedBytes(Sid{70});Put(candidate,4,65535,2);
    // 기준점 20/21의 오른쪽 이웃은 단계 0에서 반환된다. next는 저장되지만 이후 슬롯을 역참조하지 않는다.
    scene.hash.Cell(3,1,1)=65535;
    const std::vector<std::uint8_t> before(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
    CHECK(scene.neighbors->First(kSource)==Sid{70});
    CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
    CHECK(Throws([&] { scene.neighbors->First(Sid{}); }));
    CHECK(Throws([&] { RawSquidNeighbors invalid(scene.pool,scene.hash,{},scene.types,scene.frames); }));
}
