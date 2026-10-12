// 실제 36바이트 SHP 추가 헤더/두 레이어와 독립 손계산 질감을 만드는 공용 검사 입력이다.
#pragma once
#include "GumpRawSupport.h"

namespace netstorm::test::gumpbackground {
using namespace netstorm::test::gumpraw;
// 3×3 주기 질감과 두 번째 레이어의 투명 중앙을 가진 독립 데이터베이스다. 원본 파일에 쓰지 않는다.
inline ShapeDatabase Shapes() {
    std::vector<std::uint8_t> raw(24);Put(raw,0,0x30312e31);Put(raw,4,2);
    // 두 물리 프레임은 같은 논리 프레임의 레이어에 대응하며 메타 크기는 서로 다르다.
    for (std::size_t frame=0;frame<2;++frame) {
        const auto prefix=raw.size();raw.resize(prefix+60);
        Put(raw,8+frame*8,static_cast<std::uint32_t>(prefix+36));
        Put(raw,prefix+24,frame ? 5U : 3U,2);Put(raw,prefix+26,frame ? 4U : 3U,2);
        Put(raw,prefix+36,2,2);Put(raw,prefix+38,2,2);Put(raw,prefix+52,2);Put(raw,prefix+56,2);
        // 실제 출력 질감은 3×3이다. 반복 주기는 이 이미지 크기 대신 위 메타데이터를 따른다.
        constexpr std::array<std::uint8_t,9> colors{1,2,1,3,4,3,1,2,1};
        // 각 행에 독립 RLE 리터럴 run을 쓴다. 두 번째 레이어의 중앙은 투명 skip이다.
        for (std::size_t y=0;y<3;++y) {
            // 원본의 1=skip/count와 2=한 픽셀 반복 run을 구분한다.
            for (std::size_t x=0;x<3;++x) {
                if (frame && x==1 && y==1) raw.insert(raw.end(),{1,1});
                else raw.insert(raw.end(),{2,static_cast<std::uint8_t>(colors[y*3+x]+frame*4)});
            }
            raw.push_back(0);
        }
    }
    return ShapeDatabase(std::move(raw));
}
// 밝음/어두움 표는 예상 색을 손으로 확인할 수 있게 각 소스 번호에 10/20을 더한다.
inline GumpShadeMaps Maps() {
    GumpShadeMaps maps;
    // 256바이트 표의 BYTE 감김은 실제 팔레트 치환과 같다.
    for (std::size_t i=0;i<256;++i) { maps.bright[i]=static_cast<std::uint8_t>(i+10);maps.dark[i]=static_cast<std::uint8_t>(i+20); }
    return maps;
}
}
