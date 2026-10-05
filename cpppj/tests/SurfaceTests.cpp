// 두 원본의 실제 연결·flag 8 이웃 순서·붕괴 방문 목록과 새 스냅샷의 수명/고리 처리를 검사한다.
#include "TestSupport.h"
#include "o/Bridge.h"
#include "o/SquidFinder.h"
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// 마지막 빈 필드도 보존하여 이웃이 없는 TSV 결과를 정확히 읽는다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;
    std::size_t start=0;
    // 구분자가 더 없으면 남은 문자열을 빈 값까지 추가한다.
    for (;;) {
        const auto next=text.find(separator,start);
        result.emplace_back(text.substr(start,next==std::string_view::npos ? next : next-start));
        if (next==std::string_view::npos) return result;
        start=next+1;
    }
}
// 입력된 번호 목록을 원본 순서로 유지한다.
std::vector<std::uint16_t> Ids(std::string_view text) {
    std::vector<std::uint16_t> result;
    if (text.empty() || text=="-") return result;
    // 중복도 삭제하지 않는다. 원본 방문 목록에는 같은 번호가 남을 수 있다.
    for (const auto& value:Split(text,',')) result.push_back(static_cast<std::uint16_t>(std::stoul(value)));
    return result;
}
// 원본 파일이나 Python 없이 Git에 저장한 실제 기계어 기대값을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_SURFACE_FIXTURE);
        if (!input) throw std::runtime_error("Missing surface x86 fixture");
        std::vector<std::vector<std::string>> result;
        std::string line;
        // 한국어 설명 주석만 건너뛰고 결과의 마지막 빈 필드까지 유지한다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();
    return rows;
}
struct Scene {
    std::vector<SurfaceObject> objects;
    std::vector<std::uint16_t> map=std::vector<std::uint16_t>(kWorldCells*kWorldCells);
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(kWorldCells*kWorldCells);
    // 입력 발자국을 지도에 적는 도우미이며 연결/붕괴 결과를 계산하지 않는다.
    void Add(SurfaceObject object) {
        objects.push_back(object);
        // 오른쪽 아래 기준점의 발자국을 입력 지도에 채운다.
        for (int y=object.y-object.height+1;y<=object.y;++y)
            // 같은 행에서 그 표면이 차지하는 열을 적는다.
            for (int x=object.x-object.width+1;x<=object.x;++x)
                map[static_cast<std::size_t>(y*kWorldCells+x)]=object.id;
    }
    // 새 표면은 기본 사방 연결 다리다. 검사용 입력이며 원본 생성자는 아니다.
    void Add(std::uint16_t id,int x,int y,int width=1,int height=1,std::uint32_t flags2=TypeFlag2::kBridge) {
        Add({id,x,y,width,height,TypeFlag1::kSurface,flags2,{'A','A',1,0},false,false});
    }
    // 원본 가상 메모리에 넣었던 정수 입력을 C++의 이름 있는 필드로 옮긴다.
    explicit Scene(const std::vector<std::string>& row) {
        // 객체별 레코드의 11개 필드를 손실 없이 읽는다.
        for (const auto& entry:Split(row[1],';')) {
            const auto fields=Split(entry,',');
            Add({static_cast<std::uint16_t>(std::stoul(fields[0])),std::stoi(fields[1]),std::stoi(fields[2]),
                std::stoi(fields[3]),std::stoi(fields[4]),static_cast<std::uint32_t>(std::stoul(fields[5])),
                static_cast<std::uint32_t>(std::stoul(fields[6])),
                {static_cast<std::uint8_t>(std::stoul(fields[7])),static_cast<std::uint8_t>(std::stoul(fields[8])),1,0},
                fields[9]=="1",fields[10]=="1"});
        }
        // spot/지도 입력 변경만 적용한다. 기대 목록은 계산하지 않는다.
        for (std::size_t column=2;column<=3;++column) {
            if (row[column]=="-") continue;
            // 각 수정 입력은 x/y/값 세 정수다.
            for (const auto& entry:Split(row[column],';')) {
                const auto fields=Split(entry,',');
                const auto index=static_cast<std::size_t>(std::stoi(fields[1])*kWorldCells+std::stoi(fields[0]));
                if (column==2) spots[index]=static_cast<std::uint8_t>(std::stoul(fields[2]));
                else map[index]=static_cast<std::uint16_t>(std::stoul(fields[2]));
            }
        }
    }
    // 원본 파일을 쓰지 않는 작은 직접 검사 입력을 만든다.
    Scene()=default;
};
// 정의하지 않은 입력을 조용히 다른 번호로 바꾸지 않고 오류로 보고하는지 확인한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}
}

// 방향 축·반대 방향·타입 마스크의 적용 순서를 원본 정상 반환값으로 검증한다.
TEST_CASE(Surface_X86TypeAndFrameConnections) {
    std::size_t count=0;
    // 두 원본에서 같은 것으로 확인한 결과만 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Connect") continue;
        const FrameCode a{static_cast<std::uint8_t>(std::stoul(row[1])),static_cast<std::uint8_t>(std::stoul(row[2])),1,0};
        const FrameCode b{static_cast<std::uint8_t>(std::stoul(row[4])),static_cast<std::uint8_t>(std::stoul(row[5])),1,0};
        CHECK(Bridge::Connects(a,static_cast<std::uint32_t>(std::stoul(row[3])),b,
            static_cast<std::uint32_t>(std::stoul(row[6])),std::stoi(row[7]))==(row[8]=="1"));
        ++count;
    }
    CHECK(count==3016);
}
// 생성자/가상 필터/다음 함수 전체가 반환한 이웃 순서와 C++ 출력을 비교한다.
TEST_CASE(Surface_X86OrderedNeighborsAndFilters) {
    std::size_t count=0;
    // 지도 가장자리·큰 발자국·중복·죽음/표면/내부 필터를 포함한다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Neighbor") continue;
        const Scene scene(row); const SurfaceFinder surfaces(scene.objects,scene.map,scene.spots);
        CHECK(surfaces.Neighbors(static_cast<std::uint16_t>(std::stoul(row[4])))==Ids(row[5])); ++count;
    }
    CHECK(count==125);
}
// 용량/경계 삭제/짧은 접합 플래그를 실제 재귀 및 실제 이웃 탐색 결과와 함께 검증한다.
TEST_CASE(Bridge_X86DecayTraversalAndCapacity) {
    std::size_t count=0,shortCases=0,progressCases=0;
    // 고리 없는 물리적 표면 그래프에서 전체 원본 함수가 정상 복귀한 결과다.
    for (const auto& row:Fixture()) {
        if (row[0]!="Collect") continue;
        const Scene scene(row); const SurfaceFinder surfaces(scene.objects,scene.map,scene.spots);
        const auto result=Bridge::CollectDecay(surfaces,static_cast<std::uint16_t>(std::stoul(row[4])),std::stoul(row[5]));
        CHECK(result.complete); CHECK(result.shortJunction==(row[6]=="1")); CHECK(result.canDecay==(row[7]=="1"));
        CHECK(result.visited==Ids(row[8])); shortCases+=result.shortJunction; progressCases+=result.canDecay; ++count;
    }
    CHECK(count==309 && shortCases>0 && progressCases>0);
}
// 같은 큰 섬을 여러 칸에서 만난 경우 한 번만 반환하고 생성 후 외부 입력이 바뀌어도 유지한다.
TEST_CASE(Surface_MultiCellNeighborSnapshotLifetime) {
    Scene scene; scene.Add(1,20,20,3,3,TypeFlag2::kIsland); scene.Add(2,20,17,3,3,TypeFlag2::kIsland);
    const SurfaceFinder surfaces(scene.objects,scene.map,scene.spots);
    scene.map.assign(scene.map.size(),0); scene.objects.clear(); scene.spots.assign(scene.spots.size(),8);
    CHECK(surfaces.Neighbors(1)==std::vector<std::uint16_t>{2}); CHECK(surfaces.Object(2).width==3);
}
// 원본이 부모만 빼고 재귀하는 접합 고리에서 안전하게 실패하며 조회 자료를 변경하지 않는다.
TEST_CASE(Bridge_CyclicJunctionTraversalStopsWithoutMutation) {
    Scene scene; scene.Add(1,20,20); scene.Add(2,21,20); scene.Add(3,21,21); scene.Add(4,20,21);
    const SurfaceFinder surfaces(scene.objects,scene.map,scene.spots);
    const auto before=surfaces.Neighbors(1); const auto result=Bridge::CollectDecay(surfaces,1);
    CHECK(!result.complete && !result.canDecay); CHECK(surfaces.Neighbors(1)==before);
}
// 없는 번호·잘못된 지도/발자국·원본 연결 글자 범위와 수명 목록 용량을 검증한다.
TEST_CASE(Surface_RejectsInvalidMapsObjectsAndDecayInputs) {
    Scene scene; scene.Add(1,20,20);
    CHECK(Throws([&] { SurfaceFinder invalid(scene.objects,{},scene.spots); }));
    scene.objects.push_back(scene.objects[0]);
    CHECK(Throws([&] { SurfaceFinder duplicate(scene.objects,scene.map,scene.spots); }));
    scene.objects.pop_back(); scene.map[19*kWorldCells+20]=7;
    const SurfaceFinder surfaces(scene.objects,scene.map,scene.spots);
    CHECK(Throws([&] { surfaces.Neighbors(1); }));
    CHECK(Throws([&] { surfaces.Object(0); }));
    CHECK(Throws([&] { Bridge::CollectDecay(surfaces,1,0); }));
    CHECK(Throws([&] { Bridge::Connects({'Q','P',1,0},4,{'A','P',1,0},4,0); }));
    CHECK(Throws([&] { Bridge::Connects({'A','P',1,0},4,{'A','P',1,0},4,8); }));
}
