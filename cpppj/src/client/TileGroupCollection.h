// ClientBoard의 지면/다리 행별 표시 묶음을 실제 SID 풀과 0단계 공간 해시에서 복원한다.
#pragma once
#include "client/DrawSubmission.h"
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::client {
// 원본 포인터의 호스트 독립 토큰 기준이다. 객체는 base+SID*stride, 묶음은 base+행번호*8이다.
// 두 영역은 호출자가 겹치지 않게 정한다. 높이는 10.78의 행 개수 assert 상한 계산에 쓰인다.
struct TileGroupCollectionState {
    // screenHeight: 게임 화면의 픽셀 높이.
    std::int32_t screenHeight{768};
    // objectTokenBase: 객체 토큰 기본.
    // groupTokenBase: 그룹 토큰 기본.
    std::uint32_t objectTokenBase{},groupTokenBase{0x80000000U};
};
// 반환 목록은 다음 Collect/소멸까지 유효하다. 같은 객체의 재진입 수집은 허용하지 않는다.
// 화면 투영/실제 SHP 그리기는 후속이며 이 계층은 raw 월드 키와 SID 토큰만 제출한다.
class TileGroupCollection {
public:
    // 풀/해시는 객체보다 오래 살아야 한다. 화면 높이와 토큰 전체 범위를 먼저 검사한다.
    TileGroupCollection(o::SquidHash& hash,const o::SidPool& pool,TileGroupCollectionState state={});
    // 월드 경계를 +0.5→CRT 절삭→한 칸 확장→클램프한 뒤 y/x 순서로 첫 SID만 수집한다.
    // rowLimit은 각 행의 엄격한 count<limit assert다. 누적 표시 용량 제한이나 시각이 아니다.
    // 실패 전의 부분 출력/0단계 선택은 남을 수 있다. 정상 반환 때만 해시를 2단계로 바꾼다.
    std::span<const DrawGroupView> Collect(o::RenderWorldRect view,std::int32_t rowLimit,bool& backgroundCovered);
    // 현재 수집의 묶음 토큰을 kind 1 분배에 돌려준다. 지난 프레임 토큰을 보관하지 않는다.
    DrawGroupView Group(std::uint32_t token) const;
    // 객체 토큰을 실제 SID 슬롯 번호로 되돌린다. 잘못된 정렬/풀 밖 토큰은 거부한다.
    o::Sid Object(std::uint32_t token) const;
    // 수집 순서의 연속 멤버 배열이다. 각 행 내부는 공통 정렬을 적용하지 않는다.
    std::span<const o::RenderCollectionEntry> Entries() const;
private:
    // float+0.5→원본 CRT 64비트 정수의 하위 DWORD로 확장 전 셀 좌표를 만든다.
    static std::int32_t Cell(float coordinate);
    // hash_: 좌표별 객체 후보를 찾는 공간 해시.
    o::SquidHash& hash_;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const o::SidPool& pool_;
    // state_: 원본 포인터의 호스트 독립 토큰 기준이다. 객체는 base+SID*stride, 묶음은 base+행번호*8이다 (TileGroupCollectionState).
    TileGroupCollectionState state_;
    // entries_: 현재 등록하거나 읽은 항목 목록.
    std::vector<o::RenderCollectionEntry> entries_;
    // groups_: 그룹.
    std::vector<DrawGroupView> groups_;
    // counts_: 개수.
    std::array<std::int32_t,o::kWorldCells> counts_{};
};
// 공통 수집과 종류 분배의 묶음 슬롯을 같은 실제 수집기에 연결한다. collector 수명을 유지해야 한다.
void BindTileGroupCollection(DrawCollectionHooks& collection,DrawDispatchHooks& dispatch,TileGroupCollection& collector);
}
