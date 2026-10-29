// 원문 편집 본체를 공통 UI의 실제 장치와 조립하며 계산을 다시 구현하지 않는다.
#include "client/RawTextEntryControls.h"
#include "client/RawMenuControls.h"
#include "client/TextEntryStorage.h"
#include "client/ScrollGumpWidgets.h"
#include <algorithm>
#include <stdexcept>
#include <set>
namespace netstorm::client { // 편집 UI 조립의 구현이다.
struct RawTextEntryControls::Impl { // 모든 콜백은 이 고정 소유자를 참조한다.
    RawMenuTree common;FontStore& fonts;const GumpBackgroundCreate& background;const GumpBackgroundDraw& backgroundDraw; // 같은 실제 장치 수명이다.
    const ScrollGump& slider;const ScrollGumpWidgets& widgets; // 같은 등록 Slider 처리기다.
    std::function<ScrollGumpNode*(std::uint32_t)> sliderNode;std::function<std::uint32_t(std::size_t)> allocateSlider; // 실제 Slider 저장소다.
    RawTextEntryContext context;TextEntryStorage storage;GumpText metrics;TextEntryGumpState state; // 실제 문자 물리와 현재 사건/팔레트다.
    std::map<std::uint32_t,std::unique_ptr<TextEntryGumpNode>> entries;std::map<std::uint32_t,std::unique_ptr<ConsoleGumpNode>> consoles; // 복사하지 않는 파생 노드다.
    std::set<std::uint32_t> owned,buffers;std::unique_ptr<TextEntryGump> backend; // 직접 할당 후 미부착 상태도 소유한다.
    mutable const GumpTextDraw* activeText{};mutable const GumpPixelTarget* activePixels{}; // 현재 Draw 안에서만 유효하다.
    Impl(RawMenuTree tree,FontStore& suppliedFonts,const GumpBackgroundCreate& bg,const GumpBackgroundDraw& draw,
        const ScrollGump& suppliedSlider,const ScrollGumpWidgets& suppliedWidgets,std::function<ScrollGumpNode*(std::uint32_t)> nodes,
        std::function<std::uint32_t(std::size_t)> allocator,RawTextEntryContext supplied)
        :common(std::move(tree)),fonts(suppliedFonts),background(bg),backgroundDraw(draw),slider(suppliedSlider),widgets(suppliedWidgets),sliderNode(std::move(nodes)),allocateSlider(std::move(allocator)),
        context(std::move(supplied)),storage(context.clipboardRead,context.clipboardWrite),metrics(fonts,common.create.collection.edition) { // 실제 장치 경계만 설치한다.
        if (common.create.collection.edition!=o::OriginalEdition::Patch1078 || !context.now || !sliderNode || !allocateSlider) throw std::invalid_argument("편집 UI 실제 장치/10.78 연결 필요");
        TextEntryGumpHooks h;h.node=[this](auto token) { return Node(token); };h.console=[this](auto token) { const auto found=consoles.find(token);return found==consoles.end() ? nullptr : found->second.get(); };h.base=common.node;h.buffer=[this](auto token) { return storage.Find(token); };h.slider=sliderNode;
        h.allocate=[this](auto bytes) { if (bytes==0x28) { const auto token=common.token();storage.Create(token);buffers.insert(token);return token; }const auto token=allocateSlider(bytes);owned.insert(token);return token; };
        h.releaseBuffer=[this](auto token) { storage.ReleaseBuffer(token);buffers.erase(token); };
        h.constructBackground=[this](auto token,auto flags) { background.Construct(token,0,0,flags,1,0,0); };
        h.constructSlider=[this](auto token,auto width,auto height,auto color,auto edge) { widgets.Construct(token,width,height,color,edge); };
        h.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };h.attach=common.attach;h.push=[this](auto token) { common.tree.Push(token); };
        h.idle=[this](auto token,bool enabled) { common.lifetime.SetIdle(token,enabled); };h.redraw=[this](auto,bool) { common.changed(); };h.bubble=[this](auto token,auto event,auto first,auto second) { common.events.Bubble(token,event,first,second); };
        h.sliderRange=[this](auto token,auto first,auto last) { slider.SetRange(token,first,last); };h.sliderPage=[this](auto token,auto value) { slider.SetPage(token,value); };h.sliderPosition=[this](auto token,auto value) { slider.SetPosition(token,value); };h.now=context.now;
        h.measure=[this](auto token,auto text,auto length) { std::int32_t width{},height{};metrics.Measure(text,DecodeGumpTextContext(Node(token)->textContext),width,height,length);return std::array{width,height}; };h.position=[this](auto token) { return common.collection.Position(token); };
        h.drawBackground=[this](auto token) { backgroundDraw.Draw(token); };h.drawText=[this](auto token,auto text,auto offset,const auto* explicitContext) { if (!activeText) throw std::logic_error("편집 글자 실제 Draw 문맥 없음");if (explicitContext) { const auto decoded=DecodeGumpTextContext(*explicitContext);activeText->DrawGump(common.collection,token,text,offset,Node(token)->textContext,&decoded); }else activeText->DrawGump(common.collection,token,text,offset,Node(token)->textContext); };
        h.fill=[this](auto rect,auto color) { if (!activePixels) throw std::logic_error("편집 선택 실제 Draw 문맥 없음");activePixels->Fill(rect,color); };
        h.recipient=[this](auto text) { if (!context.recipient) throw std::logic_error("Console 실제 수신 대상 검사 미연결");return context.recipient(text); };
        h.allocateBytes=[this](auto bytes) { return storage.Allocate(static_cast<std::uint32_t>(bytes)); };h.releaseBytes=[this](auto token) { storage.Release(token); };h.bytes=[this](auto token) { return storage.Bytes(token); };
        h.complete=[this](auto callback,auto text,auto length,bool forward) { if (!context.complete) throw std::logic_error("Console 실제 자동완성 미연결");return context.complete(callback,text,length,forward); };h.submit=[this](auto callback,auto text) { if (!context.submit) throw std::logic_error("Console 실제 제출 미연결");context.submit(callback,text); };
        h.assertion=[](auto line) { throw std::logic_error("TextEntry 원본 진단: "+std::to_string(line)); };h.destructBase=[this](auto token) { common.lifetime.DestructBase(token); };backend=std::make_unique<TextEntryGump>(state,std::move(h));
    }
    TextEntryGumpNode* Node(std::uint32_t token) const { // 같은 실제 파생 기본 주소만 반환한다.
        if (const auto found=consoles.find(token);found!=consoles.end()) return found->second.get();const auto found=entries.find(token);return found==entries.end() ? nullptr : found->second.get();
    }
};
RawTextEntryControls::RawTextEntryControls(RawMenuTree tree,FontStore& fonts,const GumpBackgroundCreate& bg,const GumpBackgroundDraw& draw,
    const ScrollGump& slider,const ScrollGumpWidgets& widgets,std::function<ScrollGumpNode*(std::uint32_t)> nodes,std::function<std::uint32_t(std::size_t)> allocator,RawTextEntryContext context)
    :impl_(std::make_unique<Impl>(std::move(tree),fonts,bg,draw,slider,widgets,std::move(nodes),std::move(allocator),std::move(context))) {} // 고정 소유자를 만든다.
RawTextEntryControls::~RawTextEntryControls()=default; // 노드 반납은 공통 수명에서 먼저 수행한다.
//기존편집문맥의이선택장치만바꾸고실제노드/문자물리와다른콜백은유지한다.
void RawTextEntryControls::SetRecipientValidator(std::function<bool(std::string_view)> validator) { if (!validator) throw std::invalid_argument("Console 실제 대상 검사 없음");impl_->context.recipient=std::move(validator); }
void RawTextEntryControls::SetPalette(const PaletteColorTable& palette) { if (palette.named.size()<=9) throw std::invalid_argument("편집 UI 실제 갈색 팔레트 없음");impl_->state.brown=palette.named[9].index;impl_->state.black=palette.basic[0];impl_->state.yellow=palette.basic[5]; } // 파일 팔레트의 실제 색 전역이다.
std::uint32_t RawTextEntryControls::Allocate(bool console) { auto& s=*impl_;const auto token=s.common.token();if (console) s.consoles.emplace(token,std::make_unique<ConsoleGumpNode>());else s.entries.emplace(token,std::make_unique<TextEntryGumpNode>());s.owned.insert(token);return token; } // 기본 생성 전 소유권을 확보한다.
std::uint32_t RawTextEntryControls::ConstructConsole(std::int32_t width,std::int32_t height,std::optional<std::string_view> text,std::uint32_t font,std::uint32_t background,std::uint32_t color) { return impl_->backend->ConstructConsole(Allocate(true),width,height,text,font,background,color); } // 같은 실제 본체다.
TextEntryGumpNode* RawTextEntryControls::Node(std::uint32_t token) const { return impl_->Node(token); } // 실제 공통 주소다.
ConsoleGumpNode* RawTextEntryControls::Console(std::uint32_t token) const { const auto found=impl_->consoles.find(token);return found==impl_->consoles.end() ? nullptr : found->second.get(); } // Console만 조회한다.
const TextEntryGump& RawTextEntryControls::Backend() const { return *impl_->backend; } // 계산 복사본 없이 빌린다.
TextEntryStorage& RawTextEntryControls::Storage() const { return impl_->storage; } // 같은 물리를 빌린다.
void RawTextEntryControls::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const { impl_->backend->Attach(token,x,y,parent,mode); } // 실제 자식 재부착이다.
std::uint32_t RawTextEntryControls::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { return Console(token) ? impl_->backend->HandleConsole(token,event,first,second) : impl_->backend->Handle(token,event,first,second); } // 현재 파생만 선택한다.
void RawTextEntryControls::Idle(std::uint32_t token) const { impl_->backend->Idle(token); } // 실제 벽시계 점멸이다.
void RawTextEntryControls::Draw(std::uint32_t token,const GumpTextDraw& text,const GumpPixelTarget& pixels) const {
    auto& s=*impl_;if (s.activeText) throw std::logic_error("편집 Draw 재진입 불가");s.activeText=&text;s.activePixels=&pixels;
    struct Reset { Impl& state;~Reset() { state.activeText=nullptr;state.activePixels=nullptr; } }; // 정상/예외 반환에서 일시 장치를 반납한다.
    Reset reset{s};s.backend->Draw(token);
}
void RawTextEntryControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) { impl_->backend->Destruct(token,Console(token)!=nullptr);if (flags&1) Release(token); } // 기본 소멸은 본체에서 이미 실행한다.
void RawTextEntryControls::Release(std::uint32_t token) { impl_->owned.erase(token);impl_->entries.erase(token);impl_->consoles.erase(token); } // 슬라이더 반납도 공통 release가 통지한다.
void RawTextEntryControls::Close() { // 등록된 실제 노드는 공통 Kill로, 미등록 new만 저장소에서 반납한다.
    auto& s=*impl_;
    for (std::size_t id=1;id<s.common.create.entries.size();++id) { const auto token=s.common.create.entries[id];const auto* node=s.owned.contains(token) ? s.common.node(token) : nullptr;if (node && !(node->flags&0x10000U)) s.common.lifetime.Kill(token); } // 미부착 자식도 현재 등록표에서 찾는다.
    s.common.lifetime.Cleanup();
    while (!s.owned.empty()) { const auto token=*s.owned.begin();if (std::find(s.common.create.entries.begin(),s.common.create.entries.end(),token)!=s.common.create.entries.end()) throw std::logic_error("편집 정리 뒤 등록 노드 잔류");Release(token); } // 기본 생성 전 실패만 직접 반납한다.
    while (!s.buffers.empty()) { const auto token=*s.buffers.begin();s.storage.Find(token)->Destroy();s.storage.ReleaseBuffer(token);s.buffers.erase(token); } // node.buffer 대입 전 실패의 실제 할당도 반납한다.
    s.common.changed();
}
}
