// Client의 같은 raw 색상 기록이 Screen 교체와 전용 소유자 색표까지 전달되는지 검사한다.
#include "TestSupport.h"
#include "client/ClientColorSortInput.h"
#include "client/OwnerColorMaps.h"
#include "client/Screen.h"
#include "o/RawColorSortPrimary.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using namespace netstorm::client;
using netstorm::o::kColorSortPrimaryBytes;
// RGBX 입력은 서로 다른 회색 256개이며 예약 BYTE도 유지한다.
std::array<std::uint32_t,256> PrimaryGrayPalette() {
    std::array<std::uint32_t,256> logical{}; // 파일이나 화면에 의존하지 않는 명시 입력이다.
    for (std::size_t index=0;index<logical.size();++index) logical[index]=0xa5000000U|static_cast<std::uint32_t>(index)*0x010101U; // 각 RGB 채널에 같은 번호를 쓴다.
    return logical;
}
// 원시 기록의 저장 L만 명시 변경하며 RGB와 다른 필드에는 쓰지 않는다.
void StorePrimaryLightness(netstorm::o::RawColorSortPrimary& primary,std::size_t index,double value) {
    std::memcpy(primary.Bytes().data()+index*56+8,&value,sizeof(value));
}
}

// 창 없는 두 Screen의 팔레트가 바뀌어도 같은 Client primary의 주소와 전체 BYTE는 유지된다.
TEST_CASE(ClientColorSortPrimary_TwoScreensKeepInitialRawStorage) {
    auto owner=std::make_shared<ClientColorSortInput>();const auto logical=PrimaryGrayPalette(); // 두 화면보다 오래 사는 실제 입력 소유자다.
    std::array<std::uint8_t,kColorSortPrimaryBytes> allocation;allocation.fill(0xa7); // 생성자가 쓰지 않는 padding의 명시 입력이다.
    owner->CaptureWithPrimary(logical,allocation);const auto primary=owner->Primary(); // 실제 기록을 한 번만 구성한다.
    const auto* address=primary->Bytes().data();const std::vector<std::uint8_t> before(primary->Bytes().begin(),primary->Bytes().end()); // 전체 물리와 주소를 따로 비교한다.
    std::array<ScreenColor,256> colors{}; // 현재 화면의 일시 입력이며 고정 RGBX와 다르다.
    for (const int width:{1024,1280}) { // Screen을 반납하고 다른 비율의 Screen을 만들어 같은 입력에 연결한다.
        Screen screen(nullptr,nullptr,width,720);screen.BindColorSortInput([owner] { return owner->Logical(); });
        for (const std::uint8_t red:{std::uint8_t{71},std::uint8_t{193}}) { // 같은 화면도 두 번 팔레트를 변경한다.
            colors[17]={33,91,red,0};screen.SetPalette(0,256,colors.data(),false);
            CHECK(screen.LogicalPalette()[17]!=logical[17]);
            CHECK(owner->Primary().get()==primary.get() && primary->Bytes().data()==address);
            CHECK(std::equal(before.begin(),before.end(),primary->Bytes().begin()));
            CHECK(std::equal(logical.begin(),logical.end(),owner->Logical().begin()));
        }
    }
    for (std::size_t index=0;index<256;++index) { // +52..55는 constructor가 쓰지 않은 초기 물리 그대로다.
        CHECK(std::all_of(primary->Bytes().begin()+index*56+52,primary->Bytes().begin()+index*56+56,[](auto byte) { return byte==0xa7; }));
    }
}

// 전용 다섯 타입은 기존 결과를 유지하며 raw L만 바꾸면 현재 저장값을 직접 읽는다.
TEST_CASE(ClientColorSortPrimary_SpecialOwnerMapsReadStoredLightness) {
    ClientColorSortInput owner;const auto logical=PrimaryGrayPalette(); // 모든 전용 타입에 같은 고정 입력을 준다.
    std::array<std::uint8_t,kColorSortPrimaryBytes> allocation;allocation.fill(0x63);owner.CaptureWithPrimary(logical,allocation);
    const auto primary=owner.Primary(); // 아래 명시 변경도 실제 owner의 같은 기록에 수행한다.
    for (const std::uint32_t type:{111U,141U,137U,94U,97U}) { // 전용 소스 목록과 보정이 서로 다른 다섯 소비자를 비교한다.
        CHECK(BuildSpecialOwnerColorMaps(*primary,type)==BuildSpecialOwnerColorMaps(logical,type));
    }
    for (std::size_t index=0;index<256;++index) StorePrimaryLightness(*primary,index,0.0); // RGB는 그대로 두고 L만 별도 입력으로 만든다.
    StorePrimaryLightness(*primary,86,0.75);StorePrimaryLightness(*primary,7,0.75);
    std::array<std::uint32_t,72> candidates{}; // 모든 owner가 1..8의 명시 후보를 사용한다.
    for (std::size_t row=1;row<9;++row) { // 중립 행은 원문처럼 항등으로 유지한다.
        for (std::size_t shade=0;shade<8;++shade) candidates[row*8+shade]=static_cast<std::uint32_t>(shade+1); // 동률 없이 후보 7만 저장 L과 일치한다.
    }
    const auto raw=BuildSpecialOwnerColorMaps(*primary,111,candidates),legacy=BuildSpecialOwnerColorMaps(logical,111,candidates); // 재계산 adapter와 실제 저장값 소비를 구별한다.
    CHECK(raw[0][86]==86 && raw[1][17]==17 && raw!=legacy);
    for (std::size_t row=1;row<9;++row) CHECK(raw[row][86]==7 && legacy[row][86]==8); // RGB 재계산이면 이 기대를 통과할 수 없다.
    CHECK(std::equal(logical.begin(),logical.end(),owner.Logical().begin()));
}

// 누락/잘못된 초기 물리를 조용히 성공시키지 않으며 실제 malloc helper 경로도 같은 raw를 생성한다.
TEST_CASE(ClientColorSortPrimary_RequiresRawAndRejectsFailedRecapture) {
    ClientColorSortInput owner;const auto logical=PrimaryGrayPalette();bool rejected{}; // 독립 logical 캡처와 실제 raw 생성은 별개다.
    owner.Capture(logical);
    try { static_cast<void>(owner.Primary()); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);owner.CaptureWithPrimary(logical);CHECK(owner.Primary()->Constructed() && owner.Captured());
    const auto retained=owner.Primary();owner.Capture(logical);rejected=false; // 기존 소비자가 보유한 이전 raw는 참조 수명까지 유효하다.
    try { static_cast<void>(owner.Primary()); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && retained->Constructed());
    std::array<std::uint8_t,8> shortAllocation{};rejected=false; // 정확 0x3800이 아닌 입력은 실패해야 한다.
    try { owner.CaptureWithPrimary(logical,shortAllocation); }catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && !owner.Captured());rejected=false;
    try { static_cast<void>(owner.Primary()); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && retained->Constructed());
}
