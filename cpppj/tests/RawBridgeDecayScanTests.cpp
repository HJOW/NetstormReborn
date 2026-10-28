// 독립 원문73입력과 실제 raw 슬롯 변경/칸 예외로 붕괴 스캔 경계를 확인한다.
#include "TestSupport.h"
#include "o/RawBridgeDecayScan.h"
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { //기대 방문/커서/기한은 구현 계산 대신 동결한 원문 TSV에서 읽는다.
using namespace netstorm::o;
//할당 API를 대신하지 않고 스캔이 읽는 예약/전체 물리 슬롯도 입력할 수 있게 한다.
std::span<std::uint8_t> ScanSlot(SidPool& pool,std::uint16_t sid) {
    const auto bytes=pool.Bytes(); //실제 소유자는 non-const 검사 입력 풀이다. 공개 읽기 뷰로 물리 fixture만 설치한다.
    return std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size()).subspan(static_cast<std::size_t>(sid)*pool.Layout().stride,pool.Layout().stride);
}
//원문 TSV의 마지막 빈 열도 보존하는 일반 문자 분리다.
std::vector<std::string> ScanFields(const std::string& line,char delimiter='\t') {
    std::vector<std::string> result;std::stringstream input(line);std::string value;
    while (std::getline(input,value,delimiter)) result.push_back(value); //열의 저장 순서만 읽으며 기대값을 계산하지 않는다.
    return result;
}
//동일한 실제 풀/TYPE를 매 입력 새로 설치하고 free/dead/void/abstract/buried를 분리한다.
void ScanScene(SidPool& pool,std::span<RiftTypeRecord> types) {
    const std::array<std::uint8_t,7> states{0,1,2,4,0,0,0},extras{0,0,0,0,1,8,0};
    for (std::uint16_t index=0;index<7;++index) { //일곱째는 상태는정상이어도 bridge genus가 없는 타입이다.
        auto raw=ScanSlot(pool,static_cast<std::uint16_t>(15000+index));raw[10]=static_cast<std::uint8_t>(70+index);raw[11]=states[index];raw[40]=extras[index];types[70+index].flags2=index==6 ? 0U : TypeFlag2::kBridge;
    }
}
}

//기본53비트와 독립64비트가 같았던 모든73행의 호출 순서·가변 범위·기한 비트를 확인한다.
TEST_CASE(RawBridgeDecayScan_NativeCurrentPeriodAndLiveGlobals) {
    std::ifstream input(NETSTORM_BRIDGE_DECAY_SCAN_RUNTIME_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{};
    while (std::getline(input,line)) { //주석 이외의 모든 원문 관측을 빠짐없이 소비한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto v=ScanFields(line);CHECK(v.size()==17);if (v.size()!=17) continue;
        const auto integer=[&](std::size_t column) { return static_cast<std::int32_t>(std::stoll(v[column])); }; //signed DWORD 입력을 보존한다.
        const auto real=[&](std::size_t column) { return std::bit_cast<double>(std::stoull(v[column])); }; //단정도/문자열 반올림 없이 실제 double 비트를 읽는다.
        SidPool pool(OriginalEdition::Patch1078,24000,true);std::vector<RiftTypeRecord> types(188);ScanScene(pool,types);
        float period=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(v[1])));BridgeDecayScanState state{integer(2),integer(3),integer(4),real(5)};
        BridgeDecayMode mode;mode.editor=integer(9)!=0;mode.authority=integer(10)!=0;FrameTime time{real(6),0.0,real(7),0};std::vector<std::uint16_t> visits;
        RawBridgeDecayScan scan(pool,types,state,period,mode,[&](Sid sid) {
            visits.push_back(sid.value);if (visits.size()!=1) return;
            switch (integer(11)) { //원문 관측과 같은 명시 외부 효과만 설치한다.
            case 1:period=0.25F;break;
            case 2:state.last=15002;break;
            case 3:state.cursor=15003;break;
            case 4:state.first=15002;state.next=-2.0;break;
            default:break;
            }
        });
        scan.Frame(time,integer(8)!=0);std::vector<std::uint16_t> wanted;
        if (v[12]!="-") for (const auto& sid:ScanFields(v[12],',')) wanted.push_back(static_cast<std::uint16_t>(std::stoul(sid))); //기대SID는 원문의 호출 기록에서만 읽는다.
        CHECK(visits==wanted && state.first==integer(13) && state.last==integer(14) && state.cursor==integer(15) && std::bit_cast<std::uint64_t>(state.next)==std::stoull(v[16]));++rows;
    }
    CHECK(rows==73);
}

//각 칸catch는 후속 실제 슬롯 변경을 되돌리지 않고 남은 후보를 그 시점에 다시 읽는다.
TEST_CASE(RawBridgeDecayScan_CellExceptionAndFollowingSlotMutation) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);std::vector<RiftTypeRecord> types(188);ScanScene(pool,types);
    BridgeDecayScanState state{15000,15006,15000,0.0};float period=10.0F;BridgeDecayMode mode;std::vector<std::uint16_t> visits;
    RawBridgeDecayScan scan(pool,types,state,period,mode,[&](Sid sid) {
        visits.push_back(sid.value);if (sid.value==15000) { ScanSlot(pool,15001)[11]=8;throw std::runtime_error("원문 칸 catch의 외부 실패 입력"); }
    });
    scan.Frame({1.0,1.0,0.0,1},false);CHECK((visits==std::vector<std::uint16_t>{15000,15001,15005}));CHECK(state.cursor==15000 && state.next==11.0 && ScanSlot(pool,15001)[11]==8);
}

//생성/반납은 Native의 명시 리셋과 별개이며 정지/권한/편집의 현재값만 읽는다.
TEST_CASE(RawBridgeDecayScan_PersistentStateAndGates) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);std::vector<RiftTypeRecord> types(188);ScanScene(pool,types);
    BridgeDecayScanState state{15000,15006,15005,0.0};float period=7.75F;BridgeDecayMode mode;std::size_t calls{};
    { RawBridgeDecayScan scan(pool,types,state,period,mode,[&](Sid) { ++calls; });scan.Frame({1.0,1.0,1.0,1},true);mode.editor=true;scan.Frame({1.0,1.0,1.0,1},false);mode.editor=false;mode.authority=false;scan.Frame({1.0,1.0,1.0,1},false);CHECK(!calls && state.cursor==15005 && state.next==0.0);mode.authority=true;scan.Frame({1.0,1.0,1.0,1},false); }
    CHECK(calls==1 && state.cursor==15000 && state.next==8.75 && period==7.75F);
}
