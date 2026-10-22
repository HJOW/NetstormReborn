// 00498220/00455f60의 호출 순서와 선택 상태를 장치/게임 규칙 조회 경계에서 분리한다.
#include "client/SpriteDraw.h"
#include "client/SquidRenderer.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 실행한 분기의 필수 조회/출력이 빠지면 조용히 그림을 생략하지 않는다.
template<class F> const F& RequiredSprite(const F& hook) {
    if (!hook) throw std::invalid_argument("Sprite Draw hook required");return hook;
}
// signed DWORD의 넘침은 원본 하위 비트로 보존한다.
int AddSprite(int a,int b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b)); }
// 뺄셈도 signed C++ 넘침 없이 처리한다.
int SubSprite(int a,int b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b)); }
// 막대 길이 곱셈의 DWORD 하위 비트를 보존한다.
int MulSprite(int a,int b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b)); }
// raw 필드의 little endian 폭만 읽는다.
std::uint32_t ReadSprite(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 주변 packed 필드를 읽지 않고 지정한 바이트만 합친다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8);
    return value;
}
// native CRT의 signed 64비트 절삭 결과 중 하위 DWORD를 유지한다.
int TruncateSprite(double value) {
    if (!std::isfinite(value) || value< -0x1p63 || value>=0x1p63) throw std::out_of_range("Sprite Draw finite coordinate/time");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 표시/자산/풀 번호를 같은 전체 타입 표에 맞춘다.
SpriteDraw::SpriteDraw(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const SpriteTypeView> views,
    SpriteDrawState& state,SpriteDrawHooks hooks,std::uint32_t tokenBase)
    :pool_(pool),types_(types),views_(views),state_(state),hooks_(std::move(hooks)),tokenBase_(tokenBase) {
    if (types.empty() || types.size()!=views.size() || static_cast<std::uint64_t>(tokenBase)+static_cast<std::uint64_t>(pool.Capacity())*pool.Layout().stride>0x100000000ULL)
        throw std::invalid_argument("Sprite Draw tables/token range");
}
// 타입 +0xdc만 바꾼 원본 부작용을 실제 선택 헤더와 loaded 검사에도 적용한다.
const o::SquidDisplayShape& SpriteDraw::Shape(std::uint32_t type) const {
    if (type==state_.shadowAliasType && state_.shadowShapeType) type=*state_.shadowShapeType;
    if (type>=views_.size()) throw std::out_of_range("Sprite Draw shape alias");
    return views_[type].shape;
}
// shadow 조기 반환·본체·가상 출력·표식·선택 순으로 처리한다.
void SpriteDraw::Draw(const o::RenderCollectionEntry& entry) const {
    if (entry.token<tokenBase_) throw std::out_of_range("Sprite Draw token");
    // token은 수집기가 저장한 원본 raw 포인터에 대응하며 SID 0은 그림 대상이 아니다.
    const auto offset=entry.token-tokenBase_,stride=pool_.Layout().stride;
    if (!offset || offset%stride || offset/stride>=pool_.Capacity()) throw std::out_of_range("Sprite Draw SID");
    const o::Sid sid{static_cast<std::uint16_t>(offset/stride)};auto& s=state_;
    const auto raw=pool_.Slot(sid);const auto type=static_cast<std::uint32_t>(raw[10]);
    if (type>=types_.size()) throw std::out_of_range("Sprite Draw type");
    const auto& record=types_[type];const auto& view=views_[type];if (!Shape(type).loaded) return;
    const bool patch=pool_.Edition()==o::OriginalEdition::Patch1078;
    const auto frame=ReadSprite(raw,patch ? 36 : 34,patch ? 4 : 1);const auto extra=raw[patch ? 40 : 35];
    const int x=SubSprite(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadSprite(raw,22,2))),s.cameraX);
    const int y=SubSprite(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadSprite(raw,24,2))),s.cameraY);
    const float wx=std::bit_cast<float>(ReadSprite(raw,14)),wy=std::bit_cast<float>(ReadSprite(raw,18));
    if (SubSprite(TruncateSprite(static_cast<double>(wx)*16+0.5),s.cameraX)!=x || SubSprite(TruncateSprite(static_cast<double>(wy)*11+0.5),s.cameraY)!=y)
        throw std::invalid_argument("Sprite Draw world/screen mismatch");
    const ScreenPoint point{SubSprite(x,record.hotspotX),SubSprite(y,record.hotspotY)};
    // 모든 출력은 원본 프레임과 화면 기준점 그대로 전달한다.
    const auto shape=[&](std::uint32_t key,std::uint32_t index,ScreenPoint p,SpriteShapeMode mode) { RequiredSprite(hooks_.shape)({key,index,p,mode}); };
    if (entry.tail&4U) {
        if (extra&0x10U) return;
        if (patch && !s.shadowAliasPinned) s.shadowShapeType=type;
        // 그림자 레이어로의 보정은 반복 modulo가 아니라 한 번의 signed 비교/뺄셈이다.
        auto index=frame;if (view.shape.frameCount<=std::bit_cast<std::int32_t>(index)) index-=static_cast<std::uint32_t>(view.shape.frameCount);
        if (s.specialMode || (extra&1U)) return;
        shape(type,index+static_cast<std::uint32_t>(view.shape.frameCount),point,SpriteShapeMode::Shadow);return;
    }
    const auto owner=type==s.wideOwnerType ? ReadSprite(raw,12,2) : ReadSprite(raw,patch ? 34 : 32,1);
    const bool abstract=(extra&0x10U)!=0,dead=(extra&1U)!=0;
    const auto flags2=record.flags2;
    // 가상 프레임 조회 뒤에 바뀐 필터 전역도 원본처럼 현재 값으로 읽는다.
    const auto filtered=[&] { return (s.filteredFlags&flags2) || type==s.wideOwnerType || type==s.unfilteredType; };
    // 소유자 표가 있는 양수 플레이어만 current map을 바꾸고 이어서 mapped 그림을 호출한다.
    const auto normal=[&](std::uint32_t index) {
        if (type>=s.ownerMaps.size()) throw std::out_of_range("Sprite Draw owner maps");
        if (s.ownerMaps[type] && owner) { RequiredSprite(hooks_.setOwnerColors)(type,owner);shape(type,index,point,SpriteShapeMode::Owner); }
        else shape(type,index,point,SpriteShapeMode::Plain);
    };
    if (!(extra&2U)) {
        if (dead) shape(type,frame,point,SpriteShapeMode::Dead);
        else if (abstract) { if (owner==s.localPlayer) shape(type,frame,point,SpriteShapeMode::Abstract); }
        else if (s.specialMode && filtered()) shape(type,frame,point,SpriteShapeMode::Abstract);
        else normal(frame);
    } else {
        // 가상 프레임 -1은 기본 gumpFrame을 유지하며 alive 일반 경로는 별도 가상 Draw만 호출한다.
        const int custom=RequiredSprite(hooks_.virtualFrame)(sid);
        const auto fallback=custom==-1 ? view.gumpFrame : static_cast<std::uint32_t>(custom);
        if (dead) shape(type,fallback,point,SpriteShapeMode::Dead);
        else if (!s.specialMode) {
            if (custom!=-1) {
                if (!abstract) normal(static_cast<std::uint32_t>(custom));
                else if (owner==s.localPlayer) shape(type,static_cast<std::uint32_t>(custom),point,SpriteShapeMode::Abstract);
            }
            RequiredSprite(hooks_.virtualDraw)(sid,point);
        } else if (abstract) { if (owner==s.localPlayer) shape(type,static_cast<std::uint32_t>(custom),point,SpriteShapeMode::Abstract); }
        else if (filtered()) shape(type,fallback,point,SpriteShapeMode::Abstract);
        else { if (custom!=-1) normal(static_cast<std::uint32_t>(custom));RequiredSprite(hooks_.virtualDraw)(sid,point); }
    }
    if (!abstract) {
        if (s.markerEnabled && (record.flags1&0x400U) && s.markerVisible && owner) {
            const auto surface=RequiredSprite(hooks_.surface)(entry.x,entry.y);
            if (surface.value && pool_.Slot(surface)[10]!=s.markerExcludeType) {
                if (owner>=s.ownerPalettes.size()) throw std::out_of_range("Sprite Draw owner palette");
                shape(s.markerType,static_cast<std::uint32_t>(SubSprite(s.ownerPalettes[owner],1)),point,SpriteShapeMode::Plain);
            }
        }
        if (raw[patch ? 40 : 35]&0x20U) {
            if (s.statusType>=views_.size()) throw std::out_of_range("Sprite Draw status type");
            shape(s.statusType,views_[s.statusType].gumpFrame,{SubSprite(point.x,record.footX/2),SubSprite(point.y,record.footY/2)},SpriteShapeMode::Plain);
        }
    }
    // extra는 가상 Draw에서 바뀔 수 있으므로 선택/후속 상태 판단은 현재 슬롯을 다시 읽는다.
    if ((raw[patch ? 40 : 35]&4U) && !s.selectionDisabled) Selection(sid,type,owner,frame,{x,y});
}
// 원본 선택 헤더의 아래 두 모서리를 겹쳐 그리고 판본별 상태 막대를 추가한다.
void SpriteDraw::Selection(o::Sid sid,std::uint32_t type,std::uint32_t owner,std::uint32_t frame,ScreenPoint screen) const {
    auto& s=state_;const auto& view=views_[type];const auto& record=types_[type];const auto raw=pool_.Slot(sid);
    const bool patch=pool_.Edition()==o::OriginalEdition::Patch1078;
    const auto& physical=Shape(type);
    if (frame>=physical.frames.size() || (patch && (std::bit_cast<std::int32_t>(frame)<0 ||
        static_cast<std::int64_t>(frame)>=static_cast<std::int64_t>(view.shape.frameCount)*((record.flags1&0x440000U) ? 2 : 1) ||
        view.shape.frameCount<=0 || record.maxHitPoints==std::bit_cast<std::int32_t>(0xddddddddU) || record.maxHitPoints==std::bit_cast<std::int32_t>(0xcdcdcdcdU))))
        throw std::out_of_range("Sprite Draw selection frame");
    const auto& header=physical.frames[frame];s.selectedSid=sid.value;
    if (owner>=s.ownerPalettes.size() || s.ownerPalettes[owner]<0 || static_cast<std::size_t>(s.ownerPalettes[owner])>=s.selectionColors.size())
        throw std::out_of_range("Sprite Draw selection colors");
    const auto color=s.selectionColors[s.ownerPalettes[owner]];
    int left=SubSprite(screen.x,header.hotspotX),right=AddSprite(left,header.width),bottom=AddSprite(SubSprite(screen.y,header.hotspotY),header.height);
    const int horizontal=header.width/4,vertical=SubSprite(header.height,1)/4;
    // 바깥 네 선 다음 안쪽 네 선의 호출 순서를 유지한다.
    for (int layer=0;layer<2;++layer) {
        RequiredSprite(hooks_.line)({left,SubSprite(bottom,vertical)},{left,bottom},color);
        RequiredSprite(hooks_.line)({left,bottom},{AddSprite(left,horizontal),bottom},color);
        RequiredSprite(hooks_.line)({right,SubSprite(bottom,vertical)},{right,bottom},color);
        RequiredSprite(hooks_.line)({SubSprite(right,horizontal),bottom},{right,bottom},color);
        left=AddSprite(left,1);right=SubSprite(right,1);bottom=SubSprite(bottom,1);
    }
    int row=0;
    // 패치 막대는 폭 전체와 3픽셀 행 간격을 사용한다. 배경/본체 fill은 half-open이다.
    const auto bar=[&](std::uint32_t barColor,int current,int maximum) {
        const int divisor=std::max(maximum,1),amount=std::max(0,std::min(current,maximum));
        const int bx=SubSprite(screen.x,header.hotspotX),by=SubSprite(SubSprite(screen.y,MulSprite(row,3)),header.hotspotY);
        RequiredSprite(hooks_.fill)({SubSprite(bx,1),SubSprite(by,5),AddSprite(AddSprite(bx,1),header.width),SubSprite(by,1)},s.barBackground);
        RequiredSprite(hooks_.fill)({bx,SubSprite(by,4),AddSprite(bx,MulSprite(amount,header.width)/divisor),SubSprite(by,2)},barColor);++row;
    };
    const bool hp=(record.flags2&0x400000U) ? (raw[patch ? 40 : 35]&1U)!=0 : (record.flags1&0x10U)!=0;
    int maximum=s.quarterHitPoints && type==s.shadowAliasType ? record.maxHitPoints/4 : record.maxHitPoints;
    int current=patch ? std::bit_cast<std::int32_t>(ReadSprite(raw,26)) : std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadSprite(raw,26,2)));
    if (hp) {
        if (!patch) { maximum=std::max(maximum,1);current=std::clamp(current,0,maximum); }
        const auto hpColor=current<=maximum/4 ? s.hitPointColors[2] : current<=maximum/2 ? s.hitPointColors[1] : s.hitPointColors[0];
        if (patch) bar(hpColor,current,maximum);
        else {
            // CD의 체력 막대는 19픽셀 길이·5픽셀 높이이며 현재 프레임 가운데 위쪽에 놓인다.
            const int bx=AddSprite(SubSprite(header.width/2,header.hotspotX),AddSprite(screen.x,-10));
            const int by=SubSprite(screen.y,header.hotspotY);
            RequiredSprite(hooks_.fill)({SubSprite(bx,1),SubSprite(by,9),AddSprite(bx,20),SubSprite(by,2)},s.barBackground);
            RequiredSprite(hooks_.fill)({bx,SubSprite(by,8),AddSprite(bx,MulSprite(current,19)/maximum),SubSprite(by,3)},hpColor);
        }
    }
    if (!patch) return;
    if (raw[10]==s.shadowAliasType) {
        const auto target=ReadSprite(raw,42);if (target>=views_.size()) throw std::out_of_range("Sprite Draw construction type");
        if (RequiredSprite(hooks_.constructionActive)(sid)) bar(0x7b,SubSprite(views_[target].constructionTime,std::bit_cast<std::int32_t>(ReadSprite(raw,46))),views_[target].constructionTime);
    }
    if (const auto resource=RequiredSprite(hooks_.resource)(sid)) {
        if (resource->capacityType>=views_.size()) throw std::out_of_range("Sprite Draw resource type");
        const int capacity=MulSprite(views_[resource->capacityType].capacity,10);bar(0xbe,SubSprite(capacity,resource->remaining),capacity);
    }
    // 같은 타입 번호가 여러 타이머 전역에 쓰인 경우에도 각각 독립 조회/막대를 유지한다.
    for (std::size_t i=0;i<s.timerTypes.size();++i) if (raw[10]==s.timerTypes[i]) {
        if (const auto times=RequiredSprite(hooks_.timers)(sid,s.timerProcessType)) bar(i==0 ? 0xd9U : i==1 ? 9U : 0x6bU,TruncateSprite((*times)[i]-s.now),i==0 ? 20 : 60);
    }
    if (raw[40]&0x30U) {
        int abstractTime=0,statusTime=0;
        const auto processes=hooks_.statusProcesses ? hooks_.statusProcesses() : s.processes; // 본체 가상 Draw 뒤의 현재 실제 목록을 읽는다.
        // 실제 Kernel의 예약 슬롯 0은 검사하지 않는다. 현재 입력 표의 끝까지 원래 순서로 읽는다.
        for (std::size_t index=1;index<processes.size();++index) {
            const auto& process=processes[index];if (process.type!=s.statusProcessType) continue;
            // 목록 내 첫 SID 일치에서 멈추며 flag 1이 flag 2보다 우선한다.
            for (const auto member:process.members) if (member==sid.value) {
                if (process.flags&1U) abstractTime=std::max(abstractTime,TruncateSprite(process.endTime-s.now));
                else if (process.flags&2U) statusTime=std::max(statusTime,TruncateSprite(process.endTime-s.now));
                break;
            }
        }
        if (raw[40]&0x10U) { if (s.abstractStatusType>=views_.size()) throw std::out_of_range("Sprite Draw abstract status");bar(0xc3,abstractTime,views_[s.abstractStatusType].duration); }
        if (raw[40]&0x20U) { if (s.statusType>=views_.size()) throw std::out_of_range("Sprite Draw status duration");bar(0xfe,statusTime,views_[s.statusType].duration); }
    }
    if (raw[10]==s.costType) bar(0xf0,static_cast<int>(ReadSprite(raw,12,2)),TruncateSprite(record.cost));
}
// kind 0과 묶음 내부 kind 0을 같은 실제 Draw로 연결한다.
void BindSpriteDraw(DrawDispatchHooks& hooks,const SpriteDraw& draw) { hooks.sprite=[&draw](const auto& entry) { draw.Draw(entry); }; }
// 로딩 순서 그대로 SHP 선택 헤더와 gumpFrame을 연결한다.
std::vector<SpriteTypeView> MakeSpriteAssetViews(const GameAssets& assets) {
    auto shapes=SquidRenderer::Shapes(assets,assets.Edition());std::vector<SpriteTypeView> result(shapes.size());
    // 전체 타입 번호를 유지한다.
    for (std::size_t i=0;i<shapes.size();++i) result[i].shape=std::move(shapes[i]);
    // 자산 없는 내장 타입은 null shape로 남겨 둔다.
    for (std::size_t i=0;i<assets.Types().size();++i) {
        const auto& definition=assets.Types()[i].definition;auto& view=result[o::kFirstAssetTypeNumber+i]; // 같은 실제 TYPE 정의의 표시용 정수 속성이다.
        view.gumpFrame=static_cast<std::uint32_t>(definition.specialFrames.gumpFrame);
        view.capacity=TruncateSprite(definition.Number("casttime").value_or(0));
        view.constructionTime=TruncateSprite(definition.Number("praytime").value_or(0));
        view.duration=TruncateSprite(definition.Number("effecttime").value_or(0));
    }
    return result;
}
// 타입 +0xdc의 alias 쓰기를 실제 자산 출력에서도 현재 값으로 읽는다.
std::function<PreviewShapeBinding(std::uint32_t)> MakeSpriteAssetShapes(const GameAssets& assets,const SpriteDrawState& state) {
    return [&assets,&state](std::uint32_t type) {
        if (type==state.shadowAliasType && state.shadowShapeType) type=*state.shadowShapeType;
        if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets.Types().size()) throw std::out_of_range("Sprite Draw asset type");
        return PreviewShapeBinding{&assets.Shapes(),assets.Types()[type-o::kFirstAssetTypeNumber].block};
    };
}
// 0040eaf0와 CD inline의 경계 좌표/0단계 표 조회를 실제 해시에 연결한다.
std::function<o::Sid(float,float)> MakeSpriteSurfaceLookup(const o::SquidHash& hash) {
    return [&hash](float x,float y) {
        const int column=TruncateSprite(static_cast<double>(x)+static_cast<double>(0.9999F));
        const int row=TruncateSprite(static_cast<double>(y)+static_cast<double>(0.9999F));
        if (column<0 || row<0 || column>=256 || row>=256) return o::Sid{};
        return o::Sid{hash.Cell(0,column,row)};
    };
}
// 표면 크기와 clip을 검증해 실제 픽셀 쓰기 범위를 제한한다.
SpritePixelTarget::SpritePixelTarget(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,
    std::function<PreviewShapeBinding(std::uint32_t)> shape,std::function<ColorMap(SpriteShapeMode,std::uint32_t,std::uint32_t)> colors)
    :pixels_(pixels),pitch_(pitch),height_(height),clip_(ClipScreenRect(clip,{0,0,pitch,height})),shape_(std::move(shape)),colors_(std::move(colors)) {
    if (pitch<=0 || height<=0 || static_cast<std::uint64_t>(pitch)*height>pixels.size() || clip.left>clip.right || clip.top>clip.bottom)
        throw std::invalid_argument("Sprite Draw surface/clip");
}
// 게임 규칙/가상 처리기는 보존하고 픽셀 관련 출력만 이 표면에 연결한다.
SpriteDrawHooks SpritePixelTarget::Hooks(SpriteDrawHooks rules) {
    rules.setOwnerColors=[this](auto type,auto owner) { current_=RequiredSprite(colors_)(SpriteShapeMode::Owner,type,owner); };
    rules.shape=[this](auto request) { Shape(request); };rules.line=[this](auto first,auto last,auto color) { Line(first,last,color); };
    rules.fill=[this](auto rect,auto color) { Fill(rect,color); };return rules;
}
// shadow 호출도 원본 004dab90의 소스 색 치환이다. 대상 화면 색 변환 기능과 구분한다.
void SpritePixelTarget::Shape(SpriteShapeRequest request) {
    const auto binding=RequiredSprite(shape_)(request.type);if (!binding.database) throw std::invalid_argument("Sprite Draw shape binding");
    const auto blocks=binding.database->Blocks();if (binding.block>=blocks.size() || request.frame>=blocks[binding.block].frames.size()) throw std::out_of_range("Sprite Draw frame");
    const auto& frame=blocks[binding.block].frames[request.frame];if (frame.IsSpecial()) return;
    const auto key=std::make_tuple(binding.database,binding.block,static_cast<std::size_t>(request.frame));auto image=images_.find(key);
    if (image==images_.end()) image=images_.emplace(key,binding.database->Decode(binding.block,request.frame)).first;
    std::optional<ColorMap> mapped;const ColorMap* colors=nullptr;
    if (request.mode==SpriteShapeMode::Owner) { if (!current_) throw std::logic_error("Sprite Draw owner colors missing");colors=&*current_; }
    else if (request.mode!=SpriteShapeMode::Plain) { mapped=RequiredSprite(colors_)(request.mode,request.type,0);colors=&*mapped; }
    DrawIndexedImage(pixels_,pitch_,height_,image->second,AddSprite(request.position.x,frame.rect.left),AddSprite(request.position.y,frame.rect.top),clip_,colors);
}
// 양 끝 포함 직교 선분을 정규화해 자른다. 대각선 입력은 이 Draw의 계약 밖이다.
void SpritePixelTarget::Line(ScreenPoint first,ScreenPoint last,std::uint32_t color) {
    if (first.x!=last.x && first.y!=last.y) throw std::invalid_argument("Sprite Draw orthogonal line");
    if (first.y==last.y) {
        if (first.y<clip_.top || first.y>=clip_.bottom) return;
        const int low=std::max(std::min(first.x,last.x),clip_.left),high=std::min(std::max(first.x,last.x),clip_.right-1);
        // 원래 선의 양 끝을 포함한다.
        for (int x=low;x<=high;++x) pixels_[static_cast<std::size_t>(first.y)*pitch_+x]=static_cast<std::uint8_t>(color);
    } else {
        if (first.x<clip_.left || first.x>=clip_.right) return;
        const int low=std::max(std::min(first.y,last.y),clip_.top),high=std::min(std::max(first.y,last.y),clip_.bottom-1);
        // 클립 안에 남은 세로 구간만 출력한다.
        for (int y=low;y<=high;++y) pixels_[static_cast<std::size_t>(y)*pitch_+first.x]=static_cast<std::uint8_t>(color);
    }
}
// 원본 fill의 끝 좌표를 제외해 배경/막대 픽셀을 쓴다.
void SpritePixelTarget::Fill(ScreenRect rect,std::uint32_t color) {
    const auto visible=ClipScreenRect(rect,clip_);
    // 빈/역전 폭·높이에는 쓰지 않는다.
    for (int y=visible.top;y<visible.bottom;++y) {
        // 각 행의 clip 안 픽셀만 갱신한다.
        for (int x=visible.left;x<visible.right;++x) pixels_[static_cast<std::size_t>(y)*pitch_+x]=static_cast<std::uint8_t>(color);
    }
}
}
