// 원본 Screen.cpp 팔레트 로더(004a4850)가 채우는 기본/표시/날씨 색 번호 표를 옮긴 모듈이다(cpppj `client/PaletteColors.cpp/.h`, 10.78 전용).
//  - buildPaletteColorTable(logical): 실제 논리 팔레트(R | G<<8 | B<<16 DWORD 256개)의 색 검색(assets/palette.js 의 findPaletteColor: 채널 차이 제곱 합, float32 최소 거리, 동률은 앞 번호)으로
//    원본 호출 순서의 58개 목표 RGB 의 색 번호를 구한다. 같은 RGB 를 서로 다른 전역에 적는 호출(17,66,60 / 255,255,255 / 128,128,128)도 원본 순서대로 별도 항목으로 남긴다.
//    basic 은 앞 9개(검정·빨강·초록·파랑·자홍·노랑·주황·흰색·회색)의 색 번호이고, weather 는 새 검색 없이 [노랑, 파랑, 빨강, 해 원소의 갈색(named[9])] 번호를 복사한다(바람·비·천둥·해 순서).
//    반환: { basic: 9개 숫자, weather: 4개 숫자, named: [{ red, green, blue, index }…58개] } (동결).
// 색의 용도가 확정되지 않은 항목은 목표 RGB 로 식별한다. 10.72(CD)는 추가 여덟 색(12~19번째)을 제외한 50개이나 웹은 10.78 만 옮긴다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E376: 5-A 팔레트 계열 이식으로 추가, 기준선 뒤 cpppj 변경 없음)
import { findPaletteColor } from '../assets/palette.js';

// 10.78 의 색 검색 호출 수(중복 RGB 포함)와 기본 색 개수이다. (2026-11-13 WB04-E376)
export const PATCH_PALETTE_COLOR_COUNT = 58;
const BASIC_COUNT = 9;

// 004a4850 의 58개 검색 인자 순서이다. 주석 주소는 결과를 저장하는 10.78 전역이며 용도 미확정 항목의 추적에 쓴다. (2026-11-13 WB04-E376)
const NAMED_RGB = Object.freeze([
    [0, 0, 0], // 기본 검정: 005a4058.
    [255, 22, 22], // 기본 빨강: 005b5e58.
    [22, 255, 22], // 기본 초록: 005acd20.
    [22, 22, 255], // 기본 파랑: 005ad128.
    [255, 22, 255], // 기본 자홍: 005b5da0.
    [255, 255, 22], // 기본 노랑: 0059afd8.
    [240, 61, 0], // 기본 주황: 005bec34.
    [255, 255, 255], // 기본 흰색: 005bec2c.
    [128, 128, 128], // 기본 회색: 005bec3c.
    [181, 140, 111], // 해 원소의 갈색: 005acd24.
    [95, 188, 92], // 목표 RGB 결과: 005b5dc4.
    [151, 215, 210], // 목표 RGB 결과: 005a4060.
    [0, 255, 102], // 10.78 추가 목표 RGB: 0059af80.
    [255, 102, 204], // 10.78 추가 목표 RGB: 005b5e6c.
    [255, 153, 0], // 10.78 추가 목표 RGB: 005b5dcc.
    [51, 255, 102], // 10.78 추가 목표 RGB: 0059afd4.
    [255, 0, 153], // 10.78 추가 목표 RGB: 005b5dd0.
    [99, 66, 255], // 10.78 추가 목표 RGB: 005b5e54.
    [0, 128, 0], // 10.78 추가 목표 RGB: 005acd1c.
    [184, 147, 20], // 10.78 추가 목표 RGB: 0059afd0.
    [233, 214, 186], // 목표 RGB 결과: 005a4064.
    [200, 175, 144], // 목표 RGB 결과: 005b5e5c.
    [249, 233, 145], // 목표 RGB 결과: 005bec30.
    [219, 213, 203], // 목표 RGB 결과: 005b5dc8.
    [215, 207, 119], // 목표 RGB 결과: 0059b3e0.
    [45, 62, 32], // 목표 RGB 결과: 005b5df0.
    [17, 66, 60], // 목표 RGB 결과: 005b5e68.
    [17, 66, 60], // 같은 RGB 의 별도 전역: 005b5dbc.
    [108, 57, 54], // 목표 RGB 결과: 0059b3e4.
    [74, 58, 56], // 목표 RGB 결과: 005b5da4.
    [114, 106, 103], // 목표 RGB 결과: 005b5e60.
    [250, 248, 238], // 목표 RGB 결과: 005b5dd4.
    [84, 80, 81], // 목표 RGB 결과: 005b5dc0.
    [206, 193, 178], // 목표 RGB 결과: 005b5dd8.
    [255, 255, 255], // 기본 흰색과 같은 RGB 의 별도 전역: 005a405c.
    [128, 128, 128], // 기본 회색과 같은 RGB 의 별도 전역: 005b5e64.
    [47, 24, 17], // 목표 RGB 결과: 005c78d0.
    [55, 51, 54], // 돌 버튼 부모의 외곽선: 005bec38(CD 0055287c).
    [88, 121, 67], // 목표 RGB 결과: 005b5e44.
    [111, 146, 48], // 목표 RGB 결과: 005b5e48.
    [153, 179, 55], // 목표 RGB 결과: 005b5e4c.
    [216, 205, 57], // 목표 RGB 결과: 005b5e50.
    [54, 100, 130], // 목표 RGB 결과: 005accfc.
    [191, 75, 71], // 목표 RGB 결과: 005acd00.
    [204, 193, 180], // 목표 RGB 결과: 005acd04.
    [91, 129, 50], // 목표 RGB 결과: 005acd08.
    [127, 113, 182], // 목표 RGB 결과: 005acd0c.
    [226, 185, 82], // 목표 RGB 결과: 005acd10.
    [53, 122, 112], // 목표 RGB 결과: 005acd14.
    [238, 126, 40], // 목표 RGB 결과: 005acd18.
    [146, 168, 186], // 목표 RGB 결과: 005c78b0.
    [223, 133, 116], // 목표 RGB 결과: 005c78b4.
    [249, 247, 239], // 목표 RGB 결과: 005c78b8.
    [118, 173, 105], // 목표 RGB 결과: 005c78bc.
    [178, 172, 215], // 목표 RGB 결과: 005c78c0.
    [223, 206, 108], // 목표 RGB 결과: 005c78c4.
    [129, 192, 185], // 목표 RGB 결과: 005c78c8.
    [238, 144, 41], // 목표 RGB 결과: 005c78cc.
]);

// 논리 팔레트로 기본 9색·날씨 4색·전체 58색의 색 번호 표를 만든다. 사용: const { basic, weather, named } = buildPaletteColorTable(screenPalette.logical). (2026-11-13 WB04-E376)
export function buildPaletteColorTable(logical) {
    if (!logical || logical.length !== 256) throw new RangeError('팔레트 색 표는 논리 팔레트 256색이어야 합니다.');
    const basic = [];
    const named = [];
    // 원본 호출 순서를 유지해 각 목표 RGB 의 색 번호를 구한다.
    for (let index = 0; index < NAMED_RGB.length; index += 1) {
        const [red, green, blue] = NAMED_RGB[index];
        const color = findPaletteColor(logical, red, green, blue);
        named.push(Object.freeze({ red, green, blue, index: color }));
        if (index < BASIC_COUNT) basic.push(color);
    }
    return Object.freeze({ basic: Object.freeze(basic), weather: Object.freeze([basic[5], basic[3], basic[1], named[9].index]), named: Object.freeze(named) });
}
