// 원본: FUN_00425c20 @ 00425c20(생성) [신뢰도 A] ↔ CD 0041fcf0, FUN_00425860 @ 00425860(다음 칸) [A] ↔ CD 0041feb0,
//       FUN_00425700 @ 00425700(셀 해석) [A] ↔ CD 0041fbb0, FUN_00425c00 @ 00425c00(방향 글자) [A] ↔ CD 00420370.
// 범위: 영역 패턴 경로. 표는 tools/cpp_canon_tables.py가 원본 실행 파일에서 뽑아 CanonDecoderTables.inc로 만든다.
// 검증: 실제 `.fort` 전체에서 영역마다 유효 칸 수가 `TerrNN`의 청크 레코드 수와 같다(tools/cpp_fort_smoke.py).
#include "o/CanonDecoder.h"
#include <stdexcept>
#include <string_view>

namespace netstorm::o {
namespace {
#include "o/CanonDecoderTables.inc"

// 빈 칸을 뜻하는 방향 글자.
constexpr char kEmptyOrientation = '.';
// 방향 글자의 범위와, 프레임을 찾을 때 변형 글자로 쓰는 표의 열(원본 `(&DAT_005315cc)[회전 * 0x40]` = 16번째 글자).
constexpr char kFirstOrientation = 'A';
constexpr char kLastOrientation = 'P';
constexpr std::size_t kVariantColumn = 15;
// 번호 글자의 기준('a')과 변형 숫자 글자의 기준('0').
constexpr int kLabelBase = 0x61;
constexpr int kVariationBase = 0x30;
// 변형 값의 허용 범위 끝(원본 assert "0 <= variation && variation <= 10").
constexpr int kMaxVariation = 10;
}

// 실행 파일에 있는 그대로의 표.
std::span<const CanonPattern> TerritoryPatterns() { return kTerritoryPatterns; }

// 원본은 회전이 홀수면 폭과 높이를 바꿔 센다. 시작 패턴 좌표는 회전별 표에서 정한다.
CanonDecoder::CanonDecoder(const RiftTypeFrames& frames, const CanonPattern& pattern, int direction, float x, float y)
    : frames_(&frames), pattern_(&pattern), rotation_(direction / 2), originX_(x), originY_(y) {
    if (rotation_ < 0 || rotation_ > 3) throw std::out_of_range("CanonDecoder rotation");
    const int width = pattern.width, height = pattern.height;
    outerCount_ = (rotation_ & 1) != 0 ? width : height;
    innerCount_ = (rotation_ & 1) != 0 ? height : width;
    patternX_ = (width - 1) * kStartX[static_cast<std::size_t>(rotation_)];
    patternY_ = (height - 1) * kStartY[static_cast<std::size_t>(rotation_)];
    Advance();
}
// 남은 칸이 있는가.
bool CanonDecoder::Valid() const { return valid_; }
// 원본 00425860.
void CanonDecoder::Advance() {
    const auto rotation = static_cast<std::size_t>(rotation_);
    // 프레임이 있는 칸을 만날 때까지 간다.
    while (true) {
        if (inner_ >= innerCount_) {
            // 한 줄이 끝났다: 패턴 좌표를 바깥 방향으로 한 칸 옮기고 다음 줄로 간다.
            int stepX = kOuterStepX[rotation];
            if (mirrored_) stepX = -stepX;
            if (kOuterStepX[rotation] != 0) patternX_ = (stepX + outerCount_ + patternX_) % outerCount_;
            if (kOuterStepY[rotation] != 0) patternY_ = (kOuterStepY[rotation] + outerCount_ + patternY_) % outerCount_;
            if (++outer_ >= outerCount_) { valid_ = false; return; }
            inner_ = 0;
        }
        frame_ = -1;
        // 셀 해석(00425700): 줄 우선 배열에서 (패턴 x, 패턴 y)의 셀.
        const auto& cell = pattern_->cells.at(static_cast<std::size_t>(pattern_->width * patternY_ + patternX_));
        const int label = cell[3];
        if (label != 0 && (label < 0x61 || label > 0x7a)) throw std::runtime_error("num==0 || ('a' <= num && num <= 'z')"); // 원본 assert.
        label_ = label - kLabelBase;
        const char orientation = static_cast<char>(cell[1]);
        const int variation = cell[0] - kVariationBase;
        if (orientation != kEmptyOrientation) {
            if (orientation < kFirstOrientation || orientation > kLastOrientation)
                throw std::runtime_error("'A' <= orientation && orientation <= 'P'"); // 원본 assert.
            if (variation < 0 || variation > kMaxVariation) throw std::runtime_error("0 <= variation && variation <= 10"); // 원본 assert.
            // 회전에 따라 방향 글자를 바꿔 그 방향·변형·번호의 프레임을 찾는다(원본 FUN_0049a940).
            const auto side = static_cast<std::uint8_t>(kRotatedSides[rotation][static_cast<std::size_t>(orientation - kFirstOrientation)]);
            const auto variant = static_cast<std::uint8_t>(kRotatedSides[rotation][kVariantColumn]);
            frame_ = frames_->FindNumber(side, variant, static_cast<std::uint8_t>(variation));
        }
        x_ = static_cast<float>(step_ * inner_) + originX_;
        y_ = static_cast<float>(outer_ * step_) + originY_;
        // 패턴 좌표를 안쪽 방향으로 한 칸 옮긴다.
        int stepX = kInnerStepX[rotation];
        if (mirrored_) stepX = -stepX;
        if (kInnerStepX[rotation] != 0) patternX_ = (stepX + innerCount_ + patternX_) % innerCount_;
        if (kInnerStepY[rotation] != 0) patternY_ = (kInnerStepY[rotation] + innerCount_ + patternY_) % innerCount_;
        ++inner_;
        if (frame_ != -1) return;
    }
}
// 현재 프레임 번호.
int CanonDecoder::Frame() const { return frame_; }
// 현재 칸의 x.
float CanonDecoder::X() const { return x_; }
// 현재 칸의 y.
float CanonDecoder::Y() const { return y_; }
// 현재 칸의 번호.
int CanonDecoder::Label() const { return label_; }
// 프레임 코드의 첫 바이트가 방향 글자다.
char CanonDecoder::Side() const { return static_cast<char>(frames_->Codes()[static_cast<std::size_t>(frame_)].side); }
}
