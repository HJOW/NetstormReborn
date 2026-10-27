// 두 원본 PE의 BFS/Graph/접합/전체 작업 배열을 같은 실제 raw 입력으로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPathSearch.h"
#include "o/RawPathProcess.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"

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
// 검색→Pop→Kernel 프레임→도착을 같은 raw 저장소에 연결한다. 부모의 가상 도착/음향만 명시 관찰 경계다.
RawPathProcessHooks SearchProcessEffects(SearchScene& scene,SquidPop& pop,SquidUnpop& unpop,std::vector<std::string>& events) {
    RawPathProcessHooks hooks;
    hooks.unpop=[&](Sid sid,std::uint32_t flags) { events.push_back("unpop");unpop.Unpop(sid,scene.types[scene.pool.Slot(sid)[10]],flags); };
    hooks.pop=[&](Sid sid,RawPathPoint point,std::uint32_t flags) {
        events.push_back("pop");CHECK(pop.Pop(sid,scene.types[scene.pool.Slot(sid)[10]],1,1,point.x,point.y,flags)==RawPopResult::Registered);
    };
    hooks.repop=[&](Sid sid,std::uint32_t flags) {
        const auto raw=scene.pool.Slot(sid);events.push_back("repop");
        CHECK(pop.Pop(sid,scene.types[raw[10]],1,1,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),flags)==RawPopResult::Registered);
    };
    // 도착·계속·효과 요청은 입력 경계의 응답과 순서를 기록하며 게임 부모 몸체 복원으로 세지 않는다.
    hooks.halt=[](Sid) { return 10U; };
    hooks.arrival=[&](Sid,std::uint32_t,std::uint32_t) { events.push_back("arrival");return 1000U; };
    hooks.stepSound=[&](Sid,float,float,std::uint32_t sound) { CHECK(sound==0);events.push_back("sound"); };
    hooks.soundId=[](Sid) { return 0U; };
    hooks.clearBlocked=[](Sid,RawPathPoint,std::uint32_t,std::uint32_t) { throw std::logic_error("이번 경로 입력은 겹침을 만들지 않습니다"); };
    hooks.cleanup=[&](RawPathRuntime&,Sid,std::uint32_t) { events.push_back("cleanup"); };
    return hooks;
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
// 시작 표면0의 자연 반환은 누락 검색 경계보다 앞서며, 표면이 있으면 실제 BFS/flags/current에 연결한다.
TEST_CASE(rawPathSearch_init_uses_current_surface_before_search) {
    SearchScene scene(true);scene.Setup(0,1,true,6,false,0);const Sid parent{5};
    Put(scene.pool.AllocatedBytes(parent),0,0x501dd0);SquidPop pop(scene.pool,scene.hash,scene.spots);SquidUnpop unpop(scene.pool,scene.hash,scene.spots);
    SquidProcessState clock;RawPathProcessState state{clock};std::vector<SquidDisplayShape> shapes(scene.types.size());std::vector<std::string> events;
    auto hooks=SearchProcessEffects(scene,pop,unpop,events);
    RawPathProcess missing(scene.pool,scene.hash,scene.types,scene.frames,shapes,scene.spots,state,hooks,false);
    const std::array<RawPathPoint,1> previous{{{9,9}}};const std::array<std::int32_t,1> direction{0};
    missing.Runtime().SetPath(previous,direction);missing.Runtime().RestoreStep(1,45);
    const auto surface=scene.hash.Cell(0,10,10);scene.hash.Cell(0,10,10)=0;
    missing.InitializePath(parent,scene.destination,scene.start);CHECK(events.empty() && missing.Runtime().Current()==1 && missing.Runtime().Count()==1);
    scene.hash.Cell(0,10,10)=surface;CHECK(Throws([&] { missing.InitializePath(parent,scene.destination,scene.start); }));CHECK(events.empty());
    RawPathSearch search(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.state);
    RawPathProcess process(scene.pool,scene.hash,scene.types,scene.frames,shapes,scene.spots,state,MakePathSearchHooks(scene.pool,search,hooks),false);
    process.Runtime().SetPath(previous,direction);process.Runtime().RestoreStep(1,45);process.InitializePath(parent,scene.destination,scene.start,1);
    CHECK(process.Runtime().Current()==0 && process.Runtime().Count()>2 && events==std::vector<std::string>{"pop"});
    CHECK(!(scene.pool.Slot(parent)[11]&4) && scene.hash.Cell(1,4,4)==parent.value);
    SidPool foreign(OriginalEdition::Patch1078,32768,false);CHECK(Throws([&] { MakePathSearchHooks(foreign,search,hooks); }));
}
// 원본 검색 꼭짓점을 실제 Kernel에서 한 단계씩 진행해 공간 갱신·1000 도착의 Form 삭제까지 같은 저장소를 사용한다.
TEST_CASE(rawPathSearch_kernel_moves_and_arrives_in_same_space) {
    // 두 판본의 stride·Form·시각 계산을 같은 조립 순서로 사용한다.
    for (const bool patch:{true,false}) {
        SearchScene scene(patch);scene.Setup(0,1,true,6,false,0);const Sid parent{5};
        Put(scene.pool.AllocatedBytes(parent),0,patch ? 0x501dd0U : 0x5058d8U);
        SquidPop pop(scene.pool,scene.hash,scene.spots);SquidUnpop unpop(scene.pool,scene.hash,scene.spots);
        RawSquidDestroy destroy(scene.pool,unpop,scene.types);Kernel kernel(32);SquidProcessState clock;RawPathProcessState state{clock};
        SquidProcessHost host(scene.pool,scene.types,kernel,destroy,clock);std::vector<std::string> events;
        std::vector<SquidDisplayShape> shapes(scene.types.size());RawPathSearch search(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.state);
        auto process=std::make_unique<RawPathProcess>(scene.pool,scene.hash,scene.types,scene.frames,shapes,scene.spots,state,
            MakePathSearchHooks(scene.pool,search,SearchProcessEffects(scene,pop,unpop,events)),false);
        auto* path=RawPathProcess::Attach(host,std::move(process),parent,0x10);CHECK(path && kernel.Size()==1);
        path->InitializePath(parent,scene.destination,scene.start);path->Prepare(0,1,90);
        const auto form=path->Form();const auto count=path->Runtime().Count();CHECK(count>=2);const auto destination=path->Runtime().Point(count-1);
        // 한 호출에 한 꼭짓점만 넘는 실제 Step을 도착까지 필요한 횟수 안에서 호출한다.
        for (int step=0;step<count+3 && kernel.Size();++step) { clock.now=100;kernel.RunFrame(); }
        CHECK(kernel.Size()==0 && (scene.pool.Slot(form)[11]&1) && !(scene.pool.Slot(parent)[11]&4));
        CHECK(std::bit_cast<float>(Get(scene.pool.Slot(parent),14))==destination.x && std::bit_cast<float>(Get(scene.pool.Slot(parent),18))==destination.y);
        CHECK(std::find(events.begin(),events.end(),"arrival")!=events.end() && events.back()=="cleanup");
    }
}
// 실제 Init의 표면0/존재·void/live·Graph·flags·기존 경로64개 출력을 가상 Pop 인자까지 대조한다.
TEST_CASE(rawPathSearch_native_initialization_pipeline) {
    const auto fixture=LoadFixture(NETSTORM_PATHINIT_FIXTURE);std::size_t count=0;
    // 기대값은 실제 PE의 Init→표면 조회→BFS 관찰만 사용한다.
    for (const auto& row:fixture.rows) {
        const auto input=Split(row[1],',');CHECK(row.size()==12 && input.size()==5);const bool patch=row[0]=="originals";
        SearchScene scene(patch);const auto profile=Number(input[0]),flags=Number(input[1]);const bool surface=Number(input[2])!=0,voidParent=Number(input[3])!=0,legacy=Number(input[4])!=0;
        scene.Setup(static_cast<int>(profile),1,true,6,legacy,0);const Sid parent{5};auto raw=scene.pool.AllocatedBytes(parent);
        Put(raw,0,patch ? 0x501dd0U : 0x5058d8U);raw[11]=voidParent ? 4 : 0;
        Put(raw,14,std::bit_cast<std::uint32_t>(scene.start.x));Put(raw,18,std::bit_cast<std::uint32_t>(scene.start.y));
        if (!surface) scene.hash.Cell(0,10,10)=0;
        SquidPop pop(scene.pool,scene.hash,scene.spots);SquidUnpop unpop(scene.pool,scene.hash,scene.spots);
        SquidProcessState clock;RawPathProcessState state{clock};std::vector<SquidDisplayShape> shapes(scene.types.size());std::vector<std::string> events;
        RawPathSearch search(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.state);
        auto hooks=SearchProcessEffects(scene,pop,unpop,events);const auto actualPop=hooks.pop;std::string popEvent="-";
        // 명시 관찰 경계도 실제 공간 등록을 수행하며 원본이 넘긴 좌표/flags를 그대로 기록한다.
        hooks.pop=[&,actualPop](Sid sid,RawPathPoint point,std::uint32_t value) {
            popEvent="pop,"+std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+
                std::to_string(std::bit_cast<std::uint32_t>(point.y))+','+std::to_string(value);actualPop(sid,point,value);
        };
        RawPathProcess process(scene.pool,scene.hash,scene.types,scene.frames,shapes,scene.spots,state,MakePathSearchHooks(scene.pool,search,hooks),false);
        process.Runtime()=scene.path;process.Runtime().RestoreStep(legacy ? 1 : 0,0);
        process.InitializePath(parent,scene.destination,scene.start,flags);scene.path=process.Runtime();
        CHECK(popEvent==row[2] && scene.path.Current()==static_cast<std::int32_t>(Number(row[3])) &&
            scene.path.Count()==static_cast<std::int32_t>(Number(row[4])) && raw[11]==Number(row[5]) &&
            Get(raw,14)==Number(row[6]) && Get(raw,18)==Number(row[7]) && scene.Points()==row[8]);
        // 검색하지 않은 작업 배열도0이며, 검색한 배열의 전체 바이트는 native FNV와 같아야 한다.
        const std::vector<std::string> workRow{row[0],"0","0","0","0","0","0","0","0","0","-",row[9],row[10],row[11]};
        CheckWork(search.Work(),workRow);CHECK(scene.state.directionPrerequisite[3]==Number(row[11]));++count;
    }
    CHECK(count==64);
}

// 동일 검색기의 다음 호출이 현재 공유 진단값을 다시 읽고 CD에는 패치 전용 분기를 적용하지 않는다.
TEST_CASE(rawPathSearch_shared_diagnostic_current_value_and_edition) {
    for (const bool patch:{false,true}) { // 같은 현재 주소를 양판본 검색에 연결해 읽기 관문을 구별한다.
        SearchScene scene(patch);scene.Setup(0,2,true,5,false,0);std::uint32_t diagnostic=6;scene.state.sharedPolicySelector=&diagnostic;
        RawPathSearch search(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.state);
        for (const auto value:{6U,5U,1U,0U,0xffffffffU}) { // 초기6·변경5·카메라 관문1·그림자0·비영 전체 비트를 그대로 공급한다.
            diagnostic=value;scene.state.directionPrerequisite[3]=20;search.SearchCells(scene.path,10,10,14,14,false);
            CHECK(scene.state.directionPrerequisite[3]==(patch && value==5 ? 5U : 20U));CHECK(scene.state.policySelector==5);
        }
    }
}
