// 원본433660→498220→Factory +78/+a4를 실제8비트 장면의 동일 정렬 위치에서 실행한다.
#include "client/RawCampaignFactoryDisplay.h"
#include "client/ObjectCollection.h"
#include "client/SpriteDraw.h"
#include "client/SpriteColorMaps.h"
#include "client/PaletteColors.h"
#include "client/OwnerColorMaps.h"
#include "o/RawFactoryDraw.h"
#include "o/RawDaisDraw.h"
#include "o/RawTempleDraw.h"
#include "o/RawThunderCannonCapture.h"
#include "o/RawGeyserCapture.h"
#include "o/RawSunAviaryCapture.h"
#include "o/RawBurningLifetimeDraw.h"
#include "o/RawPlainAssetDraw.h"
#include "o/RawBombEffectTimer.h"
#include "o/RawContainedFinder.h"
#include "o/RawFlyerProcess.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawBridgeConnect.h"
#include <bit>
#include <stdexcept>
#include <map>
namespace netstorm::client {
namespace {
constexpr std::array<std::uint32_t,12> kStandardOwnerTypes{82,155,158,164,135,133,156,73,170,146,144,126}; // 원본43be30의 일반 색표 대상이다.
constexpr std::array<std::uint32_t,5> kSpecialOwnerTypes{111,141,137,94,97}; // 원본43bc40의 명도 정렬 전용 색표 대상이다.
}
// 같은 월드만 참조하고 게임 진행 상태는 복제하지 않는 표시 전용 저장소다.
struct RawCampaignFactoryDisplay::Impl {
    RawWorldCore core; // 실제 현재 풀/해시/TYPE/시계다.
    const GameAssets& assets; // 장면보다 오래 사는 실제 SHP다.
    o::RawFactoryDraw factory; // 두 실제 가상 표의 기본·추가 프레임 몸체다.
    ObjectCollection collection; // 같은 공간 해시의 원본 단계/깊이/그림자 수집기다.
    std::vector<SpriteTypeView> views; // 현재 SHP 선택 헤더와 기본 Gump 프레임이다.
    SpriteDrawState state; // 원본 표시 전역과 현재 카메라/로컬 소유자다.
    std::vector<SpriteStatusProcess> statusSlots; // 첫0번과 현재Kernel 순서를 유지하는 선택용 읽기 전용 뷰다.
    SpriteColorMaps colors; // 실제 파일 팔레트의 세 소스 치환 표다.
    std::map<std::uint32_t,OwnerColorMaps> ownerColors; // 원본 일반12종/전용5종의 실제9색표이며 현재 팔레트로 갱신한다.
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
    bool Bomb(o::Sid sid) const { const auto raw=core.pool.Slot(sid);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);return (raw[10]==117 && table==0x502510U) || (raw[10]==118 && table==0x502ff8U) || (raw[10]==168 && table==0x502370U); } // 공통+a0=41c0f0/+a4=41e3d0인 세 주문이다.
    bool ThunderCannon(o::Sid sid) const { return o::IsThunderCannonCapture(core.pool.Slot(sid)); } // 독립333캡처로 확인한 실제131/VT50bad0 쌍이다.
    // 별도 SID로 복제된 flyershadow의 두 번째 물리 프레임 묶음이다. 여섯 비행/벌룬 타입에 같은 원본 색 치환을 적용한다.
    bool FlyerShadow(o::Sid sid) const {
        const auto raw=core.pool.Slot(sid);const auto type=raw[10]; // 현재 타입과 DWORD 프레임만 읽으며 부모/그림자 좌표를 바꾸지 않는다.
        const auto frame=std::uint32_t(raw[36])|(std::uint32_t(raw[37])<<8)|(std::uint32_t(raw[38])<<16)|(std::uint32_t(raw[39])<<24);
        return (core.types[type].flags1&0x400000U) && frame>=static_cast<std::uint32_t>(core.shapes[type].frameCount);
    }
    // 실제4857a0이 설치한122/VT50da88만 간헐천 외형/현재 그림으로 분배한다.
    bool Geyser(o::Sid sid) const {
        return o::IsGeyserCapture(core.pool.Slot(sid));
    }
    bool Handles(o::Sid sid) const { return factory.Handles(sid) || Bomb(sid) || Flare(sid) || Generator(sid) || Temple(sid) || o::IsPatchDais(core.pool.Slot(sid)) || ThunderCannon(sid) || Geyser(sid) || o::IsBurningArcherCapture(core.pool.Slot(sid)) || o::IsSunAviaryCapture(core.pool.Slot(sid)) || o::IsPlainAssetDraw(core.pool.Slot(sid)) || FlyerShadow(sid); } // 일반 자산도 같은 원본 본체/상태색/기준점/그림자 경로에 포함한다.
    // 원본 PE 초기값만 설치하고 각 수집에서 현재 게임 값으로 갱신한다.
    Impl(RawWorldCore world,const GameAssets& data,TempleDraw supplied):core(world),assets(data),factory(core.pool,core.frames),
        collection(core.hash,core.pool,core.types,core.shapes),views(MakeSpriteAssetViews(data)),templeDraw(std::move(supplied)),templeBaseFrames(core.types.size(),-1) {
        // 실제 파싱한TYPE의 기본 프레임만 번호를 맞춰 공급한다. 템플은 원본 소유자 색표 대상이 아니다.
        for (const auto& type:assets.Types()) templeBaseFrames.at(type.block+o::kFirstAssetTypeNumber)=type.definition.specialFrames.baseFrame;
        state.filteredFlags=2421574144U;state.wideOwnerType=164;state.unfilteredType=84;state.shadowAliasType=158;
        state.shadowAliasPinned=true; // 실제5318ec의 초기DWORD6이다.0일 때만 alias를 바꾼다.
        state.markerEnabled=true;state.markerExcludeType=157;state.markerType=96;state.statusType=118;state.abstractStatusType=117;
        state.timerProcessType=31;state.statusProcessType=57;state.costType=122;state.timerTypes={138,144,73};
        state.selectionColors={84,84,249,184,140,150,112,156,222}; // 원본5319c4의 고정9DWORD다.
        state.ownerMaps.resize(core.types.size());
        for (const auto type:kStandardOwnerTypes) { state.ownerMaps.at(type)=1;ownerColors.emplace(type,BuildStandardOwnerColorMaps(type,82)); } // 실제 일반9색표를 타입별로 보존한다.
        for (const auto type:kSpecialOwnerTypes) state.ownerMaps.at(type)=2; // 존재 여부는 처음부터 실제 원본 표를 따르고 색은 SetPalette에서 만든다.
    }
    // 건설/추상/그림자는 명시 표, 아웃포스트는 같은 현재 소유자 색 번호로 변환한다.
    ColorMap Color(SpriteShapeMode mode,std::uint32_t type,std::uint32_t owner) const {
        if (mode==SpriteShapeMode::Abstract) return colors.bright;if (mode==SpriteShapeMode::Dead) return colors.dead;if (mode==SpriteShapeMode::Shadow) return colors.shadow;
        if (mode!=SpriteShapeMode::Owner || owner>=core.bridgeState.ownerColors.size()) throw std::logic_error("실제 자산 소유자 색표 분기 오류");
        const auto found=ownerColors.find(type);if (found==ownerColors.end()) throw std::logic_error("실제 자산 현재 소유자 색표 미연결");
        const auto index=core.bridgeState.ownerColors[owner];if (index<0 || index>8) throw std::out_of_range("실제 자산 현재 소유자 색 번호");return found->second[static_cast<std::size_t>(index)];
    }
    // 원본 현재 슬롯과 실제 dirty clip을 읽고 일반 Draw의 모든 픽셀을 같은 표면에 쓴다.
    void Draw(o::RenderCollectionEntry entry,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
        const auto sid=collection.Object(entry.token);if (!Handles(sid)) return;if (!ready) throw std::logic_error("Factory 파일 팔레트 미설치");
        state.now=core.processState.now;state.ownerPalettes.assign(core.bridgeState.ownerColors.begin(),core.bridgeState.ownerColors.end());
        SpritePixelTarget target(pixels,pitch,height,clip,MakeSpriteAssetShapes(assets,state),[this](auto mode,auto type,auto owner) { return Color(mode,type,owner); });
        SpriteDrawHooks rules;rules.virtualFrame=[this](o::Sid object) {
            if (Bomb(object)) return -1; // 원본 공통 주문의+a0는 빈 본체다.
            if (Temple(object)) return o::PatchTempleMainFrame(core.pool.Slot(object),templeBaseFrames);
            if (ThunderCannon(object)) return o::ThunderCannonCaptureMainFrame(core.pool.Slot(object),core.frames[core.pool.Slot(object)[10]]);
            if (o::IsSunAviaryCapture(core.pool.Slot(object))) return o::SunAviaryCaptureMainFrame(core.pool.Slot(object));
            if (o::IsBurningArcherCapture(core.pool.Slot(object)) || o::IsPlainAssetDraw(core.pool.Slot(object))) return assets.Types()[core.pool.Slot(object)[10]-o::kFirstAssetTypeNumber].definition.specialFrames.baseFrame;
            return Flare(object) || Generator(object) || Geyser(object) || o::IsPatchDais(core.pool.Slot(object)) ? assets.Types()[core.pool.Slot(object)[10]-o::kFirstAssetTypeNumber].definition.specialFrames.baseFrame : factory.MainFrame(object);
        };rules.surface=MakeSpriteSurfaceLookup(core.hash);
        rules.constructionActive=[this](o::Sid object) { return core.processes.FindEvent(object,0x25d)!=nullptr; }; // 491af0은 현재 사제의 기도 Regular를 조회한다.
        rules.timers=[this](o::Sid object,std::uint32_t type)->std::optional<std::array<double,3>> {
            const auto form=o::FindAttachedProcess(core.pool,object,type);if (!form.value) return std::nullopt;
            const auto* process=dynamic_cast<const o::FlyerProcess*>(core.processes.FromForm(form)); // 4ae2d0의 현재 inner 포인터에 대응하는 같은 소유 프로세스다.
            if (!process) throw std::logic_error("비행 선택 타이머의 현재 프로세스 미연결");
            std::array<double,3> result{};const auto& bytes=process->Fields().bytes;
            constexpr std::array<std::size_t,3> offsets{72,104,40}; // 원본498220의 outer+48/+68/+28이며 inner에서-30 보정한 뒤 읽는다.
            for (std::size_t index=0;index<offsets.size();++index) { // 각 선택 막대의 실제 double 기한을 그대로 전달한다.
                std::uint64_t bits{};for (std::size_t byte=0;byte<8;++byte) bits|=std::uint64_t(bytes[offsets[index]+byte])<<(byte*8); // unaligned 원문을 little endian으로 읽는다.
                result[index]=std::bit_cast<double>(bits);
            }
            return result;
        };
        rules.resource=[this](o::Sid object)->std::optional<SpriteResourceStatus> {
            const auto* cast=core.processes.FindEvent(object,0x493);if (!cast) return std::nullopt;
            const o::ContainedFinderState current{core.processState.boss,6,false};o::RawContainedFinder finder(core.pool,current);o::Sid content{}; // 같은 실제 기본 finder의 타입6/mask1 조회다. 현재 제품의 개발자 dead 진단은 꺼져 있다.
            for (auto candidate=finder.Begin(object);candidate.value;candidate=finder.Next()) { // 원본처럼 상태를 추가로 거르지 않고 첫 mask1 일치를 고른다.
                const auto bytes=core.pool.Slot(candidate);if ((std::uint32_t(bytes[20])|(std::uint32_t(bytes[21])<<8))&1U) { content=candidate;break; }
            }
            const auto bytes=core.pool.Slot(content);return SpriteResourceStatus{std::uint32_t(bytes[18])|(std::uint32_t(bytes[19])<<8),std::bit_cast<std::int32_t>(cast->Count())}; // 빈 내용물도 원본은0번 슬롯의 kind를 읽는다.
        };
        rules.statusProcesses=[this]()->std::span<const SpriteStatusProcess> {
            statusSlots.clear();statusSlots.push_back({}); // 원본이 건너뛰는 첫 슬롯을 유지한다.
            for (o::ProcessId pid=1;pid<=core.kernel.Capacity();++pid) { // 실제 현재 Kernel을 원래 PID 순서로 읽으며57만 순서를 보존해 모은다.
                const auto* process=dynamic_cast<const o::SquidProcess*>(core.kernel.Get(pid));if (!process || core.pool.Slot(process->Form())[10]!=state.statusProcessType) continue;
                const auto* effect=dynamic_cast<const o::BombEffectTimerProcess*>(process);if (!effect) throw std::logic_error("상태57의 실제 타이머 필드 미연결");
                const auto& fields=effect->Fields();statusSlots.push_back({state.statusProcessType,fields.flags,fields.members,fields.endTime}); // 별도 대상/시계를 만들지 않고 같은 현재 필드를 빌린다.
            }
            return statusSlots;
        };
        auto hooks=target.Hooks(std::move(rules));
        // +a4의0044c180은 같은 기준점/단위 배율의 plain SHP다. 일반 owner 문맥으로 바꾸지 않는다.
        hooks.virtualDraw=[this,shape=hooks.shape](o::Sid object,ScreenPoint point) {
            const auto composite=core.pool.Slot(object); // 같은 프레임의 현재 본체/추가 그림 분배에 사용한다.
            if (Bomb(object)) { const auto frame=std::uint32_t(composite[36])|(std::uint32_t(composite[37])<<8)|(std::uint32_t(composite[38])<<16)|(std::uint32_t(composite[39])<<24);if (!(composite[40]&0x10U)) shape({composite[10],frame,point,SpriteShapeMode::Plain});else if (composite[34]==state.localPlayer) shape({composite[10],frame,point,SpriteShapeMode::Abstract});return; } // 같은 현재497280 상태색/로컬 관문으로 추가 그림을 그린다.
            if (o::IsPlainAssetDraw(composite)) return; // 검증한 일반 자산의+a4=41c100은 원본처럼 빈ret8이다.
            if (o::IsSunAviaryCapture(composite) || o::IsBurningArcherCapture(composite)) {
                const auto draw=[this,object,&shape](auto type,auto frame,auto x,auto y) { // 실제497280과 같은 현재 추상/로컬 소유자 관문으로 추가 그림을 출력한다.
                    const auto current=core.pool.Slot(object);
                    if (!(current[40]&0x10U)) shape({type,frame,{x,y},SpriteShapeMode::Plain});
                    else if (current[34]==state.localPlayer) shape({type,frame,{x,y},SpriteShapeMode::Abstract});
                };
                if (o::IsSunAviaryCapture(composite)) o::DrawSunAviaryCapture(composite,draw,point.x,point.y);
                else {
                    const auto frame=std::uint32_t(composite[36])|(std::uint32_t(composite[37])<<8)|(std::uint32_t(composite[38])<<16)|(std::uint32_t(composite[39])<<24); // 현재 동작 프레임의 포구 위치다.
                    const auto& header=core.shapes[composite[10]].frames.at(frame); // 본체가 아닌 현재 프레임의 디스크 기준점을 읽는다.
                    o::DrawBurningArcherCapture(composite,assets.Types()[composite[10]-o::kFirstAssetTypeNumber].definition.specialFrames.baseFrame,header.muzzleX,header.muzzleY,draw,point.x,point.y);
                }
                return;
            }
            if (o::IsPatchDais(core.pool.Slot(object))) {
                o::DrawDaisAdditional(core.pool.Slot(object),core.shapes[o::kPatchDaisExtraType].frameCount,[this,object,point,&shape](std::uint32_t type,std::int32_t frame) {
                    if (frame<0) throw std::out_of_range("다이스 추가 그림 음수 프레임");const auto current=core.pool.Slot(object); // 실제497280의 현재 추상/로컬 관문이다.
                    if (!(current[40]&0x10U)) shape({type,static_cast<std::uint32_t>(frame),point,SpriteShapeMode::Plain});
                    else if (current[34]==state.localPlayer) shape({type,static_cast<std::uint32_t>(frame),point,SpriteShapeMode::Abstract});
                });return;
            }
            if (Flare(object) || Generator(object)) return; // 실제+a4의 빈 몸체41c100이다.
            if (Geyser(object)) {
                o::DrawGeyserCapture(core.pool.Slot(object),[this,object,&shape](auto type,auto frame,auto x,auto y) {
                    const auto current=core.pool.Slot(object); // 실제497280의 현재 추상/로컬 관문을 각 그림 요청에서 읽는다.
                    if (!(current[40]&0x10U)) shape({type,frame,{x,y},SpriteShapeMode::Plain});
                    else if (current[34]==state.localPlayer) shape({type,frame,{x,y},SpriteShapeMode::Abstract});
                },point.x,point.y);return;
            }
            if (ThunderCannon(object)) {
                // 원본46ae30→497280은 고정 본체 뒤 현재 그림을 같은 기준점에 겹친다.
                o::DrawThunderCannonCapture(core.pool.Slot(object),[this,object,&shape](auto type,auto frame,auto x,auto y) {
                    const auto current=core.pool.Slot(object);
                    if (!(current[40]&0x10U)) shape({type,frame,{x,y},SpriteShapeMode::Plain});
                    else if (current[34]==state.localPlayer) shape({type,frame,{x,y},SpriteShapeMode::Abstract});
                },point.x,point.y);return;
            }
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
    for (const auto type:kSpecialOwnerTypes) impl_->ownerColors.insert_or_assign(type,BuildSpecialOwnerColorMaps(logical,type)); // 파일 팔레트 변경 때 전용 명도 표도 같은 시점에 갱신한다.
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
        RenderSprite sprite;sprite.clip=clip;sprite.order={entry.x,entry.y,entry.depth};sprite.shadow=(entry.tail&4U)!=0; // 실제 수집기의 그림자 표식을 외부 스프라이트 메타데이터에도 보존한다.
        sprite.sourceSid=self.collection.Object(entry.token).value; // 겹친 본체/임시 개체도 좌표 대신 원본 객체 신원으로 구별한다.
        sprite.procedure=RenderProcedure{clip,[this,entry](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect cut) { impl_->Draw(entry,pixels,pitch,height,cut); }};result.push_back(std::move(sprite));
    }
    return result;
}
}
