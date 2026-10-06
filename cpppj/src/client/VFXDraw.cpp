// VFXDraw.cpp의 압축 해제·클리핑을 원본/CD 기계어와 대조하여 복원한다.
#include "client/VFXDraw.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// VFX 1.10 셰이프 블록의 리틀 엔디언 매직.
constexpr std::uint32_t kShapeMagic = 0x30312e31;
// 프레임의 bounds·origin·사각형 헤더 크기.
constexpr std::size_t kFrameHeaderSize = 24;
// 원본 자산에서 그림 대신 메타데이터가 들어 있는 좌표 값의 시작.
constexpr std::int32_t kSpecialCoordinate = 0x7fff0000;
// 손상된 파일 때문에 거대한 버퍼를 할당하지 않도록 정한 새 구현의 상한.
constexpr std::size_t kMaxDecodedPixels = 64 * 1024 * 1024;

// 덧셈 오버플로 없이 파일 범위를 확인한다.
void CheckRange(std::span<const std::uint8_t> bytes, std::size_t offset, std::size_t size) {
    if (offset > bytes.size() || size > bytes.size() - offset)
        throw std::runtime_error("Truncated shape database");
}
// 정렬과 호스트 엔디언에 의존하지 않고 16비트를 읽는다.
std::uint16_t U16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    CheckRange(bytes, offset, 2);
    return static_cast<std::uint16_t>(bytes[offset] | (static_cast<unsigned int>(bytes[offset+1]) << 8));
}
// 정렬과 호스트 엔디언에 의존하지 않고 32비트를 읽는다.
std::uint32_t U32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    CheckRange(bytes, offset, 4);
    std::uint32_t value = 0;
    // 낮은 바이트부터 4바이트를 모은다.
    for (std::size_t i = 0; i < 4; ++i) value |= static_cast<std::uint32_t>(bytes[offset+i]) << (i*8);
    return value;
}
// 좌표의 32비트 부호를 유지한다.
std::int32_t I32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return std::bit_cast<std::int32_t>(U32(bytes, offset));
}
}

// 아직 용도가 밝혀지지 않은 특수 레코드와 역전된 영역은 이미지로 할당하지 않는다.
bool ShapeFrame::IsSpecial() const {
    return rect.left >= kSpecialCoordinate || rect.right < rect.left || rect.bottom < rect.top;
}

// 실제 파일은 블록 헤더들이 앞에 연속으로 있고 압축 프레임들이 뒤에 놓인다.
ShapeDatabase::ShapeDatabase(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {
    std::size_t position = 0;
    std::vector<std::size_t> frameOffsets;
    // 이어지는 블록 헤더를 순서대로 읽어 타입 인덱스를 보존한다.
    while (position <= bytes_.size() && bytes_.size()-position >= 4 && U32(bytes_, position) == kShapeMagic) {
        const auto count = U32(bytes_, position+4);
        CheckRange(bytes_, position, 8);
        if (count > (bytes_.size()-position-8)/8) throw std::runtime_error("Invalid shape frame count");
        ShapeBlock block;
        block.offset = position;
        block.frames.reserve(count);
        // 테이블에 같은 오프셋이 여러 번 있어도 각각의 프레임 인덱스를 유지한다.
        for (std::size_t i = 0; i < count; ++i) {
            const auto relative = U32(bytes_, position+8+i*8);
            if (relative > bytes_.size()-position) throw std::runtime_error("Invalid shape frame offset");
            const auto offset = position+relative;
            CheckRange(bytes_, offset, kFrameHeaderSize);
            ShapeFrame frame;
            frame.offset = offset;
            frame.colorMapOffset = U32(bytes_, position+12+i*8);
            if (frame.colorMapOffset != 0 && frame.colorMapOffset >= bytes_.size()-position)
                throw std::runtime_error("Invalid shape color map offset");
            frame.bounds = {U16(bytes_, offset), U16(bytes_, offset+2)};
            frame.origin = {U16(bytes_, offset+4), U16(bytes_, offset+6)};
            frame.rect = {I32(bytes_, offset+8), I32(bytes_, offset+12), I32(bytes_, offset+16), I32(bytes_, offset+20)};
            block.frames.push_back(frame);
            frameOffsets.push_back(offset);
        }
        position += 8+static_cast<std::size_t>(count)*8;
        blocks_.push_back(std::move(block));
    }
    if (blocks_.empty()) throw std::runtime_error("Missing VFX 1.10 shape header");
    headerEnd_=position;
    std::sort(frameOffsets.begin(), frameOffsets.end());
    frameOffsets.erase(std::unique(frameOffsets.begin(), frameOffsets.end()), frameOffsets.end());
    // 블록 헤더와 겹치는 주소는 거부하고 공유 프레임은 같은 경계를 갖게 한다.
    for (auto& block : blocks_) {
        // 다음 고유 레코드 전까지만 압축 스트림을 읽도록 제한한다.
        for (auto& frame : block.frames) {
            if (frame.offset < position) throw std::runtime_error("Shape frame overlaps block headers");
            const auto next = std::upper_bound(frameOffsets.begin(), frameOffsets.end(), frame.offset);
            frame.end = next == frameOffsets.end() ? bytes_.size() : *next;
            if (frame.end-frame.offset < kFrameHeaderSize) throw std::runtime_error("Overlapping shape frames");
        }
    }
}

// 파일의 블록 순서가 곧 타입의 그래픽 순서다.
std::span<const ShapeBlock> ShapeDatabase::Blocks() const { return blocks_; }

// 런타임 frame table이 가리키는 VFX 레코드 앞의 원본 Squid 전용 헤더다.
SquidFrameMetrics ShapeDatabase::SquidMetrics(std::size_t block,std::size_t frame) const {
    const auto offset=blocks_.at(block).frames.at(frame).offset;
    // 원본 추가 헤더는 36바이트다. 순수 VFX 테스트/글꼴 파일에서 존재한다고 추측하지 않는다.
    constexpr std::size_t prefix=36;
    if (offset<headerEnd_ || offset-headerEnd_<prefix) throw std::runtime_error("Squid SHP 추가 헤더가 없습니다");
    return {std::bit_cast<float>(U32(bytes_,offset-prefix)),std::bit_cast<float>(U32(bytes_,offset-prefix+4)),
        std::bit_cast<std::int16_t>(U16(bytes_,offset-12)),std::bit_cast<std::int16_t>(U16(bytes_,offset-10)),
        std::bit_cast<std::int16_t>(U16(bytes_,offset-8)),std::bit_cast<std::int16_t>(U16(bytes_,offset-6))};
}

// 원본의 0=행 끝, 1=투명 건너뛰기, 홀수=직접 복사, 짝수=반복 복사를 구현한다.
IndexedImage ShapeDatabase::Decode(std::size_t blockIndex, std::size_t frameIndex) const {
    const auto& frame = blocks_.at(blockIndex).frames.at(frameIndex);
    if (frame.IsSpecial()) throw std::runtime_error("Special shape record has no decoded image");
    const auto width = static_cast<std::uint64_t>(static_cast<std::int64_t>(frame.rect.right)-frame.rect.left+1);
    const auto height = static_cast<std::uint64_t>(static_cast<std::int64_t>(frame.rect.bottom)-frame.rect.top+1);
    if (width > kMaxDecodedPixels || height > kMaxDecodedPixels || width*height > kMaxDecodedPixels)
        throw std::runtime_error("Shape image exceeds decoded pixel limit");
    IndexedImage image;
    image.width = static_cast<std::size_t>(width); image.height = static_cast<std::size_t>(height);
    image.indices.resize(image.width*image.height);
    image.opacity.resize(image.indices.size());
    std::size_t position = frame.offset+kFrameHeaderSize;
    // 헤더가 선언한 모든 행을 끝 토큰까지 해석한다.
    for (std::size_t row = 0; row < image.height; ++row) {
        std::size_t column = 0;
        // 픽셀 폭을 채웠더라도 행 끝 0은 반드시 소비한다.
        while (true) {
            if (position >= frame.end) throw std::runtime_error("Missing shape row terminator");
            const auto token = bytes_[position++];
            if (token == 0) break;
            if (position >= frame.end) throw std::runtime_error("Truncated shape run");
            const std::size_t count = token == 1 ? bytes_[position++] : token >> 1;
            if (count > image.width-column) throw std::runtime_error("Shape run exceeds row width");
            if (token != 1) {
                const auto inputSize = (token & 1) != 0 ? count : 1;
                if (inputSize > frame.end-position) throw std::runtime_error("Truncated shape run pixels");
                // 팔레트 번호 0을 포함하여 실제로 기록된 픽셀에만 불투명 마스크를 켠다.
                for (std::size_t i = 0; i < count; ++i) {
                    const auto output = row*image.width+column+i;
                    image.indices[output] = bytes_[position+((token & 1) != 0 ? i : 0)];
                    image.opacity[output] = 255;
                }
                position += inputSize;
            }
            column += count;
        }
    }
    return image;
}

// 원본 pane의 원점과 inclusive 사각형을 유지하며 8비트 화면에 합성한다.
int DrawShape(std::span<std::uint8_t> destination, int width, int height,
    const ShapeDatabase& database, std::size_t block, std::size_t frame,
    int x, int y, ShapeRect pane) {
    if (width <= 0 || height <= 0) return -1;
    const auto size = static_cast<std::size_t>(width)*static_cast<std::size_t>(height);
    if (size > destination.size()) throw std::invalid_argument("Shape destination is too small");
    const auto left = std::max(pane.left, 0), top = std::max(pane.top, 0);
    const auto right = std::min(pane.right, width-1), bottom = std::min(pane.bottom, height-1);
    if (right < left || bottom < top) return -2;
    if (block >= database.Blocks().size()) throw std::out_of_range("Shape block index");
    const auto& record = database.Blocks()[block].frames.at(frame);
    if (record.rect.right < record.rect.left || record.rect.bottom < record.rect.top) return -4;
    const auto originX = static_cast<std::int64_t>(x)+pane.left;
    const auto originY = static_cast<std::int64_t>(y)+pane.top;
    const auto frameLeft = originX+record.rect.left, frameTop = originY+record.rect.top;
    const auto frameRight = originX+record.rect.right, frameBottom = originY+record.rect.bottom;
    if (frameRight < left || frameBottom < top || frameLeft > right || frameTop > bottom) return -3;
    const auto image = database.Decode(block, frame);
    // 화면과 pane에 모두 들어가는 행만 처리한다.
    for (auto row = std::max<std::int64_t>(frameTop, top); row <= std::min<std::int64_t>(frameBottom, bottom); ++row) {
        // 불투명 픽셀만 써서 투명 영역의 기존 화면 값을 유지한다.
        for (auto column = std::max<std::int64_t>(frameLeft, left); column <= std::min<std::int64_t>(frameRight, right); ++column) {
            const auto input = static_cast<std::size_t>(row-frameTop)*image.width+static_cast<std::size_t>(column-frameLeft);
            if (image.opacity[input]) destination[static_cast<std::size_t>(row)*static_cast<std::size_t>(width)+static_cast<std::size_t>(column)] = image.indices[input];
        }
    }
    return 0;
}
}
