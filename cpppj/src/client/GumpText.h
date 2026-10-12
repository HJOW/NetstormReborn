// 기본 Gump의 글자 측정을 원본 비트맵 캐시와 슬롯 높이에 연결한다.
#pragma once
#include "client/BitmapFont.h"
#include "client/GumpVisual.h"
#include "o/RiftType.h"

namespace netstorm::client {
// 004a3df0/CD 00425f90의 폭 표 합산과 출력 쓰기 순서다. 음수 길이는 -1(NUL 탐색) 외에는 0글자다.
// 명시 길이는 NUL도 한 바이트로 측정한다. -1에는 전달 범위 안의 NUL이 필요하다.
// width/height가 같아도 원본 쓰기 순서를 유지한다. 그림자 덧셈은 패치의 현재 출력/CD의 저장한 합계를 구분한다.
// widths/fontHeight/context는 출력 참조와 겹치지 않는 안정된 입력으로 전달한다.
void MeasureGumpText(std::span<const char> bytes,std::span<const std::int32_t,256> widths,
    std::int32_t fontHeight,const GumpTextContext& context,std::int32_t& width,std::int32_t& height,
    std::int32_t length=-1,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
// 32바이트 기본 Gump 문맥을 호스트 필드로 읽는다. 마지막 두 DWORD의 signed 비트를 보존한다.
GumpTextContext DecodeGumpTextContext(const std::array<std::uint32_t,8>& words);
// 현재 글꼴 저장소의 측정 폭과 원본이 별도 전역에 저장하는 슬롯 높이를 공급한다.
// 누락 캐시는 기존 FontStore의 메모리 GDI 생성 경로를 사용하며 원본 파일을 쓰지 않는다.
class GumpText {
public:
    // 파일 시스템/글꼴 저장소는 측정기보다 오래 살아야 한다. 기준 판본은 10.78이다.
    explicit GumpText(FontStore& fonts,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 잘못된 스타일은 폭에 normal 슬롯 0을 쓰지만 높이는 요청 슬롯을 유지한다.
    // 비어 있는 슬롯/스타일 표는 폭 0이며, 범위 밖 슬롯이나 부족한 문자열 범위는 예외로 거부한다.
    void Measure(std::span<const char> bytes,const GumpTextContext& context,std::int32_t& width,
        std::int32_t& height,std::int32_t length=-1) const;
private:
    FontStore& fonts_;
    o::OriginalEdition edition_;
};
// 실제 FontStore 측정기를 돌 버튼의 FitToText/SetLabel 경계에 연결한다. 측정기는 훅보다 오래 살아야 한다.
struct StoneButtonGumpHooks;
void BindStoneButtonText(StoneButtonGumpHooks& hooks,const GumpText& text);
}
