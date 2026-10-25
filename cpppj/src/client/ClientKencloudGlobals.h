// 같은 Client의 실제 구름 전역·주소 공간과 팔레트 이름을 메뉴/화면/Session보다 오래 소유한다.
#pragma once
#include "o/RawKencloud.h"
#include "o/RawFortPaletteTransition.h"
#include "o/RawKencloudRender.h"
#include <array>
#include <map>
#include <vector>

namespace netstorm::o { class BaseFileSystem;class ConfigInterface; } // 실제 VFS/설정 본체를 동기 장치에서 빌린다.
namespace netstorm::client { // 구름 계산은 기존 원문 본체를 사용하고 메모리/파일/설정만 연결한다.
class Renderer; // 존재 토큰과 같은 실제 객체의 수명을 연결하며 소유하지 않는다.
class Screen; // 실제 현재 clip/잠금/DIB 장치를 표시 부모가 동기로 빌린다.
class ClientKencloudGlobals {
public:
    enum class Option:std::size_t { Cloudiness,Speed,Fury,Spacer,Recenter,SeedCount,LowColor,HighColor }; // 원문의 현재 여덟 DWORD를 순서대로 식별한다.
    explicit ClientKencloudGlobals(std::function<void(std::span<std::uint8_t>)> allocation={}); // PE 최초값과 BSS0만 구성하며 파일/난수/구름 생성을 실행하지 않는다.
    ClientKencloudGlobals(const ClientKencloudGlobals&)=delete;ClientKencloudGlobals& operator=(const ClientKencloudGlobals&)=delete; // 같은 현재 주소와 고정 전역을 복제하지 않는다.
    ClientKencloudGlobals(ClientKencloudGlobals&&)=delete;ClientKencloudGlobals& operator=(ClientKencloudGlobals&&)=delete; // 빌린 DWORD/행/이름 주소를 유지한다.
    o::KencloudGlobals Globals(); //005697f0..0056b807의 같은 실제 물리와 옵션을 빌린다.
    o::FortPaletteTransitionGlobals PaletteGlobals(); // 같은 cloud/cache/speed/renderer/dirty와 별도 palette 이름이다.
    std::span<std::uint32_t> State();std::span<const std::uint32_t> State() const; // 실제 연속 전역2054DWORD이며 인접 별칭을 보존한다.
    std::uint32_t& Value(Option option);const std::uint32_t& Value(Option option) const; // 같은 현재 옵션 DWORD를 읽고 쓴다.
    std::span<const std::uint32_t,16> PhaseRows() const; // 보호 PE의 실제 고정16행 순서다.
    std::span<std::uint8_t> PaletteName(); // 원문 이름의 같은128BYTE 물리 버퍼다.
    std::span<std::uint8_t> CloudName(); // 이름 뒤 kenCloud/algorithm/dirty/phase까지 같은 물리를 보존한다.
    void ReadOptions(o::ConfigInterface& config); // 현재값을 기본 입력으로 ReadInt하고 실제 성공한 store만 적용한다. 파일 생성은 하지 않는다.
    std::uint32_t Allocate(std::uint32_t bytes); // 겹치지 않는32비트 주소와 정확한 실제 블록을 연결한다.
    std::span<std::uint8_t> Memory(std::uint32_t address); // 내부 주소부터 남은 물리와 길이0 one-past를 조회한다. 해제된 주소는 거부한다.
    void Release(std::uint32_t token); // 정확한 생존 base만 반납한다. Native 부모의 cache0 쓰기는 대신하지 않는다.
    std::size_t Allocations() const; // 현재 실제 생존 블록 수만 관측한다.
    void Build(const o::BaseFileSystem& files,o::ConfigInterface& config,o::GameRandom& random); // 같은 실제 난수와 파일 부모로 전체 원문 생성을 실행한다.
    void Step(); // 같은 현재 캐시/행/dirty에 원문 행 이동을 실행한다.
    o::KencloudRenderGlobals RenderGlobals(); // 같은 packed2054DWORD와 현재 speed를 표시/정리 부모에 빌린다.
    void Update(const o::BaseFileSystem& files,o::ConfigInterface& config,o::GameRandom& random); //00470c50의 생성 후 속도 재조회와 캡처 반복을 실제 생성/step에 합성한다.
    void MarkDirty();void CleanDirty(); // 같은 실제 이전/현재 행과 all/partial을 원문 순서로 갱신한다.
    std::uint32_t RowDirty(std::uint32_t first,std::uint32_t second); // signed 나머지의 음수 인접 alias도 같은 물리에서 읽는다.
    void Draw(Screen& screen,const std::array<std::uint32_t,6>& area,const o::BaseFileSystem& files,o::ConfigInterface& config,o::GameRandom& random,bool alternate=false); // 같은 실제 cache/clip/잠금/DIB 픽셀에 두 표시 부모를 실행한다.
    void Transition(const std::span<const std::uint8_t>& name,const o::BaseFileSystem& files,o::ConfigInterface& config,o::GameRandom& random,o::FortPaletteTransitionHooks devices); // 실제 Screen/추가표 장치를 받아 전체 기존 부모를 조립한다.
    void BindRenderer(Renderer* renderer); // 실제 생성/반납 위치에서 현재 객체와 opaque32 토큰을 연결한다.
    const std::uint32_t& RendererToken() const;std::uint32_t& WorldRedraw(); // 실제 현재 renderer 존재/0059a8b0을 같은 소비자에 빌린다.
private:
    std::uint32_t LoadFile(const o::BaseFileSystem& files,o::ConfigInterface& config); // 실제 FileSpec→exists→치수/픽셀→512판정/free 부모다.
    o::KencloudHooks Devices(const o::BaseFileSystem& files,o::ConfigInterface& config); // 실제 현재 메모리/할당/파일 장치만 만든다.
    std::array<std::uint32_t,2054> state_{}; // 실제8216BYTE의 이전dirty/주소4/현재dirty/seed1000/이름/flags/phase다.
    std::array<std::uint32_t,8> options_{72,2,10,32,99999,400,228,245}; // 보호 PE00540a7c..90와 색 두 DWORD의 최초값이다.
    std::array<std::uint32_t,16> rows_{0,8,4,12,1,11,5,15,3,10,2,7,14,9,6,13}; // 보호 PE00540a98의 고정 행 순서다.
    std::array<std::uint8_t,128> paletteName_{}; //005c8910 이름의 최초 NUL이며 Session 교체로 지우지 않는다.
    std::map<std::uint32_t,std::vector<std::uint8_t>> blocks_; // 생존 주소의 실제 BYTE만 보관하고 해제 뒤의 조회는 실패한다.
    std::uint32_t nextAddress_{0x36000000U}; // 원문 DWORD 산술에 충분한 간격의 host 주소 토큰이다.
    std::function<void(std::span<std::uint8_t>)> allocation_; // 실제 미작성 할당/지역 입력을 공급하며 게임 계산은 하지 않는다.
    std::vector<std::uint8_t> filePath_,palettePath_; // 각 실제 PathSpec 반환을 부모 동기 호출 동안 안정된 NUL 물리로 보관한다.
    Renderer* renderer_{};std::uint32_t rendererToken_{},nextRendererToken_{0x3b000000U},redraw_{}; // 실제 현재 장치의 비소유 연결과 같은 Native dirty DWORD다.
};
}
