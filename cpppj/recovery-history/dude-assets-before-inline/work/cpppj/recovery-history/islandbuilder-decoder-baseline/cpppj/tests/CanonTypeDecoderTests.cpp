// 세 실제 PE의 타입별 패턴/전체 순회/범위/끝 상태와 공통 생성자를 대조한다.
#include "RawSceneSupport.h"
#include "o/CanonTypeDecoder.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// PE 초기 전역의 타입 번호와 일반 사제 번호다.
constexpr std::array<std::uint32_t,9> kTypes{107,82,94,157,131,129,140,142,158};
// 기계어 분석 입력과 같은 프레임 순서다. 기대 결과는 fixture에서만 읽는다.
RiftTypeFrames InputFrames(int profile) {
    std::vector<FrameCode> codes;
    // 정상 표 또는 빈 비패턴 표를 만든다.
    for (int side='A';side<='P';++side)
        // 번호 0~10은 셀 해석의 정상 계약 범위다.
        for (int number=0;number<=10;++number) if (profile!=2) codes.push_back({static_cast<std::uint8_t>(side),'P',static_cast<std::uint8_t>(number),0});
    if (profile==1) std::reverse(codes.begin(),codes.end());
    if (profile!=2) codes.push_back(codes.front());
    return RiftTypeFrames(std::move(codes));
}
// 비트/정수 사각형/선택 표와 현재 원본 상태를 C++ 결과에 대조한다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";const auto version=patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    const auto rows=LoadFixture(NETSTORM_CANONTYPE_FIXTURE).rows;std::size_t count=0;
    // 모든 회전과 타입 전역의 별칭 입력을 독립적으로 구성한다.
    for (const auto& row:rows) {
        if (row[0]!=edition) continue;CHECK(row.size()==21);const auto kind=Number(row[1]);const auto frames=InputFrames(std::stoi(row[4]));
        std::array<std::uint32_t,8> patterns;std::copy_n(kTypes.begin(),8,patterns.begin());
        if (row[10]=="1") patterns[0]=patterns[1];if (row[10]=="2") patterns[2]=patterns[4];
        auto decoder=DecodeCanonType(frames,std::stoi(row[11]),patterns,{kTypes[kind],std::stoi(row[2]),std::stoi(row[3]),
            std::bit_cast<float>(Number(row[5])),std::bit_cast<float>(Number(row[6])),Number(row[9])!=0},version);
        const auto bounds=Split(row[13],',');const auto actualBounds=decoder.Bounds(std::stoi(row[7]),std::stoi(row[8]));
        // 각 사각형의 기대 정수는 실제 Bounds getter 출력이다.
        for (std::size_t i=0;i<4;++i) CHECK(actualBounds[i]==std::stoi(bounds[i]));
        const auto expected=row[14]=="-" ? std::vector<std::string>{} : Split(row[14],';');
        // 생성자의 첫 Advance와 전체 남은 칸 순서/라벨/방향을 비교한다.
        for (const auto& entry:expected) {
            CHECK(decoder.Valid());if (!decoder.Valid()) break;const auto fields=Split(entry,',');
            CHECK(decoder.Frame()==std::stoi(fields[0]) && std::bit_cast<std::uint32_t>(decoder.X())==Number(fields[1]) && std::bit_cast<std::uint32_t>(decoder.Y())==Number(fields[2]));
            CHECK(decoder.Label()==std::stoi(fields[3]) && decoder.Side()==std::stoi(fields[4]));decoder.Advance();
        }
        CHECK(!decoder.Valid() && decoder.Frame()==std::stoi(row[15]) && Number(row[16])==0);
        CHECK(std::bit_cast<std::uint32_t>(decoder.X())==Number(row[17]) && std::bit_cast<std::uint32_t>(decoder.Y())==Number(row[18]) && decoder.Label()==std::stoi(row[19]));
        const auto end=Split(row[20],',');const auto actualEnd=decoder.Bounds(std::stoi(row[7]),std::stoi(row[8]));
        // 칸이 없어도 마지막 좌표/개수로 계산한 원본 범위를 보존한다.
        for (std::size_t i=0;i<4;++i) CHECK(actualEnd[i]==std::stoi(end[i]));
        ++count;
    }
    CHECK(count==(patch ? 2079U : 924U));
}
}
// 패치의 홀수 방향·signed -1까지 원본 생성/전체 순회 결과를 대조한다.
TEST_CASE(canon_type_patch_x86) { Replay("originals"); }
// CD의 다른 생성 코드와 실제 범위/프레임 helper를 대조한다.
TEST_CASE(canon_type_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE도 자체 기계어 기대값을 사용한다.
TEST_CASE(canon_type_1037_x86) { Replay("original1037"); }
// 패턴 수/선행 전역 비교·범위 밖/홀수 CD와 C++의 누락 프레임 계약을 검사한다.
TEST_CASE(canon_type_tables_precedence_and_guards) {
    CHECK(CanonSpecialPatterns(0).size()==2 && CanonSpecialPatterns(1).size()==1 && CanonSpecialPatterns(2).size()==1);
    CHECK(CanonSpecialPatterns(0)[1].cells[0][0]=='3' && CanonSpecialPatterns(1)[0].width==3 && CanonSpecialPatterns(2)[0].cells[0][1]=='L');
    const auto frames=InputFrames(0);std::array<std::uint32_t,8> patterns;std::copy_n(kTypes.begin(),8,patterns.begin());
    CHECK(Throws([] { CanonSpecialPatterns(3); }));
    // 패턴 표 밖이나 CD 홀수 방향은 원본 assert/UI/잘못된 접근 대신 예외로 진단한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        CHECK(Throws([&] { DecodeCanonType(frames,3,patterns,{94,2,0,0,0,false},edition); }));
        CHECK(Throws([&] { DecodeCanonType(frames,3,patterns,{157,-1,0,0,0,false},edition); }));
        CHECK(Throws([&] { DecodeCanonType(frames,3,patterns,{131,1,0,0,0,true},edition); }));
        const auto missing=InputFrames(2);auto empty=DecodeCanonType(missing,3,patterns,{157,0,0,0,0,false},edition);CHECK(!empty.Valid());
    }
    CHECK(Throws([&] { DecodeCanonType(frames,3,patterns,{94,0,1,0,0,false},OriginalEdition::Cd1072); }));
    CHECK(Throws([&] { DecodeCanonType(frames,3,patterns,{94,0,8,0,0,false},OriginalEdition::Patch1078); }));
    CHECK(Throws([&] { DecodeCanonType(frames,3,patterns,{94,0,0,std::numeric_limits<float>::infinity(),0,false},OriginalEdition::Patch1078); }));
    auto explicitPattern=DecodeCanonType(frames,3,patterns,{94,1,0,0,0,true},OriginalEdition::Patch1078);CHECK(explicitPattern.Frame()==168);
}
