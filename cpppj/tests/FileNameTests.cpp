// 실제459c60의352입력과 현재256BYTE·예외 뒤 부분 출력·Client 수명을 확인한다.
#include "RawSceneSupport.h"
#include "o/RawFileName.h"
#include "client/ClientStaticStrings.h"
#include <array>
#include <string>
#include <vector>

namespace { // fixture 해독만 수행하며 경로 기대값을 계산하지 않는 검사 도구다.
std::string FileNameBytes(std::string_view hex) {
    std::string result;if (hex=="-") return result;
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)); // 원문 입력의 NUL/상위 바이트도 유지한다.
    return result;
}
}
// 기대 결과는 제품 작성 전에 원본 전체 함수/strchr 두 정밀도로 고정한 출력만 사용한다.
TEST_CASE(FileName_ReplaysNative352WholeBuffersAndPartialThrows) {
    using namespace netstorm::test::rawscene;using namespace netstorm::o;std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_FILE_NAME_FIXTURE).rows) { // 현재 칸 전체256BYTE와 예외 유무를 각각 대조한다.
        CHECK(row.size()==6);if (row.size()!=6) throw std::logic_error("경로 원문 fixture 폭");RawFileName formatter;
        for (std::uint32_t advance=0;advance<Number(row[1]);++advance) static_cast<void>(formatter.Format({})); // 같은 초기 BSS0에서 명시 현재 번호까지 빈 호출로 전진한다.
        std::vector<std::pair<std::int32_t,std::string>> storage;
        if (row[2]!="-") for (const auto& field:Split(row[2],';')) { // 임시 vector 확장이 끝난 뒤에만 문자열 뷰를 빌린다.
            const auto colon=field.find(':');CHECK(colon!=std::string::npos);storage.emplace_back(std::stoi(field.substr(0,colon)),FileNameBytes(field.substr(colon+1)));
        }
        std::vector<FileNamePart> parts;
        for (const auto& [kind,text]:storage) parts.push_back({kind,text}); // 원문 가변 인자 순서와 중간 종료0을 그대로 보존한다.
        bool thrown{};std::string_view value;
        try { value=formatter.Format(parts,static_cast<char>(Number(row[0]))); }catch (const std::invalid_argument&) { thrown=true; }
        CHECK(thrown==(Number(row[3])!=0));CHECK(formatter.Index()==Number(row[4]));CHECK(Hex(formatter.CurrentBytes())==row[5]);
        if (!thrown) { const auto bytes=FileNameBytes(row[5]);CHECK(value==std::string_view(bytes.data(),bytes.find('\0')));CHECK(value.data()==reinterpret_cast<const char*>(formatter.CurrentBytes().data())); }
        ++count;
    }
    CHECK(count==352);
}
// 출력은 owning 문자열이 아니며 열 번째 후속 호출의 선행 clear와 동일한 주소에 덮어써진다.
TEST_CASE(FileName_TenSlotsWrapAndInputAliasObservesClear) {
    using namespace netstorm::o;RawFileName formatter;const std::array<FileNamePart,1> firstParts{{{-3,"first"}}};const auto first=formatter.Format(firstParts);CHECK(first==".\\first" && formatter.Index()==1);
    for (std::uint32_t call=0;call<9;++call) static_cast<void>(formatter.Format({})); // 다른 아홉 칸을 사용해도 최초 실제 주소의 바이트는 유지된다.
    CHECK(first==".\\first" && formatter.Index()==0);
    const std::array<FileNamePart,1> aliasParts{{{-3,first}}};const auto aliased=formatter.Format(aliasParts);CHECK(aliased==".\\" && aliased.data()==first.data() && formatter.Index()==1); // 원문은 출력 칸을 먼저0으로 만든 다음 입력 포인터를 읽는다.
    CHECK(first[0]=='.' && first[1]=='\\' && first[2]=='\0');
}
// 일반 임시 고리 초기화와 경로 고리는 다른 원문 전역이며 새 Client 소유자는 독립 BSS0이다.
TEST_CASE(FileName_ClientOwnerKeepsSeparateRingAcrossStaticReinitialization) {
    using namespace netstorm::o;netstorm::client::ClientStaticStrings client,fresh;const std::array<FileNamePart,4> parts{{{-2,""},{-2,"d"},{-3,"folder\\"},{-4,"col"}}};
    auto& paths=client.FileNames();const auto value=paths.Format(parts);CHECK(value==".\\d\\folder.col" && paths.Index()==1);client.Initialize(10240);CHECK(&paths==&client.FileNames() && value==".\\d\\folder.col" && paths.Index()==1);
    CHECK(fresh.FileNames().Index()==0);CHECK(std::all_of(fresh.FileNames().CurrentBytes().begin(),fresh.FileNames().CurrentBytes().end(),[](auto byte) { return byte==0; })); // 다른 장기 소유자의 초기 출력은 섞이지 않는다.
}
// 원문 지역/출력 배열을 넘는 미정 쓰기는 호스트 메모리를 손상시키거나 정상 경로로 절단하지 않는다.
TEST_CASE(FileName_PhysicalOverflowRejectsAndKeepsEarlierSlots) {
    using namespace netstorm::o;RawFileName formatter;const std::array<FileNamePart,1> valid{{{-3,"kept"}}};const auto kept=formatter.Format(valid);const std::string tooLong(256,'x');const std::array<FileNamePart,1> input{{{-3,tooLong}}};bool rejected{};
    try { static_cast<void>(formatter.Format(input)); }catch (const std::length_error&) { rejected=true; }CHECK(rejected && formatter.Index()==2 && kept==".\\kept");
    CHECK(std::all_of(formatter.CurrentBytes().begin(),formatter.CurrentBytes().end(),[](auto byte) { return byte==0; })); // 입력256BYTE 거부는 현재 칸 clear 뒤, 부품 append 전이다.
    const std::string combined(254,'x');const std::array<FileNamePart,1> output{{{-3,combined}}};rejected=false;
    try { static_cast<void>(formatter.Format(output)); }catch (const std::length_error&) { rejected=true; }CHECK(rejected && formatter.Index()==3 && kept==".\\kept");
    const auto bytes=formatter.CurrentBytes();CHECK(bytes[0]=='.' && bytes[1]=='\\' && bytes[2]==0); // 선택된 선행 접두는 보존하지만 넘는 부품은 부분 복사하지 않는다.
}
