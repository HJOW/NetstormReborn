// 원본 VFX 셰이프 형식과 8비트 그리기. 창·DirectDraw 장치 관리는 별도 복원 대상이다.
#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::client {
struct ShapeRect {
    std::int32_t left{}, top{}, right{}, bottom{}; // 원본의 양 끝을 포함하는 좌표.
};
struct ShapeFrame {
    std::array<std::uint16_t, 2> bounds{}; // 원본 헤더 순서: 그림 상자 높이·폭.
    std::array<std::uint16_t, 2> origin{}; // 원본 헤더 순서: 그림 상자 안의 기준점 y·x.
    ShapeRect rect{}; // 기준점에 상대적인 실제 압축 픽셀 영역.
    std::size_t offset{}, end{}; // 파일 안의 프레임 시작과 다음 레코드 경계.
    std::uint32_t colorMapOffset{}; // 블록 상대 주소. 기본 VFX 그리기는 이 값을 사용하지 않는다.
    // 좌표 대신 메타데이터가 들어 있는 레코드인지 검사한다. 바이트는 그대로 보존한다.
    bool IsSpecial() const;
};
struct ShapeBlock {
    std::size_t offset{}; // 프레임 주소의 기준이 되는 "1.10" 헤더 위치.
    std::vector<ShapeFrame> frames;
};
struct SquidFrameMetrics {
    float cellWidth{},cellHeight{}; // SHP의 VFX 앞 36바이트: Pop의 hash level 입력.
    std::int16_t width{},height{},hotspotX{},hotspotY{}; // VFX 앞 12바이트: Renderer 경계 입력.
};
struct IndexedImage {
    std::size_t width{}, height{};
    std::vector<std::uint8_t> indices; // 0과 255도 유효한 불투명 색이다.
    std::vector<std::uint8_t> opacity; // 투명 건너뛰기는 색 번호와 분리하여 0/255로 저장한다.
};

class ShapeDatabase {
public:
    // 블록 상대 오프셋과 공유 프레임을 해석하고 파일 경계를 검증한다.
    explicit ShapeDatabase(std::vector<std::uint8_t> bytes);
    // 타입 로딩 순서에 대응하는 블록 목록을 돌려준다.
    std::span<const ShapeBlock> Blocks() const;
    // 일반 VFX 파일과 달리 _shapes.shp에 들어 있는 Squid의 추가 헤더를 읽는다.
    SquidFrameMetrics SquidMetrics(std::size_t block,std::size_t frame) const;
    // RLE를 풀되 투명 픽셀과 팔레트 번호를 각각 보존한다.
    IndexedImage Decode(std::size_t block, std::size_t frame) const;
private:
    std::vector<std::uint8_t> bytes_;
    std::vector<ShapeBlock> blocks_;
    std::size_t headerEnd_{}; // 추가 헤더가 블록 테이블을 읽는 일을 막는 경계다.
};

// 원본 00401d92 ↔ CD 00465772: pane 원점 이동·양 끝 포함 클리핑·투명 유지.
// 반환: 0 성공, -1 잘못된 화면 크기, -2 빈 pane, -3 화면 밖, -4 역전된 프레임 영역.
int DrawShape(std::span<std::uint8_t> destination, int width, int height,
    const ShapeDatabase& database, std::size_t block, std::size_t frame,
    int x, int y, ShapeRect pane);
}
