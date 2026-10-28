// 기존Sprite/Preview의동일계산을재사용하고아직노출되지않은1.6RGB/명도표만원문전체9표선관측에대조한다.
#include "client/ClientColorSortInput.h"
#include "client/OwnerColorTableState.h"
#include "client/CurrentAllocationBytes.h"
#include "o/RawColorSortPrimary.h"
#include "client/SpriteColorMaps.h"
#include "client/PreviewColorMaps.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
ClientColorSortInput::ClientColorSortInput():ownerTables_(std::make_shared<OwnerColorTableState>()) {} // 현재 표는 입력 캡처/Session 교체로 재생성하지 않는다.
const std::shared_ptr<OwnerColorTableState>& ClientColorSortInput::OwnerTables() const { return ownerTables_; } // 같은 장기 owner의 정체성을 빌린다.
// 기존 독립 adapter의 logical캡처는 실제primary가 있다고 가장하지않고 이전raw연결을끊는다.
void ClientColorSortInput::Capture(std::span<const std::uint32_t,256> logical) { primary_.reset();std::copy(logical.begin(),logical.end(),logical_.begin());captured_=true;built_=false; }
// 기존 장치와 같은 실제malloc 초기BYTE를 읽으며 임의0이나 별도Client소유자를 만들지않는다.
void ClientColorSortInput::CaptureWithPrimary(std::span<const std::uint32_t,256> logical) {
    std::array<std::uint8_t,o::kColorSortPrimaryBytes> allocation; // helper가전체를채우는실제초기물리의필수입력이다.
    CurrentAllocationBytes(allocation);
    CaptureWithPrimary(logical,allocation);
}
// 정상 constructor반환뒤에만새primary를발행한다.실패때부분raw를성공입력으로제공하지않는다.
void ClientColorSortInput::CaptureWithPrimary(std::span<const std::uint32_t,256> logical,std::span<const std::uint8_t> allocationBytes) {
    primary_.reset();captured_=false;built_=false;
    std::copy(logical.begin(),logical.end(),logical_.begin());
    auto primary=std::make_shared<o::RawColorSortPrimary>(allocationBytes); // raw클래스가초기padding전체를그대로소유한다.
    primary->Construct({reinterpret_cast<const std::uint8_t*>(logical_.data()),sizeof(logical_)});
    primary_=std::move(primary);captured_=true;
}
// 같은현재슬롯만빌린다.기존독립logical adapter에숨은raw생성을추가하지않는다.
const std::shared_ptr<o::RawColorSortPrimary>& ClientColorSortInput::Primary() const {
    if (!primary_ || !primary_->Constructed()) throw std::logic_error("ColorSort 실제primary 미연결");return primary_;
}
bool ClientColorSortInput::Captured() const { return captured_; }
std::span<const std::uint32_t,256> ClientColorSortInput::Logical() const { if (!captured_) throw std::logic_error("ColorSort 최초 논리RGB 미캡처");return logical_; }
const std::array<ColorMap,9>& ClientColorSortInput::Maps() const { if (!built_) throw std::logic_error("ColorSort 실제9표 미생성");return maps_; }
const std::array<ColorMap,9>& ClientColorSortInput::Rebuild(std::uint32_t currentRed) {
    const auto logical=Logical(); //현재Screen팔레트로치환하지않는같은초기입력주소다.
    const auto sprite=BuildSpriteColorMaps(logical);const auto preview=BuildPreviewColorMaps(logical,currentRed); //기존검증본문을그대로합성한다.
    maps_[1]=preview.destroy;maps_[2]=preview.illegal;maps_[4]=sprite.bright;maps_[5]=preview.labelShadow;maps_[6]=sprite.dead;maps_[7]=sprite.shadow;
    constexpr double normalize=1.0/255.0,scale=1.600000023841858,offset=0.20000000298023224,third=0.3333333333333333; //원문503608/5066f8/5066e0/506558의double상수다.
    std::array<std::array<double,3>,256> rgb{};std::array<double,256> light{}; //원문439de0의저장double의미필드다.
    for (std::size_t index=0;index<256;++index) { //고정초기RGB의정규화와명도를원본번호순서로작성한다.
        rgb[index]={(logical[index]&255)*normalize,((logical[index]>>8)&255)*normalize,((logical[index]>>16)&255)*normalize};
        light[index]=(std::max({rgb[index][0],rgb[index][1],rgb[index][2]})+std::min({rgb[index][0],rgb[index][1],rgb[index][2]}))*0.5;
    }
    for (std::size_t index=0;index<256;++index) { //두미노출표와항등표의전체256BYTE를같은초기입력에서작성한다.
        const std::array<double,3> target{rgb[index][0]*scale,rgb[index][1]*scale,rgb[index][2]*scale}; //원문은1.6배값을1로clamp하지않는다.
        const auto targetLight=(light[index]+offset)*third;float minimum=1e6f;double lightMinimum=1e6;std::uint8_t selected=0; //RGB최솟값만float로좁힌다.
        for (std::size_t candidate=0;candidate<256;++candidate) { //동률/색번호순서를보존한현재후보검색이다.
            const auto distance=std::abs(target[0]-rgb[candidate][0])+std::abs(target[1]-rgb[candidate][1])+std::abs(target[2]-rgb[candidate][2]);
            if (distance<minimum) { minimum=static_cast<float>(distance);maps_[0][index]=static_cast<std::uint8_t>(candidate); }
            const auto lightDistance=std::abs(targetLight-light[candidate]); //HLS명도최솟값은double로그대로저장한다.
            if (lightDistance<lightMinimum) { lightMinimum=lightDistance;selected=static_cast<std::uint8_t>(candidate); }
        }
        maps_[8][index]=selected;maps_[3][index]=static_cast<std::uint8_t>(index);
    }
    for (std::size_t index=228;index<=245;++index) maps_[0][index]=static_cast<std::uint8_t>(index); //43c010꼬리의첫표예약번호덮어쓰기다.
    maps_[0][254]=254;maps_[0][255]=255;maps_[8][254]=254;maps_[8][255]=255;built_=true;
    return maps_;
}
}
