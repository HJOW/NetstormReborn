// 원본433660→498220→Factory +78/+a4를 실제8비트 장면의 동일 정렬 위치에서 실행한다.
#include "client/RawCampaignFactoryDisplay.h"
#include "client/ObjectCollection.h"
#include "client/SpriteDraw.h"
#include "client/SpriteColorMaps.h"
#include "client/PaletteColors.h"
#include "client/OwnerColorMaps.h"
#include "o/RawFactoryDraw.h"
#include "o/RawTempleDraw.h"
#include "o/RawBridgeConnect.h"
#include <bit>
#include <stdexcept>
namespace netstorm::client {
// 같은 월드만 참조하고 게임 진행 상태는 복제하지 않는 표시 전용 저장소다.
struct RawCampaignFactoryDisplay::Impl {
    RawWorldCore core; // 실제 현재 풀/해시/TYPE/시계다.
    const GameAssets& assets; // 장면보다 오래 사는 실제 SHP다.
    o::RawFactoryDraw factory; // 두 실제 가상 표의 기본·추가 프레임 몸체다.
    ObjectCollection collection; // 같은 공간 해시의 원본 단계/깊이/그림자 수집기다.
    std::vector<SpriteTypeView> views; // 현재 SHP 선택 헤더와 기본 Gump 프레임이다.
    SpriteDrawState state; // 원본 표시 전역과 현재 카메라/로컬 소유자다.
    SpriteColorMaps colors; // 실제 파일 팔레트의 세 소스 치환 표다.
    OwnerColorMaps outpost; // 원본43be30의 아웃포스트 일반9색 표다.
    TempleDraw templeDraw; // 같은 Session의 현재 전용Draw/상승음이며 그리기 시점에만 호출한다.
    std::vector<std::int32_t> templeBaseFrames; // 같은TYPE+128의 고정 본체 번호이며 썬더의-1을 보존한다.
    bool ready{}; // 파일 팔레트가 설치되기 전에 출력하지 않는다.

    // 플레어의+78은 TYPE+128 baseFrame이며+a4는 빈41c100이다. -1의gumpFrame 대체는 일반 Draw가 처리한다.
    bool Flare(o::Sid sid) const {
        const auto raw=core.pool.Slot(sid);return raw[10]==106 && raw[0]==0xa0 && raw[1]==0x96 && raw[2]==0x50 && raw[3]==0;
    }
    // 세 제네레이터의41c300 생성자가 설치하는 실제 VT다. +78은 TYPE baseFrame, +a4는 빈41c100이다.
    bool Generator(o::Sid sid) const {
        const auto raw=core.pool.Slot(sid);return raw[0]==0xa0 && raw[1]==0x1e && raw[2]==0x50 && raw[3]==0;
    }
    bool Temple(o::Sid sid) const { return o::IsPatchTemple(core.pool.Slot(sid)); } // 같은10.78 세 전용VT의 실제+a4 대상이다.
    bool Handles(o::Sid sid) const { return factory.Handles(sid) || Flare(sid) || Generator(sid) || Temple(sid); } // 확인된 실제 일곱VT를 같은 수집기에 포함한다.
    // 원본 PE 초기값만 설치하고 각 수집에서 현재 게임 값으로 갱신한다.
    Impl(RawWorldCore world,const GameAssets& data,TempleDraw supplied):core(world),assets(data),factory(core.pool,core.frames),
        collection(core.hash,core.pool,core.types,core.shapes),views(MakeSpriteAssetViews(data)),outpost(BuildStandardOwnerColorMaps(126,82)),templeDraw(std::move(supplied)),templeBaseFrames(core.types.size(),-1) {
        // 실제 파싱한TYPE의 기본 프레임만 번호를 맞춰 공급한다. 템플은 원본 소유자 색표 대상이 아니다.
        for (const auto& type:assets.Types()) templeBaseFrames.at(type.block+o::kFirstAssetTypeNumber)=type.definition.specialFrames.baseFrame;
        state.filteredFlags=2421574144U;state.wideOwnerType=164;state.unfilteredType=84;state.shadowAliasType=158;
        state.shadowAliasPinned=true; // 실제5318ec의 초기DWORD6이다.0일 때만 alias를 바꾼다.
        state.markerEnabled=true;state.markerExcludeType=157;state.markerType=96;state.statusType=118;state.abstractStatusType=117;
        state.timerProcessType=31;state.statusProcessType=57;state.costType=122;state.timerTypes={138,144,73};
        state.selectionColors={84,84,249,184,140,150,112,156,222}; // 원본5319c4의 고정9DWORD다.
        state.ownerMaps.resize(core.types.size());state.ownerMaps.at(126)=1;
    }
    // 건설/추상/그림자는 명시 표, 아웃포스트는 같은 현재 소유자 색 번호로 변환한다.
    ColorMap Color(SpriteShapeMode mode,std::uint32_t type,std::uint32_t owner) const {
        if (mode==SpriteShapeMode::Abstract) return colors.bright;if (mode==SpriteShapeMode::Dead) return colors.dead;if (mode==SpriteShapeMode::Shadow) return colors.shadow;
        if (mode!=SpriteShapeMode::Owner || type!=126 || owner>=core.bridgeState.ownerColors.size()) throw std::logic_error("Factory 원본 소유자 색표 미연결");
        const auto index=core.bridgeState.ownerColors[owner];if (index<0 || index>8) throw std::out_of_range("Factory 현재 소유자 색 번호");return outpost[static_cast<std::size_t>(index)];
    }
    // 원본 현재 슬롯과 실제 dirty clip을 읽고 일반 Draw의 모든 픽셀을 같은 표면에 쓴다.
    void Draw(o::RenderCollectionEntry entry,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
        const auto sid=collection.Object(entry.token);if (!Handles(sid)) return;if (!ready) throw std::logic_error("Factory 파일 팔레트 미설치");
        const auto raw=core.pool.Slot(sid);
        if ((raw[40]&4U) && (raw[40]&0x30U)) throw std::logic_error("Factory 선택 상태 프로세스 목록 표시 미연결");
        state.now=core.processState.now;state.ownerPalettes.assign(core.bridgeState.ownerColors.begin(),core.bridgeState.ownerColors.end());
        SpritePixelTarget target(pixels,pitch,height,clip,MakeSpriteAssetShapes(assets,state),[this](auto mode,auto type,auto owner) { return Color(mode,type,owner); });
        SpriteDrawHooks rules;rules.virtualFrame=[this](o::Sid object) { if (Temple(object)) return o::PatchTempleMainFrame(core.pool.Slot(object),templeBaseFrames);return Flare(object) || Generator(object) ? assets.Types()[core.pool.Slot(object)[10]-o::kFirstAssetTypeNumber].definition.specialFrames.baseFrame : factory.MainFrame(object); };rules.surface=MakeSpriteSurfaceLookup(core.hash);
        rules.resource=[this](o::Sid object)->std::optional<SpriteResourceStatus> {
            if (core.processes.FindEvent(object,0x493)) throw std::logic_error("Factory 자원 내용물 막대 표시 미연결");return std::nullopt;
        };
        auto hooks=target.Hooks(std::move(rules));
        // +a4의0044c180은 같은 기준점/단위 배율의 plain SHP다. 일반 owner 문맥으로 바꾸지 않는다.
        hooks.virtualDraw=[this,shape=hooks.shape](o::Sid object,ScreenPoint point) {
            if (Flare(object) || Generator(object)) return; // 실제+a4의 빈 몸체41c100이다.
            if (Temple(object)) {
                if (!templeDraw) throw std::logic_error("실제 템플 전용Draw 미연결");const auto type=core.pool.Slot(object)[10]; // 원본Draw의 첫 TYPE/SHP snapshot이다.
                templeDraw(object,[this,object,point,type,&shape](std::int32_t frame) {
                    if (frame<0) throw std::out_of_range("템플 추가 그림 음수 프레임");const auto current=core.pool.Slot(object);
                    if (!(current[40]&0x10U)) shape({type,static_cast<std::uint32_t>(frame),point,SpriteShapeMode::Plain});
                    else if (current[34]==state.localPlayer) shape({type,static_cast<std::uint32_t>(frame),point,SpriteShapeMode::Abstract}); // 실제497280의 현재 추상/로컬 소유자 관문이다.
                });return;
            }
            factory.Additional(object,point.x,point.y,[&shape](auto x,auto y,auto type,auto frame) { if (frame<0) throw std::out_of_range("Factory 추가 그림 음수 프레임");shape({type,static_cast<std::uint32_t>(frame),{x,y},SpriteShapeMode::Plain}); });
        };
        SpriteDraw draw(core.pool,core.types,views,state,std::move(hooks),0);draw.Draw(entry);
    }
};
// 같은 Session의 수명이 끝나기 전에 표시기를 해제한다.
RawCampaignFactoryDisplay::RawCampaignFactoryDisplay(RawWorldCore core,const GameAssets& assets,TempleDraw templeDraw):impl_(std::make_unique<Impl>(core,assets,std::move(templeDraw))) {}
RawCampaignFactoryDisplay::~RawCampaignFactoryDisplay()=default; // procedure가 더 이상 호출되지 않을 때 반납한다.
bool RawCampaignFactoryDisplay::Handles(o::Sid sid) const { return impl_->Handles(sid); } // 별도 TYPE 이름 휴리스틱을 쓰지 않는다.
// 장치 BGRX와 별개로 원본 R/G/B/예약 DWORD를 만들어 실제 파일 색 표를 계산한다.
void RawCampaignFactoryDisplay::SetPalette(std::span<const ScreenColor> palette) {
    if (palette.size()!=256) throw std::invalid_argument("Factory 파일 팔레트256색 계약");std::array<std::uint32_t,256> logical{};
    // 실제 현재 파일 팔레트의 모든 번호를 보존한다.
    for (std::size_t i=0;i<logical.size();++i) logical[i]=std::uint32_t(palette[i].red)|(std::uint32_t(palette[i].green)<<8)|(std::uint32_t(palette[i].blue)<<16)|(std::uint32_t(palette[i].reserved)<<24);
    const auto named=BuildPaletteColorTable(logical,impl_->assets.Edition());impl_->colors=BuildSpriteColorMaps(logical);impl_->state.barBackground=named.basic[0];
    impl_->state.hitPointColors={named.named.at(10).index,named.basic[5],named.basic[1]};impl_->ready=true;
}
// 원본 수집기의 전체 순서/한 번의 그림자 보정/해시2단계 반환을 유지한다.
std::vector<RenderSprite> RawCampaignFactoryDisplay::Sprites(ScreenRect clip,int cameraX,int cameraY,std::uint32_t localPlayer) {
    auto& self=*impl_;self.state.cameraX=cameraX;self.state.cameraY=cameraY;self.state.localPlayer=localPlayer;
    const o::RenderWorldRect view{static_cast<float>((static_cast<double>(clip.left)+cameraX)/16),static_cast<float>((static_cast<double>(clip.top)+cameraY)/11),
        static_cast<float>((static_cast<double>(clip.right)+cameraX)/16),static_cast<float>((static_cast<double>(clip.bottom)+cameraY)/11)};
    std::array<o::RenderCollectionEntry,kDrawSubmissionCapacity> entries{};const auto count=self.collection.Collect(view,entries,kDrawSubmissionCapacity);std::vector<RenderSprite> result;
    // 지원이 확인된 실제 가상 표만 제출하고 나머지는 기존 월드 출력에 남긴다.
    for (std::int32_t i=0;i<count;++i) {
        const auto entry=entries[static_cast<std::size_t>(i)];if (!self.Handles(self.collection.Object(entry.token))) continue;
        RenderSprite sprite;sprite.clip=clip;sprite.order={entry.x,entry.y,entry.depth};
        sprite.procedure=RenderProcedure{clip,[this,entry](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect cut) { impl_->Draw(entry,pixels,pitch,height,cut); }};result.push_back(std::move(sprite));
    }
    return result;
}
}
