// Squid의 저장 필드와 이동 상태 일부. 원본 raw 풀/base 생성은 SidPool/SquidFactory에 별도로 복원했으며 월드/파생 수명 연결은 남았다.
#pragma once
#include "o/TerrainBuilder.h"
#include "o/Template.h"

namespace netstorm::o {
// 현재 월드 수명 안에서만 유효한 런타임 번호. 원본 SID 비트 형식과 혼동하지 않는다.
using SquidId = std::uint32_t;
struct Squid {
    SquidId id{};
    int type{}, owner{}, territory{-1};
    double x{},y{},hitPoints{},maxHitPoints{},speed{};
    std::size_t frame{},initialFrame{},next{};
    std::int32_t quantity{};
    std::optional<std::uint8_t> factoryState;
    FortContentList contents;
    bool mobile{},selectable{},visible{},walking{};
    int heading{-1};
    CellPoint cell{},goal{};
    std::vector<CellPoint> route;
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
