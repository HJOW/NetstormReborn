// 원본 몸체를 현재 Windows 할당과 실제 Renderer의8비트 표면에 연결한다.
#include "client/RawCampaignParticles.h"
#include "client/VFXLine.h"
#include "o/ParticleGradient.h"
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// raw 풀/시계는 복사하지 않으며 같은 공통 Host를 실제 표시 시스템에 공급한다.
RawCampaignParticles::RawCampaignParticles(RawWorldCore core,o::GameRandom& random,const GamePalette& palette,
    std::function<void(float,float,std::string_view)> sound,std::optional<CampaignShrapnelHooks> shrapnel)
    :core_(core),random_(random),source_(palette),sound_(std::move(sound)),rendering_(core.pool,core.processes,renderState_,RegistryHooks()),
     fiery_(rendering_,random,lifetime_,frame_,colors_,FieryHooks()),lightning_(rendering_,random,lightningLifetime_,frame_,LightningHooks()) {
    if (shrapnel) {
        if (!shrapnel->crt) throw std::invalid_argument("파편 실제 공유 CRT 상태 미연결");auto& crt=*shrapnel->crt;
        dissolve_=std::make_unique<o::DissolveVisualSystem>(rendering_,random_,frame_,DissolveHooks(*shrapnel));
        shrapnel_=std::make_unique<o::ShrapnelVisualSystem>(rendering_,random_,crt,frame_,ShrapnelHooks(std::move(*shrapnel)));
    }
}
// 실제 포인터 배열의 메모리는 RawRenderRegistry의 vector가 소유한다. 중복 외부 확보/반납을 하지 않는다.
o::RenderRegistryHooks RawCampaignParticles::RegistryHooks() {
    o::RenderRegistryHooks hooks;hooks.reserve=[](std::uint32_t) { return true; };hooks.releaseArray=[] {};return hooks;
}
// 색 선택/초기화/프레임은 기존 원본 몸체이며 외부 메모리와 장치 경계만 공급한다.
o::FieryTailSystemHooks RawCampaignParticles::FieryHooks() {
    o::FieryTailSystemHooks hooks;
    // 입자 vector와 색 메모리 객체가 실제 확보를 수행한다. 이 경계는 원본 요청 종류를 확인한다.
    hooks.lifetime.reserve=[](std::uint32_t bytes) { if (bytes!=2880 && bytes!=8) throw std::logic_error("Fiery Tail 실제 확보량 오류");return true; };
    hooks.lifetime.createPalette=[] { return 1U; }; // 단일 공유 색 객체의 호스트 토큰이며 색 번호가 아니다.
    hooks.lifetime.buildPalette=[this](std::uint32_t token) {
        if (token!=1) throw std::logic_error("Fiery Tail 공유 색 객체 토큰 오류");
        std::array<std::uint32_t,256> logical{}; // 실제 COL의 RGBX DWORD 입력이다.
        // 예약 바이트는 색 축 계산에 쓰지 않으며 실제256색 RGB만 옮긴다.
        for (std::size_t index=0;index<logical.size();++index) {
            const auto color=source_.Color(static_cast<std::uint8_t>(index));
            logical[index]=std::uint32_t(color.red)|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16);
        }
        const auto built=o::BuildFieryTailPalette(logical);memory_.Assign(built);
        colors_.count=static_cast<std::int32_t>(memory_.Count());colors_.readable=memory_.Snapshot();
    };
    hooks.frame.invalidate=[this](o::FieryTailDirtyRect rect) { Invalidate(rect); };
    hooks.frame.line=[this](o::FieryTailLine line) {
        if (!target_) throw std::logic_error("Fiery Tail 실제 그리기 표면 미연결");
        DrawVfxLine(target_->pixels,target_->pitch,target_->height,target_->clip,{line.x1,line.y1},{line.x2,line.y2},line.color);
    };
    hooks.endpoint=[this](o::Sid sid) {
        const auto raw=core_.pool.Slot(sid);std::uint32_t x=0,y=0;
        // 현재 부모 raw 좌표를 읽으며 최초 발사 위치를 캐시하지 않는다.
        for (std::size_t index=0;index<4;++index) { x|=std::uint32_t(raw[14+index])<<(8*index);y|=std::uint32_t(raw[18+index])<<(8*index); }
        const auto point=SoundToScreen(CurrentView(),std::bit_cast<float>(x),std::bit_cast<float>(y));return std::array<std::int32_t,2>{point.x,point.y};
    };
    hooks.release=[](o::FieryTailRelease) {}; // 실제 입자 저장소/객체는 원본 수명 몸체와 Kernel이 반납한다.
    return hooks;
}
// 지원하지 않는 효과를 빈 성공으로 통과시키지 않는다.
void RawCampaignParticles::Add(std::uint32_t type,o::Sid parent) { RefreshCamera();
    if (type!=11) throw std::logic_error("캠페인 비행 표시 효과 미복원 TYPE"+std::to_string(type));
    static_cast<void>(fiery_.Add(parent));
}
// 공유 빔의 계산은 원본 몸체이며 현재 중심/메모리/출력 장치만 실제 클라이언트에 연결한다.
o::LightningSystemHooks RawCampaignParticles::LightningHooks() {
    o::LightningSystemHooks hooks;hooks.lifetime.reserve=[](std::uint32_t bytes) {
        if (bytes!=2880 && bytes!=8 && bytes!=64) throw std::logic_error("빔 실제 확보량 오류");return true;
    };
    hooks.lifetime.createPalette=[] { return 2U; }; // 빔 전용 공유 색 객체 토큰이며 색 번호가 아니다.
    hooks.lifetime.buildPalette=[this](std::uint32_t token) {
        if (token!=2) throw std::logic_error("빔 공유 색 객체 토큰 오류");std::array<std::uint32_t,256> logical{}; // 현재 실제 RGBX 입력이다.
        // 예약 바이트와 무관한 원본256색을 같은 순서로 옮긴다.
        for (std::size_t index=0;index<logical.size();++index) {
            const auto color=source_.Color(static_cast<std::uint8_t>(index));logical[index]=std::uint32_t(color.red)|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16);
        }
        const auto built=o::BuildLightningPalette(logical);lightningMemory_.Assign(built);
    };
    hooks.center=[this](o::Sid sid) { return core_.neighbors.Center(sid); };
    hooks.frame.invalidate=[this](o::LightningDirtyRect rect) { Invalidate(rect); };
    hooks.frame.line=[this](o::LightningLine line) {
        if (!target_) throw std::logic_error("빔 실제 그리기 표면 미연결");DrawVfxLine(target_->pixels,target_->pitch,target_->height,target_->clip,{line.x1,line.y1},{line.x2,line.y2},line.color);
    };
    hooks.frame.sound=[this](float x,float y,const std::string& name) {
        if (!sound_) throw std::logic_error("빔 실제 파일명 위치 음향 미연결");sound_(x,y,name);
    };
    hooks.release=[](o::LightningRelease) {}; // 경로/입자 저장소와 객체는 실제 수명 몸체/Kernel이 반납한다.
    return hooks;
}
// 죽은 표적의 raw 주소 의미와 확보 실패0을 구별하고 실제 생성 성공을 반환한다.
o::LightningCreateResult RawCampaignParticles::AddBeam(o::Sid parent,o::Sid target,std::optional<std::string> sound) { RefreshCamera();
    return o::CreateLightning(core_.pool,parent,target,std::move(sound),[this](o::Sid source,o::Sid victim,std::optional<std::string> name) {
        if (name && !sound_) throw std::logic_error("빔 실제 파일명 음향 소비자 미연결");
        try { return lightning_.Add(source,victim,std::move(name)); }
        catch (const std::bad_alloc&) { return static_cast<o::LightningVisualProcess*>(nullptr); }
    });
}
// 현재viewport와공유색/표시목록을사용하며대체표적SID를만들지않는다.
o::LightningVisualProcess* RawCampaignParticles::AddScreenBolt(o::Sid parent,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float life) { RefreshCamera();
    return o::CreateLightningAtScreen(parent,from,to,life,CurrentView().cameraX,CurrentView().cameraY,[this](o::Sid source,std::array<std::int32_t,2> first,std::array<std::int32_t,2> last,float duration,std::string name) {
        if (!sound_) throw std::logic_error("화면빔 실제파일명 음향미연결");try { return lightning_.AddAtScreen(source,first,last,duration,std::move(name)); }catch (const std::bad_alloc&) { return static_cast<o::LightningVisualProcess*>(nullptr); }
    });
}
// 실제 캡처 그림은 원본 준비 몸체가 읽고 현재8비트 장치에 같은 색 번호를 쓴다.
o::ShrapnelSystemHooks RawCampaignParticles::ShrapnelHooks(CampaignShrapnelHooks input) {
    o::ShrapnelSystemHooks hooks;hooks.lifetime=std::move(input.lifetime);hooks.destroyUnpaired=std::move(input.destroyUnpaired);
    hooks.lifetime.reserve=[](std::uint32_t bytes) { if (bytes!=104) throw std::logic_error("파편 실제 객체 확보량 오류");return true; };
    hooks.frame.reserve=[](std::uint32_t bytes) { if (!bytes) throw std::logic_error("파편 실제 저장소 확보량0 오류");return true; }; // 실제 vector가 요청 저장소를 확보한다.
    hooks.frame.release=[](bool) {}; // 실제 이전 입자/작업 복사본은 RawShrapnelFrame의 소유 저장소가 반납한다.
    hooks.frame.invalidate=[this](o::FieryTailDirtyRect rect) { Invalidate(rect); };
    hooks.frame.pixel=[this](o::ShrapnelPixel pixel) {
        if (!target_) throw std::logic_error("파편 실제 픽셀 표면 미연결");const auto& cut=target_->clip;
        if (pixel.x<cut.left || pixel.x>=cut.right || pixel.y<cut.top || pixel.y>=cut.bottom) return;
        if (pixel.x<0 || pixel.x>=target_->pitch || pixel.y<0 || pixel.y>=target_->height) throw std::out_of_range("파편 실제 clip/표면 경계 불일치");
        target_->pixels[static_cast<std::size_t>(pixel.y)*static_cast<std::size_t>(target_->pitch)+static_cast<std::size_t>(pixel.x)]=static_cast<std::uint8_t>(pixel.color);
    };
    hooks.release=[](o::ShrapnelRelease) {}; // 실제 bitmap/입자/객체는 원본 수명 몸체와 Kernel이 반납한다.
    return hooks;
}
// 실패0과 실제 생성 성공만 반환하며 미연결 캡처를 빈 그림으로 대체하지 않는다.
o::ShrapnelVisualProcess* RawCampaignParticles::AddShrapnel(o::Sid source,o::ShrapnelKind kind) {
    if (!shrapnel_) throw std::logic_error("캠페인 실제 파편 세계/캡처 미연결");
    try { return shrapnel_->Add(source,kind); }catch (const std::bad_alloc&) { return nullptr; }
}
// 캡처가 소유한 픽셀을 같은 원본18의 무중력 이동/수명/현재 표면에 연결한다.
o::DissolveSystemHooks RawCampaignParticles::DissolveHooks(const CampaignShrapnelHooks& input) {
    const auto shared=ShrapnelHooks(input);o::DissolveSystemHooks hooks;
    hooks.lifetime.create=input.lifetime.create;hooks.lifetime.pop=input.lifetime.pop;hooks.lifetime.depend=input.lifetime.depend;hooks.lifetime.capture=input.lifetime.capture;hooks.destroyUnpaired=input.destroyUnpaired;
    hooks.lifetime.reserve=[](std::uint32_t bytes) { if (bytes!=120) throw std::logic_error("분해 실제 객체 확보량 오류");return true; };
    hooks.frame=shared.frame;hooks.frame.reserve=[](std::uint32_t) { return true; }; //원본new0도 성공 포인터이며 실제 저장소는vector가 소유한다.
    hooks.release=shared.release;return hooks;
}
// 실제 실패는nullptr이며 캡처/색표 오류를 빈 효과로 무시하지 않는다.
o::DissolveVisualProcess* RawCampaignParticles::AddDissolve(o::Sid source,o::Sid first,o::Sid second,std::int32_t xOriented) {
    if (!dissolve_) throw std::logic_error("캠페인 실제 분해 세계/캡처 미연결");
    try { return dissolve_->Add(source,first,second,xOriented); }catch (const std::bad_alloc&) { return nullptr; }
}
// 원본 게임 시각을 별도 고정 틱이나 벽시각으로 대체하지 않는다.
void RawCampaignParticles::PrepareFrame(double now,std::uint32_t frameNumber,bool weakenedMana) { RefreshCamera();
    if (!std::isfinite(now)) throw std::invalid_argument("캠페인 입자 게임 시각 오류");frame_.now=now;constructionFrame_.now=now;
    burningFrame_.now=now;burningFrameNumber_=frameNumber;burningSources_.weakenedMana=weakenedMana;
    if (constructionMemory_.Count()) constructionColors_.entries=constructionMemory_.Snapshot();
}
// 카메라가 바뀌면 표시 영역을 함께 다시 그린다. 같은 view 호출도 대기 dirty를 장치에 전달한다.
void RawCampaignParticles::SetView(SoundView view,SurfaceDisplayHooks display) {
    const auto pixels=cameraView_ ? cameraView_->pixels : std::array<std::int32_t,2>{view.cameraX,view.cameraY}; // 현재 소유자와 직전 통지 이력을 구별한다.
    const bool changed=pixels!=notifiedPixels_ || view.left!=view_.left || view.top!=view_.top || view.right!=view_.right || view.bottom!=view_.bottom;
    notifiedPixels_=pixels;
    view_=view;if (cameraView_) { view_.cameraX=0;view_.cameraY=0; }RefreshCamera();display_=std::move(display);
    burningFrame_.viewport={view.left,view.top,view.right,view.bottom};
    if (changed) { pending_=DirtyRegions(view.right,view.bottom);pending_.InvalidateAll(); }FlushDirty();
}
// 원본 현재/이전 영역 요청을 실제 renderer 또는 제한된 대기 표로 보낸다.
void RawCampaignParticles::Invalidate(o::FieryTailDirtyRect rect) {
    if (display_.invalidate) { display_.invalidate({rect.left,rect.top,rect.right,rect.bottom},0);return; }
    pending_.Add({rect.left,rect.top,rect.right,rect.bottom});
}
// 실제 장치가 없을 때는 큐를 지우지 않는다. 장치 연결 후에는 각 원본 요청을 한 번씩 전달한다.
void RawCampaignParticles::FlushDirty() {
    if (!display_.invalidate) return;
    if (pending_.FullRedraw()) display_.invalidate({view_.left,view_.top,view_.right,view_.bottom},0);
    else {
        // 장치가 연결된 뒤에도 원본 변경 표의 요청 순서를 보존한다.
        for (const auto& entry:pending_.Entries()) display_.invalidate({entry.rect.left,entry.rect.top,entry.rect.right,entry.rect.bottom},entry.flags);
    }
    pending_.Clear();
}
// 하나의 원본 수집 결과를 다른 SHP와 같은 전체 정렬에 넣는다. 실제 출력은 dirty 표면이 생긴 뒤 수행한다.
std::vector<RenderSprite> RawCampaignParticles::Sprites(ScreenRect clip) { RefreshCamera();
    std::vector<o::RenderCollectionEntry> entries(rendering_.Slots().size());
    if (entries.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::length_error("입자 표시 목록 크기 오류");
    const o::RenderWorldRect view{static_cast<float>((static_cast<double>(clip.left)+CurrentView().cameraX)/16),static_cast<float>((static_cast<double>(clip.top)+CurrentView().cameraY)/11),
        static_cast<float>((static_cast<double>(clip.right)+CurrentView().cameraX)/16),static_cast<float>((static_cast<double>(clip.bottom)+CurrentView().cameraY)/11)};
    const auto count=rendering_.Collect(view,entries,static_cast<std::int32_t>(entries.size()));std::vector<RenderSprite> result;
    // 공통 kind3의 실제 부모 x/y+0.001·signed 깊이를 보존하고 여기서 종류별 정렬을 하지 않는다.
    for (std::int32_t index=0;index<count;++index) {
        const auto& entry=entries[static_cast<std::size_t>(index)];const auto* fields=rendering_.Lookup(entry.token);if (!fields) continue;
        const auto bounds=o::RenderProcessRemovalBounds(*fields,frame_.cameraX,frame_.cameraY);RenderSprite sprite;
        sprite.clip=clip;sprite.order={entry.x,entry.y,entry.depth};
        sprite.procedure=RenderProcedure{{bounds[0],bounds[1],bounds[2],bounds[3]},[this,token=entry.token](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect cut) { Draw(token,pixels,pitch,height,cut); }};
        result.push_back(std::move(sprite));
    }
    return result;
}
// C++ 범위 밖 읽기를 하지 않고 실제 allocator snapshot을 원본 색 조회 span에 공급한다.
void RawCampaignParticles::Draw(std::uint32_t token,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) { RefreshCamera();
    if (!rendering_.Lookup(token)) return;if (target_) throw std::logic_error("입자 표면 중첩 그리기 오류");
    target_=Target{pixels,pitch,height,clip};
    try { if (memory_.Count()) colors_.readable=memory_.Snapshot();if (burningMemory_.Count()) burningColors_.readable=burningMemory_.Snapshot();rendering_.DrawToken(token); }
    catch (...) { target_.reset();throw; }
    target_.reset();
}
// 실제 등록/프로세스 수명 검사용 단일 저장소다.
const o::RenderProcessSystem& RawCampaignParticles::Rendering() const { return rendering_; }
// 건설 계산은 기존 복원 몸체에 맡기고 실제 캡처/색 할당/화면 장치만 공급한다.
void RawCampaignParticles::ConnectConstruction(std::function<o::ConstructionVisualImage(o::Sid)> capture,std::function<std::int32_t(o::Sid)> maximum) {
    if (construction_ || !capture || !maximum) throw std::invalid_argument("실제 건설 표시 연결 오류");
    o::ConstructionVisualHooks visual; // 원본 Create가 요청하는 저장소/두 색 띠/현재 SHP 경계다.
    visual.reserve=[](std::uint32_t) { return true; }; // 실제 vector와 색 할당이 실패하면 예외로 보고한다.
    visual.createPalette=[] { return 3U; }; // 건설 전용 호스트 토큰이며 팔레트 색 번호와 구별한다.
    visual.gradient=[this](std::uint32_t count,std::array<double,3> from,std::array<double,3> to,double width) {
        std::array<std::uint32_t,256> palette{}; // 현재 원본 RGBX 팔레트다.
        // 실제256색을 원래 순서로 색 띠 계산에 공급한다.
        for (std::size_t index=0;index<palette.size();++index) { const auto rgb=source_.Color(static_cast<std::uint8_t>(index));palette[index]=std::uint32_t(rgb.red)|(std::uint32_t(rgb.green)<<8)|(std::uint32_t(rgb.blue)<<16); }
        constructionGradients_.push_back(o::BuildParticleGradient(palette,from,to,static_cast<std::int32_t>(count),width));
    };
    visual.buildPalette=[this](std::uint32_t token) {
        if (token!=3 || constructionGradients_.size()!=2) throw std::logic_error("건설 실제 공유 색 띠 연결 오류");
        constructionMemory_.Assign(o::MergeParticleGradients(constructionGradients_));constructionColors_={token,static_cast<std::int32_t>(constructionMemory_.Count()),constructionMemory_.Snapshot()};
        constructionGradients_.clear();
    };
    visual.capture=std::move(capture);visual.releaseParticles=[] {}; // 이전 입자 vector는 원본 몸체가 직접 반납한다.
    o::ConstructionFrameHooks frame;frame.maxHitPoints=std::move(maximum); // 같은 실제 보상 최대 HP 조회다.
    frame.invalidate=[this](o::ConstructionFrameRect rect) { Invalidate({rect.left,rect.top,rect.right,rect.bottom}); };
    frame.draw=[this](const o::ConstructionFrameDraw& draw) {
        if (!target_) throw std::logic_error("건설 실제 그리기 표면 미연결");
        const auto& target=*target_; // 이번 Draw 동안만 살아 있는 실제8비트 출력 표면이다.
        // 원본 입력 사각형의 각 행을 현재 dirty clip 안에서만 복사한다.
        for (int row=draw.clip.top;row<draw.clip.bottom;++row) {
            const auto y=static_cast<std::int64_t>(draw.y)+row;if (y<target.clip.top || y>=target.clip.bottom || y<0 || y>=target.height) continue;
            // 255 투명 픽셀은 기존 화면을 보존한다.
            for (int column=draw.clip.left;column<draw.clip.right;++column) {
                const auto x=static_cast<std::int64_t>(draw.x)+column;if (x<target.clip.left || x>=target.clip.right || x<0 || x>=target.pitch) continue;
                const auto color=draw.pixels[static_cast<std::size_t>(row)*draw.pitch+column];if (color!=255) target.pixels[static_cast<std::size_t>(y)*target.pitch+static_cast<std::size_t>(x)]=color;
            }
        }
    };
    construction_=std::make_unique<o::ConstructionVisualSystem>(rendering_,constructionFrame_,constructionColors_,random_,std::move(visual),std::move(frame),[](o::ConstructionRelease) {});
}
// 공통 등록 배열에 실제17 프로세스를 추가한다. 미연결을 빈 효과로 바꾸지 않는다.
o::ConstructionVisualProcess* RawCampaignParticles::AddConstruction(o::Sid parent,bool instant) { RefreshCamera();
    if (!construction_) throw std::logic_error("실제 건설 표시 미연결");return construction_->Add(parent,instant);
}
// 첫 색 표도 원본 gradient/병합 몸체로 만들고 실제 같은 단일 표시 목록을 사용한다.
void RawCampaignParticles::ConnectBurning(std::function<o::ConstructionVisualImage(o::Sid)> capture,std::function<void(std::uint32_t)> stopSound) {
    if (burning_ || !capture || !stopSound) throw std::invalid_argument("실제 Burning 연결 중복/필수 경계 누락");o::BurningSystemHooks hooks;
    hooks.visual.reserve=[](std::uint32_t bytes) { if (bytes!=3600 && bytes!=8) throw std::logic_error("Burning 실제 확보량 오류");return true; }; // 실제 vector/공유 색 객체가 저장소를 소유한다.
    hooks.visual.createPalette=[] { return 4U; }; // 색 번호와 다른 공유 객체 토큰이다.
    hooks.visual.gradient=[this](std::array<double,3> from,std::array<double,3> to,std::uint32_t count,double width) {
        std::array<std::uint32_t,256> logical{}; // 같은 원본 COL RGBX 입력이다.
        for (std::size_t index=0;index<logical.size();++index) { const auto color=source_.Color(static_cast<std::uint8_t>(index));logical[index]=std::uint32_t(color.red)|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 실제 전체 팔레트 순서를 유지한다.
        burningGradients_.push_back(o::BuildParticleGradient(logical,from,to,static_cast<std::int32_t>(count),width));
    };
    hooks.visual.buildPalette=[this](std::uint32_t token) {
        if (token!=4 || burningGradients_.size()!=1) throw std::logic_error("Burning 실제 첫 색표 연결 오류");burningMemory_.Assign(o::MergeParticleGradients(burningGradients_));burningColors_={static_cast<std::int32_t>(burningMemory_.Count()),burningMemory_.Snapshot()};burningGradients_.clear();
    };
    hooks.frame.capture=std::move(capture);hooks.frame.stopSound=std::move(stopSound);
    hooks.invalidate=[this](o::SquidDisplayRect rect) { Invalidate({rect.left,rect.top,rect.right,rect.bottom}); };
    hooks.clip=[this] { if (!target_) throw std::logic_error("Burning 실제 VFX clip 미연결");const auto& clip=target_->clip;return o::SquidDisplayRect{clip.left,clip.top,clip.right,clip.bottom}; };
    hooks.pixel=[this](std::int32_t x,std::int32_t y,std::uint8_t color) { if (!target_ || x<0 || x>=target_->pitch || y<0 || y>=target_->height) throw std::out_of_range("Burning 실제 표면/좌표 오류");target_->pixels[static_cast<std::size_t>(y)*target_->pitch+x]=color; };
    hooks.release=[](o::BurningRelease) {}; // 실제 버퍼와 객체는 원본 몸체/Kernel이 반납한다.
    burningFrame_.pixelScaleX=renderState_.scaleX;burningFrame_.pixelScaleY=renderState_.scaleY;RefreshCamera();burningFrame_.viewport={view_.left,view_.top,view_.right,view_.bottom};
    burning_=std::make_unique<o::BurningVisualSystem>(rendering_,random_,core_.types,burningVisual_,burningSources_,burningFrame_,burningColors_,burningFrameNumber_,std::move(hooks));
}
// wrapper의new90 실패만 자연0이며 실제 미연결/계산 오류를 숨기지 않는다.
o::BurningVisualProcess* RawCampaignParticles::AddBurning(o::Sid victim,std::uint32_t mode) { RefreshCamera(); if (!burning_) throw std::logic_error("실제 Burning14 캡처/수명 미연결");try { return burning_->Add(victim,mode); }catch (const std::bad_alloc&) { return nullptr; } }

}

namespace netstorm::client { // 실제 전역 owner와 기존 값형 하위 frame입력의 경계를 드러낸다.
void RawCampaignParticles::BindCameraView(const RawCameraViewState& view) { cameraView_=&view;view_.cameraX=0;view_.cameraY=0;RefreshCamera(); }
SoundView RawCampaignParticles::CurrentView() const { auto view=view_;if (cameraView_) { view.cameraX=cameraView_->pixels[0];view.cameraY=cameraView_->pixels[1]; }return view; }
void RawCampaignParticles::RefreshCamera() { const auto view=CurrentView();frame_.cameraX=cameraView_ ? std::bit_cast<float>(cameraView_->pixelFloatBits[0]) : static_cast<float>(view.cameraX);frame_.cameraY=cameraView_ ? std::bit_cast<float>(cameraView_->pixelFloatBits[1]) : static_cast<float>(view.cameraY);constructionFrame_.cameraX=burningFrame_.cameraX=frame_.cameraX;constructionFrame_.cameraY=burningFrame_.cameraY=frame_.cameraY; }
}
