// 두 PE의 Pop/Unpop 결과와 실제 등록 지도의 표면 탐색 연결을 검사한다.
#include "TestSupport.h"
#include "o/SquidSpatial.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// TSV와 객체/지도 결과를 구분하며 빈 결과는 fixture의 '-'로 표현한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> fields; std::size_t start=0;
    // 마지막 필드까지 그대로 남긴다.
    for (;;) {
        const auto end=text.find(separator,start);
        fields.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));
        if (end==std::string_view::npos) return fields;
        start=end+1;
    }
}
// native 입력의 float bit pattern을 손실 없이 복원한다.
float Float(const std::string& text) { return std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(text))); }
// 반복 검사에서 원본/Unicorn 없이 저장된 기계어 결과만 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_SPATIAL_FIXTURE);
        if (!input) throw std::runtime_error("Missing spatial x86 fixture");
        std::vector<std::vector<std::string>> result; std::string line;
        // UTF-8 설명을 건너뛰고 정상 TSV 행을 읽는다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();
    return rows;
}
// 필드 값은 fixture 입력을 그대로 옮기며 기대 상태를 계산하지 않는다.
SpatialObject Object(const std::vector<std::string>& row) {
    SpatialObject object;
    // oracle.add가 void 입력에 놓은 좌표다. Pop 이전 상태 비교를 위한 입력이며 기대 계산이 아니다.
    object.x=object.y=20;
    object.id=static_cast<std::uint16_t>(std::stoul(row[1]));
    object.flags1=static_cast<std::uint32_t>(std::stoul(row[2])); object.flags2=static_cast<std::uint32_t>(std::stoul(row[3]));
    object.width=std::stoi(row[4]); object.height=std::stoi(row[5]);
    object.frameWidth=Float(row[6]); object.frameHeight=Float(row[7]);
    object.state=static_cast<std::uint8_t>(std::stoul(row[8])); object.extra=static_cast<std::uint8_t>(std::stoul(row[9]));
    object.surfaceWord=static_cast<std::uint16_t>(std::stoul(row[10])); object.island=static_cast<std::uint16_t>(std::stoul(row[11]));
    return object;
}
// 전체 객체/머리/spot/효과 결과를 native와 같은 순서로 직렬화한다.
std::array<std::string,4> Snapshot(const SquidSpatial& spatial,const std::vector<std::uint16_t>& ids,const SpatialChange& change) {
    std::array<std::ostringstream,4> out;
    // 제거한 객체의 next·좌표 캐시도 포함하여 원본 상태 보존을 검사한다.
    for (const auto id:ids) {
        if (out[0].tellp()>0) out[0]<<';';
        const auto& object=spatial.Object(id);
        out[0]<<id<<','<<object.next<<','<<object.island<<','<<static_cast<int>(object.state)<<','<<static_cast<int>(object.extra)
            <<','<<object.screenX<<','<<object.screenY<<','<<static_cast<int>(object.level)<<','<<std::bit_cast<std::uint32_t>(object.x)
            <<','<<std::bit_cast<std::uint32_t>(object.y)<<','<<object.surfaceWord;
    }
    // 4단계의 모든 버킷을 검사해 별도 지점의 예기치 않은 쓰기도 확인한다.
    for (int level=0;level<4;++level) {
        const auto heads=spatial.Hash().Entries(level);
        // 0이 아닌 머리만 fixture에 출력되어 있지만 비교는 모든 칸을 포함한다.
        for (std::size_t i=0;i<heads.size();++i) {
            if (!heads[i]) continue;
            if (out[1].tellp()>0) out[1]<<';';
            out[1]<<level<<':'<<i<<':'<<heads[i];
        }
    }
    const auto spots=spatial.Spots();
    // 원본 low byte spot 지도 전체를 대조한다.
    for (std::size_t i=0;i<spots.size();++i) {
        if (!spots[i]) continue;
        if (out[2].tellp()>0) out[2]<<';';
        out[2]<<i<<':'<<static_cast<int>(spots[i]);
    }
    // 실제 가상 효과 자체 대신 검사한 호출 순서/인자 계약을 비교한다.
    for (const auto& event:change.events) {
        if (out[3].tellp()>0) out[3]<<';';
        switch (event.kind) {
        case SpatialEventKind::Update88: out[3]<<"update88"; break;
        case SpatialEventKind::Update8c: out[3]<<"update8c"; break;
        case SpatialEventKind::FirstPop: out[3]<<"first:"<<event.value; break;
        case SpatialEventKind::PostPop: out[3]<<"post:"<<event.value; break;
        case SpatialEventKind::RegionQuery: out[3]<<"region:"<<event.value; break;
        case SpatialEventKind::SurfaceChanged: out[3]<<"notify:"<<event.value; break;
        }
    }
    std::array<std::string,4> result;
    // 빈 지도/사건도 명시적인 기대값으로 비교한다.
    for (std::size_t i=0;i<result.size();++i) result[i]=out[i].tellp()>0 ? out[i].str() : "-";
    return result;
}
// 검증 입력을 거부하는 새 보호 경로를 검사한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}
// 표면 탐색 연결용 단일 칸 다리 입력이다. 실제 파생 생성자의 복원을 뜻하지 않는다.
SpatialObject BridgeObject(std::uint16_t id) {
    SpatialObject object; object.id=id; object.flags1=TypeFlag1::kSurface; object.flags2=TypeFlag2::kBridge;
    object.frame={'A','A',1,0}; return object;
}
}

// 원본 기계어의 238개 시퀀스·878회 전이를 판본별 상태/전체 지도/사건과 비교한다.
TEST_CASE(Spatial_X86RegistrationRemovalAndEditionDifferences) {
    std::size_t transitions=0,scenarios=0,differences=0;
    // CD판의 overlap 진행 결과는 패치판과 다른 열로 검증한다.
    for (const auto edition:{SpatialEdition::Patch1078,SpatialEdition::CD1072}) {
        std::unique_ptr<SquidSpatial> spatial; std::vector<std::uint16_t> ids; std::string scenario;
        // 기대값을 모델로 재생성하지 않고 저장한 두 PE 결과를 읽는다.
        for (const auto& row:Fixture()) {
            if (row[0]=="Begin") {
                scenario=row[1]; ids.clear();
                const bool supported=row[2]=="1"; const auto first=static_cast<std::uint32_t>(std::stoul(row[3]));
                std::vector<std::uint8_t> spots(kWorldCells*kWorldCells);
                if (row[4]!="-") {
                    // 초기 spot만 입력으로 사용한다. 기대 결과는 다음 Step 열에 있다.
                    for (const auto& entry:Split(row[4],';')) {
                        const auto fields=Split(entry,':'); spots[std::stoul(fields[0])]=static_cast<std::uint8_t>(std::stoul(fields[1]));
                    }
                }
                // 가상 firstPop/영역 조회가 반환하는 입력 계약을 명시한다.
                spatial=std::make_unique<SquidSpatial>(edition,SpatialDependencies{
                    [first](auto) { return first; },[supported](float,float) { return supported; }},spots);
                ++scenarios;
            } else if (row[0]=="Object") {
                auto object=Object(row); ids.push_back(object.id); spatial->Add(object);
            } else if (row[0]=="Step") {
                const auto id=static_cast<std::uint16_t>(std::stoul(row[2])); const auto flags=static_cast<std::uint32_t>(std::stoul(row[5]));
                const auto change=row[1]=="Pop" ? spatial->Pop(id,Float(row[3]),Float(row[4]),flags) : spatial->Unpop(id,flags);
                const auto result=Snapshot(*spatial,ids,change); const std::size_t column=edition==SpatialEdition::Patch1078 ? 6 : 10;
                // 결과 필드 전체를 비교하고 실패 때 어느 시나리오인지 출력한다.
                for (std::size_t i=0;i<result.size();++i) {
                    if (result[i]!=row[column+i]) std::fprintf(stderr,"Spatial mismatch %s %s edition=%zu field=%zu\n",scenario.c_str(),row[1].c_str(),column,i);
                    CHECK(result[i]==row[column+i]);
                }
                if (edition==SpatialEdition::Patch1078 && !std::equal(row.begin()+6,row.begin()+10,row.begin()+10)) ++differences;
                ++transitions;
            }
        }
    }
    CHECK(scenarios==476); CHECK(transitions==1756); CHECK(differences==32);
}
// overlap 검사 전의 부분 OR는 남으며 CD의 AND 해제는 기존 겹침 bit도 지운다.
TEST_CASE(Spatial_OverlapPreservesNativePartialChanges) {
    std::vector<std::uint8_t> spots(kWorldCells*kWorldCells); spots[18*kWorldCells+19]=4;
    SpatialObject object; object.id=1; object.flags2=4; object.width=object.height=3;
    SquidSpatial patch(SpatialEdition::Patch1078,{},spots); patch.Add(object);
    CHECK(patch.Pop(1,20,20).result==SpatialResult::Overlap);
    CHECK(patch.Spots()[18*kWorldCells+18]==4 && patch.Spots()[18*kWorldCells+20]==0);
    CHECK(patch.Object(1).state==4 && patch.Object(1).extra==0 && patch.Object(1).x==20);
    CHECK(patch.Hash().Cell(0,20,20)==0);
    CHECK(patch.Unpop(1).result==SpatialResult::Unchanged);
    SquidSpatial cd(SpatialEdition::CD1072,{},spots); cd.Add(object);
    CHECK(cd.Pop(1,20,20).result==SpatialResult::Registered);
    CHECK(cd.Hash().Cell(0,20,20)==1 && cd.Hash().Cell(0,18,18)==0);
    CHECK(cd.Unpop(1).result==SpatialResult::Removed && cd.Spots()[18*kWorldCells+19]==0);
}
// 다른 기준점이 같은 큰 버킷을 공유해도 중간 객체를 제거할 수 있다.
TEST_CASE(Spatial_SharedBucketUnlinksAndRetainsRemovedNext) {
    SquidSpatial spatial;
    // 폭 3의 SHP는 2단계에 등록하며 spot에 쓰지 않는 비표면 타입을 사용한다.
    for (std::uint16_t id=1;id<=3;++id) {
        SpatialObject object; object.id=id; object.frameWidth=3; spatial.Add(object);
        CHECK(spatial.Pop(id,static_cast<float>(31+id),32).result==SpatialResult::Registered);
    }
    CHECK(spatial.Hash().Bucket(2,32,32)==3 && spatial.Object(3).next==2 && spatial.Object(2).next==1);
    CHECK(spatial.Unpop(2).result==SpatialResult::Removed);
    CHECK(spatial.Object(3).next==1 && spatial.Object(2).next==1);
    CHECK(spatial.Pop(2,48,48).result==SpatialResult::Registered && spatial.Object(2).next==0);
}
// 기존 표면 탐색은 합성 발자국 stamp 대신 실제 등록 머리/spot의 스냅샷을 사용한다.
TEST_CASE(Spatial_SurfaceSnapshotTracksRegistrationAndLifetime) {
    SquidSpatial spatial; spatial.Add(BridgeObject(1)); spatial.Add(BridgeObject(2));
    spatial.Pop(1,20,20); spatial.Pop(2,21,20);
    const auto snapshot=spatial.Surfaces();
    CHECK(snapshot.Neighbors(1)==std::vector<std::uint16_t>{2});
    spatial.Unpop(2);
    CHECK(spatial.Surfaces().Neighbors(1).empty());
    CHECK(snapshot.Neighbors(1)==std::vector<std::uint16_t>{2});
    spatial.Pop(2,21.25f,20);
    CHECK(Throws([&] { static_cast<void>(spatial.Surfaces()); }));
}
// 잘못된 SID/상태/발자국과 NaN을 거부하며 원본의 정상 finite 좌표 복구는 보존한다.
TEST_CASE(Spatial_RejectsUnsafeInputsBeforeFootprintWrites) {
    SquidSpatial spatial; SpatialObject object; object.id=1; object.flags2=4; object.width=3;
    CHECK(Throws([&] { auto invalid=object; invalid.id=0; spatial.Add(invalid); }));
    CHECK(Throws([&] { auto invalid=object; invalid.state=12; spatial.Add(invalid); }));
    spatial.Add(object);
    CHECK(Throws([&] { spatial.Add(object); }));
    CHECK(Throws([&] { spatial.Pop(1,1,20); }));
    CHECK(spatial.Object(1).x==0 && spatial.Object(1).state==4);
    CHECK(Throws([&] { spatial.Pop(1,std::numeric_limits<float>::quiet_NaN(),20); }));
    CHECK(spatial.Pop(1,0,20).result==SpatialResult::Registered && spatial.Object(1).x==10 && spatial.Object(1).y==10);
    CHECK(spatial.Pop(1,30,30).result==SpatialResult::Unchanged);
}
// firstPop는 한 번이고 상태의 dead 비트는 비전투 재등록/해제에서 보존한다.
TEST_CASE(Spatial_FirstPopContractRunsOnceAndPreservesDead) {
    int calls=0;
    // 원본 파생 효과 대신 입력 반환 비트의 전달을 검사한다.
    SquidSpatial spatial(SpatialEdition::Patch1078,{[&](auto) { ++calls; return 8u; },{}});
    SpatialObject object; object.id=1; object.state=6; spatial.Add(object);
    const auto first=spatial.Pop(1,20,20);
    CHECK(calls==1 && spatial.Object(1).state==2 && spatial.Object(1).extra==128);
    CHECK(first.events.back()==(SpatialEvent{SpatialEventKind::PostPop,8}));
    spatial.Unpop(1);
    const auto again=spatial.Pop(1,20,20);
    CHECK(calls==1 && spatial.Object(1).state==2);
    CHECK(again.events.back()==(SpatialEvent{SpatialEventKind::PostPop,16}));
}
