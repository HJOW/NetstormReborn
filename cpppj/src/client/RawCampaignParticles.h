// 같은 캠페인 Host/시계와 실제 색 할당·dirty/Renderer·VFX 선 출력을 연결한다.
#pragma once
#include "client/ParticlePaletteMemory.h"
#include "client/RawSurfaceWorld.h"
#include "client/Renderer.h"
#include "client/Sound.h"
#include "o/ConstructionVisualSystem.h"
#include "o/ParticleGradient.h"
#include "o/FieryTailVisualSystem.h"
#include "o/LightningVisualSystem.h"
#include "o/ShrapnelVisualSystem.h"
#include "o/DissolveVisualSystem.h"
#include "o/BurningVisualSystem.h"

namespace netstorm::client {
// 같은 Session의 실제 dummy/공간/캡처/의존과 부착 전 실패 삭제 경계다.
struct CampaignShrapnelHooks {
    o::ShrapnelLifetimeHooks lifetime; // create/pop/depend/capture는 실제 월드에 연결한다.
    std::function<void(o::Sid)> destroyUnpaired; // 아직 감시 쌍이 없는 dummy의 실제 가상 삭제다.
    o::CrtRandom* crt{}; // 같은 Session의 원본 시드 초기화/지형과 공유하는 실제 CRT 상태다.
};
// Session의 실제 입자 표시 수명이다. 게임 계산/출력을 빈 효과로 대신하지 않는다.
class RawCampaignParticles {
public:
    // core/난수/팔레트는 이 객체보다 오래 살아야 한다. 해제 전 상위가 같은 Kernel을 비운다.
    RawCampaignParticles(RawWorldCore core,o::GameRandom& random,const GamePalette& palette,
        std::function<void(float,float,std::string_view)> sound={},std::optional<CampaignShrapnelHooks> shrapnel={});
    RawCampaignParticles(const RawCampaignParticles&)=delete; // 실제 콜백 대상 주소를 유지한다.
    RawCampaignParticles& operator=(const RawCampaignParticles&)=delete; // 다른 캠페인에 상태를 복사하지 않는다.
    // 실제 생성한 투사체의 process11을 표시 Pop과 Missile 부착 사이에 붙인다.
    void Add(std::uint32_t type,o::Sid parent);
    // 실제 wrapper의 생존 검사 뒤 공유 빔을 같은 Host/표시 목록에 생성한다. NULL 파일명은 무음이다.
    o::LightningCreateResult AddBeam(o::Sid parent,o::Sid target,std::optional<std::string> sound);
    // 실제472770의화면좌표→현재카메라→직접좌표빔을같은Host/표시수명에생성한다.
    o::LightningVisualProcess* AddScreenBolt(o::Sid parent,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float life);
    // 실제 wrapper의 세 종류 파편을 같은 Host/Renderer에 생성한다. 확보 실패는nullptr다.
    o::ShrapnelVisualProcess* AddShrapnel(o::Sid source,o::ShrapnelKind kind);
    o::DissolveVisualProcess* AddDissolve(o::Sid source,o::Sid first,o::Sid second,std::int32_t xOriented); //현재 캡처/게임 난수/표시 목록으로 애시드18을 생성한다.
    // 같은 표시 목록에 실제 건설 캡처/최대 HP를 연결한다. 첫 건설 전에 한 번만 호출한다.
    void ConnectConstruction(std::function<o::ConstructionVisualImage(o::Sid)> capture,std::function<std::int32_t(o::Sid)> maximum);
    // 실제17 표시와 캡처/입자를 만들며 부모 완료 뒤 같은 Kernel에서 종료한다.
    o::ConstructionVisualProcess* AddConstruction(o::Sid parent,bool instant);
    void ConnectBurning(std::function<o::ConstructionVisualImage(o::Sid)> capture,std::function<void(std::uint32_t)> stopSound); // 같은14의 실제 캡처/장치 정지를 설치한다.
    o::BurningVisualProcess* AddBurning(o::Sid victim,std::uint32_t mode); // 실제0/1 wrapper의 확보 실패는nullptr다.
    // 같은 전체 Kernel 실행 직전 현재 게임 시각을 공급한다.
    void PrepareFrame(double now,std::uint32_t frameNumber=0,bool weakenedMana=false); // 원본 현재DWORD frame과 약화 마나도 같은 월드에서 공급한다.
    // 현재 실제 카메라/표시 영역과 장치 재생성에도 유효한 dirty 경계를 갱신한다.
    void SetView(SoundView view,SurfaceDisplayHooks display);
    // 같은 원본 수집/깊이 키로 SHP와 함께 정렬할 직접 그리기 항목을 만든다.
    std::vector<RenderSprite> Sprites(ScreenRect clip);
    // 실제 등록/수명을 확인하는 읽기 전용 조회다.
    const o::RenderProcessSystem& Rendering() const;
private:
    // Raw registry 자체가 소유하는 vector의 확보/반납과 실제 Host 부착을 사용한다.
    o::RenderRegistryHooks RegistryHooks();
    // 실제 팔레트 생성·OS snapshot·현재 raw 부모 좌표·VFX 출력으로 연결한다.
    o::FieryTailSystemHooks FieryHooks();
    // 같은 현재 중심/실제 공유 색/파일명 음향과 VFX 선 표면을 process19에 공급한다.
    o::LightningSystemHooks LightningHooks();
    // 실제 bitmap 픽셀을 현재 clip/pitch 표면에 연결하고 메모리는 소유 vector가 반납한다.
    o::ShrapnelSystemHooks ShrapnelHooks(CampaignShrapnelHooks hooks);
    o::DissolveSystemHooks DissolveHooks(const CampaignShrapnelHooks& hooks); //같은 실제 dummy/캡처/반납과0바이트 입자 확보를 연결한다.
    // 현재 dirty 큐와 실제 renderer에 같은 요청을 전달한다.
    void Invalidate(o::FieryTailDirtyRect rect);
    // 장치 연결 전 쌓인 원본 변경 표는 버리지 않고 실제 장치에 전달한다.
    void FlushDirty();
    // 하나의 실제 dirty 표면 동안만 VFX 선 대상이 유효하다. 예외 뒤에도 대상을 해제한다.
    void Draw(std::uint32_t token,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip);
    // 실제 현재 표면 잠금 범위다. Frame 단계에는 활성 표면이 없다.
    struct Target {
        std::span<std::uint8_t> pixels; // 잠금된 실제8비트 화면 또는 SceneImage 버퍼다.
        int pitch{},height{}; // 실제 행 폭과 버퍼 높이다.
        ScreenRect clip; // renderer가 현재 그리는 dirty 사각형이다.
    };
    RawWorldCore core_; // 실제 투사체/부모/Form이 사용하는 단일 저장소 참조다.
    o::GameRandom& random_; // 같은 게임의 원본 난수 상태다.
    const GamePalette& source_; // 실제 현재 게임의 RGB 팔레트다.
    std::function<void(float,float,std::string_view)> sound_; // 실제 SoundList/위치 한 번 재생의 파일명 경계다.
    o::ConstructionVisualState renderState_; // 공통 collector가 읽는16/11 배율이다.
    o::FieryTailLifetimeState lifetime_; // 원본 최초 공유 팔레트 생성/재사용 상태다.
    o::FieryTailFrameState frame_; // 현재 게임 시각·카메라·배율이다.
    o::FieryTailFramePalette colors_; // 색수11과 실제 인접 DWORD를 구별하는 현재 읽기 span이다.
    ParticlePaletteMemory memory_; // 원본 요청 크기의 실제 할당과 OS 메모리 읽기 경계다.
    o::LightningLifetimeState lightningLifetime_; // Fiery와 다른0055a758 공유 색 상태다.
    ParticlePaletteMemory lightningMemory_; // 실제11색44바이트 할당이며 빔 Draw는 인접 DWORD를 읽지 않는다.
    SoundView view_{0,0,0,0,1024,768}; // 실제00497220과 같은 정수 카메라 원점이다.
    SurfaceDisplayHooks display_; // 현재 클라이언트 Renderer의 dirty 소비자다.
    DirtyRegions pending_{1024,768}; // 장치 연결 전 요청도 원본100항목/전체 갱신 규칙으로 보존한다.
    std::optional<Target> target_; // 현재 Draw 동안만 존재하는 출력 대상이다.
    o::RenderProcessSystem rendering_; // 모든 표시가 사용하는 공통 원본 목록이다.
    o::FieryTailVisualSystem fiery_; // 같은 Host/Kernel의 실제 process11 관리자다.
    o::LightningVisualSystem lightning_; // 같은 공통 목록/Host/Kernel의 실제 process19 관리자다.
    o::ConstructionFrameState constructionFrame_; // 같은 현재 게임 시각/정수 카메라를 쓰는 건설 표시 전역이다.
    o::ConstructionFramePalette constructionColors_; // 원본11색과 실제 읽을 수 있는 인접 DWORD의 현재 view다.
    ParticlePaletteMemory constructionMemory_; // 빔/비행과 별개인 건설 전용 공유 색 할당이다.
    std::vector<o::ParticleGradient> constructionGradients_; // 생성자가 요청하는 두 실제 색 띠다.
    std::unique_ptr<o::ConstructionVisualSystem> construction_; // 같은 표시 목록/Host/난수를 쓰는 실제17 수명이다.
    std::unique_ptr<o::ShrapnelVisualSystem> shrapnel_; // 실제 세계/캡처 경계가 제공된 Session의 process16이다.
    std::unique_ptr<o::DissolveVisualSystem> dissolve_; //동일한 실제 세계/원본 캡처로 대상 삭제 뒤 유지하는18이다.
    o::BurningVisualState burningVisual_; // 같은 월드의 실제14/첫 공유 색 표다.
    o::BurningSourcesState burningSources_; // 같은 현재 약화 마나 옵션이다.
    o::BurningFrameState burningFrame_; // 현재 시각/카메라/16·11/바람이다. 바람 초기화의 전체Client 복원은 후속이다.
    o::BurningDrawPalette burningColors_; // 실제8색과 인접 DWORD snapshot이다.
    ParticlePaletteMemory burningMemory_; // 다른 표시와 공유하지 않는 실제8색 할당이다.
    std::vector<o::ParticleGradient> burningGradients_; // 첫 생성자의 실제 색 띠 한 개다.
    std::uint32_t burningFrameNumber_{}; // 실제487940 gate가 읽는 현재55b4a8이다.
    std::unique_ptr<o::BurningVisualSystem> burning_; // 모든 하위가 살아 있을 때 먼저 해제하는 실제14 수명이다.
};
}
