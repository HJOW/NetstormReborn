// 제품 작성 전 관측한 Dude24 생성자 전체와 실제 공유 가상 슬롯을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPathRuntime.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 생성 성공3개는 제품 전체 이미지, 실패3개는 원본의 heap 보존과 비교한다.
TEST_CASE(DudePath_NativeConstructorAndSharedVtables) {
    std::size_t constructors{},attachments{}; // 서로 다른 두 원본 관측을 구분한다.
    // 원본30행의 두 정밀도 결과를 확인하되 부착 경계 관측을 제품 Host 실행으로 표시하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_DUDE_PATH_FIXTURE).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;CHECK(row[1]==row[2]);const auto input=Split(row[0],',');CHECK(input.size()==4);if (input.size()!=4) return;
        if (Number(input[0])==1) { CHECK(row[1].find("attach:24:51:")!=std::string::npos);++attachments;continue; }
        const auto result=Split(row[1],':');CHECK(result.size()==2);if (result.size()!=2) return;const auto fill=static_cast<std::uint8_t>(Number(input[1]));
        if (Number(input[2])) { RawPathRuntime runtime(OriginalEdition::Patch1078,false,fill,false,false,false,true);CHECK(Number(result[0])==1 && Adler(runtime.Bytes())==Number(result[1]) && Get(runtime.Bytes(),0)==0x50e3d8U); }
        else { const std::vector<std::uint8_t> heap(0x186f0,fill);CHECK(Number(result[0])==0 && Adler(heap)==Number(result[1])); }++constructors;
    }
    CHECK(constructors==6 && attachments==24);std::map<std::uint32_t,std::vector<std::uint32_t>> tables;
    // 보호PE의 실제 주소를 호스트에서 실행하지 않고 공유 몸체 선택 근거로 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_DUDE_PATH_VTABLES).rows) {
        std::vector<std::uint32_t> values;
        // 각 슬롯 DWORD의 순서와 전체 폭을 보존한다.
        for (const auto& value:row) values.push_back(Number(value));if (!values.empty()) tables.emplace(values[0],std::move(values));
    }
    CHECK(tables.size()==3);const auto& dude=tables.at(24);const auto& priest=tables.at(54);const auto& parent=tables.at(70);CHECK(dude.size()==15 && priest.size()==15 && parent.size()==6);
    // 실제13슬롯 중+2c 부착만 다르며 Init/Run/Arrive/Halt/cleanup/Prepare는 같다.
    for (std::size_t index=2;index<15;++index) if (index!=13) CHECK(dude[index]==priest[index]);
    CHECK(dude[1]==0x50e3d8U && dude[13]==0x48bab0U && priest[13]==0x48bd30U);
    CHECK(parent[1]==0x508cc8U && parent[2]==0x450e60U && parent[3]==0x450960U && parent[4]==0x4437f0U && parent[5]==0x480260U);
}
