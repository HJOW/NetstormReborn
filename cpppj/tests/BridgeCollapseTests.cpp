// 새 연결 전에 동결한 다리 wrapper/보호60프레임의 소유자 캡처 전체를 비교한다.
#include "DudeSpawnSupport.h"
#include "client/OwnerColorMaps.h"
#include "client/RawShapeToBuffer.h"
#include "o/RawShrapnelFrame.h"
#include "o/RawBridgeDraw.h"
using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 원본 hex를 해석할 뿐 픽셀이나 메타데이터 기대값을 계산하지 않는다.
std::vector<std::uint8_t> DecodeBridge(std::string_view value) {
    if (value.size()%2) throw std::invalid_argument("다리 캡처 hex 폭 오류");std::vector<std::uint8_t> result;
    // 두 문자마다 한 바이트를 원문 순서대로 복원한다.
    for (std::size_t i=0;i<value.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(value.substr(i,2)),nullptr,16)));
    return result;
}
}
// wrapper의 종류2와 owner0/9색·모든 현재 프레임의 전체 픽셀을 동결 원문과 대조한다.
TEST_CASE(BridgeCollapse_NativeWrapperAndAllOwnerFrames) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SidPool pool(OriginalEdition::Patch1078,32768,true);
    CHECK(assets.shapes[82].frameCount==60);auto types=assets.types;types[82].hotspotX=types[82].hotspotY=0; // 동결 원문의 명시 TYPE 기준점0 입력이며 실제 게임 기본값과 구별한다.
    std::printf("다리 캡처의 실제 TYPE 기준점 %d,%d / 원본 대조 입력0,0\n",assets.types[82].hotspotX,assets.types[82].hotspotY);
    BridgeDrawState drawState;RawBridgeDraw bridgeDraw(pool,drawState);auto rows=LoadFixture(NETSTORM_BRIDGE_COLLAPSE_FIXTURE).rows;
    // 별도로 동결한 실제 extra2의600행도 원래603행과 구별해서 읽는다.
    for (auto row:LoadFixture(NETSTORM_BRIDGE_EXTRA_CAPTURE_FIXTURE).rows) { row.insert(row.begin(),"E");rows.push_back(std::move(row)); }
    // 원본과 같은 source50 슬롯을 확보하며 실제 공간/삭제는 별도 Session 검사에서 실행한다.
    for (std::uint16_t id=5;id<=50;++id) CHECK(pool.Allocate(2)==Sid{id});
    const auto maps=BuildStandardOwnerColorMaps(82,82);std::array<std::uint32_t,188> tokens{};tokens[82]=1;std::size_t captures{},wrappers{};
    // 같은 명시 입력을 설치한 뒤 저장된 출력만 기대값으로 사용한다.
    for (const auto& row:rows) {
        if (row[0]=="W") { CHECK(row.size()==3 && static_cast<std::uint32_t>(ShrapnelKind::Variant2)==Number(row[2]));++wrappers;continue; }
        CHECK(row.size()==6 && (row[0]=="C" || row[0]=="E"));const auto owner=Number(row[2]),color=Number(row[3]);auto raw=pool.AllocatedBytes({50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x5034c8);raw[10]=82;raw[40]=row[0]=="E" ? 2 : 0;raw[34]=static_cast<std::uint8_t>(owner);Put(raw,36,Number(row[1]));
        ShapeToBufferState state;state.addTypeHotspot=true;state.ownerTypeMaps=tokens;std::size_t selected{};
        state.ownerColors=[&](std::uint32_t type,std::uint32_t currentOwner)->std::span<const std::uint8_t> { CHECK(type==82 && currentOwner==owner);++selected;return maps.at(color); };
        state.mainFrame=[&](Sid sid) { return bridgeDraw.MainFrame(sid); };state.additional=[&](Sid sid,ConstructionVisualImage&,std::int32_t x,std::int32_t y) { bridgeDraw.Additional(sid,x,y); }; // 실제 getter/기본0 추가 그림의 같은 생산 몸체다.
        RawShapeToBuffer capture(pool,types,assets.shapes,assets.game.Shapes(),state);const auto image=capture.Capture({50});std::vector<std::uint8_t> metadata;
        // signed 메타데이터도 원본 little endian DWORD 그대로 비교한다.
        for (const auto value:{image.width,image.height,image.hotspotX,image.hotspotY}) for (std::size_t i=0;i<4;++i) metadata.push_back(static_cast<std::uint8_t>(static_cast<std::uint32_t>(value)>>(i*8)));
        CHECK(metadata==DecodeBridge(row[4]) && image.pixels==DecodeBridge(row[5]) && selected==(owner ? 1U : 0U));++captures;
    }
    CHECK(captures==1200 && wrappers==3);
}
