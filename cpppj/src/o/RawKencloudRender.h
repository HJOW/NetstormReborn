// 원문 구름 갱신/dirty/행 조회와 두 표시 부모를 같은 packed 전역에 실행한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o { // 화면 장치는 호출자가 제공하며 계산/반복/쓰기 순서는 원문 몸체다.
struct KencloudRenderGlobals {
    std::span<std::uint32_t> state; //005697f0부터2054DWORD의 같은 이전행/캐시/현재행/flags 물리다.
    const std::uint32_t& speed; //00540a80의 현재값이며 갱신 부모는 생성 복귀 뒤만 다시 읽는다.
};
struct KencloudRenderHooks {
    std::function<void()> build,step; // 기존 실제 생성/행 이동을 선택된 시점에만 합성한다.
    std::function<std::array<std::uint32_t,4>()> clip; // 현재005c7a44..50의 네 DWORD를 그대로 빌려 읽는다.
    std::function<void(const std::array<std::uint32_t,4>&)> setClip; //004a1580의 현재 clip 저장 효과다.
    std::function<void(bool)> lock,unlock; // false=004a14e0/1550, true=004a15d0/1650의 별개 장치다.
    std::function<void(const std::array<std::uint32_t,8>&)> blit; //004a1fe0의 destXY/cache/stride/sourceLTRB 원시 인자다.
};
void UpdateKencloud(const KencloudRenderGlobals& globals,const KencloudRenderHooks& hooks); //00470c50의 cache 관문과 캡처한 count 루프다.
void MarkKencloudDirty(std::span<std::uint32_t> state); //00470c80의 all1→partial0 순서다.
void CleanKencloudDirty(std::span<std::uint32_t> state); //00470ca0의 실제 표시된 행만 순서대로 지운다.
std::uint32_t KencloudRowDirty(std::span<std::uint32_t> state,std::uint32_t first,std::uint32_t second); //00470ce0의 signed 나머지/인접 alias를 보존한다.
void DrawKencloud(const KencloudRenderGlobals& globals,const std::array<std::uint32_t,6>& area,const KencloudRenderHooks& hooks,bool alternate=false); //004711d0/00471410의 clip/잠금/전체·부분 복사/복구다.
}
