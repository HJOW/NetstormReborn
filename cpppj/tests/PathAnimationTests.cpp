// 원본 도착 애니메이션 접두를 독립 x86 fixture와 실제 raw 등록/해제 경로로 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPathAnimation.h"
#include "o/SquidFactory.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 실행기에 입력한 위치와 판본마다 저장된 관찰 행 수다.
constexpr float kX=20.75f,kY=21.9f;
constexpr std::size_t kReadRows=608,kStopRows=1408;
struct FixtureData {
    std::map<int,RiftTypeFrames> codes;
    std::vector<std::vector<std::string>> rows;
};
// PE/Python 없이 저장한 프레임 표와 독립 관찰 행을 읽는다.
const FixtureData& Fixture() {
    static const auto data=[] {
        std::ifstream input(NETSTORM_PATHANIMATION_FIXTURE);
        if (!input) throw std::runtime_error("도착 프레임 fixture 없음");
        FixtureData result;std::string line;
        // 탭 구분 레코드를 끝까지 읽으며 입력 표는 별도로 보관한다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (line.empty() || line.front()=='#') continue;
            auto row=Split(line,'\t');
            if (row[0]=="Codes") {
                std::vector<FrameCode> codes;const auto& text=row.at(2);
                CHECK(text.size()%8==0);
                // 네 바이트를 원본 순서로 읽는다. 기대 프레임 번호는 여기서 계산하지 않는다.
                for (std::size_t i=0;i<text.size();i+=8) {
                    const auto byte=[&](std::size_t n) { return static_cast<std::uint8_t>(std::stoul(text.substr(i+n*2,2),nullptr,16)); };
                    codes.push_back({byte(0),byte(1),byte(2),byte(3)});
                }
                result.codes.emplace(std::stoi(row.at(1)),RiftTypeFrames(std::move(codes)));
            } else result.rows.push_back(std::move(row));
        }
        return result;
    }();return data;
}
struct AnimationScene {
    SidPool pool;
    std::vector<RiftTypeFrames> frames;
    std::vector<std::string> events;
    bool patch;
    // 실행기의 객체 번호 50까지 client 슬롯을 확보한다.
    explicit AnimationScene(OriginalEdition edition):pool(edition,kCapacity,false),
        frames(96,RiftTypeFrames({})),patch(edition==OriginalEdition::Patch1078) {
        // 원본 관찰의 SID와 맞춰 가상 사건의 번호도 그대로 비교한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 판본의 프레임 폭과 위치를 구한다.
    std::size_t Offset() const { return patch ? 36 : 34; }
    std::size_t Width() const { return patch ? 4 : 1; }
    // 입력 프레임·extra·소유자와 전체 슬롯을 원본 실행기와 같게 만든다.
    void Prepare(const RiftTypeFrames& codes,std::uint32_t frame,std::uint8_t extra,std::uint8_t owner) {
        frames[kBridgeType]=codes;events.clear();
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,kVtable);raw[10]=kBridgeType;
        Put(raw,14,std::bit_cast<std::uint32_t>(kX));Put(raw,18,std::bit_cast<std::uint32_t>(kY));
        Put(raw,Offset(),frame,Width());raw[patch ? 40 : 35]=extra;raw[patch ? 34 : 32]=owner;
    }
    // 프레임 필드의 비트를 십진수로 기록한다.
    std::string Frame() const { return std::to_string(Get(pool.Slot(kSource),Offset(),Width())); }
    // 가상 Unpop/Pop은 원본 실행기와 같은 호출 기록 경계다.
    PathAnimationHooks Hooks() {
        return {
            [this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)+':'+Frame()); },
            [this](Sid sid,std::uint32_t flags) {
                const auto raw=pool.Slot(sid);
                events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(Get(raw,14))+':'+
                    std::to_string(Get(raw,18))+':'+std::to_string(flags)+':'+Frame());
            }
        };
    }
    // 가상 효과를 순서대로 합쳐 원본 관찰과 비교한다.
    std::string Events() const {
        std::string text;
        // 사건 사이만 세미콜론을 넣는다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text;
    }
};
// 한 판본에서 getter 두 값과 도착 접두의 사건/슬롯 전체를 재생한다.
void Replay(const char* edition) {
    AnimationScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::size_t reads=0,stops=0;int failures=0;
    // 각 기대값은 해당 PE의 실행 결과이며 다른 판본 결과를 복사하지 않는다.
    for (const auto& row:Fixture().rows) {
        if (row.at(1)!=edition) continue;
        const auto& codes=Fixture().codes.at(std::stoi(row.at(2)));const int frame=std::stoi(row.at(3));
        if (row[0]=="Read") {
            CHECK(row.size()==6);
            CHECK(RawPathAnimation::Side(codes,frame)==std::stoi(row[4]));
            CHECK(RawPathAnimation::Direction(codes,frame)==std::stoi(row[5]));++reads;continue;
        }
        CHECK(row.size()==8);
        scene.Prepare(codes,static_cast<std::uint32_t>(frame),static_cast<std::uint8_t>(Number(row[4])),static_cast<std::uint8_t>(Number(row[5])));
        RawPathAnimation animation(scene.pool,scene.frames,scene.Hooks());animation.Stop(kSource);
        const bool same=scene.Events()==row[6] && Hex(scene.pool.Slot(kSource))==row[7];CHECK(same);
        if (!same) {
            std::printf("  %s 도착 접두 행 %zu: 기대 %s / 실제 %s\n",edition,stops,row[6].c_str(),scene.Events().c_str());
            if (++failures>=6) break;
        }
        ++stops;
    }
    CHECK(failures==0 && reads==kReadRows && stops==kStopRows);
}
}
// 패치는 DWORD 프레임과 signed side를 사용한다.
TEST_CASE(path_animation_patch_x86_reads_and_arrival_prefix) { Replay("originals"); }
// CD의 BYTE 넘침과 인접 extra 보존을 포함한다.
TEST_CASE(path_animation_cd_x86_reads_and_arrival_prefix) { Replay("originalCD"); }
// 10.37도 자신의 별도 PE 관찰과 비교한다.
TEST_CASE(path_animation_1037_x86_reads_and_arrival_prefix) { Replay("original1037"); }

// 원본의 배열 밖 메모리 읽기는 호스트에서 가상 효과/슬롯 변경 전 예외로 거부한다.
TEST_CASE(path_animation_rejects_invalid_inputs_before_effects) {
    AnimationScene scene(OriginalEdition::Patch1078);const auto& codes=Fixture().codes.at(0);
    CHECK(Throws([&] { RawPathAnimation missing(scene.pool,scene.frames,{}); }));
    auto hooks=scene.Hooks();hooks.repop={};CHECK(Throws([&] { RawPathAnimation missing(scene.pool,scene.frames,hooks); }));
    scene.Prepare(codes,0,0,3);RawPathAnimation animation(scene.pool,scene.frames,scene.Hooks());
    const auto reject=[&](Sid sid) {
        const auto before=Hex(scene.pool.Slot(kSource));
        CHECK(Throws([&] { animation.Stop(sid); }));CHECK(scene.events.empty() && Hex(scene.pool.Slot(kSource))==before);
    };
    // 타입/음수 프레임/배열 끝/예약/free SID 각각을 부작용 없이 거부한다.
    for (const std::uint8_t type:{std::uint8_t{69},std::uint8_t{96},std::uint8_t{255}}) {
        scene.pool.AllocatedBytes(kSource)[10]=type;reject(kSource);
    }
    scene.pool.AllocatedBytes(kSource)[10]=kBridgeType;
    Put(scene.pool.AllocatedBytes(kSource),scene.Offset(),0xffffffffU);reject(kSource);
    Put(scene.pool.AllocatedBytes(kSource),scene.Offset(),64);reject(kSource);
    Put(scene.pool.AllocatedBytes(kSource),scene.Offset(),0);reject(Sid{0});reject(Sid{51});
    CHECK(Throws([&] { RawPathAnimation::Side(codes,-1); }));
    CHECK(Throws([&] { RawPathAnimation::Direction(codes,64); }));
    CHECK(Throws([&] { RawPathAnimation::RestFrame(Fixture().codes.at(3),0); }));
}

// 실제 일반 Unpop/Pop은 프레임 변경 전 해제를 마치고 새 크기의 해시 단계로 재등록해야 한다.
TEST_CASE(path_animation_arrival_integrates_raw_unpop_pop_and_repeated_stop) {
    // 콘솔 검사 두 판본에서 동일한 정상 등록 흐름을 수행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;
        // 일반 생성/등록을 지원하는 실제 bridgeConnector 타입 번호다. 사제 전용 훅은 이 검사의 범위 밖이다.
        constexpr std::uint32_t type=155;
        // 패치/CD의 프레임 필드와 해시 단계 바이트 위치다.
        const std::size_t offset=patch ? 36 : 34,level=patch ? 33 : 31;
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));
        types[type].constructorAddress=TypeConstructorAddress(edition,type);
        types[type].flags1=0x28000002;types[type].flags2=0x02000000;types[type].footX=types[type].footY=1;
        frames[type]=RiftTypeFrames({{'A','P',0,0},{'A','P',4,0},{'A','P',1,0}});
        SquidUnpop unpop(pool,hash,spots);SquidPop pop(pool,hash,spots);SquidFactory factory(pool,types,false,&unpop);
        const auto sid=factory.Create(type);
        CHECK(pop.Pop(sid,types[type],1,1,kX,kY)==RawPopResult::Registered);
        std::vector<std::string> order;
        RawPathAnimation animation(pool,frames,{
            [&](Sid object,std::uint32_t flags) {
                order.push_back("unpop");CHECK(flags==0);unpop.Unpop(object,types[type],flags);
                CHECK(hash.Bucket(static_cast<int>(pool.Slot(object)[level]),kX,kY)==0);
            },
            [&](Sid object,std::uint32_t flags) {
                order.push_back("repop");CHECK(flags==0x40 && Get(pool.Slot(object),offset,patch ? 4 : 1)==1);
                CHECK(pop.Pop(object,types[type],3,4,kX,kY,flags)==RawPopResult::Registered);
            }
        });
        animation.Stop(sid);
        CHECK(order==std::vector<std::string>({"unpop","repop"}));
        CHECK(pool.Slot(sid)[level]==2 && hash.Bucket(1,kX,kY)==0 && hash.Bucket(2,kX,kY)==sid.value);
        CHECK((pool.Slot(sid)[11]&4)==0 && std::bit_cast<float>(Get(pool.Slot(sid),14))==kX && std::bit_cast<float>(Get(pool.Slot(sid),18))==kY);
        // 원본은 이미 정지 프레임이어도 가상 호출 두 번을 반복한다. 중복 체인은 생기면 안 된다.
        order.clear();animation.Stop(sid);
        CHECK(order==std::vector<std::string>({"unpop","repop"}) && Get(pool.Slot(sid),4,2)==0);
        CHECK(hash.Bucket(2,kX,kY)==sid.value && pool.FreeCount()==kCapacity-6);
    }
}
