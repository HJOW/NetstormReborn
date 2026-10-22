// 실제 원본 모양/점유/끝점 점수/8개 예산의 독립 전체 바이트를 대조한다.
#include "o/RawAiBridgeSearch.h"
#include "o/RawAiBridgeCandidates.h"
#include "o/RawAiBridgePath.h"
#include "RawSceneSupport.h"
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace {
// 보고서의 실제bridge.type 코드 입력240BYTE이며 모양/점수 결과가 아니다.
constexpr std::string_view kBridgeCodes="4150010041500b2041501440425001004250020042500b2042500c2042501440435001004350020043500b2043500c2043501440445001004450020044500b2044500c2044501440455001004550020045500b2045500c20455014404650010046500b20465014404750010047500b20475014404850010048500b20485014404950010049500b20495014404a5001004a5002004a500b204a500b204a500c204a5014404b5001004b5002004b500b204b500b204b500c204b5014404c5001004c500b204c5014404d5001004d500b204d5014404e5001004e500b204e5014404f5001004f500b204f50144050500000";
// caller 입력/원문 출력을 같은 little endian BYTE로 복원한다.
std::vector<std::uint8_t> Bytes(std::string_view text) {
    std::vector<std::uint8_t> raw;
    for (std::size_t index=0;index<text.size();index+=2) raw.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); //한 쌍마다 실제BYTE 하나만 만든다.
    return raw;
}
// fixture의 XY 두float 비트는 NaN/음수0도 바꾸지 않는다.
RawPathPoint Point(std::string_view text) { const auto raw=Bytes(text);return {std::bit_cast<float>(Get(raw,0)),std::bit_cast<float>(Get(raw,4))}; }
// 임시XY 출력 전체 비트를 원문과 비교한다.
std::string PointHex(RawPathPoint point) { std::array<std::uint8_t,8> raw{};Put(raw,0,std::bit_cast<std::uint32_t>(point.x));Put(raw,4,std::bit_cast<std::uint32_t>(point.y));return Hex(raw); }
// 현재 파일 기준으로 새 독립 원본 fixture만 읽는다.
FixtureData Fixture(const char* name) { const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures"/(std::string("ai-bridge-")+name+"-x86.tsv");return LoadFixture(path.string().c_str()); }
// 결과를 계산하지 않고 현재caller의 모양 프레임/spot 입력만 조립한다.
struct SearchScene {
    RawAiBrainMemory memory{0x55000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //실제본체의 유일 소유자다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //실제0xb08 본체다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); //현재TYPE별 실제 코드 입력이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); //현재 점유BYTE 지도다.
    std::uint32_t bridge{82}; //원본과 같은 bridge 선택이며 타입번호 자체는 코드 결과에 관여하지 않는다.
    GameRandom random; //한 장면의 실제 공용 난수다.
    // 보호 원본에서 추출한 프레임 코드 입력만 구성한다.
    SearchScene() {
        const auto raw=Bytes(kBridgeCodes);std::vector<FrameCode> codes;
        for (std::size_t index=0;index<raw.size();index+=4) codes.push_back({raw[index],raw[index+1],raw[index+2],raw[index+3]}); //실제 코드 순서/중복을 유지한다.
        frames[bridge]=RiftTypeFrames(std::move(codes));
    }
    // 제품과 검사 모두 같은 현재 저장소를 참조한다.
    RawAiBridgeSearch Search() { return {memory,frames,spots,bridge,random}; }
    // 원본 관측에 쓴 지도 입력이며 후보 점수/배치를 만들지 않는다.
    void Spots(std::uint32_t mode,bool search) {
        for (std::uint32_t y=0;y<256;++y) { //행 순서를 원본 flat주소와 같게 한다.
            for (std::uint32_t x=0;x<256;++x) spots[y*256+x]=static_cast<std::uint8_t>(mode==1 ? (search ? 0x80 : 1) : mode==2 ? ((x+3*y)%11==0 ? (search ? 0x40 : 0x80) : 0) : mode==3 && x==50 ? 0x40 : 0); //비대칭 입력으로x/y전도를 검출한다.
        }
    }
};
}
//26개패턴×4회전에서 grid/끝점 여백 두 초기값을 포함한0xbc 전체를 대조한다.
TEST_CASE(AiBridgeSearch_Native208ShapeBuffers) {
    SearchScene scene;std::size_t count{};
    for (const auto& row:Fixture("shape").rows) { //원본 입력만 제품에 공급하며 끝점 기대를 재계산하지 않는다.
        CHECK(row.size()==4);if (row.size()!=4) return;std::array<std::uint8_t,RawAiBridgeSearch::kShapeBytes> raw{};raw.fill(static_cast<std::uint8_t>(Number(row[2])));scene.Search().Shape(Number(row[0]),Number(row[1]),raw);
        const auto same=Hex(raw)==row[3];if (!same) std::printf("다리 모양 원본 차이 행%zu 패턴%s 회전%s\n",count,row[0].c_str(),row[1].c_str());CHECK(same);++count;
    }
    CHECK(count==208);
}
// 실제 decoder 순서와 비대칭 점유/소수 좌표에서 첫 차단 반환을 대조한다.
TEST_CASE(AiBridgeSearch_Native416ClearReturns) {
    SearchScene scene;std::size_t count{};
    for (const auto& row:Fixture("clear").rows) { //모든26개 조각/4회전/4지도 입력을 실제 계산한다.
        CHECK(row.size()==6);if (row.size()!=6) return;scene.Spots(Number(row[4]),false);const AiBridgePlacement placement{{std::bit_cast<float>(Number(row[2])),std::bit_cast<float>(Number(row[3]))},Number(row[0]),Number(row[1])};CHECK(scene.Search().Clear(placement)==(Number(row[5])!=0));++count;
    }
    CHECK(count==416);
}
// x87의X보존/Yfloat저장·signed감김·금지/벌점·미갱신 결과를 대조한다.
TEST_CASE(AiBridgeSearch_Native640EndpointScores) {
    SearchScene scene;std::size_t count{};
    for (const auto& row:Fixture("score").rows) { //shape는 검증된 실제 제품helper로 만들고 독립 점수/출력과 비교한다.
        CHECK(row.size()==10);if (row.size()!=10) return;std::array<std::uint8_t,RawAiBridgeSearch::kShapeBytes> shape{};scene.Search().Shape(Number(row[0]),Number(row[1]),shape);const AiBridgePlacement placement{{std::bit_cast<float>(Number(row[2])),std::bit_cast<float>(Number(row[3]))},Number(row[0]),Number(row[1])};std::array<std::uint8_t,8> fill{};fill.fill(static_cast<std::uint8_t>(Number(row[7])));auto scratch=Point(Hex(fill));
        const auto score=scene.Search().Score(placement,std::bit_cast<std::int32_t>(Number(row[4])),std::bit_cast<std::int32_t>(Number(row[5])),shape,Number(row[6]),scratch);const auto same=static_cast<std::uint32_t>(score)==Number(row[8]) && PointHex(scratch)==row[9];if (!same) std::printf("다리 끝점 원본 차이 행%zu 점수%d/%s XY%s/%s\n",count,score,row[8].c_str(),PointHex(scratch).c_str(),row[9].c_str());CHECK(same);++count;
    }
    CHECK(count==640);
}
//72장면을 각4번 재개하며 본체0xb08/공용난수/임시XY/완료값을 모두 비교한다.
TEST_CASE(AiBridgeSearch_Native288BudgetedFullBodies) {
    std::size_t count{},returns{};
    for (const auto& row:Fixture("search").rows) { //native 입력의 실제 초기 본체를 그대로 설치한다.
        CHECK(row.size()==6);if (row.size()!=6) return;SearchScene scene;scene.Spots(Number(row[1]),true);scene.random.SetState(Number(row[2]));auto scratch=Point(row[3]);const auto body=Bytes(row[4]);std::copy(body.begin(),body.end(),scene.memory.Bytes(scene.token).begin());
        for (const auto& observation:Split(row[5],';')) { //변경된 실제본체/난수를 다음 호출에 이어 쓰며 결과를 덮어주지 않는다.
            const auto expected=Split(observation,',');CHECK(expected.size()==4);if (expected.size()!=4) return;const auto complete=scene.Search().Search(scene.token,scratch);const auto same=complete==(Number(expected[0])!=0) && scene.random.State()==Number(expected[1]) && PointHex(scratch)==expected[2] && Hex(scene.memory.Bytes(scene.token))==expected[3];
            if (!same) std::printf("다리 검색 원본 차이 장면%zu 호출%zu 완료%d/%s 난수%u/%s XY%s/%s\n",count,returns,complete,expected[0].c_str(),scene.random.State(),expected[1].c_str(),PointHex(scratch).c_str(),expected[2].c_str());CHECK(same);++returns;
        }
        ++count;
    }
    CHECK(count==72 && returns==288);
}
// 실제 초기화→24칸후보→8개검색→대기→결과반환을 원본 Step과 함께 수행한다.
TEST_CASE(AiBridgeSearch_ComposesActualPreparationAndStep) {
    SearchScene scene;SidPool pool(OriginalEdition::Patch1078,32768,false);
    for (int index=5;index<=50;++index) CHECK(pool.Allocate(2).value==index); //실제50번 지면 슬롯을 확보한다.
    auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=97;raw[30]=7;raw[34]=2;std::vector<RiftTypeRecord> types(188);types[97].flags1=0x800;scene.frames[97]=RiftTypeFrames({FrameCode{'P','A',0,0}});std::vector<std::uint16_t> surfaces(65536,50);std::uint32_t fencemark{90},windArcher{140};RawAiBridgeCandidates candidates(scene.memory,pool,types,scene.frames,surfaces,scene.spots,{scene.bridge,fencemark,windArcher});
    ContainedFinderState finder;double now{};std::int32_t limit{256};std::uint32_t logKind{},marker{};RawPathPoint scratch{-10000,-10000};AiBridgePathHooks hooks;
    hooks.initialize=[&](auto token,auto source,auto target) { RawAiBridgePath path(scene.memory,pool,finder,{now,limit,scene.bridge,logKind,marker,marker,marker},{});CHECK(path.Initialize(token,source,target)); };
    hooks.prepare=[&](auto token) { return candidates.Prepare(token); };hooks.search=[&](auto token) { return scene.Search().Search(token,scratch); };RawAiBridgePath path(scene.memory,pool,finder,{now,limit,scene.bridge,logKind,marker,marker,marker},std::move(hooks));auto body=scene.memory.Bytes(scene.token);Put(body,0x5f0,1);Put(body,0x5f8,7);Put(body,0x5fc,2);Put(body,0x604,8);Put(body,0x628,8);
    for (std::uint32_t index=0;index<8;++index) Put(body,0x608+index*4,index); //현재 재고8개를 설치하며 결과 조각을 미리 고르지 않는다.
    RawPathPoint source{50,63};bool complete{},pending{};
    for (int call=0;call<100 && !complete;++call) { const auto stage=Get(body,0x5f0);complete=path.Step(scene.token,source,{80,90});if (stage==3 && Get(body,0x5f0)==3) pending=true; } //원본 단계마다한번 처리하며 실제 예산 중단을 확인한다.
    CHECK(complete && pending && Get(body,0x5f0)==0);CHECK(source.x>0 && source.y>0 && (source.x!=50 || source.y!=63));CHECK(Get(body,0x6a8)<65536 && Get(body,0x6b4)<8 && Get(body,0x6b8)<=6);
}
