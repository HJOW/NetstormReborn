// 독립 PE의 현재 관문·실제 Canon 범위·진단 뒤 상태를 모두 대조한다.
#include "RawSceneSupport.h"
#include "o/RawNuggetReplenish.h"

using namespace netstorm::test::rawscene;
namespace {
// DWORD 비트/전역을 원문과 같은 순서로 기록한다.
std::string Words(std::span<const std::uint32_t> values,char separator=',') {
    std::string result;
    // 모든 필드를 빠뜨리지 않고 직렬화한다.
    for (const auto value:values) { if (!result.empty()) result+=separator;result+=std::to_string(value); }return result;
}
// 원문 내부 진입 표식은 별도 native 집계이며 외부 효과 비교에서만 제외한다.
std::string Effects(std::string_view native) {
    std::string result;
    // 실제 진단/배치 기대값은 fixture에서만 읽는다.
    for (const auto& part:Split(native,';')) if (part!="R" && part!="O" && part!="C" && part!="-") { if (!result.empty()) result+=';';result+=part; }
    return result.empty() ? "-" : result;
}
}

// 전체 패턴/회전·폭 감김과 현재 전역 재독을 새 독립 원문으로 검증한다.
TEST_CASE(NuggetReplenish_NativeParentAndCanon) {
    std::size_t count{}; // 원문 입력 행 수다.
    // C++에서 기대 사각형을 계산하지 않고 같은 입력 자료만 준비한다.
    for (const auto& row:LoadFixture(NETSTORM_NUGGET_REPLENISH_FIXTURE).rows) {
        CHECK(row.size()==5);if (row.size()!=5) continue;const auto input=Split(row[0],',');CHECK(input.size()==11);if (input.size()!=11) continue;
        CHECK(row[1]==row[3] && row[2]==row[4]);std::array<std::uint32_t,11> args{}; // 실제 원문 정수 입력이다.
        // 입력은 원문 순서대로 변환한다.
        for (std::size_t index=0;index<args.size();++index) args[index]=Number(input[index]);
        bool boss=args[0]!=0,single=args[1]!=0,fort=args[2]!=0,battle=true,mission=true,metamap=false,changed=false;std::uint32_t editor=args[3],piece=107;std::int32_t span=std::bit_cast<std::int32_t>(args[4]);std::uint8_t mapSize{}; // 현재 공유 전역이다.
        SidPool pool(OriginalEdition::Patch1078,32768);ChunkMap chunks;std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<std::uint8_t> players(9*0xb4); // 같은 실제 Canon/지역 조회 자료다.
        constexpr std::array<std::pair<int,int>,4> feet{{{1,1},{2,3},{0,1},{-1,2}}};const auto [footX,footY]=feet.at(args[7]);std::vector<FrameCode> codes; // 원문과 동일한 현재 TYPE 발자국이다.
        if (!args[8]) {
            // 모든 방향/셀 번호를 명시 입력한다.
            for (std::uint8_t side='A';side<='P';++side) {
                // 실제 프레임 getter가 검색할 입력 번호다.
                for (std::uint8_t number=0;number<11;++number) codes.push_back({side,'P',number,0});
            }
        }
        // 진단 뒤 선택할 타입도 같은 입력을 가진다.
        for (const auto type:{107,108}) { types[type].footX=footX;types[type].footY=footY;frames[type]=RiftTypeFrames(codes); }
        // 원본 준비의 기본 지도 전체를 동일한 stale record로 초기화한다.
        for (int x=0;x<16;++x) {
            // 실제 검색은 y 바깥/x 안쪽이며 여기서는 자료만 쓴다.
            for (int y=0;y<16;++y) { chunks.At(x,y).island=127;chunks.At(x,y).record={31,41,59,26,53,58}; }
        }
        if (args[9]!=2) { chunks.At(2,3).island=0;chunks.At(2,3).record={static_cast<std::uint8_t>(args[5]|(args[6]<<6)),0xa5,0x32,17,29,41}; }
        if (args[9]==1) { chunks.At(3,0).island=0;chunks.At(3,0).record={static_cast<std::uint8_t>(((args[5]+7)&63U)|(args[6]<<6)),0xa5,0xff,17,29,41}; }
        const std::array<std::uint32_t,9> directions{0,4,0,6,2,7,3,5,1};std::string events; // 기존 배치의 현재 방향표와 외부 효과다.
        const auto event=[&](std::string text) { if (!events.empty()) events+=';';events+=text; }; // 원문과 같은 외부 순서를 기록한다.
        RawPlayerGeyserBounds bounds(pool,chunks,types,frames,players,{battle,mission,metamap,mapSize,piece,directions},[&](PlayerGeyserBoundsAssertion assertion) {
            CHECK(assertion==PlayerGeyserBoundsAssertion::MissingFrame);event("A:299");if (changed) return;changed=true;
            if (args[10]==6) span=0x8000000;else if (args[10]==7) { types[107].footX=2;types[107].footY=3;piece=108; }
        });
        NuggetReplenishHooks hooks;hooks.assertion=[&](std::uint32_t line) {
            CHECK(line==0x246);event("A:"+std::to_string(line));if (!args[10]) return;boss=true;
            if (args[10]==2) single=true;else if (args[10]==3) fort=true;else if (args[10]==4) editor=1;else if (args[10]==5) span=0x7fffffff;
        };
        hooks.place=[&](FortRandomRectangle area,std::uint32_t quantity) { const std::array<std::uint32_t,5> values{std::bit_cast<std::uint32_t>(area.left),std::bit_cast<std::uint32_t>(area.top),std::bit_cast<std::uint32_t>(area.right),std::bit_cast<std::uint32_t>(area.bottom),quantity};event("P:"+Words(values,':')); };
        RawNuggetReplenish body(bounds,{boss,single,fort,editor,span},std::move(hooks));body.Run();
        const std::array<std::uint32_t,8> current{boss,single,fort,editor,static_cast<std::uint32_t>(span),piece,static_cast<std::uint32_t>(types[107].footX),static_cast<std::uint32_t>(types[107].footY)}; // 같은 현재 저장소 전체다.
        if ((events.empty() ? "-" : events)!=Effects(row[1]) || Words(current)!=row[2]) { const auto text="너겟 보충 원문 행"+std::to_string(count)+" 입력"+row[0]+" 실제"+events+" 원본"+row[1];netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str()); }++count;
    }
    CHECK(count>350);
}

// 모드 관문에서는 미선택 하위를 요구하지 않고 선택된 미연결 효과는 거부한다.
TEST_CASE(NuggetReplenish_CurrentModesAndRequiredEffects) {
    SidPool pool(OriginalEdition::Patch1078,32768);ChunkMap chunks;std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<std::uint8_t> players(9*0xb4);std::array<std::uint32_t,9> directions{};bool battle=true,mission=true,metamap=false,boss=true,single=true,fort=false;std::uint8_t mapSize{};std::uint32_t piece=107,editor{};std::int32_t span=1; // 현재 상태를 같은 객체에서 변경한다.
    RawPlayerGeyserBounds bounds(pool,chunks,types,frames,players,{battle,mission,metamap,mapSize,piece,directions},[](PlayerGeyserBoundsAssertion) { throw std::logic_error("선택되지 않은 범위 조회"); });RawNuggetReplenish body(bounds,{boss,single,fort,editor,span},{});
    body.Run();single=false;fort=true;body.Run();fort=false;editor=1;body.Run();editor=0;boss=false;CHECK(Throws([&] { body.Run(); }));
    boss=true;CHECK(Throws([&] { body.Run(); })); //선택된 Canon 또는 배치 경계의 누락은 성공으로 처리되지 않는다.
}
