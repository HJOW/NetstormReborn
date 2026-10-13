// 두 원본 PE의 BFS/Graph/접합/전체 작업 배열을 같은 실제 raw 입력으로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPathSearch.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 동일 입력 프레임 목록이며 연결 결과는 실제 Bridge::Connects에 맡긴다.
const std::array<FrameCode,6> kSearchFrames{{{'A','A',1,0},{'A','P',1,0},{'M','A',1,0},{'P','P',1,0},{'K','A',1,0},{'N','A',1,0}}};
// 원본 전체 byte 관찰의 FNV32다.
std::uint32_t SearchFnv(std::span<const std::uint8_t> bytes) {
    std::uint32_t value=2166136261U;
    // 미사용 거리/큐까지 전부 대조한다.
    for (const auto byte:bytes) value=(value^byte)*16777619U;
    return value;
}
// native 입력 장면의 현재 type/frame/Graph/owner/spot를 same hash0 배열에 넣는다.
struct SearchScene {
    // patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool patch;
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;
    // hash: 좌표별 객체 후보를 찾는 공간 해시. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidHash hash;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<RiftTypeRecord> types;
    // frames: 타입별 프레임 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<RiftTypeFrames> frames;
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    RawPathSearchState state;
    // path: 원본 메모리 이미지의 뜻 있는 필드를 보존한다. 호스트 포인터와 원본 vtable 주소를 혼동하지 않는다 (RawPathRuntime).
    RawPathRuntime path;
    // start: 현재 처리 구간의 시작 위치 또는 시작 시각. 입력 구성 또는 실행 결과 대조에 사용한다.
    // destination: 경로에 저장되는 두 float 좌표다. 런타임의 double 좌표와 섞지 않는다 (RawPathPoint).
    RawPathPoint start,destination;
    // 원본 관찰의 parent5/surface6..30을 명시 client 입력으로 확보한다.
    explicit SearchScene(bool chosen):patch(chosen),pool(chosen ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,false),types(chosen ? 188 : 171),
        frames(types.size(),RiftTypeFrames({})),path(pool.Edition()) {
        // 실제 allocated raw 슬롯에만 장면 필드를 공급한다.
        for (std::uint16_t sid=5;sid<32;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        // 독립 코드 표만 제공하고 접합 비트를 미리 계산하지 않는다.
        for (const auto number:{82,83,84}) { types[number].flags1=TypeFlag1::kSurface;types[number].flags2=number==82 ? 4U : number==83 ? 2U : 0x40000U;frames[number]=RiftTypeFrames(std::vector<FrameCode>(kSearchFrames.begin(),kSearchFrames.end())); }
    }
    // 입력 profile은 Python과 같은 셀/상태 공급이며 경로/거리 결과를 만들지 않는다.
    void Setup(int profile,int owner,bool avoid,int policy,bool legacy,int guard) {
        state.avoidFlagTwo=avoid;state.policySelector=policy;state.graphsEnabled=guard!=1;state.searchEnabled=guard!=2;
        const int side=profile==4 ? 1 : profile==5 ? 3 : 5;std::uint16_t next=6;
        // 각 raw 표면과 지도 셀을 같은 순서로 등록한다.
        for (int y=10;y<10+side;++y) {
            // 동일한 row의 모든 현재 후보 필드를 입력으로 공급한다.
            for (int x=10;x<10+side;++x) {
                std::uint32_t type=83,frame=0;std::uint8_t spot=6,flag=0,graph=3;const Sid sid{static_cast<std::uint16_t>(profile==5 ? 6 : next)};
                if (profile==1 && ((x==11 && y==11)||(x==12 && y==12)||(x==13 && y==13))) spot=8;
                if (profile==2 && x==12) spot=0;if (profile==3 && (x+y)%3==0) flag=2;
                if (profile==6 && x==12 && y<14) { type=82;frame=4; }
                if (profile==7 && x==12 && y==12) type=84;
                if (profile==8 && x==14 && y==14) graph=254;if (profile==9 && x==14 && y==14) graph=4;
                if (profile!=5 || next==6) {
                    auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);raw[12]=flag;
                    Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(x)));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(y)));
                    raw[patch ? 30 : 28]=graph;raw[patch ? 34 : 32]=static_cast<std::uint8_t>(owner);Put(raw,patch ? 36 : 34,frame,patch ? 4 : 1);
                }
                hash.Cell(0,x,y)=sid.value;spots[y*256+x]=spot;++next;
            }
        }
        pool.AllocatedBytes(Sid{5})[10]=static_cast<std::uint8_t>(owner%2==0 ? 158 : 134);
        start={9.2F,9.3F};destination={9.2F+side-1,9.3F+side-1};
        if (legacy) { const std::array<RawPathPoint,1> points{{{9,9}}};const std::array<std::int32_t,1> directions{1};const std::array<double,1> times{12.5};path.SetPath(points,directions,times); }
    }
    // 원본 관찰의 꼭짓점/방향/QWORD 시각을 같은 형식으로 만든다.
    std::string Points() const {
        std::string text;
        // 배열 순서로 직접 저장 필드를 조회한다.
        for (std::int32_t i=0;i<path.Count();++i) {
            if (!text.empty()) text+=';';const auto point=path.Point(i);
            text+=std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+std::to_string(std::bit_cast<std::uint32_t>(point.y))+','+std::to_string(path.Direction(i))+','+std::to_string(std::bit_cast<std::uint64_t>(path.Time(i)));
        }
        return text.empty() ? "-" : text;
    }
};
// 전체 signed WORD 거리와 두 BYTE 큐를 little endian 관찰로 비교한다.
void CheckWork(const RawPathSearchWork& work,const std::vector<std::string>& row) {
    std::vector<std::uint8_t> distance;distance.reserve(131072);std::vector<std::uint8_t> queue;queue.reserve(32768);
    // 거리 WORD의 비트를 독립 byte 관찰로 직렬화한다.
    for (const auto value:work.distance) { const auto bits=std::bit_cast<std::uint16_t>(value);distance.push_back(static_cast<std::uint8_t>(bits));distance.push_back(static_cast<std::uint8_t>(bits>>8)); }
    // 미사용 queue 흔적도 보존되어야 한다.
    for (const auto& point:work.queue) { queue.push_back(point[0]);queue.push_back(point[1]); }
    CHECK(SearchFnv(distance)==Number(row[11]));CHECK(SearchFnv(queue)==Number(row[12]));
}
}
// type/frame/Graph/거리/queue 전체 원본 입력을 실행한다.
TEST_CASE(rawPathSearch_native_bfs_and_compression) {
    const auto fixture=LoadFixture(NETSTORM_PATHSEARCH_FIXTURE);std::size_t count=0;
    // 입력마다 실제 raw 풀/hash0/spot을 새로 준비한다.
    for (const auto& row:fixture.rows) {
        SearchScene scene(row[0]=="originals");scene.Setup(std::stoi(row[1]),std::stoi(row[5]),Number(row[3])!=0,std::stoi(row[4]),Number(row[6])!=0,std::stoi(row[7]));
        RawPathSearch search(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.state);bool result=false;
        if (Number(row[9])!=0) CHECK(Throws([&] { search.Search(scene.path,Sid{5},scene.start,scene.destination,Number(row[2])); }));
        else { result=search.Search(scene.path,Sid{5},scene.start,scene.destination,Number(row[2]));CHECK(result==(Number(row[8])!=0)); }
        CHECK(scene.Points()==row[10]);CheckWork(search.Work(),row);CHECK(scene.state.directionPrerequisite[3]==Number(row[13]));++count;
    }
    CHECK(count==644);
}
// 실제 원본 표면 alias를 바꾸면 후속 Graph/경로가 즉시 바뀌고 별도 사본은 남지 않는다.
TEST_CASE(rawPathSearch_current_surface_alias) {
    SearchScene scene(true);scene.Setup(0,2,true,6,false,0);RawPathSearch search(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.state);
    CHECK(search.SurfaceAt(scene.start)==Sid{6});CHECK(search.GraphAt(scene.start)==std::uint8_t{3});
    scene.hash.Cell(0,10,10)=0;CHECK(search.SurfaceAt(scene.start)==Sid{});CHECK(!search.GraphAt(scene.start));CHECK(!search.Search(scene.path,Sid{5},scene.start,scene.destination,0));
    CHECK(scene.path.Count()==0);
}
