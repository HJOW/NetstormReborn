// 원본: FUN_00425c20 @ 00425c20(생성) [신뢰도 A] ↔ CD 0041fcf0, FUN_00425860 @ 00425860(다음 칸) [A] ↔ CD 0041feb0,
//       FUN_00425700 @ 00425700(셀 해석) [A] ↔ CD 0041fbb0, FUN_00425c00 @ 00425c00(방향 글자) [A] ↔ CD 00420370.
// 범위: 영역·다리 패턴과 비패턴 한 칸 경로/정수 범위. 패턴 표는 tools/cpp_canon_tables.py가 원본에서 뽑는다.
// 검증: 실제 `.fort` 전체에서 영역마다 유효 칸 수가 `TerrNN`의 청크 레코드 수와 같다(tools/cpp_fort_smoke.py).
#include "o/CanonDecoder.h"
#include <stdexcept>
#include <string_view>
#include <cmath>
#include <limits>

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
// 다리도 같은 셀 반복자를 사용하며 첫 정수만 추첨에 사용한다.
std::span<const CanonPattern> BridgePatterns(OriginalEdition edition) {
    return edition == OriginalEdition::Cd1072 ? std::span<const CanonPattern>(kCdBridgePatterns) : kBridgePatterns;
}
// 원본은 r=0에서 가중치 0인 첫 항목을 고른다. 음수 입력도 signed 나머지 그대로 첫 항목이다.
std::size_t SelectBridgePattern(std::int32_t random, OriginalEdition edition) {
    const auto patterns = BridgePatterns(edition);
    std::int32_t total = 0;
    // 원본 26개 항목의 가중치 합을 구한다.
    for (const auto& pattern : patterns) total += pattern.first;
    const auto value = random % total;
    std::int32_t accumulated = 0;
    // 누적 경계는 엄격한 <가 아닌 <=다.
    for (std::size_t i = 0; i < patterns.size(); ++i) {
        accumulated += patterns[i].first;
        if (value <= accumulated) return i;
    }
    throw std::logic_error("Bridge pattern weights");
}

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
// 패턴 없는 경로의 폭/높이는 타입 발자국과 별개로 모두 1이다.
CanonDecoder::CanonDecoder(const RiftTypeFrames& frames,int defaultFrame,int argument,int direction,float x,float y,
    bool explicitFrame,OriginalEdition edition):frames_(&frames),pattern_(nullptr),rotation_(direction/2),
    originX_(x),originY_(y),outerCount_(1),innerCount_(1),defaultFrame_(defaultFrame),argument_(argument),explicitFrame_(explicitFrame) {
    if (rotation_<0 || rotation_>3 || (edition==OriginalEdition::Cd1072 && (direction&1))) throw std::out_of_range("패턴 없는 CanonDecoder 회전 오류");
    if (!std::isfinite(x) || !std::isfinite(y)) throw std::invalid_argument("패턴 없는 CanonDecoder 좌표 오류");
    Advance();
}
// 원본은 유효 칸이 없어도 현재 x/y와 순회 개수로 범위를 계산한다.
std::array<int,4> CanonDecoder::Bounds(int footX,int footY) const {
    const double x=std::trunc(x_),y=std::trunc(y_);
    const double left=x-static_cast<double>(innerCount_)*footX,top=y-static_cast<double>(outerCount_)*footY;
    // 원본 int 산술/CRT 범위를 벗어나는 입력은 별도 C++ 계약으로 진단한다.
    for (const double value:{left,top,x,y}) if (!std::isfinite(value) || value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max())
        throw std::out_of_range("CanonDecoder 범위 산술 오류");
    return {static_cast<int>(left),static_cast<int>(top),static_cast<int>(x),static_cast<int>(y)};
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
        if (!pattern_) {
            frame_=explicitFrame_ ? argument_ : defaultFrame_;
            if (explicitFrame_ && (frame_<0 || static_cast<std::size_t>(frame_)>=frames_->Codes().size())) throw std::out_of_range("CanonDecoder 명시 프레임 오류");
        } else {
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
