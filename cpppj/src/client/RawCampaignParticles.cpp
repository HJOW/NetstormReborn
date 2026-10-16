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
        const auto point=SoundToScreen(view_,std::bit_cast<float>(x),std::bit_cast<float>(y));return std::array<std::int32_t,2>{point.x,point.y};
    };
    hooks.release=[](o::FieryTailRelease) {}; // 실제 입자 저장소/객체는 원본 수명 몸체와 Kernel이 반납한다.
    return hooks;
}
// 지원하지 않는 효과를 빈 성공으로 통과시키지 않는다.
void RawCampaignParticles::Add(std::uint32_t type,o::Sid parent) {
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
o::LightningCreateResult RawCampaignParticles::AddBeam(o::Sid parent,o::Sid target,std::optional<std::string> sound) {
    return o::CreateLightning(core_.pool,parent,target,std::move(sound),[this](o::Sid source,o::Sid victim,std::optional<std::string> name) {
        if (name && !sound_) throw std::logic_error("빔 실제 파일명 음향 소비자 미연결");
        try { return lightning_.Add(source,victim,std::move(name)); }
        catch (const std::bad_alloc&) { return static_cast<o::LightningVisualProcess*>(nullptr); }
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
// 원본 게임 시각을 별도 고정 틱이나 벽시각으로 대체하지 않는다.
void RawCampaignParticles::PrepareFrame(double now) {
    if (!std::isfinite(now)) throw std::invalid_argument("캠페인 입자 게임 시각 오류");frame_.now=now;
}
// 카메라가 바뀌면 표시 영역을 함께 다시 그린다. 같은 view 호출도 대기 dirty를 장치에 전달한다.
void RawCampaignParticles::SetView(SoundView view,SurfaceDisplayHooks display) {
    const bool changed=view.cameraX!=view_.cameraX || view.cameraY!=view_.cameraY || view.left!=view_.left || view.top!=view_.top || view.right!=view_.right || view.bottom!=view_.bottom;
    view_=view;frame_.cameraX=static_cast<float>(view.cameraX);frame_.cameraY=static_cast<float>(view.cameraY);display_=std::move(display);
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
std::vector<RenderSprite> RawCampaignParticles::Sprites(ScreenRect clip) {
    std::vector<o::RenderCollectionEntry> entries(rendering_.Slots().size());
    if (entries.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::length_error("입자 표시 목록 크기 오류");
    const o::RenderWorldRect view{static_cast<float>((static_cast<double>(clip.left)+view_.cameraX)/16),static_cast<float>((static_cast<double>(clip.top)+view_.cameraY)/11),
        static_cast<float>((static_cast<double>(clip.right)+view_.cameraX)/16),static_cast<float>((static_cast<double>(clip.bottom)+view_.cameraY)/11)};
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
void RawCampaignParticles::Draw(std::uint32_t token,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
    if (!rendering_.Lookup(token)) return;if (target_) throw std::logic_error("입자 표면 중첩 그리기 오류");
    target_=Target{pixels,pitch,height,clip};
    try { if (memory_.Count()) colors_.readable=memory_.Snapshot();rendering_.DrawToken(token); }
    catch (...) { target_.reset();throw; }
    target_.reset();
}
// 실제 등록/프로세스 수명 검사용 단일 저장소다.
const o::RenderProcessSystem& RawCampaignParticles::Rendering() const { return rendering_; }
}
