// Squid의 저장 필드와 이동 상태 일부. 원본 200바이트 배열/생성자 표/SID 세대 해시는 아직 복원하지 않았다.
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
    // 프레임 시간차만큼 움직인다. 한 걸음의 대각선 길이는 sqrt(2)다.
    bool Advance(double seconds,GroundGrid& grid,std::uint16_t allies);
    // 다음 칸으로 향하는 방향 번호(A=북)를 갱신한다.
    void Face(CellPoint nextCell);
};
}
