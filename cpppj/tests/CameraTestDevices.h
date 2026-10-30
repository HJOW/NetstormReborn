// 화면이 없는 독립 검사도 실제433aa0/497960을 사용하고 OS 장치 경계만 명시한다.
#pragma once
#include "client/CameraHome.h"
#include "client/RawCampaignSession.h"
#include "client/RawCameraCommit.h"
#include <memory>
#include <stdexcept>
#include <utility>

namespace netstorm::test::camera { // 제품의 Client 경계와 섞지 않는 독립 검사 장치다.
// world float를 실제433aa0으로 변환하고 commit 직전의 픽셀/현재 카운터만 기존 관측기에 전달한다.
inline std::function<void(float,float)> MakePointDevice(client::CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> observe) {
    auto camera=std::make_shared<client::ClientCameraGlobals>(); // 관측 lambda 수명 동안 독립 원문 Renderer의 목표를 보존한다.
    return [&state,camera,observe=std::move(observe)](float x,float y) {
        camera->CenterImmediately(x,y,state.width,state.height,state.requests,state.revision,[&] { observe(camera->Target().pixels[0],camera->Target().pixels[1]); });
    };
}
// 도킹/커서가 없는 명시 전체 viewport 장치다. 투영/두setter/dirty이동은 실제497960 본체에 맡긴다.
inline void CommitScreenless(const client::CampaignCameraCommitContext& context) {
    auto& camera=context.camera;std::uint32_t redraw{};std::vector<client::DirtyRegion> dirty; // 독립 장치가 소유하는 현재 Renderer 변경 표다.
    const std::uint32_t flags=2;const auto width=context.width,height=context.height;const client::ScreenRect clip{0,0,width,height};const client::RawCameraCommitViewport viewport{0,0,width,height};const client::ScreenPoint mouse{}; // 실제 DIB와 같은 외부 화면 입력이며 현재view를 임의로 쓰지 않는다.
    client::RawCameraCommitHooks hooks;hooks.lockCursor=[](auto) {};hooks.unlockCursor=[] {};hooks.redrawRoots=[] {};hooks.scroll=[](auto,auto,auto,auto,auto,auto) {}; // 독립 검사에 HWND/커서/Gump가 없다는 명시 장치 경계다.
    hooks.scrollBackground=[](auto,auto,auto,auto,auto,auto) { throw std::logic_error("독립 DIB의 비DIB 배경 호출"); };
    hooks.addDirty=[&](auto rect,auto value) { dirty.push_back({rect,value}); };hooks.addBackgroundDirty=[](auto) { throw std::logic_error("독립 DIB의 배경dirty 호출"); };hooks.updateMouse=[](auto) {};hooks.assertion=[](auto,auto) {}; // 실제 본체가 선택한 변경 표는 저장하고 원문 진단 반환만 공급한다.
    client::CommitCameraTarget({camera.RendererView(),camera.Snapshot(),camera.Target(),redraw,context.worldRedraw,dirty},{flags,camera.Denominator().current,width,height,viewport,clip,mouse},hooks);
}
}
