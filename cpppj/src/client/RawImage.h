// 원본 Image의 bitmap 소비·VFX 변환·깊은 복사·소멸 수명을 복원한다.
#pragma once
#include "o/RawTerrainBitmap.h"

namespace netstorm::client { // 동적 이미지는 자산 TYPE 그림과 별도 수명으로 소유한다.
class RawImage {
public:
    void Convert(o::TerrainBitmapImage bitmap,bool crop); //0046d790: 입력을 소비하고 오른쪽/아래 여백만 줄여 실제 VFX shape를 만든다.
    void CopyFrom(const RawImage& source); //0046d900: 먼저 현재 버퍼를 파괴한 뒤 source를 깊게 복사한다. 자기 복사도 이 순서다.
    void Destroy(); //0046d8a0: 두 버퍼/폭/높이를 지우며 shapeSize는 원문대로 보존한다. 중복 호출이 가능하다.
    std::int32_t Width() const; // 변환 뒤 crop0/1이 확정한 실제 폭이다.
    std::int32_t Height() const; // 변환 뒤 crop0/1이 확정한 실제 높이다.
    std::uint32_t ShapeSize() const; // 마지막 shape byte 수이며 소멸 후에도 남는다.
    std::span<const std::uint8_t> Shape() const; // 원본 1.10 블록/프레임/RLE 전체를 빌린다.
    std::span<const std::uint8_t> Bits() const; // 변환 성공 후에는 소비되어 빈 bitmap 버퍼다.
private:
    std::vector<std::uint8_t> bits_,shape_; // 원본 +0 bitmap과+4 shape의 독립 소유 버퍼다.
    std::int32_t width_{},height_{}; // 원본 +8/+c의 crop 전/후 크기다.
    std::uint32_t shapeSize_{}; // 원본 +10은 Destroy가 쓰지 않는 DWORD다.
};
}
