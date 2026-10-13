// 원본 VFX 셰이프 형식과 8비트 그리기. 창·DirectDraw 장치 관리는 별도 복원 대상이다.
#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::client {
// 셰이프 그림의 사각형 경계이다.
struct ShapeRect {
    std::int32_t left{}, top{}, right{}, bottom{}; // 원본의 양 끝을 포함하는 좌표.
};
// 셰이프 프레임의 위치·크기와 픽셀 데이터 정보이다.
struct ShapeFrame {
    std::array<std::uint16_t, 2> bounds{}; // 원본 헤더 순서: 그림 상자 높이·폭.
    std::array<std::uint16_t, 2> origin{}; // 원본 헤더 순서: 그림 상자 안의 기준점 y·x.
    ShapeRect rect{}; // 기준점에 상대적인 실제 압축 픽셀 영역.
    std::size_t offset{}, end{}; // 파일 안의 프레임 시작과 다음 레코드 경계.
    std::uint32_t colorMapOffset{}; // 블록 상대 주소. 기본 VFX 그리기는 이 값을 사용하지 않는다.
    // 좌표 대신 메타데이터가 들어 있는 레코드인지 검사한다. 바이트는 그대로 보존한다.
    bool IsSpecial() const;
};
// 같은 셰이프에 속한 프레임과 부가 정보의 묶음이다.
struct ShapeBlock {
    std::size_t offset{}; // 프레임 주소의 기준이 되는 "1.10" 헤더 위치.
    // frames: 타입별 프레임 자료.
    std::vector<ShapeFrame> frames;
};
// 객체 발자국·출력 크기·핫스폿을 계산할 프레임 치수이다.
struct SquidFrameMetrics {
    float cellWidth{},cellHeight{}; // SHP의 VFX 앞 36바이트: Pop의 hash level 입력.
    std::int16_t width{},height{},hotspotX{},hotspotY{}; // VFX 앞 12바이트: Renderer 경계 입력.
    float cellHotspotX{},cellHotspotY{}; // VFX 앞 28/24바이트: 월드 표시 수집의 float 기준점.
};
// 팔레트 인덱스 픽셀로 구성한 그림이다.
struct IndexedImage {
    // width: 가로 크기.
    // height: 세로 크기.
    std::size_t width{}, height{};
    std::vector<std::uint8_t> indices; // 0과 255도 유효한 불투명 색이다.
    std::vector<std::uint8_t> opacity; // 투명 건너뛰기는 색 번호와 분리하여 0/255로 저장한다.
};

// 셰이프 블록과 프레임을 읽어 그림으로 변환하는 저장소이다.
class ShapeDatabase {
public:
    // 블록 상대 오프셋과 공유 프레임을 해석하고 파일 경계를 검증한다.
    explicit ShapeDatabase(std::vector<std::uint8_t> bytes);
    // 타입 로딩 순서에 대응하는 블록 목록을 돌려준다.
    std::span<const ShapeBlock> Blocks() const;
    // 일반 VFX 파일과 달리 _shapes.shp에 들어 있는 Squid의 추가 헤더를 읽는다.
    SquidFrameMetrics SquidMetrics(std::size_t block,std::size_t frame) const;
    // VFX 프레임 앞의 원본 36바이트를 복사한다. Markup의 두 추가 WORD도 정규화 없이 보존한다.
    // 추가 헤더가 없는 순수 VFX/글꼴 프레임은 기존 메타데이터 조회와 같은 범위 오류로 거부한다.
    std::array<std::uint8_t,36> SquidHeader(std::size_t block,std::size_t frame) const;
    // RLE를 풀되 투명 픽셀과 팔레트 번호를 각각 보존한다.
    IndexedImage Decode(std::size_t block, std::size_t frame) const;
private:
    // bytes_: 필드 접근이나 파일 읽기에 사용하는 원시 바이트.
    std::vector<std::uint8_t> bytes_;
    // blocks_: 블록.
    std::vector<ShapeBlock> blocks_;
    std::size_t headerEnd_{}; // 추가 헤더가 블록 테이블을 읽는 일을 막는 경계다.
};

// 원본 00401d92 ↔ CD 00465772: pane 원점 이동·양 끝 포함 클리핑·투명 유지.
// 반환: 0 성공, -1 잘못된 화면 크기, -2 빈 pane, -3 화면 밖, -4 역전된 프레임 영역.
int DrawShape(std::span<std::uint8_t> destination, int width, int height,
    const ShapeDatabase& database, std::size_t block, std::size_t frame,
    int x, int y, ShapeRect pane);
}
