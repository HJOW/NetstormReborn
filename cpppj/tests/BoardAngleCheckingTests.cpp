// 실제 CRT fmod와 현재 checking의 양판본 원문 출력을 이진 double 전체로 비교한다.
#include "RawSceneSupport.h"
#include "o/RawBoardAngle.h"
#include "client/RawSurfaceWorld.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // Native fstp qword의 결과 바이트와 같은 순서로 직렬화한다.
// 64비트 double을 padding이나 로케일 변환 없이 원문 little endian HEX로 보존한다.
std::string DoubleHex(double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);std::vector<std::uint8_t> bytes(8);
    // 원문 Native 저장의 여덟 바이트를 비교한다.
    for (std::size_t index=0;index<8;++index) bytes[index]=static_cast<std::uint8_t>(bits>>(index*8));
    return Hex(bytes);
}
}
// 별도 modulus720은 원문의 float360/361과 독립된 실제 double 전역이다. CRT 결과 주입 없이 진단을 선택한다.
TEST_CASE(BoardAngleChecking_ReplaysNativeRealFmodAndCurrentGate) {
    const auto rows=LoadFixture(NETSTORM_BOARD_ANGLE_CHECKING_FIXTURE).rows;std::size_t count=0;
    // 기대값은 두 실제 PE와 CRT의 결과이며 C++ 모델로 다시 계산하지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==8);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        std::uint32_t checking=Number(row[3]);const auto action=Number(row[4]);std::string events;
        BoardAngleDiagnostics diagnostics{RawCheckingFlag{checking},[&](auto expression,auto file,auto line) {
            if (!events.empty()) events+='|';events+=std::to_string(line)+':'+std::string(expression)+':'+std::string(file);if (action) checking=0;
        }};
        const auto result=DoubleHex(NormalizeBoardAngle(std::bit_cast<float>(Number(row[1])),edition,diagnostics,std::stod(row[2])));
        const bool equal=result==row[5] && checking==Number(row[6]) && (events.empty() ? "-" : events)==row[7];
        if (!equal) std::printf("각도 불일치 %s bits=%s modulus=%s checking=%s action=%s expected=%s actual=%s expectedEvents=%s actualEvents=%s\n",row[0].c_str(),row[1].c_str(),row[2].c_str(),row[3].c_str(),row[4].c_str(),row[5].c_str(),result.c_str(),row[7].c_str(),events.c_str());
        CHECK(equal);++count;
    }
    CHECK(count==1088);
}
// 실제 Storage가 만든 같은 현재 Client 뷰는 생성 뒤 checking 변경과 진단 복귀를 그대로 전달한다.
TEST_CASE(BoardAngleChecking_StorageViewUsesCurrentOwnerAndOriginalDiagnostic) {
    using namespace netstorm::client;std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);
    RawSurfaceStorage storage(OriginalEdition::Patch1078,types,frames,shapes);std::int32_t checking=0;storage.BindChecking(RawCheckingFlag{checking});
    std::optional<std::string> detail;std::size_t assertions=0;storage.BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) {
        CHECK(expression=="angle < 361.0f" && file=="\\Ns\\O\\Boardcoord.cpp" && line==0xb3);++assertions;checking=0;
    });
    const auto view=storage.AngleDiagnostics();CHECK(NormalizeBoardAngle(361,OriginalEdition::Patch1078,view,720)==359.9999F && assertions==0);
    checking=std::bit_cast<std::int32_t>(0x80000000U);CHECK(NormalizeBoardAngle(361,OriginalEdition::Patch1078,view,720)==359.9999F && assertions==1 && checking==0);
    checking=1;CHECK(NormalizeBoardAngle(361,OriginalEdition::Cd1072,view,720)==359.9999F && assertions==1 && checking==1);
}
// 장치 누락은 원문 진단이 선택될 때만 실패하며 CD는 장치를 요구하지 않는다.
TEST_CASE(BoardAngleChecking_MissingSelectedServiceHasExactBoundary) {
    BoardAngleDiagnostics diagnostics;diagnostics.checking=1U;bool rejected=false;
    CHECK(NormalizeBoardAngle(360.5F,OriginalEdition::Patch1078,diagnostics,720)==359.9999F);
    try { static_cast<void>(NormalizeBoardAngle(361,OriginalEdition::Patch1078,diagnostics,720)); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);CHECK(NormalizeBoardAngle(361,OriginalEdition::Cd1072,diagnostics,720)==359.9999F);
}
