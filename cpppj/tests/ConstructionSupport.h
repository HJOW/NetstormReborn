// 건설 배치/확정 검사가 함께 쓰는 입력 도구다. 기계어 실행기(decomp_constructionplace_oracle.py)의 입력 생성과 같은 값을 만든다.
#pragma once
#include "RawSceneSupport.h"
#include <algorithm>

namespace netstorm::test::construction {
using namespace netstorm::test::rawscene;
// 기계어 실행기가 쓰는 타입 번호와 타입 flags1의 HP 보유 비트다.
inline constexpr std::uint32_t kTypePlain=100,kTypeBridge=82,kTypeIsland=94,kTypeNoIsland=157,kTypeAltar=164,kHasHp=0x10;
// float 입력/출력을 비트 그대로 비교한다.
inline std::uint32_t Bits(float value) { return std::bit_cast<std::uint32_t>(value); }
inline float Float(std::uint32_t bits) { return std::bit_cast<float>(bits); }
// 실행기의 frame_codes와 같은 프레임 코드 표 입력이다. 0=기본, 1=역순, 2=부호 있는 플래그, 3=품질 프레임 없음.
inline RiftTypeFrames InputFrames(std::uint32_t profile) {
    std::vector<FrameCode> codes;const std::uint8_t base=profile==2 ? 0x80 : 0;
    // 방향 글자 A~P마다 번호 0~10과 품질 플래그 프레임 둘을 둔다.
    for (int side='A';side<='P';++side) {
        // 번호 프레임은 패턴 셀이 찾는 대상이다.
        for (int number=0;number<=10;++number) codes.push_back({static_cast<std::uint8_t>(side),'P',static_cast<std::uint8_t>(number),base});
        if (profile!=3) {
            codes.push_back({static_cast<std::uint8_t>(side),'P',11,static_cast<std::uint8_t>(base|0x20)});
            codes.push_back({static_cast<std::uint8_t>(side),'P',12,static_cast<std::uint8_t>(base|0x40)});
        }
    }
    if (profile==1) std::reverse(codes.begin(),codes.end());
    codes.push_back(codes.front());
    return RiftTypeFrames(std::move(codes));
}
// 입력 슬롯을 실제 할당 없이 공급할 때만 쓰는 raw 접근이다.
inline std::span<std::uint8_t> InputRaw(SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
}
// 쉼표로 이은 입력 칸을 DWORD 목록으로 읽는다.
inline std::vector<std::uint32_t> Inputs(const std::string& text) {
    std::vector<std::uint32_t> values;
    // 칸마다 부호 없는 정수로 읽는다.
    for (const auto& field:Split(text,',')) values.push_back(Number(field));
    return values;
}
}
