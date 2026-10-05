// Squid Pop/Unpop의 공간 등록 부분을 복원한다. SID 할당/세대/파생 가상 효과는 별도다.
#pragma once
#include "o/SquidHash.h"
#include "o/SquidFinder.h"
#include <functional>
#include <map>

namespace netstorm::o {
// 중복 spot에 대한 원본 두 판본의 서로 다른 경로를 명시적으로 선택한다.
enum class SpatialEdition { Patch1078, CD1072 };
// 등록 전 입력과 원본 공간 필드. 원본 50/36바이트 메모리 배치를 그대로 쓰는 구조체는 아니다.
struct SpatialObject {
    // 127은 원본 무효 섬 표식이다. 나머지 입력 기본값은 파생 생성자의 복원 결과가 아니다.
    std::uint16_t id{},next{},island{127},surfaceWord{};
    std::uint32_t flags1{},flags2{};
    int width{1},height{1};
    float frameWidth{1},frameHeight{1},x{},y{};
    std::int16_t screenX{},screenY{};
    std::uint8_t state{4},extra{},level{}; // state: free=1/dead=2/void=4/contained=8; extra: buried=8/firstPop=128.
    FrameCode frame{}; // 표면 스냅샷용 입력이며 Pop/Unpop이 프레임을 변경하지는 않는다.
};
// 복원하지 않은 가상 firstPop과 영역 조회에 필요한 반환 계약만 호출자에게 받는다.
struct SpatialDependencies {
    std::function<std::uint32_t(std::uint16_t)> firstPopFlags;
    std::function<bool(float,float)> regionSupported;
};
// 원본 효과 호출 순서와 인자를 보존한다. 실제 화면/소유자/생산/프로세스 효과의 구현은 아니다.
enum class SpatialEventKind { Update88,Update8c,FirstPop,PostPop,RegionQuery,SurfaceChanged };
struct SpatialEvent {
    SpatialEventKind kind{};
    std::uint32_t value{};
    // 기계어 fixture와 후속 효과 소비자가 같은 사건을 비교하도록 한다.
    bool operator==(const SpatialEvent&) const = default;
};
// Overlap에서는 패치판이 이미 쓴 spot과 좌표가 남는다. 배치 가능 여부 검사로 사용하지 않는다.
enum class SpatialResult { Unchanged,Registered,Removed,Overlap };
struct SpatialChange {
    SpatialResult result{SpatialResult::Unchanged};
    std::vector<SpatialEvent> events;
};

class SquidSpatial {
public:
    // 지도/체인을 소유한다. 초기 spot 입력은 이미 존재하는 비트에 대한 원본 갱신 검사에도 쓰인다.
    explicit SquidSpatial(SpatialEdition edition=SpatialEdition::Patch1078,
        SpatialDependencies dependencies={},std::span<const std::uint8_t> initialSpots={});
    // 이미 할당된 void 객체 번호를 입력한다. 원본 생성자·번호 할당기·세대 검사는 아직 아니다.
    void Add(SpatialObject object);
    // 004b02d0 ↔ CD 004ad490: 좌표/spot→anchor 버킷 머리→한 번의 firstPop→비전투 Activate.
    SpatialChange Pop(std::uint16_t id,float x,float y,std::uint32_t flags=0);
    // 004afe50 ↔ CD 004ad0b0: void 설정→spot 해제→4단계 검색/체인 제거→화면 사건.
    SpatialChange Unpop(std::uint16_t id,std::uint32_t flags=0);
    // 외부에서 체인/상태를 직접 바꾸지 못하게 등록 객체와 지도를 읽기 전용으로 제공한다.
    const SpatialObject& Object(std::uint16_t id) const;
    const SquidHash& Hash() const;
    std::span<const std::uint8_t> Spots() const;
    // 실제 등록한 0단계 머리/spot을 기존 표면 탐색기에 전달한다. 이동 중 소수 좌표는 거부한다.
    SurfaceFinder Surfaces() const;
private:
    // 등록 루프의 정수 칸 범위. EffectiveGenus에서 사용하는 소수 기준점 경계와 별도다.
    struct Bounds { int left,top,right,bottom; };
    // 없는 번호와 잘못된 수명 상태를 호스트 메모리 산술로 처리하지 않고 보고한다.
    SpatialObject& Mutable(std::uint16_t id);
    // 타입/매몰/contained 조건이 원본의 발자국 갱신 경로를 선택하는지 확인한다.
    static bool WritesSpots(const SpatialObject& object);
    // 004210a0/CD 인라인: 화면 캐시는 x*16+0.5, y*11+0.5를 절삭한 short다.
    static void Coordinates(SpatialObject& object,float x,float y);
    // 오른쪽 아래 기준점의 정수 발자국을 검증한다. 등록 루프는 먼저 좌표를 절삭한다.
    static Bounds Footprint(const SpatialObject& object);
    // 건물의 좌우 변에서 기존 표면 word를 갱신하고 영역/알림 계약의 사건을 기록한다.
    void AttachedSurface(const SpatialObject& object,int x,int y,bool pop,SpatialChange& change);
    SpatialEdition edition_;
    SpatialDependencies dependencies_;
    std::map<std::uint16_t,SpatialObject> objects_;
    SquidHash hash_;
    std::vector<std::uint8_t> spots_;
};
}
