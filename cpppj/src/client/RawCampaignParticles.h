// 같은 캠페인 Host/시계와 실제 색 할당·dirty/Renderer·VFX 선 출력을 연결한다.
#pragma once
#include "client/ParticlePaletteMemory.h"
#include "client/RawSurfaceWorld.h"
#include "client/Renderer.h"
#include "client/Sound.h"
#include "o/FieryTailVisualSystem.h"
#include "o/LightningVisualSystem.h"

namespace netstorm::client {
// Session의 실제 입자 표시 수명이다. 게임 계산/출력을 빈 효과로 대신하지 않는다.
class RawCampaignParticles {
public:
    // core/난수/팔레트는 이 객체보다 오래 살아야 한다. 해제 전 상위가 같은 Kernel을 비운다.
    RawCampaignParticles(RawWorldCore core,o::GameRandom& random,const GamePalette& palette,
        std::function<void(float,float,std::string_view)> sound={});
    RawCampaignParticles(const RawCampaignParticles&)=delete; // 실제 콜백 대상 주소를 유지한다.
    RawCampaignParticles& operator=(const RawCampaignParticles&)=delete; // 다른 캠페인에 상태를 복사하지 않는다.
    // 실제 생성한 투사체의 process11을 표시 Pop과 Missile 부착 사이에 붙인다.
    void Add(std::uint32_t type,o::Sid parent);
    // 실제 wrapper의 생존 검사 뒤 공유 빔을 같은 Host/표시 목록에 생성한다. NULL 파일명은 무음이다.
    o::LightningCreateResult AddBeam(o::Sid parent,o::Sid target,std::optional<std::string> sound);
    // 같은 전체 Kernel 실행 직전 현재 게임 시각을 공급한다.
    void PrepareFrame(double now);
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
};
}
