// 원본433660→498220→Factory +78/+a4를 실제8비트 장면의 동일 정렬 위치에서 실행한다.
#include "client/ClientColorSortInput.h"
#include "client/RawCampaignFactoryDisplay.h"
#include "client/RawObjectHit.h"
#include "client/SpellFrameVisibility.h"
#include "client/ObjectCollection.h"
#include "client/SpriteDraw.h"
#include "client/SpriteColorMaps.h"
#include "client/PaletteColors.h"
#include "client/OwnerColorMaps.h"
#include "client/OwnerColorTableState.h"
#include "o/RawFactoryDraw.h"
#include "o/RawDaisDraw.h"
#include "o/RawManaDraw.h"
#include "o/RawAuxiliaryDraw.h"
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
namespace { // 보호PE 슬롯은 실제 실행분배만 확인하며 새로운 상태를 소유하지 않는다.
struct ObjectHitVtable { std::uint32_t table,frame,additional,hit; }; // 실제VT와+78/+a4/+b0 주소다.
#include "client/ObjectHitVtables.inc"
std::uint32_t DisplayDword(std::span<const std::uint8_t> raw,std::size_t at) { // 현재 비정렬DWORD만 읽는다.
    std::uint32_t value{}; // 원문DWORD 비트다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(raw[at+byte])<<(8*byte); // 정렬이나이웃필드를가정하지않는다.
    return value;
}
const ObjectHitVtable& HitVtable(std::span<const std::uint8_t> raw) { // 실제VT가확인되지않은타입을빈+a4로대체하지않는다.
    const auto address=DisplayDword(raw,0); // 현재슬롯의실제가상표다.
    for (const auto& entry:kObjectHitVtables) if (entry.table==address) return entry; // 보호PE의83실제표만분배한다.
    throw std::logic_error("픽셀 선택의 실제 가상 표 미복원");
}
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
    std::shared_ptr<OwnerColorTableState> sharedOwnerColors; // 실제 Client의 현재 표와 배너 DWORD이며 Session 반납으로 초기화하지 않는다.
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
    bool Bomb(o::Sid sid) const { const auto raw=core.pool.Slot(sid);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);return (raw[10]==117 && table==0x502510U) || (raw[10]==118 && table==0x502ff8U) || (raw[10]==168 && table==0x502370U) || (raw[10]==171 && table==0x5025d8U) || (raw[10]==172 && table==0x502e68U) || (raw[10]==173 && table==0x502f30U); } // 공통+a0=41c0f0/+a4=41e3d0인 여섯 주문이다.
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
    bool ManaBolt(o::Sid sid) const { const auto raw=core.pool.Slot(sid);return raw[10]==105 && DisplayDword(raw,0)==0x509810U; } // 원본105의+a0는 빈 몸체이며+a4가 그림자와 실제 원소색 그림을 그린다.
    bool Handles(o::Sid sid) const { return factory.Handles(sid) || Bomb(sid) || Flare(sid) || Generator(sid) || Temple(sid) || o::IsPatchDais(core.pool.Slot(sid)) || ThunderCannon(sid) || Geyser(sid) || ManaBolt(sid) || o::IsBurningArcherCapture(core.pool.Slot(sid)) || o::IsSunAviaryCapture(core.pool.Slot(sid)) || o::IsPlainAssetDraw(core.pool.Slot(sid)) || FlyerShadow(sid); } // 일반 자산도 같은 원본 본체/상태색/기준점/그림자 경로에 포함한다.
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
        const auto index=core.bridgeState.ownerColors[owner];if (index<0 || index>8) throw std::out_of_range("실제 자산 현재 소유자 색 번호");
        if (sharedOwnerColors) return sharedOwnerColors->Color(type,static_cast<std::uint32_t>(index));
        const auto found=ownerColors.find(type);if (found==ownerColors.end()) throw std::logic_error("실제 자산 현재 소유자 색표 미연결");return found->second[static_cast<std::size_t>(index)];
    }
    // 같은+a4 본체를화면과픽셀선택에서호출한다. 선택만의새로운파생분배를만들지않는다.
    void Additional(o::Sid object,ScreenPoint point,const std::function<void(SpriteShapeRequest)>& shape,const std::function<void(o::Sid,int,int)>& bridgeDraw={}) {
            const auto composite=core.pool.Slot(object); // 같은 프레임의 현재 본체/추가 그림 분배에 사용한다.
            const auto& dispatch=HitVtable(composite); // 같은 실제+a4 분배를 일반화면과선택캡처가공유한다.
            if (dispatch.additional==0x41c100U) return; // 실제빈ret8만정상반환한다.
            if (dispatch.additional==0x479340U || dispatch.additional==0x485f90U) { // 같은 TYPE/SHP 토큰과 현재 표면의 배율/추상색 출력으로 연결한다.
                const o::AuxiliaryDrawHooks hooks{[](auto type) { return type; },[&shape](const auto& value) {
                    SpriteShapeRequest request{value.shape,value.frame,{value.x,value.y},value.abstract ? SpriteShapeMode::Abstract : SpriteShapeMode::Plain};
                    request.scaled=value.scale!=0;request.scaleX=request.scaleY=value.scale;shape(request);
                }};
                if (dispatch.additional==0x479340U) o::DrawScaledMetadisplay(composite,point.x,point.y,hooks);else o::DrawAbstractAdditional(composite,point.x,point.y,hooks);return;
            }
            if (dispatch.additional==0x473710U) { // 마나103의 TYPE/SHP 호스트 토큰은 같은 실제 자산 번호이며105 마나볼트의+a4와 구별한다.
                o::DrawManaAdditional(composite,point.x,point.y,{[](auto type) { return type; },[&shape](const auto& request) {
                    shape({request.shape,request.frame,{request.x,request.y},request.shadow ? SpriteShapeMode::Shadow : SpriteShapeMode::Plain});
                }});return;
            }
            if (ManaBolt(object)) { // 원본45ac40은 현재 프레임을10/10 그림자 뒤 원래 위치의 일반색으로 그린다.
                const auto frame=DisplayDword(composite,36);shape({105,frame,{point.x+10,point.y+10},SpriteShapeMode::Shadow});shape({105,frame,point,SpriteShapeMode::Plain});return;
            }
            if (dispatch.additional==0x4223c0U) { if (!bridgeDraw) throw std::logic_error("선택용 다리의 실제 추가Draw 미연결");bridgeDraw(object,point.x,point.y);return; }
            if (dispatch.additional==0x41e3d0U) { const auto frame=std::uint32_t(composite[36])|(std::uint32_t(composite[37])<<8)|(std::uint32_t(composite[38])<<16)|(std::uint32_t(composite[39])<<24);if (SuppressSpellHelpFrame(assets.Edition(),core.types[composite[10]],assets.Types()[composite[10]-o::kFirstAssetTypeNumber].definition.specialFrames,frame)) return;if (!(composite[40]&0x10U)) shape({composite[10],frame,point,SpriteShapeMode::Plain});else if (composite[34]==state.localPlayer) shape({composite[10],frame,point,SpriteShapeMode::Abstract});return; } // 같은 현재497280 상태색/로컬 관문으로 추가 그림을 그린다.
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
            if (!factory.Handles(object)) throw std::logic_error("픽셀 선택의 파생 추가Draw 미복원 TYPE="+std::to_string(composite[10]));
            factory.Additional(object,point.x,point.y,[&shape](auto x,auto y,auto type,auto frame) { if (frame<0) throw std::out_of_range("Factory 추가 그림 음수 프레임");shape({type,static_cast<std::uint32_t>(frame),{x,y},SpriteShapeMode::Plain}); });
    }
    // 원문498220과 픽셀 선택 모두 실제+78을 읽으며 일반 주문도 TYPE+128을 반환한다.
    std::int32_t HitFrame(o::Sid sid) const {
        const auto raw=core.pool.Slot(sid); // 실제getter호출마다현재TYPE/단계를읽는다.
        switch (HitVtable(raw).frame) {
        case 0x4acfe0U:return assets.Types()[raw[10]-o::kFirstAssetTypeNumber].definition.specialFrames.baseFrame;
        case 0x4222f0U:return std::bit_cast<std::int32_t>(DisplayDword(raw,36));
        case 0x454cd0U:return factory.MainFrame(sid);
        case 0x45b530U:return o::SunAviaryCaptureMainFrame(raw);
        case 0x46ae10U:return o::ThunderCannonCaptureMainFrame(raw,core.frames[raw[10]]);
        default:throw std::logic_error("픽셀 선택의 실제+78 미복원");
        }
    }
    // 현재카메라와로컬소유자를읽고고정200픽셀plain캡처를실제RLE에연결한다.
    bool Hit(o::Sid sid,int x,int y,const std::array<std::int32_t,2>& camera,std::uint32_t localPlayer,const std::function<void(o::Sid,int,int)>& bridgeDraw) {
        state.localPlayer=localPlayer;ObjectHitHooks hooks; // 화면호출여부와무관하게같은현재소유자관문을쓴다.
        hooks.frame=[this](o::Sid object) { return HitFrame(object); };
        hooks.plain=[this](auto type,auto frame,auto px,auto py,std::span<std::uint8_t> pixels) { // 선택기본그림에는소유자색/추상색/배율을적용하지않는다.
            if (frame<0) throw std::out_of_range("픽셀 선택의 plain 음수 프레임");
            static_cast<void>(DrawShape(pixels,200,200,assets.Shapes(),type-o::kFirstAssetTypeNumber,static_cast<std::size_t>(frame),px,py,{0,0,199,199}));
        };
        hooks.additional=[this,&bridgeDraw](o::Sid object,auto px,auto py,std::span<std::uint8_t> pixels) { // 실제497280의추상/로컬관문과Factory의plain을보존한다.
            SpritePixelTarget target(pixels,200,200,{0,0,200,200},MakeSpriteAssetShapes(assets,state),[this](auto mode,auto type,auto owner) { return Color(mode,type,owner); });
            Additional(object,{px,py},target.Hooks().shape,bridgeDraw);
        };
        return RawObjectHit(core.pool,core.types,core.shapes,std::move(hooks)).Hit(sid,x,y,camera);
    }
    // 원본 현재 슬롯과 실제 dirty clip을 읽고 일반 Draw의 모든 픽셀을 같은 표면에 쓴다.
    void Draw(o::RenderCollectionEntry entry,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,bool native=false,const std::function<void(o::Sid,int,int)>& bridge={}) {
        const auto sid=collection.Object(entry.token);if (!native && !Handles(sid)) return;if (!ready) throw std::logic_error("Factory 파일 팔레트 미설치");
        state.now=core.processState.Now();state.ownerPalettes.assign(core.bridgeState.ownerColors.begin(),core.bridgeState.ownerColors.end());state.markerVisible=core.ownerMode.battle; // 원문594fbc의 현재 전투 관문이며 Editor의 fort 모드와 구별한다.
        SpritePixelTarget target(pixels,pitch,height,clip,MakeSpriteAssetShapes(assets,state),[this](auto mode,auto type,auto owner) { return Color(mode,type,owner); });
        SpriteDrawHooks rules;rules.virtualFrame=[this,native](o::Sid object) {
            if (native) return HitFrame(object); //원문498220도+78을 조회한다. 실제83개 VT의 다섯 getter를 지형/다리/일반 자산에 함께 적용한다.
            if (Bomb(object) || ManaBolt(object)) return -1; // 원본 공통 주문/마나볼트의+a0는 빈 본체다.
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
            const o::ContainedFinderState current{core.processState.Boss(),6,false};o::RawContainedFinder finder(core.pool,current);o::Sid content{}; // 같은 실제 기본 finder의 타입6/mask1 조회다. 현재 제품의 개발자 dead 진단은 꺼져 있다.
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
        hooks.virtualDraw=[this,shape=hooks.shape,&bridge](o::Sid object,ScreenPoint point) { Additional(object,point,shape,bridge); }; //같은+a4와 실제 Session의 다리 숫자 전역을 공유한다.
        SpriteDraw draw(core.pool,core.types,views,state,std::move(hooks),0);draw.Draw(entry);
    }
};
// 같은 Session의 수명이 끝나기 전에 표시기를 해제한다.
RawCampaignFactoryDisplay::RawCampaignFactoryDisplay(RawWorldCore core,const GameAssets& assets,TempleDraw templeDraw,const std::uint32_t* patchDiagnostic):impl_(std::make_unique<Impl>(core,assets,std::move(templeDraw))) { impl_->state.sharedPatchDiagnostic=patchDiagnostic; }
RawCampaignFactoryDisplay::~RawCampaignFactoryDisplay()=default; // procedure가 더 이상 호출되지 않을 때 반납한다.
bool RawCampaignFactoryDisplay::Hit(o::Sid sid,int x,int y,const std::array<std::int32_t,2>& camera,std::uint32_t localPlayer,const std::function<void(o::Sid,int,int)>& bridgeDraw) const { return impl_->Hit(sid,x,y,camera,localPlayer,bridgeDraw); } // 선택의현재raw와실제추가그림을공유한다.
bool RawCampaignFactoryDisplay::Handles(o::Sid sid) const { return impl_->Handles(sid); } // 별도 TYPE 이름 휴리스틱을 쓰지 않는다.
// 통합 목록의 원문 토큰은 같은 풀/stride이며 Draw 직전의 카메라/현재 소유자를 읽는다.
void RawCampaignFactoryDisplay::DrawEntry(const o::RenderCollectionEntry& entry,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,const std::array<std::int32_t,2>& camera,std::uint32_t localPlayer,std::uint32_t hideBuildings,const std::function<void(o::Sid,int,int)>& bridge) {
    auto& state=impl_->state;state.cameraX=camera[0];state.cameraY=camera[1];state.localPlayer=localPlayer;state.specialMode=hideBuildings;
    impl_->Draw(entry,pixels,pitch,height,clip,true,bridge);
}
void RawCampaignFactoryDisplay::BindOwnerColors(std::shared_ptr<OwnerColorTableState> owner) { // 실제 현재557940/5319c0의 주소를 같은 Draw 상태에 빌린다.
    if (!owner || !owner->Ready()) throw std::invalid_argument("월드 공통 소유자 색표 최초 등록 누락");
    impl_->sharedOwnerColors=std::move(owner);impl_->ownerColors.clear();impl_->state.sharedOwnerMaps=impl_->sharedOwnerColors->Tokens();impl_->state.sharedUseBanners=&impl_->sharedOwnerColors->Enabled();
}
// 장치 BGRX와 별개로 원본 R/G/B/예약 DWORD를 만들어 실제 파일 색 표를 계산한다.
void RawCampaignFactoryDisplay::SetPalette(std::span<const ScreenColor> palette,std::span<const std::uint32_t> colorSortLogical) {
    if (palette.size()!=256) throw std::invalid_argument("Factory 파일 팔레트256색 계약");std::array<std::uint32_t,256> logical{};
    // 실제 현재 파일 팔레트의 모든 번호를 보존한다.
    for (std::size_t i=0;i<logical.size();++i) logical[i]=std::uint32_t(palette[i].red)|(std::uint32_t(palette[i].green)<<8)|(std::uint32_t(palette[i].blue)<<16)|(std::uint32_t(palette[i].reserved)<<24);
    const auto named=BuildPaletteColorTable(logical,impl_->assets.Edition());impl_->colors=BuildSpriteColorMaps(ColorSortLogicalInput(colorSortLogical,logical));impl_->state.barBackground=named.basic[0];
    if (!impl_->sharedOwnerColors) for (const auto type:kSpecialOwnerTypes) impl_->ownerColors.insert_or_assign(type,BuildSpecialOwnerColorMaps(ColorSortLogicalInput(colorSortLogical,logical),type)); // 명시 독립 표시 입력만 지역 표를 갱신한다. 실제 Client 표는 최초 primary와 현재 옵션을 유지한다.
    impl_->state.hitPointColors={named.named.at(10).index,named.basic[5],named.basic[1]};impl_->ready=true;
}
// 원본 수집기의 전체 순서/한 번의 그림자 보정/해시2단계 반환을 유지한다.
std::vector<RenderSprite> RawCampaignFactoryDisplay::Sprites(ScreenRect clip,int cameraX,int cameraY,std::uint32_t localPlayer,std::uint32_t hideBuildings) {
    auto& self=*impl_;self.state.cameraX=cameraX;self.state.cameraY=cameraY;self.state.localPlayer=localPlayer;self.state.specialMode=hideBuildings; // 기존SpriteDraw의그림자/Abstract/가상Draw분기에현재메뉴값을공급한다.
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
