// 원본 GIF 창의 현재 포인터 재조회와 입력/원시 픽셀 호출 순서를 유지한다.
#include "client/BackgroundGifGump.h"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace netstorm::client { // 원본 BackgroundGif 창의 구현이다.
namespace { // 원문 상수/선택 장치/숫자 변환을 모은다.
constexpr std::uint32_t kVtable=0x501b60,kRelease=0x80000000,kCodeMask=0x03ffffff; // 실제10.78 가상표와 입력 비트다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("BackgroundGif 실제 장치 미연결");return hook; } // 선택한 효과에서만 장치를 요구한다.
std::int32_t Ftol(double value) { // 원문 _ftol의64비트 절삭 하위DWORD이며 무효값은 하위0이다.
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
BackgroundGifGump::BackgroundGifGump(BackgroundGifGumpState& state,BackgroundGifGumpHooks hooks):state_(state),hooks_(std::move(hooks)) { if (!hooks_.node || !hooks_.base) throw std::invalid_argument("BackgroundGif 같은 실제 노드 필요"); } // 장치는 실제 분기에서 확인한다.
BackgroundGifGumpNode& BackgroundGifGump::Node(std::uint32_t token) const { auto* node=token ? hooks_.node(token) : nullptr;if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::logic_error("BackgroundGif 공유 노드 없음");return *node; } // 표시 사본을 만들지 않는다.
std::uint32_t BackgroundGifGump::Construct(std::uint32_t token,std::string_view name) const {
    Node(token);Need(hooks_.constructBase)(token);auto& node=Node(token);node.vtable=kVtable;node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.kind));node.bitmap=0;
    Refresh(token,name);Node(token).event=0;Node(token).first=0;Node(token).second=0;return token; // 원문은 적재 효과 뒤 세 필드를 쓴다.
}
void BackgroundGifGump::Refresh(std::uint32_t token,std::string_view name) const {
    auto& node=Node(token);std::int32_t width{},height{}; // 원문 두 실제 출력 DWORD의 초기0이다.
    if (node.bitmap) { Need(hooks_.releaseBitmap)(node.bitmap);Node(token).bitmap=0; }
    const auto path=Need(hooks_.pathSpec)(name);if (Need(hooks_.exists)(path)) Node(token).bitmap=Need(hooks_.load)(path,width,height);
    Node(token).baseWidth=static_cast<float>(width);Node(token).width=static_cast<float>(double(width)*Node(token).scale);
    Node(token).baseHeight=static_cast<float>(height);Node(token).height=static_cast<float>(double(height)*Node(token).scale); // FILD 중간값과 FST32 사본을 구별한다.
}
void BackgroundGifGump::SetEvent(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { auto& node=Node(token);node.event=event;node.second=second;node.first=first;node.flags|=0x600; } // 원문 쓰기 순서다.
void BackgroundGifGump::Reattach(std::uint32_t token) const { auto& node=Node(token);Need(hooks_.attach)(token,node.x,node.y,0,0); } // 현재 가상 부착에 맡긴다.
std::uint32_t BackgroundGifGump::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    auto& node=Node(token);const auto code=event&kCodeMask;
    if (node.event && !(event&kRelease) && (code==0x10000 || code==0x20000 || code==27)) { const auto pending=node.event,a=node.first,b=node.second;Need(hooks_.dispatch)(pending,a,b);return 1; } // dispatch가 창을 반납해도 노드를 다시 읽지 않는다.
    return Need(hooks_.baseHandle)(token,event,first,second);
}
void BackgroundGifGump::Draw(std::uint32_t token) const {
    if (!Node(token).bitmap) return;const auto position=Need(hooks_.position)(token);const auto width=Ftol(Node(token).baseWidth),height=Ftol(Node(token).baseHeight);const auto bitmap=Node(token).bitmap;
    Need(hooks_.copy)({position,bitmap,width,{0,0,width,height}}); // 원문은 배율 치수44/48이 아닌 base3c/40으로 복사한다.
}
void BackgroundGifGump::Destruct(std::uint32_t token) const { auto& node=Node(token);node.vtable=kVtable;if (node.bitmap) Need(hooks_.releaseBitmap)(node.bitmap);Need(hooks_.destructBase)(token); } // bitmap0/추가 상태를 임의 저장하지 않는다.
}
