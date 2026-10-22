// 원본의 호출/순회/절삭/난수 소비와현재필드재조회순서를보존한다.
#include "o/RawAiBridgePlanning.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본DWORD의signed해석을구현의존변환없이적용한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 실제__ftol의signed64하위DWORD이며indefinite하위는0이다.
std::uint32_t Truncate(float value) { if (!std::isfinite(value) || static_cast<double>(value)<-0x1p63 || static_cast<double>(value)>=0x1p63) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(value)); }
// 실제슬롯의비정렬DWORD만읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) {
    if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 다리 계획 DWORD 범위");std::uint32_t result{};
    for (std::size_t index=0;index<4;++index) result|=static_cast<std::uint32_t>(raw[at+index])<<(index*8); //네BYTE를little endian으로조합한다.
    return result;
}
}
// 생성시지도/본체/난수효과는없다.
RawAiBridgePlanning::RawAiBridgePlanning(const RawAiBridgeCandidates& candidates,GameRandom& random,const double& now,std::function<std::uint32_t(Sid)> graphFor):candidates_(candidates),random_(random),now_(now),graphFor_(std::move(graphFor)) {}
// 원본은free/dead필터나graph재비교를가장자리판정에추가하지않는다.
bool RawAiBridgePlanning::Edge(std::uint32_t token,std::uint32_t x,std::uint32_t y,std::uint8_t side) const {
    if (x>=256 || y>=256) return false;const Sid surface{candidates_.surfaces_[static_cast<std::size_t>(y)*256+x]};if (!surface.value) return false;const auto raw=candidates_.pool_.Slot(surface);const auto type=raw[10];
    if (static_cast<std::uint32_t>(type)!=candidates_.globals_.bridge || raw[34]!=candidates_.Field(token,0x260)) return false;const auto frame=Read(raw,36);if (type<70 || type>=candidates_.frames_.size() || frame>=candidates_.frames_[type].Codes().size()) throw std::out_of_range("AI 다리 연결 현재 코드 범위");return candidates_.frames_[type].Codes()[frame].side==side;
}
// graph하위뒤현재본체와실제목표좌표를다시읽으며J12/K12로범위를넓히지않는다.
bool RawAiBridgePlanning::Connected(std::uint32_t token,Sid geyser) const {
    static_cast<void>(candidates_.Body(token));if (!graphFor_) throw std::logic_error("AI 다리 연결 현재 graph 미연결");const auto graph=graphFor_(geyser);if (graph!=candidates_.Field(token,0x298)) return false;const auto raw=candidates_.pool_.Slot(geyser);const auto x=Truncate(std::bit_cast<float>(Read(raw,14))),y=Truncate(std::bit_cast<float>(Read(raw,18)));
    for (std::uint32_t offset=0;offset<3;++offset) { //x-2..x에서북/남의J를이순서로검사한다.
        if (Edge(token,x-2U+offset,y-3U,'J') || Edge(token,x-2U+offset,y+1U,'J')) return true;
    }
    for (std::uint32_t offset=0;offset<3;++offset) { //y-2..y에서서/동의K를이순서로검사한다.
        if (Edge(token,x-3U,y-2U+offset,'K') || Edge(token,x+1U,y-2U+offset,'K')) return true;
    }
    return false;
}
// 실제count전체DWORD가0인지만검사하고다른본문바이트를보존한다.
void RawAiBridgePlanning::DelayRetry(std::uint32_t token) const {
    const auto delay=candidates_.Field(token,0x350)==0 ? 40.0 : 120.0;const auto bits=std::bit_cast<std::uint64_t>(now_+delay);candidates_.Field(token,0x5d0,static_cast<std::uint32_t>(bits));candidates_.Field(token,0x5d4,static_cast<std::uint32_t>(bits>>32));
}
// 고정graph/owner와절삭한사각형을보관하지만현재지도/코드는각후보마다읽는다.
RawPathPoint RawAiBridgePlanning::RandomStart(std::uint32_t token,const AiBuildSearchBounds& bounds) const {
    const auto graph=candidates_.Field(token,0x298),owner=candidates_.Field(token,0x260);const auto bottom=Truncate(bounds.bottom),top=Truncate(bounds.top),right=Truncate(bounds.right),left=Truncate(bounds.left); //원본의bottom→top→right→left CRT순서다.
    for (int remaining=20;remaining>0;--remaining) { //상한제외y→x두난수를항상소비하고최대20번시도한다.
        const auto y=random_.Next(bottom-top)+top,x=random_.Next(right-left)+left;if (x>=256 || y>=256) continue;const Sid surface{candidates_.surfaces_[static_cast<std::size_t>(y)*256+x]};
        if (surface.value && candidates_.Graph(surface)==graph && candidates_.pool_.Slot(surface)[34]==owner && (!candidates_.Spot(Signed(x)-1,Signed(y)) || !candidates_.Spot(Signed(x)+1,Signed(y)) || !candidates_.Spot(Signed(x),Signed(y)-1) || !candidates_.Spot(Signed(x),Signed(y)+1)) && candidates_.Open(surface,Signed(x),Signed(y)).value) return {static_cast<float>(Signed(x)),static_cast<float>(Signed(y))};
    }
    return {-10000,-10000};
}
// 행경계의실제flat주소도보존하며물리지도바깥은기존spotgetter가거부한다.
bool RawAiBridgePlanning::RectangleFree(std::int32_t left,std::int32_t top,std::int32_t right,std::int32_t bottom) const {
    for (auto x=static_cast<std::uint32_t>(left);Signed(x)<=right;++x) { //원본의inclusive X순회이며반전폭은빈사각형이다.
        for (auto y=static_cast<std::uint32_t>(top);Signed(y)<=bottom;++y) if (candidates_.Spot(Signed(x),Signed(y))!=0) return false; //각열의inclusive Y순서에서첫점유즉시실패한다.
    }
    return true;
}
}
