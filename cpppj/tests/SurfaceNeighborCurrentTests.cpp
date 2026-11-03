// 실제 raw 표면 지도 이웃 순회가 원문 순서와 현재 지도/후보·보관한 원천을 소비하는지 검사한다.
#include "RawSceneSupport.h"
#include "o/RawSquidNeighbors.h"
#include <map>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 합성 raw 입력은 원문 fixture의 배치를 옮기며 기대 이웃을 계산하지 않는다.
// 원문 번호를 실제 할당 SID로 옮겨 예약 슬롯에 쓰지 않는 표면 장면이다.
struct SurfaceWalkScene {
    SidPool pool; // 실제 고정 분할을 만족하는 장면 전용 풀이다.
    SquidHash hash; // flags8이 Next마다 읽는 실제 단계0 표면 지도다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 현재 내부 bit8 필터 입력이다.
    std::vector<RiftTypeRecord> types; // 원문 객체 번호별 합성 타입의 발자국/플래그다.
    std::vector<RiftTypeFrames> frames; // 원문 코드 한 개씩을 타입 번호에 연결한다.
    std::map<std::uint16_t,Sid> ids; // 원문 fixture 번호에서 실제 할당 번호로의 입력 대응이다.
    // 판본별 TYPE 크기를 준비하고 빈 프레임도 같은 개수로 채운다.
    explicit SurfaceWalkScene(OriginalEdition edition):pool(edition,32768,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {
        // 원본 프레임 표가 없는 타입도 배열의 정체성과 번호를 보존한다.
        for (std::size_t index=0;index<types.size();++index) frames.emplace_back(std::vector<FrameCode>{});
    }
    // fixture 한 객체를 할당하고 타입/프레임·현재 raw 필드와 발자국 전체 지도만 작성한다.
    void Add(std::uint16_t id,int x,int y,int width,int height,std::uint32_t flags1,std::uint32_t flags2,
        std::uint8_t side,std::uint8_t variant,bool dead,bool buried) {
        const auto sid=pool.Allocate(2);ids[id]=sid;const auto type=static_cast<std::uint8_t>(70+id);
        types[type].flags1=flags1;types[type].flags2=flags2;types[type].footX=width;types[type].footY=height;
        frames[type]=RiftTypeFrames({{side,variant,1,0}});
        auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[11]=dead ? 2 : 0;
        Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(x)));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(y)));
        raw[pool.Edition()==OriginalEdition::Patch1078 ? 40 : 35]=buried ? 8 : 0;
        // 표면 지도는 원본 입력 준비와 같이 뒤 객체가 앞 객체의 칸을 덮는다.
        for (int row=y-height+1;row<=y;++row)
            // 지도 안의 같은 발자국 행만 입력한다.
            for (int column=x-width+1;column<=x;++column)
                if (column>=0 && row>=0 && column<256 && row<256) hash.Cell(0,column,row)=sid.value;
    }
    // 이웃 반환을 fixture 번호로 되돌려 원본의 순서/중복을 손실 없이 비교한다.
    std::uint16_t Original(Sid sid) const {
        // 실제 할당 번호는 fixture의 한 객체에만 대응한다.
        for (const auto& [id,current]:ids) if (current==sid) return id;
        throw std::logic_error("표면 이웃 결과의 원문 번호가 없습니다");
    }
};
// 쉼표 이웃 목록을 번호 벡터로 읽으며 빈 목록은 그대로 보존한다.
std::vector<std::uint16_t> SurfaceIds(std::string_view text) {
    std::vector<std::uint16_t> result;if (text.empty() || text=="-") return result;
    // 기대 번호의 순서와 중복을 지우지 않는다.
    for (const auto& field:Split(text,',')) result.push_back(static_cast<std::uint16_t>(Number(field)));
    return result;
}
// 실제 Next를 끝까지 호출하며 호스트 무한 순회가 성공으로 기록되지 않게 한다.
std::vector<std::uint16_t> WalkSurface(SurfaceWalkScene& scene,RawSquidNeighborWalk& walk) {
    std::vector<std::uint16_t> result;
    // 원문 지도에서 반환된 현재 번호만 읽고 다음 후보는 Next가 새로 찾는다.
    for (auto sid=walk.Current();sid.value;sid=walk.Next()) {
        if (result.size()>128) throw std::logic_error("표면 이웃 검사 입력의 순회 범위 오류");
        result.push_back(scene.Original(sid));
    }
    CHECK(walk.Next().value==0);return result;
}
}
// 기존 두 원본의 공통125장면을 각 판본의 raw 슬롯/현재 지도에서 다시 대조한다.
TEST_CASE(SurfaceNeighborCurrent_ReplaysExistingNativeFootprintsAndFilters) {
    const auto rows=LoadFixture(NETSTORM_SURFACE_FIXTURE).rows;std::size_t count=0;
    // 판본별 객체 슬롯 폭과 frame 폭을 각각 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        // 원본 생성/가상 필터/Next의 Neighbor 결과만 선택한다.
        for (const auto& row:rows) {
            if (row[0]!="Neighbor") continue;SurfaceWalkScene scene(edition);
            // 입력 객체는 원문 발자국 전체를 지도에 놓으며 결과를 미리 필터하지 않는다.
            for (const auto& entry:Split(row[1],';')) {
                const auto f=Split(entry,',');scene.Add(static_cast<std::uint16_t>(Number(f[0])),std::stoi(f[1]),std::stoi(f[2]),std::stoi(f[3]),std::stoi(f[4]),
                    Number(f[5]),Number(f[6]),static_cast<std::uint8_t>(Number(f[7])),static_cast<std::uint8_t>(Number(f[8])),f[9]=="1",f[10]=="1");
            }
            // spot과 명시 지도 덮어쓰기를 입력 순서대로 적용한다.
            for (std::size_t column=2;column<=3;++column) {
                if (row[column]=="-") continue;
                // 명시 입력은 x/y/원문 번호 또는 spot 비트다.
                for (const auto& entry:Split(row[column],';')) {
                    const auto f=Split(entry,',');const auto value=Number(f[2]);const auto x=std::stoi(f[0]),y=std::stoi(f[1]);
                    if (column==2) scene.spots[static_cast<std::size_t>(y*256+x)]=static_cast<std::uint8_t>(value);
                    else scene.hash.Cell(0,x,y)=value ? scene.ids.at(static_cast<std::uint16_t>(value)).value : 0;
                }
            }
            RawSquidNeighbors neighbors(scene.pool,scene.hash,scene.spots,scene.types,scene.frames);
            RawSquidNeighborWalk walk(neighbors,scene.ids.at(static_cast<std::uint16_t>(Number(row[4]))),8);
            const auto actual=WalkSurface(scene,walk),expected=SurfaceIds(row[5]);
            if (actual!=expected) {
                std::printf("표면 이웃 불일치 edition=%d source=%s objects=%s spots=%s map=%s expected=%s actual=",static_cast<int>(edition),row[4].c_str(),row[1].c_str(),row[2].c_str(),row[3].c_str(),row[5].c_str());
                // 실패한 실제 번호만 출력하여 입력/순회 차이를 추적한다. 기대 목록은 바꾸지 않는다.
                for (auto id:actual) std::printf("%u,",static_cast<unsigned>(id));
                std::printf("\n");
            }
            CHECK(actual==expected);++count;
        }
    }
    CHECK(count==250);
}
// 첫 반환 뒤 현재 지도/후보와 원천 좌표를 바꾸는 독립 원문32행을 그대로 재생한다.
TEST_CASE(SurfaceNeighborCurrent_ReplaysNativeMutationDuringWalk) {
    const auto rows=LoadFixture(NETSTORM_SURFACE_NEIGHBOR_CURRENT_FIXTURE).rows;std::size_t count=0;
    // source 발자국/코드는 생성 시점, 나머지는 Next 시점이라는 원문 경계를 비교한다.
    for (const auto& row:rows) {
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;SurfaceWalkScene scene(edition);
        const std::array<std::array<int,2>,6> coordinates{{{20,20},{20,19},{19,20},{21,20},{20,21},{19,20}}}; // 원문 실행기의 여섯 입력 위치다.
        // 동일 프레임의 여섯 표면을 원문 순서로 입력한다.
        for (std::size_t index=0;index<coordinates.size();++index)
            scene.Add(static_cast<std::uint16_t>(index+1),coordinates[index][0],coordinates[index][1],1,1,TypeFlag1::kSurface,4,'A','P',false,false);
        scene.hash.Cell(0,19,20)=scene.ids.at(3).value;
        RawSquidNeighbors neighbors(scene.pool,scene.hash,scene.spots,scene.types,scene.frames);
        RawSquidNeighborWalk walk(neighbors,scene.ids.at(1),Number(row[1]));CHECK(scene.Original(walk.Current())==2);
        const auto action=Number(row[2]);
        if (action==1) { scene.hash.Cell(0,19,20)=scene.ids.at(6).value;scene.hash.Cell(0,20,19)=scene.ids.at(5).value; }
        else if (action==2) { scene.pool.AllocatedBytes(scene.ids.at(3))[11]=2;scene.pool.AllocatedBytes(scene.ids.at(4))[edition==OriginalEdition::Patch1078 ? 40 : 35]=1; }
        else if (action==3) { auto raw=scene.pool.AllocatedBytes(scene.ids.at(1));Put(raw,14,std::bit_cast<std::uint32_t>(80.0F));Put(raw,18,std::bit_cast<std::uint32_t>(80.0F)); }
        CHECK(WalkSurface(scene,walk)==SurfaceIds(row[3]));++count;
    }
    CHECK(count==32);
}
