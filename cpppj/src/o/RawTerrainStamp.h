// 초기 TerrainBuilder의 두 이웃mask·독립 CRTcache·실제 지면/fringe 생성을 같은 raw 풀에 연결한다.
#pragma once
#include "o/RawFrameChoose.h"
#include "o/SidPool.h"
#include "o/TerrainBuilder.h"
#include <array>
#include <functional>
#include <optional>

namespace netstorm::o {
// 실제 TerrainBuilder code/지역 지도와 변경 가능한 문자 타입mapping 및 context다.
struct TerrainStampMap {
    // codes: 원본 X열 우선 65,536BYTE 지면 문자다. 렌더의 Y행 자료는 명시적으로 전치해야 한다.
    std::span<const std::uint8_t> codes;
    // regions: codes와 같은 X열 우선 지역BYTE 지도다.
    std::span<const std::uint8_t> regions;
    // typeMapping: 문자별 현재 타입DWORD다. Stamp는 x/X/= 항목을 현재 전역 타입으로 다시 쓴다.
    std::span<std::int32_t> typeMapping;
    // aliases: 문자별 대응문자 DWORD다. FrameAt는 기준 문자의 하위BYTE를 부호 확장한다.
    std::span<const std::int32_t> aliases;
    // bounds: left/top/right/bottom이며 stamp 루프는 반열림, 이웃 검사는 양 끝을 포함한다.
    std::array<std::int32_t,4> bounds{};
    // makePredictable: factory의 두 번째 DWORD 인자로 매 생성마다 현재 값을 읽는다.
    std::uint32_t makePredictable{};
    // lastSid: 지면 factory 뒤, owner 호출 전에 저장하는 원본 context의 마지막 SID다.
    Sid lastSid{};
    // excludedFlags: fringe의 signed codeflags와 겹치면 실제 검색/게임 난수를 반복 소비한다.
    std::uint32_t excludedFlags{};
};
// 초기 stamp만 사용하는 전역99cache다. 같은 월드의 RegionPaint cache와 섞지 않는다.
struct TerrainStampState {
    // variants: 첫 FrameAt에서 현재 CRTstate를99회 연속 소비해 만들고 월드 교체 뒤에도 보존한다.
    std::optional<std::array<std::int32_t,99>> variants;
    // islandType: 원본 기본 본섬 타입이다. 실제 로드 전역값을 주입할 수 있다.
    std::uint32_t islandType{97};
    // edgeType: 문자X에 매핑하는 원본 기본 경계 타입이다.
    std::uint32_t edgeType{98};
    // fringeType: 현재 frame flags1에서 추가하는 실제 장식 타입이다.
    std::uint32_t fringeType{102};
    // noRegion: 현재 ChunkMap 조회가 필요한 원본 지역 없음 값이다.
    std::int32_t noRegion{127};
    // fringePrefixFlags: 검색 실패−1 뒤 원본이 읽는 코드배열 바로 앞BYTE의 명시 입력이다.
    std::uint8_t fringePrefixFlags{};
};
// 원본의 두 assertion 지점을 가리킨다. 판본에 따라 실제 호출 유무가 다르다.
enum class TerrainStampAssertion {
    // MissingRegion: 패치의 섬genus 지면이 지역127이면 현재ChunkMap 조회 전에 보고한다.
    MissingRegion,
    // UnpredictableFringe: fringe 생성 직전 makePredictable이0이면 두 판본의 assertion을 보고한다.
    UnpredictableFringe
};
// 객체 생성·가상 owner/Pop·현재 지역 setter를 월드의 실제 복원 몸체에 연결한다.
struct TerrainStampHooks {
    // create: 같은 pool에 type/predictable을 전달해 생성한 실제 SID를 반환한다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create;
    // setOwner: 지면의 가상 +0x74(0)이며 raw 소유자와 후속 장부를 갱신한다.
    std::function<void(Sid,std::uint32_t)> setOwner;
    // pop: 가상 +0x90이며 같은 SID의 실제 Pop/PostPop과 flags0x02000061을 실행한다.
    std::function<void(Sid,float,float,std::uint32_t)> pop;
    // setRegion: 실제 004acd20/CD004aca80이며 현재 타입 조건과 raw 지역WORD 쓰기를 보존한다.
    std::function<void(Sid,std::int32_t)> setRegion;
    // chunkRegion: 지역127에서 x/16,y/16을 원본 절삭으로 얻어 현재 ChunkMap 지역BYTE를 조회한다.
    std::function<std::int8_t(std::int32_t,std::int32_t)> chunkRegion;
    // assertion: 원본 assertion 뒤 계속 진행하는 경로를 호출자에게 보고한다.
    std::function<void(TerrainStampAssertion)> assertion;
};
// 패치004c04b0/004c0640/004bfe50와 CD0049cf20/0049d2c0의 초기 프레임/stamp 몸체다.
class RawTerrainStamp {
public:
    // 타입/프레임/지도와 두 난수는 실제 월드의 같은 저장소이며 필수 하위 효과를 처음에 확인한다.
    RawTerrainStamp(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
        TerrainStampMap& map,GameRandom& random,std::uint32_t& crtState,TerrainStampState& state,TerrainStampHooks hooks);
    // 현재 좌표의 직선/대각선 mask·타입·첫/재사용cache를 읽어 실제 물리 프레임을 고른다.
    std::int32_t FrameAt(std::int32_t x,std::int32_t y);
    // 현재 bounds의 Y외부/X내부로 alias가 요청문자와 같은 nonzero mapping만 실제 생성한다.
    void Stamp(std::int8_t code);
    // 같은 raw pool을 사용하는 월드/팝 결합인지 검사할 때 쓴다.
    const SidPool& Pool() const;
private:
    // 현재 좌표의 X열 우선 index를 얻고 원본 지도 범위 밖 읽기는 진단한다.
    static std::size_t Index(std::int32_t x,std::int32_t y);
    // 기준alias/지역과 실제 이웃이 일치하면 true이며 bounds 검사에는 끝 칸을 포함한다.
    bool Same(std::int32_t x,std::int32_t y,std::int32_t alias,std::uint8_t region) const;
    // 첫 호출에서 현재 공유 CRTstate를99회 소비하며 직전mod4 중복을1 증가시킨다.
    void Initialize();
    // 원본 frame 필드만 DWORD 또는BYTE로 직접 쓴다. 가상 SetFrame을 호출하지 않는다.
    void Frame(Sid sid,std::int32_t frame) const;
    // 단일 지면의 실제 factory/owner/frame/Pop/region/fringe 사슬을 수행한다.
    void Create(std::int32_t x,std::int32_t y,std::uint32_t type,std::int32_t frame,std::int32_t region);
    // pool_: 생성/owner/프레임/Pop이 공유하는 실제 raw 슬롯이다.
    SidPool& pool_;
    // types_: 현재 타입의 genus를 실제 생성 뒤 조회하는 표다.
    std::span<const RiftTypeRecord> types_;
    // frames_: 실제 J시작·codeflags/공유 후보검색에 사용하는 프레임 표다.
    std::span<const RiftTypeFrames> frames_;
    // map_: 현재 code/지역/mapping/bounds/context다.
    TerrainStampMap& map_;
    // random_: 비AA와 fringe 검색이 소비하는 실제 GameRandom이다.
    GameRandom& random_;
    // crtState_: 초기99cache가 소비하는 공유 CRT holdrandDWORD이며 GameRandom과 독립이다.
    std::uint32_t& crtState_;
    // state_: 전역 수명의 초기cache와 현재 특수 타입/원본 입력이다.
    TerrainStampState& state_;
    // hooks_: 필수 생성/owner/Pop/지역/진단 효과다.
    TerrainStampHooks hooks_;
};
}
