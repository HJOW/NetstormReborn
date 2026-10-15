// 실제 PE의 현재 청크/Player/Canon 배치 출력16BYTE와 진단 효과 순서를 대조한다.
#include "ConstructionSupport.h"
#include "o/RawPlayerGeyserBounds.h"
#include <utility>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
constexpr std::uint32_t kPlayerBoundsCases=439; // 중복 제거한 신규 실제 원본 입력 수다.
// 기대 사각형을 계산하지 않고 동일 record6BYTE만 공급한다.
std::array<std::uint8_t,6> BoundsRecord(const std::vector<std::string>& row,bool alternate=false) {
    return {static_cast<std::uint8_t>(((Number(row[2])+(alternate ? 7U : 0U))&63U)|(Number(row[3])<<6)),0xa5,
        static_cast<std::uint8_t>(Number(row[4])^(alternate ? 0x55U : 0U)),17,29,41};
}
// 같은 유효 ChunkMap의 명시 크기다.
std::pair<int,int> BoundsDimensions(std::uint32_t profile) {
    constexpr std::array<std::pair<int,int>,3> dimensions{{{16,16},{3,4},{1,1}}}; // 실제 입력 계획의 지도 크기다.
    return dimensions.at(profile);
}
// 실제 원문 검색을 검증할 지도만 입력하며 첫 일치는 모듈이 찾는다.
void BoundsMap(ChunkMap& chunks,const std::vector<std::string>& row) {
    // 전체 물리 항목은 원본과 같은 기본127/stale6BYTE 입력이다.
    for (int x=0;x<16;++x) {
        // 유효 지도 안만 쓰면 clamp에 따른 별도 덮어쓰기도 없다.
        for (int y=0;y<16;++y) if (x<chunks.XLength() && y<chunks.YLength()) {
            chunks.At(x,y).island=127;chunks.At(x,y).record={31,41,59,26,53,58};
        }
    }
    const auto x=std::min(2,chunks.XLength()-1),y=std::min(3,chunks.YLength()-1),layout=static_cast<int>(Number(row[11])); // 원본 입력의 첫 record 위치와 사례 종류다.
    const auto put=[&](int px,int py,std::uint8_t region,bool alternate=false) { auto& entry=chunks.At(px,py);entry.island=region;entry.record=BoundsRecord(row,alternate); }; // 입력 기록의6BYTE만 지정한다.
    if (layout==0) put(x,y,7);
    else if (layout==1) { put(std::min(3,chunks.XLength()-1),0,7);put(0,std::min(3,chunks.YLength()-1),7,true); }
    else if (layout==3) { put(x,y,7);put(0,0,8,true); }
    else if (layout==4) put(x,y,255);else if (layout==5) put(x,y,128);
}
// native와 같은 DWORD 문자열로 출력과 현재 상태를 비교한다.
std::string BoundsWords(std::span<const std::uint32_t> words) {
    std::string result;
    // 실제 모든 출력 항목을 순서대로 기록한다.
    for (const auto word:words) { if (!result.empty()) result+=',';result+=std::to_string(word); }return result;
}
}

// 새 원본439행의 실제 검색/양쪽 배치/원본 Canon과 전체 상태/진단 순서를 검증한다.
TEST_CASE(PlayerGeyserBounds_IndependentPeFixture) {
    std::ifstream input(NETSTORM_PLAYER_GEYSER_BOUNDS_FIXTURE);if (!input) throw std::runtime_error("Player 간헐천 범위 fixture 없음");
    std::string line;std::uint32_t count=0;
    // 사례마다 동일 입력을 독립 저장소에 공급한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=20) throw std::runtime_error("Player 간헐천 범위 fixture 열 수 오류");++count;
        SidPool pool(OriginalEdition::Patch1078,32768);const auto [width,height]=BoundsDimensions(Number(row[12]));ChunkMap chunks(width,height);BoundsMap(chunks,row);
        std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames;frames.reserve(188);
        // 실제 타입 번호에 대응하는 동일 프레임 슬롯을 구성한다.
        for (std::size_t index=0;index<188;++index) frames.emplace_back(std::vector<FrameCode>{});
        constexpr std::array<std::pair<int,int>,4> feet{{{1,1},{2,3},{0,1},{-1,2}}}; // 원본에 공급한 같은 발자국 입력이다.
        const auto [footX,footY]=feet.at(Number(row[13]));std::vector<FrameCode> codes;
        if (Number(row[14])==0) {
            // 현재 방향마다 실제 variation번호0~10을 입력한다.
            for (std::uint8_t side='A';side<='P';++side) {
                // 패턴 셀이 조회하는 번호를 모두 포함한 명시 입력이다.
                for (std::uint8_t number=0;number<11;++number) codes.push_back({side,'P',number,0});
            }
        }
        // 진단 효과가 선택할 타입도 같은 원문 표에 존재한다.
        for (const auto type:{107,108}) { types[type].footX=footX;types[type].footY=footY;frames[type]=RiftTypeFrames(codes); }
        const auto owner=static_cast<std::int32_t>(std::stoll(row[1]));const auto player=owner>=0 && owner<9 ? static_cast<std::uint32_t>(owner) : 1U; // direct배치의 잘못된 번호는 원문주소와 분리한 명시 입력이다.
        std::vector<std::uint8_t> players(9*0xb4);Put(players,player*0xb4+0x50,Number(row[6]),1);Put(players,player*0xb4+0x78,static_cast<std::uint32_t>(std::stoll(row[5])));
        bool battle=Number(row[7])!=0,mission=Number(row[8])!=0,metamap=Number(row[9])!=0,mutated=false;std::uint8_t mapSize=static_cast<std::uint8_t>(Number(row[10]));std::uint32_t piece=107;
        std::array<std::uint32_t,9> directions{0,4,0,6,2,7,3,5,1};std::string trace;const auto mutation=Number(row[15]);
        if (Number(row[16])) {
            // 현재 배치표 자체가 바뀐 명시 입력이며 ROM 기본 순서로 고정하지 않는다.
            for (auto& direction:directions) direction=(direction+2)&7U;
        }
        RawPlayerGeyserBounds body(pool,chunks,types,frames,players,{battle,mission,metamap,mapSize,piece,directions},[&](PlayerGeyserBoundsAssertion kind) {
            if (!trace.empty()) trace+=',';trace+=std::to_string(static_cast<std::uint32_t>(kind));
            if (mutated || !mutation) return;mutated=true;
            if (mutation==1) Put(players,player*0xb4+0x78,8);
            else if (mutation==2) { mission=false;mapSize=3; }
            else if (mutation==3) { mission=true;types[107].footX=3; }
            else if (mutation==4) { types[107].footX=2;types[107].footY=3;piece=108; }
            else if (mutation==5) chunks.At(std::min(2,width-1),std::min(3,height-1)).record=BoundsRecord(row,true);
        });CHECK(&body.Pool()==&pool);CHECK(&body.Map()==&chunks);
        std::array<std::uint8_t,16> output;
        // 부분 record출력의 뒤 stale BYTE도 원본과 같은 값으로 보존한다.
        for (std::size_t index=0;index<4;++index) Put(output,index*4,0xdeadbeef);
        if (row[0]=="B") { const auto area=body.Bounds(static_cast<std::uint32_t>(owner));Put(output,0,Bits(area.left));Put(output,4,Bits(area.top));Put(output,8,Bits(area.right));Put(output,12,Bits(area.bottom)); }
        else if (row[0]=="R") { const auto raw=body.RegionRecord(static_cast<std::int32_t>(std::stoll(row[5])));std::copy(raw.begin(),raw.end(),output.begin()); }
        else if (row[0]=="P") { const auto area=body.Placement(owner,BoundsRecord(row));Put(output,0,static_cast<std::uint32_t>(area.left));Put(output,4,static_cast<std::uint32_t>(area.top));Put(output,8,static_cast<std::uint32_t>(area.right));Put(output,12,static_cast<std::uint32_t>(area.bottom)); }
        else throw std::runtime_error("Player 간헐천 범위 fixture 연산 오류");
        const std::array<std::uint32_t,4> actual{Get(output,0),Get(output,4),Get(output,8),Get(output,12)}; // 실제 전체 출력16BYTE다.
        const std::array<std::uint32_t,8> current{battle,mission,metamap,mapSize,piece,static_cast<std::uint32_t>(types[107].footX),static_cast<std::uint32_t>(types[107].footY),Get(players,player*0xb4+0x78)}; // 원본 진단 효과 뒤의 현재 저장소다.
        CHECK(BoundsWords(actual)==row[17]);CHECK(BoundsWords(current)==row[18]);CHECK((trace.empty() ? "-" : trace)==row[19]);
    }
    CHECK(count==kPlayerBoundsCases);
}

// 필수 실제 진단이 없으면 손상 모드/프레임을 조용한 성공으로 바꾸지 않는다.
TEST_CASE(PlayerGeyserBounds_RejectsMissingAssertion) {
    SidPool pool(OriginalEdition::Patch1078,32768);ChunkMap chunks;std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames;
    // 현재 판본 전체의 프레임 슬롯을 공급한다.
    for (std::size_t index=0;index<188;++index) frames.emplace_back(std::vector<FrameCode>{});
    std::vector<std::uint8_t> players(9*0xb4);bool battle=true,mission=true,metamap=false;std::uint8_t size=0;std::uint32_t piece=107;std::array<std::uint32_t,9> directions{};bool rejected=false;
    try { const RawPlayerGeyserBounds body(pool,chunks,types,frames,players,{battle,mission,metamap,size,piece,directions},{}); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
