// BackgroundGif의 실제 소유 GIF와 기존 공통 입력/클립/수명을 조립한다.
#include "client/RawBackgroundGifControls.h"
#include "client/RawMenuControls.h"
#include "client/GifImage.h"
#include "o/BaseFile.h"
#include "o/ConfigInterface.h"
#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>
namespace netstorm::client { // 실제 GIF 창 조립의 구현이다.
struct RawBackgroundGifControls::Impl { // 모든 원문 콜백은 이 고정 소유자/현재 장치를 읽는다.
    RawMenuTree common;RawBackgroundGifContext context;BackgroundGifGumpState state; // 같은 실제 공통 트리와 현재 Client 경계다.
    std::map<std::uint32_t,std::unique_ptr<BackgroundGifGumpNode>> nodes;std::map<std::uint32_t,IndexedImage> bitmaps; // 현재 고정 노드와 독립 소유 픽셀이다.
    std::uint32_t nextBitmap{0x72000000U}; // 입력/Help/Chat 문자열과 구별하는 호스트 원시 이미지 주소 별칭이다.
    std::unique_ptr<BackgroundGifGump> backend;mutable const GumpPixelTarget* activePixels{}; // 원문 본체와 Draw 안에서만 유효한 장치다.
    Impl(RawMenuTree tree,RawBackgroundGifContext supplied,std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle):common(std::move(tree)),context(std::move(supplied)) { // 실제 파일/설정만 미리 요구한다.
        if (common.create.collection.edition!=o::OriginalEdition::Patch1078 || !context.files || !context.config || !baseHandle) throw std::invalid_argument("BackgroundGif 실제10.78 파일/설정/기본 입력 필요");
        BackgroundGifGumpHooks h;h.node=[this](auto token) { return Node(token); };h.base=common.node;h.constructBase=[this](auto token) { common.factory.Construct(token,0,0,0,1); };h.destructBase=[this](auto token) { common.lifetime.DestructBase(token); };
        h.pathSpec=[this](auto name) { return context.config->PathSpec("BackgroundGifSpec",name); };h.exists=[this](auto path) { return context.files->TryRead(path).has_value(); };
        h.load=[this](auto path,std::int32_t& width,std::int32_t& height) { auto image=DecodeBackgroundGif(context.files->Read(path));if (image.width>std::numeric_limits<std::int32_t>::max() || image.height>std::numeric_limits<std::int32_t>::max()) throw std::overflow_error("BackgroundGif 실제 치수 범위 초과");width=static_cast<std::int32_t>(image.width);height=static_cast<std::int32_t>(image.height);if (nextBitmap==0x72ffffffU) throw std::overflow_error("BackgroundGif 이미지 주소 별칭 소진");const auto token=nextBitmap++;bitmaps.emplace(token,std::move(image));return token; };
        h.releaseBitmap=[this](auto token) { if (!token || bitmaps.erase(token)!=1) throw std::logic_error("BackgroundGif 같은 소유 메모리 반납 불일치"); };
        h.position=[this](auto token) { return common.collection.Position(token); };h.attach=common.attach;h.baseHandle=std::move(baseHandle);h.dispatch=[this](auto event,auto first,auto second) { if (!context.dispatch) throw std::logic_error("BackgroundGif 실제 원시 입력 큐 미연결");context.dispatch(event,first,second); };
        h.copy=[this](auto request) { if (!activePixels) throw std::logic_error("BackgroundGif 실제 Draw 문맥 없음");const auto found=bitmaps.find(request.bitmap);if (found==bitmaps.end()) throw std::logic_error("BackgroundGif 실제 원시 메모리 없음");
            const auto outside=context.outside ? std::function<std::uint8_t(std::int64_t)>([this,token=request.bitmap](auto offset) { return context.outside(token,offset); }) : std::function<std::uint8_t(std::int64_t)>{};
            activePixels->RawBitmap(found->second.indices,request.stride,std::int64_t(request.source.top)*request.stride+request.source.left,{request.source.right-request.source.left,request.source.bottom-request.source.top},request.position,{std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::max()},outside); }; // 원문 raw blt는 GIF 투명 마스크/팔레트 변환/리샘플링을 추가하지 않는다.
        backend=std::make_unique<BackgroundGifGump>(state,std::move(h));
    }
    BackgroundGifGumpNode* Node(std::uint32_t token) const { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : found->second.get(); } // 현재 같은 파생 주소다.
};
RawBackgroundGifControls::RawBackgroundGifControls(RawMenuTree tree,RawBackgroundGifContext context,std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle):impl_(std::make_unique<Impl>(std::move(tree),std::move(context),std::move(baseHandle))) {} // 장기 공통 수명에 연결한다.
RawBackgroundGifControls::~RawBackgroundGifControls()=default; // 공통 수명으로 노드를 먼저 반납한다.
std::uint32_t RawBackgroundGifControls::Allocate() { auto& s=*impl_;const auto token=s.common.token();s.nodes.emplace(token,std::make_unique<BackgroundGifGumpNode>());return token; } // 기본 생성 전부터 실제 저장소를 소유한다.
std::uint32_t RawBackgroundGifControls::Create(std::string_view name) { return impl_->backend->Construct(Allocate(),name); } // 원문 전체 생성자다.
BackgroundGifGumpNode* RawBackgroundGifControls::Node(std::uint32_t token) const { return impl_->Node(token); } // 같은 현재 노드다.
const BackgroundGifGump& RawBackgroundGifControls::Backend() const { return *impl_->backend; } // 같은 본체다.
const IndexedImage& RawBackgroundGifControls::Bitmap(std::uint32_t token) const { return impl_->bitmaps.at(token); } // 실제 소유 바이트를 빌린다.
std::size_t RawBackgroundGifControls::ImageCount() const { return impl_->bitmaps.size(); } // 게임 값을 바꾸지 않는 관측이다.
void RawBackgroundGifControls::Draw(std::uint32_t token,const GumpPixelTarget& pixels) const {
    auto& s=*impl_;if (s.activePixels) throw std::logic_error("BackgroundGif Draw 재진입 불가");s.activePixels=&pixels;
    struct Reset { Impl& state;~Reset() { state.activePixels=nullptr; } }; // 정상/예외 반환에서도 일시 표면을 반납한다.
    Reset reset{s};s.backend->Draw(token);
}
std::uint32_t RawBackgroundGifControls::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { return impl_->backend->Handle(token,event,first,second); } // 원문 입력의 전체DWORD 반환이다.
void RawBackgroundGifControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) { impl_->backend->Destruct(token);if (flags&1) Release(token); } // 이미지/기본/객체 반납 순서다.
void RawBackgroundGifControls::Release(std::uint32_t token) { impl_->nodes.erase(token); } // 공통 scalar가 반납한다.
void RawBackgroundGifControls::Close() {
    auto& s=*impl_;
    for (std::size_t id=1;id<s.common.create.entries.size();++id) { const auto token=s.common.create.entries[id];const auto* node=Node(token);if (node && !(node->flags&0x10000U)) s.common.lifetime.Kill(token); } // 미부착 창도 실제 등록표에서 반납한다.
    s.common.lifetime.Cleanup();
    while (!s.nodes.empty()) { const auto token=s.nodes.begin()->first;if (std::find(s.common.create.entries.begin(),s.common.create.entries.end(),token)!=s.common.create.entries.end()) throw std::logic_error("BackgroundGif 정리 뒤 등록 노드 잔류");Release(token); } // 기본 생성 전 실패만 직접 반납한다.
    s.common.changed();
}
}
