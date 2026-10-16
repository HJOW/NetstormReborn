// 원본 최근접/검색 경계/축 사거리 관찰과 실제 해시의 방향별 연결을 검증한다.
#include "RawSceneSupport.h"
#include "o/RawFenceNeighbors.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 입력 raw 다섯 슬롯과 현재TYPE/동맹/해시를 같은 제품 객체에 공급한다.
struct FenceNeighborScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 실제 서버 할당을 포함한 공유 풀이다.
    SquidHash hash; // 일반 finder의 실제 네 공간 해시다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 같은 현재 타입 표의 발자국이다.
    std::vector<float> ranges=std::vector<float>(188); // 현재TYPE+44를 별도로 공급한다.
    std::array<std::uint32_t,81> alliances{}; // 후보 소유자→현재 소유자의 현재 행렬이다.
    RawFenceNeighbors body{pool,hash,types,ranges,alliances}; // 위 저장소를 사본 없이 읽는 실제 제품이다.
    // 슬롯 번호50..54까지 실제로 확보한다.
    FenceNeighborScene() {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==54) break;if (!sid.value || sid.value>54) throw std::logic_error("펜스 이웃 검사 슬롯 확보 실패"); } // 관찰 입력을 실제 할당 슬롯에만 쓴다.
    }
    // 독립 관찰기에 보존한 전체250BYTE와 현재 타입/동맹만 준비한다.
    void Setup(const std::vector<std::string>& row,const std::vector<std::string>& input) {
        CHECK(row[1].size()==500);
        for (std::uint16_t number=50;number<=54;++number) { // 부모와 네 후보를 같은 순서로 복원한다.
            auto raw=pool.AllocatedBytes(Sid{number});
            for (std::size_t offset=0;offset<50;++offset) raw[offset]=static_cast<std::uint8_t>(std::stoul(row[1].substr(((number-50)*50+offset)*2,2),nullptr,16)); // 원문 보존 바이트만 읽는다.
        }
        for (const auto type:{135,136}) { types[type].footX=static_cast<int>(Number(row[3]));types[type].footY=static_cast<int>(Number(row[4]));ranges[type]=std::bit_cast<float>(Number(input[3])); } // 원문 두TYPE의 물리 입력이다.
        if (Number(row[5])==1) alliances[2*9+1]=1;
        if (Number(row[5])==2) alliances[1*9+2]=1;
    }
    // 실제 해시에 새 현재 좌표를 등록한다. 기대 후보 순서는 계산하지 않는다.
    void Place(Sid sid,float x,float y,std::uint8_t owner) {
        auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=136;raw[34]=owner;
        Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));auto& head=hash.Bucket(1,x,y);Put(raw,4,head,2);head=sid.value;
    }
};
}
// 선택/동맹/float 경계는 원본 후보 입력을 받아 독립 원문1904행과 전수 대조한다.
TEST_CASE(FenceNeighbors_Native1904) {
    const auto fixture=LoadFixture(NETSTORM_FENCE_NEIGHBORS_FIXTURE);CHECK(fixture.rows.size()==1905 && fixture.rows.front()[0]=="input");
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 이전 원문 행의 결과를 다음 입력에 재사용하지 않는다.
        const auto& row=fixture.rows[index];CHECK(row.size()==9);const auto input=Split(row[0],',');CHECK(input.size()==8);FenceNeighborScene scene;scene.Setup(row,input);
        if (input[0]=="N") {
            std::vector<Sid> candidates; // 실제 finder 외부 경계의 독립 후보 순서다.
            for (const auto& number:Split(row[2],',')) candidates.push_back(Sid{static_cast<std::uint16_t>(Number(number))}); // 중복/자기 자신도 원문에 공급한 대로 유지한다.
            CHECK(scene.body.Select(Sid{50},candidates,Number(input[2])!=0).value==Number(row[7]));
            const auto bounds=scene.body.SearchBounds(Sid{50},Number(input[1]));const auto native=Split(row[8],',');CHECK(native.size()==4);
            for (std::size_t corner=0;corner<4;++corner) CHECK(std::bit_cast<std::uint32_t>(bounds[corner])==Number(native[corner])); // 양 끝float의 전체비트를 대조한다.
        } else {
            const auto endpoints=Split(row[6],',');CHECK(endpoints.size()==4);std::array<float,4> points{}; // 원본 사거리 함수의 네float 인자다.
            for (std::size_t point=0;point<4;++point) points[point]=std::bit_cast<float>(Number(endpoints[point])); // 보존한 입력 비트를 그대로 복원한다.
            CHECK(scene.body.WithinRange(Sid{50},points)==(Number(row[7])!=0));
        }
        std::string current; // 조회가 입력 객체를 바꾸지 않았는지 전체250BYTE를 대조한다.
        for (std::uint16_t number=50;number<=54;++number) current+=Hex(scene.pool.Slot(Sid{number})); // padding을 포함해 읽는다.
        CHECK(current==row[1]);
    }
}
// 실제 finder는 방향을 나누며 가까운 적대를 건너뛰어 먼 동맹으로 대체하지 않는다.
TEST_CASE(FenceNeighbors_CurrentHashDirectionsAndReverseAlliance) {
    FenceNeighborScene scene;scene.types[136].footX=1;scene.types[136].footY=1;scene.ranges[136]=8.5f;
    scene.Place(Sid{50},50,50,1);scene.Place(Sid{51},50,46,2);scene.Place(Sid{52},54,50,2);scene.Place(Sid{53},50,54,2);scene.Place(Sid{54},46,50,2);
    for (std::uint32_t direction=0;direction<8;direction+=2) { // 실제 네 방향은 서로 다른 해시 후보를 골라야 한다.
        CHECK(scene.body.Neighbor(Sid{50},direction).value==51+direction/2);CHECK(scene.body.Neighbor(Sid{50},direction,true).value==0);
    }
    scene.alliances[1*9+2]=1;CHECK(scene.body.Connections(Sid{50})==0);scene.alliances[2*9+1]=1;CHECK(scene.body.Connections(Sid{50})==4);
    scene.pool.AllocatedBytes(Sid{51})[40]=0x20;CHECK(scene.body.Connections(Sid{50})==4);
    scene.pool.AllocatedBytes(Sid{51})[11]=2;CHECK(scene.body.Neighbor(Sid{50},0).value==0 && scene.body.Connections(Sid{50})==3);
    scene.pool.AllocatedBytes(Sid{51})[11]=1;CHECK(scene.body.Neighbor(Sid{50},0).value==51);
    const auto nearer=scene.pool.Allocate(2);CHECK(nearer.value==55);scene.Place(nearer,50,48,3);
    CHECK(scene.body.Neighbor(Sid{50},0).value==55 && scene.body.Neighbor(Sid{50},0,true).value==0 && scene.body.Connections(Sid{50})==3);
    scene.pool.AllocatedBytes(Sid{50})[40]=8;CHECK(scene.body.Connections(Sid{50})==0);
}
