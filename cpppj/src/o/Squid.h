// Squid의 저장 필드와 이동 상태 일부. 원본 raw 풀/base 생성은 SidPool/SquidFactory에 별도로 복원했으며 월드/파생 수명 연결은 남았다.
#pragma once
#include "o/TerrainBuilder.h"
#include "o/Template.h"

namespace netstorm::o {
// 현재 월드 수명 안에서만 유효한 런타임 번호. 원본 SID 비트 형식과 혼동하지 않는다.
using SquidId = std::uint32_t;
// 현재 클라이언트 월드 객체의 배치·HP·이동·애니메이션 상태이다.
struct Squid {
    // id: 대상을 구별하는 식별 번호.
    SquidId id{};
    // type: 자산 표에서 객체 종류를 찾는 타입 번호.
    // owner: 객체를 소유한 플레이어 번호.
    // territory: 객체가 속한 맵 영역 번호.
    int type{}, owner{}, territory{-1};
    // x: 현재 월드 X 좌표(칸).
    // y: 현재 월드 Y 좌표(칸).
    // hitPoints: 현재 HP.
    // maxHitPoints: 최대 HP.
    // speed: 한 초에 이동하는 월드 칸 수.
    double x{},y{},hitPoints{},maxHitPoints{},speed{};
    // frame: 현재 그림 프레임 번호.
    // initialFrame: 초기화 때 선택한 프레임 번호.
    // next: 이동 경로에서 다음에 도착할 칸의 인덱스.
    std::size_t frame{},initialFrame{},next{};
    // quantity: 객체에 저장된 수량.
    std::int32_t quantity{};
    // factoryState: 팩토리의 저장 상태.
    std::optional<std::uint8_t> factoryState;
    // contents: 객체 안에 보관된 내용물 목록과 읽기 완료 상태.
    FortContentList contents;
    // mobile: 이동 가능한 객체인지.
    // selectable: 선택 입력의 대상이 될 수 있는지.
    // visible: 표시 대상 또는 표시 허용 상태.
    // walking: 이동 애니메이션을 진행 중인지.
    bool mobile{},selectable{},visible{},walking{};
    // heading: 이동 방향 번호.
    int heading{-1};
    // cell: 이동·점유 판정에 사용할 현재 정수 칸 좌표.
    // goal: 월드 격자의 정수 X·Y 칸 좌표 (CellPoint).
    CellPoint cell{},goal{};
    // route: 이동할 경로의 칸 목록.
    std::vector<CellPoint> route;
    // progress: 현재 경로 구간에서 이동한 거리(칸).
    // animation: 이동 애니메이션 프레임 선택에 누적하는 시간(초).
    double progress{},animation{};
    // 원본 004afd30 ↔ CD 004acfc0: 타입 genus에 발자국 안쪽 비트 8을 보정한다.
    // 기준점은 +0.9999 후 0 방향으로 절삭한다. 발자국/지붕 경계만 계산하며 spot 지도에 쓰지 않는다.
    static std::uint32_t EffectiveGenus(std::uint32_t flags2,float x,float y,int width,int height,int cellX,int cellY);
    // 프레임 시간차만큼 움직인다. 한 걸음의 대각선 길이는 sqrt(2)다.
    bool Advance(double seconds,GroundGrid& grid,std::uint16_t allies);
    // 다음 칸으로 향하는 방향 번호(A=북)를 갱신한다.
    void Face(CellPoint nextCell);
};
}
