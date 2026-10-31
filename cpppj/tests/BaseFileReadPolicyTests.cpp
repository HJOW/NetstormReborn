// 실제 FileSpec/조회 부모가 고정한1024개 trace를 공용 읽기 정책과 비교한다.
#include "RawSceneSupport.h"
#include "o/RawBaseFileReadPolicy.h"

// 원시 입력과 출력의 변환만 공유하며 기대 분기나 접두 길이는 계산하지 않는다.
namespace {
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// 명시 hex의 전체 BYTE를 복원한다. 뒤의 NUL도 입력으로 유지한다.
std::string PolicyBytes(std::string_view value) {
    std::string result;if (value=="-") return result;
    for (std::size_t byte=0;byte<value.size();byte+=2) result+=static_cast<char>(std::stoul(std::string(value.substr(byte,2)),nullptr,16)); // 원시 칸의 순서/폭을 바꾸지 않는다.
    return result;
}
// 실제 하위 조회에 전달된 C문자열만 hex로 기록한다.
std::string PolicyHex(std::string_view value) {
    value=value.substr(0,value.find('\0'));return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
}
// 최초 불일치의 행/필드와 실제 출력도 남겨 전체1024입력 중 실패 위치를 찾는다.
void PolicyCheck(std::size_t row,const char* field,const std::string& actual,const std::string& expected) {
    if (actual!=expected) std::fprintf(stderr,"BaseFilePolicy row%zu %s actual=%s expected=%s\n",row,field,actual.c_str(),expected.c_str());CHECK(actual==expected);
}
}

// 장치 판정/직접 성공/접두 일부·대소문자·UNC·실패·중단을 원문 조회 인자까지 대조한다.
TEST_CASE(BaseFile_ReadPolicyReplaysIndependentNative1024) {
    const auto rows=LoadFixture(NETSTORM_BASEFILE_READ_POLICY_FIXTURE).rows;std::size_t count{}; // 최초 Native 결과만 기대값으로 사용한다.
    for (const auto& row:rows) { // 모든 조회 성공 mask와 아카이브 열기 실패 조합을 같은 입력으로 재생한다.
        const auto root=PolicyBytes(row.at(0)),name=PolicyBytes(row.at(1));const auto mask=Number(row.at(2));const bool opened=Number(row.at(3))!=0;std::string trace,outcome;int selected=-1;bool archiveFailed{};
        try {
            const auto result=ReadBaseFileByPolicy(name,root,[&](BaseFileReadSite site,std::string_view query)->std::optional<std::vector<std::uint8_t>> {
                const auto code=static_cast<std::uint32_t>(site);auto actual=std::string(query); // 원문 parent가 선택한 현재 조회 이름이다.
                if (site==BaseFileReadSite::PrimaryDisk || site==BaseFileReadSite::SecondaryDisk) actual=JoinBaseFileRoot(site==BaseFileReadSite::PrimaryDisk?root:"D:\\disc",query);
                if (!trace.empty()) trace+=';';trace+=std::to_string(code)+':'+PolicyHex(actual);
                if (!(mask&(1U<<code))) return {};
                if ((site==BaseFileReadSite::PrimaryArchive || site==BaseFileReadSite::SecondaryArchive) && !opened) { archiveFailed=true;throw std::runtime_error("명시 아카이브 열기 실패"); }
                selected=static_cast<int>(code);return std::vector<std::uint8_t>{static_cast<std::uint8_t>(code)};
            });
            outcome=result?"ok":"missing";
        }catch (const std::runtime_error&) { if (!archiveFailed) throw;outcome="archive-failed"; }
        PolicyCheck(count,"outcome",outcome,row.at(4));PolicyCheck(count,"selected",std::to_string(selected),row.at(5));PolicyCheck(count,"trace",trace,row.at(6));++count;
    }
    CHECK(count==1024);
}

// 빈 파일 성공은 하위 재조회로 이어지지 않으며 미발견만 nullopt로 보존한다.
TEST_CASE(BaseFile_ReadPolicyEmptyFileIsSuccessAndMissingLookupIsExplicit) {
    std::size_t calls{};const auto result=ReadBaseFileByPolicy("C:\\base\\empty","C:\\base",[&](auto site,auto)->std::optional<std::vector<std::uint8_t>> { ++calls;CHECK(site==BaseFileReadSite::DirectDisk);return std::vector<std::uint8_t>{}; });
    CHECK(result.has_value() && result->empty() && calls==1);
    bool rejected{};try { static_cast<void>(ReadBaseFileByPolicy("d\\a","C:\\base",{})); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}
