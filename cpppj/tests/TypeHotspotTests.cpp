// 숫자 속성 구간의 세 실제 PE 기대값과 타입 로더의 입력 계약을 대조한다.
#include "RawSceneSupport.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 실제 명령이 저장한 두 기준점을 비교하며 기대값은 C++에서 계산하지 않는다.
void Replay(std::string_view edition) {
    const auto rows=LoadFixture(NETSTORM_TYPEHOTSPOT_FIXTURE).rows;std::size_t count=0;
    // 순서/중복 속성은 동일한 float32 입력으로 적용한다.
    for (const auto& row:rows) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);std::int32_t x=std::stoi(row[4]),y=std::stoi(row[5]);
        // 첫 입력 이후 다른 값을 써서 마지막 속성이 앞의 값을 덮는지 확인한다.
        for (std::size_t i=0;i<row[1].size();++i) {
            const bool vertical=row[1][i]=='Y';const float value=std::bit_cast<float>(Number(row[i==0 ? 2 : 3]));
            (vertical ? y : x)=TypeHotFootOffset(value,vertical);
        }
        CHECK(x==std::stoi(row[6]) && y==std::stoi(row[7]));++count;
    }
    CHECK(count>400 && rows.size()==count*3);
}
// 로딩 목록을 만족하는 최소 타입 표에서 실제 속성 순서를 적용한다.
RiftTypeTable Table(OriginalEdition edition,const RiftTypeDefinition& definition) {
    std::vector<RiftTypeSource> sources;
    // 모든 파일 이름은 원본 로딩 순서를 유지한다.
    for (const auto name:TypeLoadOrder(edition)) sources.push_back({name,&definition});
    return RiftTypeTable(edition,sources);
}
}
// 10.78의 숫자 속성 구간/실제 ftol 출력이다.
TEST_CASE(type_hotspot_patch_x86) { Replay("originals"); }
// CD의 다른 지역 스택/저장 위치를 별도로 대조한다.
TEST_CASE(type_hotspot_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE도 자체 관찰값을 사용한다.
TEST_CASE(type_hotspot_1037_x86) { Replay("original1037"); }
// 초기값/대소문자/중복 속성/AST float32 변환과 고정 배율을 검증한다.
TEST_CASE(type_hotspot_loader_order_and_float32) {
    // 두 판본의 전체 타입 표가 같은 속성 규칙을 사용한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto empty=RiftTypeDefinition::Parse("typename test typeflags default_hotspot; {} A00:default:\"a.gif\"#0;");
        const auto initial=Table(edition,empty);CHECK(initial.Types()[70].hotspotX==0 && initial.Types()[70].hotspotY==0);
        const auto definition=RiftTypeDefinition::Parse("typename test { foot_x=3; foot_y=6; hotFootRatioX=-1.9; hotFootRatioY=0.99999999; HOTFOOTRATIOX=0.49999999; } A00:default:\"a.gif\"#0;");
        const auto table=Table(edition,definition);const auto& record=table.Types()[70];
        CHECK(record.footX==3 && record.footY==8 && record.hotspotX==8 && record.hotspotY==11);
        CHECK(table.Types()[0].hotspotX==0 && table.Types()[0].hotspotY==0);
    }
}
// 원본이 assert하는 절대 기준점/문자열과 분석 계약 밖 숫자를 예외로 보고한다.
TEST_CASE(type_hotspot_invalid_properties) {
    // 지원하지 않는 속성을 조용히 무시하지 않는다.
    for (const auto body:{"hotFootX=1;","hotFootY=1;","hotFootRatioX=\"1\";","hotFootRatioY=\"1\";"}) {
        const auto definition=RiftTypeDefinition::Parse(std::string("typename test {")+body+"} A00:default:\"a.gif\"#0;");
        CHECK(Throws([&] { Table(OriginalEdition::Patch1078,definition); }));
        CHECK(Throws([&] { Table(OriginalEdition::Cd1072,definition); }));
    }
    CHECK(Throws([] { TypeHotFootOffset(std::numeric_limits<float>::infinity(),false); }));
    CHECK(Throws([] { TypeHotFootOffset(std::numeric_limits<float>::quiet_NaN(),true); }));
    CHECK(Throws([] { TypeHotFootOffset(std::numeric_limits<float>::max(),false); }));
}
