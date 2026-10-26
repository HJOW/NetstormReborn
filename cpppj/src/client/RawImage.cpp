// 실제 VFX 인코더의 투명 건너뛰기·literal·repeat 토큰 선택을 유지한다.
#include "client/RawImage.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client { //0046d790과004042cd의 동적 이미지 경로다.
namespace { // 인코딩 중 전역 포인터 대신 같은 호출의 지역 버퍼를 사용한다.
// 원문 16비트 좌표 필드를 낮은 바이트부터 쓴다.
void ImageWord(std::vector<std::uint8_t>& output,std::size_t offset,std::uint16_t value) {
    output[offset]=static_cast<std::uint8_t>(value);output[offset+1]=static_cast<std::uint8_t>(value>>8);
}
// 원문 DWORD의 감김도 부호 넘침 없이 같은 비트로 기록한다.
void ImageDword(std::vector<std::uint8_t>& output,std::size_t offset,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) output[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); // 비정렬 헤더도 같은 little endian이다.
}
//0040463b/004047ab: 행 시작 pair와 literal 내부 triple을 구분하고 끝의 투명 run은 버린다.
void ImageRow(std::vector<std::uint8_t>& output,std::span<const std::uint8_t> row) {
    std::size_t cursor{},pending{}; // 현재 입력 위치와 아직 출력하지 않은 투명 run 길이다.
    while (cursor<row.size()) { // 한 행의 모든 source 픽셀을 원문 선택 순서로 소비한다.
        if (!row[cursor]) { ++cursor;++pending;continue; }
        while (pending) { // 다음 불투명 run이 있을 때만255 단위 skip 토큰을 쓴다.
            const auto count=std::min<std::size_t>(pending,255);output.push_back(1);output.push_back(static_cast<std::uint8_t>(count));pending-=count;
        }
        auto end=cursor+1; // 시작 위치의 같은 색을 먼저 확인한다.
        while (end<row.size() && row[end]==row[cursor]) ++end; // 첫 pair는 길이2부터 repeat로 선택한다.
        if (end-cursor>=2) {
            while (cursor<end) { // repeat는127픽셀마다 같은 색을 다시 쓴다.
                const auto count=std::min<std::size_t>(end-cursor,127);output.push_back(static_cast<std::uint8_t>(count*2));output.push_back(row[cursor]);cursor+=count;
            }
            continue;
        }
        const auto start=cursor++; // 단일 색은 literal의 시작이며 중간 pair는 literal에 남긴다.
        while (cursor<row.size() && row[cursor]) { // 세 번 같은 색이 나오는 위치 직전까지 literal을 모은다.
            if (cursor+2<row.size() && row[cursor]==row[cursor+1] && row[cursor]==row[cursor+2]) break;
            ++cursor;
        }
        auto first=start;
        while (first<cursor) { // literal도127단위로 나누되 payload의255/0 의미를 바꾸지 않는다.
            const auto count=std::min<std::size_t>(cursor-first,127);output.push_back(static_cast<std::uint8_t>(count*2+1));
            output.insert(output.end(),row.begin()+static_cast<std::ptrdiff_t>(first),row.begin()+static_cast<std::ptrdiff_t>(first+count));first+=count;
        }
    }
    output.push_back(0); // 행 끝의 pending skip은 flush하지 않고 terminator만 쓴다.
}
//004dad20→004042cd: 실제 nonzero 사각형만 인코딩하되 상자/기준점은 crop 크기를 보존한다.
std::vector<std::uint8_t> EncodeImage(std::span<const std::uint8_t> bits,std::int32_t stride,std::int32_t width,std::int32_t height) {
    std::int32_t left=std::numeric_limits<std::int32_t>::max(),top=left,right=-left,bottom=-left; // 빈 그림의 원문 sentinel 네 값이다.
    for (std::int32_t y=0;y<height;++y) { // crop 영역의 실제 불투명 경계를 먼저 찾는다.
        for (std::int32_t x=0;x<width;++x) { // 투명 색0과 불투명255를 구별한다.
            if (bits[static_cast<std::size_t>(y)*stride+x]) { left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y); }
        }
    }
    std::vector<std::uint8_t> result(40); // 1.10의 단일 프레임 표16BYTE와 실제 프레임 헤더24BYTE다.
    ImageDword(result,0,0x30312e31);ImageDword(result,4,1);ImageDword(result,8,16);
    ImageWord(result,16,static_cast<std::uint16_t>(height-1));ImageWord(result,18,static_cast<std::uint16_t>(width-1));
    ImageWord(result,20,static_cast<std::uint16_t>(height-1));ImageWord(result,22,static_cast<std::uint16_t>(width-1));
    ImageDword(result,24,static_cast<std::uint32_t>(left)-static_cast<std::uint32_t>(width-1));
    ImageDword(result,28,static_cast<std::uint32_t>(top)-static_cast<std::uint32_t>(height-1));
    ImageDword(result,32,static_cast<std::uint32_t>(right)-static_cast<std::uint32_t>(width-1));
    ImageDword(result,36,static_cast<std::uint32_t>(bottom)-static_cast<std::uint32_t>(height-1));
    for (auto y=top;y<=bottom;++y) { // 빈 sentinel은 반복을 시작하지 않으며40BYTE shape를 그대로 유지한다.
        const auto first=static_cast<std::size_t>(y)*stride+left;ImageRow(result,bits.subspan(first,static_cast<std::size_t>(right-left+1)));
    }
    return result;
}
}
// 입력은 행 우선 원본 bitmap이고 crop은 오른쪽/아래 마지막 nonzero만 사용한다.
void RawImage::Convert(o::TerrainBitmapImage bitmap,bool crop) {
    if (!bits_.empty()) throw std::logic_error("Image imageBits == NULL 원문 진단");
    if (bitmap.width<=0 || bitmap.height<=0 || static_cast<std::uint64_t>(bitmap.width)*bitmap.height!=bitmap.pixels.size()) throw std::invalid_argument("Image bitmap 크기 오류");
    const auto stride=bitmap.width;bits_=std::move(bitmap.pixels);width_=bitmap.width;height_=bitmap.height;
    if (crop) {
        std::int32_t lastX{},lastY{}; // 전부0이면 원문과 같은 최소1×1 크기를 남긴다.
        for (std::int32_t y=0;y<height_;++y) { // 각 행의 마지막 nonzero가 crop 경계를 갱신한다.
            for (std::int32_t x=0;x<width_;++x) { // 왼쪽과 위쪽 여백은 자르지 않는다.
                if (bits_[static_cast<std::size_t>(y)*stride+x]) { lastX=std::max(lastX,x);lastY=y; }
            }
        }
        width_=lastX+1;height_=lastY+1;
    }
    shape_=EncodeImage(bits_,stride,width_,height_);shapeSize_=static_cast<std::uint32_t>(shape_.size());
    std::vector<std::uint8_t>().swap(bits_); //0046d790 마지막 free(imageBits)와 NULL 저장이다.
}
// Destroy가 먼저이므로 자기 복사는 원문대로 빈 이미지가 된다.
void RawImage::CopyFrom(const RawImage& source) {
    Destroy();width_=source.width_;height_=source.height_;
    if (!source.bits_.empty()) bits_=source.bits_;
    if (!source.shape_.empty()) { shapeSize_=source.shapeSize_;if (shapeSize_) shape_=source.shape_; }
}
//0046d8a0의 idempotent 해제이며 shapeSize_를0으로 쓰지 않는다.
void RawImage::Destroy() { std::vector<std::uint8_t>().swap(bits_);std::vector<std::uint8_t>().swap(shape_);height_=0;width_=0; }
std::int32_t RawImage::Width() const { return width_; } // crop 전후 원문+8의 값이다.
std::int32_t RawImage::Height() const { return height_; } // crop 전후 원문+c의 값이다.
std::uint32_t RawImage::ShapeSize() const { return shapeSize_; } // 소멸로 지워지지 않는 원문+10이다.
std::span<const std::uint8_t> RawImage::Shape() const { return shape_; } // 전용 MetaImage가 실제 RLE를 빌린다.
std::span<const std::uint8_t> RawImage::Bits() const { return bits_; } // 성공한 Convert 뒤에는 항상 빈 버퍼다.
}
