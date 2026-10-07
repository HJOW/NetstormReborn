// 독립 x86의 그래프 표/전체 스택/번호와 보호 경로를 검사한다.
#include "TestSupport.h"
#include "o/Graph.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

using namespace netstorm;
namespace {
// 원본 fixture의 열을 그대로 나눈다.
std::vector<std::string> Fields(const std::string& text) {
    std::istringstream input(text); std::vector<std::string> result; std::string item;
    // 빈 필드도 순서를 유지한다.
    while (std::getline(input,item,'\t')) result.push_back(item);
    return result;
}
// 객체/멤버/spot 입력의 행을 읽는다.
std::vector<std::vector<int>> Rows(std::string text) {
    std::vector<std::vector<int>> result; if (text=="-") return result;
    std::istringstream input(text); std::string row;
    // 세미콜론 행과 쉼표 정수를 순서대로 읽는다.
    while (std::getline(input,row,';')) {
        std::replace(row.begin(),row.end(),',',' '); std::istringstream words(row); std::vector<int> values; int value;
        // 부호 있는 WORD와 상태 입력도 손실 없이 읽는다.
        while (words>>value) values.push_back(value);
        result.push_back(std::move(values));
    }
    return result;
}
// 호스트 엔디언과 관계없이 원본 폭으로 직렬화한다.
void Put(std::vector<std::uint8_t>& data,std::uint32_t value,std::size_t width) {
    // low byte부터 기록한다.
    for (std::size_t i=0;i<width;++i) data.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
// Python zlib의 전체 버퍼 Adler-32와 비교한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // Adler 모듈러와 두 초기 누적값이다.
    constexpr std::uint32_t prime=65521; std::uint32_t a=1,b=0;
    // 버퍼의 모든 바이트를 포함한다.
    for (auto byte:bytes) { a=(a+byte)%prime; b=(b+a)%prime; }
    return (b<<16)|a;
}
// 표의 세 WORD와 sentinel/미사용 영역도 전부 검사한다.
std::uint32_t RecordsHash(const o::Graph& graph) {
    std::vector<std::uint8_t> data;
    // 각 레코드의 reserved 값도 포함한다.
    for (const auto& r:graph.Records()) { Put(data,static_cast<std::uint16_t>(r.surfaces),2); Put(data,static_cast<std::uint16_t>(r.inUse),2); Put(data,static_cast<std::uint16_t>(r.reserved),2); }
    return Adler(data);
}
// 스택의 사용하지 않는 흔적까지 비교한다.
std::uint32_t StackHash(const o::Graph& graph) {
    std::vector<std::uint8_t> data;
    // DWORD의 원본 폭을 유지한다.
    for (auto value:graph.FloodStack()) Put(data,value,4);
    return Adler(data);
}
// 작은 표면의 전체 발자국 번호 지도를 독립 입력으로 만든다.
std::vector<std::uint16_t> Map(std::span<const o::SurfaceObject> objects) {
    std::vector<std::uint16_t> map(o::kWorldCells*o::kWorldCells);
    // 뒤 입력 객체가 같은 칸을 덮어쓰는 fixture의 준비 순서다.
    for (const auto& obj:objects) {
        // 발자국의 행을 채운다.
        for (int y=obj.y-obj.height+1;y<=obj.y;++y)
            // 같은 행의 오른쪽 아래 기준 사각형이다.
            for (int x=obj.x-obj.width+1;x<=obj.x;++x) map[static_cast<std::size_t>(y*o::kWorldCells+x)]=obj.id;
    }
    return map;
}
// 실제 표 hex를 WORD 세 개의 필드로 읽는다.
std::vector<o::GraphRecord> Records(const std::string& hex) {
    std::vector<o::GraphRecord> records(o::Graph::kTableSize);
    // 6바이트를 명시적으로 읽어 구조체 padding에 기대지 않는다.
    for (std::size_t i=0;i<records.size();++i) {
        // 세 필드의 부호 있는 WORD 해석을 유지한다.
        for (std::size_t j=0;j<3;++j) {
            const auto p=i*12+j*4; const auto low=std::stoul(hex.substr(p,2),nullptr,16),high=std::stoul(hex.substr(p+2,2),nullptr,16);
            const auto v=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(low|(high<<8)));
            if (!j) records[i].surfaces=v; else if (j==1) records[i].inUse=v; else records[i].reserved=v;
        }
    }
    return records;
}
// 예외가 호출자에게 보고되는지 확인한다.
template<class F> bool Throws(F function) { try { function(); } catch (const std::exception&) { return true; } return false; }
// 합성 한 칸 섬 표면이며 실제 프레임 입력과 분리한다.
o::SurfaceObject Node(std::uint16_t id,int x,int y,int width=1) { return {id,x,y,width,1,o::TypeFlag1::kSurface,8,{65,80,1,0},false,false}; }
}

// 두 판본의 Add/flood/할당/반납/감소가 남긴 모든 멤버와 버퍼를 대조한다.
TEST_CASE(Graph_X86_AllocationFloodMergeAndPreservedStorage) {
    std::ifstream input(NETSTORM_GRAPH_FIXTURE); CHECK(input.good()); std::string line;
    std::unique_ptr<o::SurfaceFinder> finder; std::unique_ptr<o::Graph> graph; int begins=0,checks=0,lineNumber=0;
    // 준비와 연속 작업을 원본 실행 순서대로 재생한다.
    while (std::getline(input,line)) {
        ++lineNumber; if (line.empty() || line[0]=='#') continue;
        const auto row=Fields(line); const auto before=test::FailureCount();
        if (row[0]=="Begin") { ++begins; continue; }
        if (row[0]=="Setup") {
            graph.reset(); finder.reset(); std::vector<o::SurfaceObject> objects; std::vector<o::GraphMembership> members;
            // 합성 객체의 원본 필드를 해석한다.
            for (const auto& v:Rows(row[1])) objects.push_back({static_cast<std::uint16_t>(v[0]),v[1],v[2],v[3],v[4],static_cast<std::uint32_t>(v[5]),static_cast<std::uint32_t>(v[6]),
                {static_cast<std::uint8_t>(v[7]),static_cast<std::uint8_t>(v[8]),1,0},v[10]!=0,v[11]!=0});
            std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells);
            // 명시한 내부 비트만 준비한다.
            for (const auto& v:Rows(row[2])) spots[static_cast<std::size_t>(v[1]*o::kWorldCells+v[0])]=static_cast<std::uint8_t>(v[2]);
            // membership 순서도 fixture의 번호와 같게 둔다.
            for (const auto& v:Rows(row[3])) members.push_back({static_cast<std::uint16_t>(v[0]),static_cast<std::uint8_t>(v[1]),static_cast<std::uint8_t>(v[2])});
            finder=std::make_unique<o::SurfaceFinder>(objects,Map(objects),spots); graph=std::make_unique<o::Graph>(*finder,members,Records(row[4])); continue;
        }
        const auto args=row[1]=="-" ? std::vector<int>{} : Rows(row[1])[0];
        if (row[0]=="Add") graph->Add(static_cast<std::uint16_t>(args[0]));
        else if (row[0]=="Flood") CHECK(graph->Flood(static_cast<std::uint16_t>(args[0]),static_cast<std::uint8_t>(args[1]))==std::stoul(row[2]));
        else if (row[0]=="Allocate") CHECK(graph->Allocate()==std::stoi(row[2]));
        else if (row[0]=="Free") graph->Free(static_cast<std::uint8_t>(args[0]));
        else if (row[0]=="Remove") graph->Remove(static_cast<std::uint8_t>(args[0])); else CHECK(false);
        CHECK(RecordsHash(*graph)==std::stoul(row[3])); CHECK(StackHash(*graph)==std::stoul(row[4]));
        // graph/state는 모든 객체별로 직접 비교한다.
        for (const auto& v:Rows(row[5])) { CHECK(graph->Number(static_cast<std::uint16_t>(v[0]))==v[1]); CHECK(graph->State(static_cast<std::uint16_t>(v[0]))==v[2]); }
        ++checks;
        if (test::FailureCount()!=before) { std::cerr<<"graph fixture 행: "<<lineNumber<<'\n'; break; }
    }
    CHECK(begins==4); CHECK(checks==2560);
}

// 크기 동률은 y/x 순서의 위쪽 그래프를 택하고 두 노드짜리 왼쪽 무리를 합친다.
TEST_CASE(Graph_TieUsesFirstNeighborAndMergesConnectedLoser) {
    const std::vector<o::SurfaceObject> objects{Node(5,20,20),Node(6,19,20),Node(7,21,19),Node(8,20,19),Node(9,18,20)};
    const std::vector<o::GraphMembership> members{{5,254,0},{6,1,0},{7,0,0},{8,0,0},{9,1,0}};
    std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells); o::SurfaceFinder finder(objects,Map(objects),spots);
    std::array<o::GraphRecord,o::Graph::kTableSize> records{}; records[0]={2,1,71}; records[1]={2,1,83};
    o::Graph graph(finder,members,records); graph.Add(5); CHECK(graph.InUse()==1); CHECK(graph.Records()[0].surfaces==5);
    CHECK(graph.Number(5)==0); CHECK(graph.Number(6)==0); CHECK(graph.Number(9)==0); CHECK(graph.Records()[1].surfaces==0);
    CHECK(graph.Records()[0].reserved==71); CHECK(graph.Records()[1].reserved==83);
}

// 여러 칸 발자국도 한 표면 객체로 세며 WORD 증가/감소는 16비트로 감긴다.
TEST_CASE(Graph_MultiCellCountsOneObjectAndWordWrapIsPreserved) {
    const std::vector<o::SurfaceObject> objects{Node(5,30,30,3)}; const std::vector<o::GraphMembership> members{{5,254,0}};
    std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells); o::SurfaceFinder finder(objects,Map(objects),spots);
    o::Graph graph(finder,members); graph.Add(5); CHECK(graph.Records()[0].surfaces==1); CHECK(graph.InUse()==1);
    graph.Remove(0); CHECK(graph.Records()[0].surfaces==0); CHECK(graph.InUse()==0); CHECK(graph.Number(5)==0);
    std::array<o::GraphRecord,o::Graph::kTableSize> records{}; records[0]={32767,1,123};
    o::Graph wrap(finder,members,records); CHECK(wrap.Flood(5,0)==1); CHECK(wrap.Records()[0].surfaces==-32768);
    CHECK(wrap.Flood(5,254)==1); CHECK(wrap.Records()[0].surfaces==32767); CHECK(wrap.Records()[0].reserved==123);
}

// 소진/손상/미확보 membership은 원본의 부분 변경을 호출자에게 전파하지 않는다.
TEST_CASE(Graph_InvalidAndExhaustedInputsRejectBeforeMutation) {
    const std::vector<o::SurfaceObject> objects{Node(5,20,20),Node(6,21,20)}; const std::vector<o::GraphMembership> members{{5,254,0}};
    std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells); o::SurfaceFinder finder(objects,Map(objects),spots);
    o::Graph missing(finder,members); const auto before=RecordsHash(missing),stack=StackHash(missing);
    CHECK(Throws([&]{missing.Add(5);})); CHECK(RecordsHash(missing)==before); CHECK(StackHash(missing)==stack); CHECK(missing.Number(5)==254);
    CHECK(Throws([&]{missing.Flood(5,251);})); CHECK(Throws([&]{missing.Free(253);})); CHECK(Throws([&]{missing.Remove(254);}));
    std::array<o::GraphRecord,o::Graph::kTableSize> records{};
    // 정상 레코드만 소진시키며 sentinel/미사용 레코드를 보존한다.
    for (std::size_t i=0;i<o::Graph::kCount;++i) records[i]={1,1,123};
    o::Graph full(finder,members,records); const auto fullBefore=RecordsHash(full);
    CHECK(Throws([&]{full.Allocate();})); CHECK(RecordsHash(full)==fullBefore); CHECK(full.InUse()==251);
}

// 마지막 연결 무리는 원래 번호에 남고 소진/잘못된 감소 수는 모든 표/스택/번호를 보존한다.
TEST_CASE(Graph_DetachKeepsLastComponentAndRejectsExhaustionAtomically) {
    auto dead=Node(5,20,20); dead.dead=true;
    const std::vector<o::SurfaceObject> objects{dead,Node(6,19,20),Node(7,21,20),Node(8,18,20)};
    const std::vector<o::GraphMembership> members{{5,0,2},{6,0,0},{7,0,0},{8,0,0}};
    std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells); o::SurfaceFinder finder(objects,Map(objects),spots);
    std::array<o::GraphRecord,o::Graph::kTableSize> records{}; records[0]={4,1,71}; records[1].reserved=83;
    const std::array<std::uint16_t,2> connections{6,7}; o::Graph graph(finder,members,records);
    graph.Detach(5,connections); CHECK(graph.Number(5)==0 && graph.State(5)==2);
    CHECK(graph.Number(6)==1 && graph.Number(8)==1 && graph.Number(7)==0);
    CHECK(graph.Records()[0].surfaces==1 && graph.Records()[1].surfaces==2);
    CHECK(graph.Records()[0].reserved==71 && graph.Records()[1].reserved==83);
    const auto hash=RecordsHash(graph),stack=StackHash(graph);
    CHECK(Throws([&]{graph.Detach(5,connections,false,2);})); CHECK(RecordsHash(graph)==hash && StackHash(graph)==stack);
    // 첫 새 무리를 만들 번호가 없으면 복사본의 부분 flood도 원본에 반영하지 않는다.
    for (std::size_t i=0;i<o::Graph::kCount;++i) records[i]={4,1,71};
    o::Graph full(finder,members,records); const auto before=RecordsHash(full),oldStack=StackHash(full);
    CHECK(Throws([&]{full.Detach(5,connections,true);})); CHECK(RecordsHash(full)==before && StackHash(full)==oldStack);
    CHECK(full.Number(5)==0 && full.Number(6)==0 && full.Number(7)==0 && full.Number(8)==0);
}
