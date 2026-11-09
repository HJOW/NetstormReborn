// 원본 VFX mapped RLE 그리기(0040229f)를 옮긴 모듈이다(cpppj `client/VFXMappedDraw.cpp`, 10.78).
// plain 그리기(`vfx-draw.js`)와 같은 pane 원점·양 끝 포함 클리핑·투명 마스크를 쓰고, 불투명 소스 픽셀의 색 번호만 256바이트 표로 치환한다.
// 배경 픽셀은 치환하지 않으며 색 0/255 도 불투명 픽셀이면 치환한다.
//  - drawMappedShape(destination, width, height, database, block, frame, x, y, pane, colors): colors 는 길이 256 의 색 번호 표이다.
//    반환값은 drawShape 와 같은 0/-1/-2/-3/-4 이다. 표 길이가 256 이 아니면 예외다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E199: ShapeToBuffer 의 소유자 색 캡처 경로로 추가)
import { drawShapeWith } from './vfx-draw.js';

// 0040229f: 소유자 색표를 적용한 mapped RLE 그리기이다. (2026-11-09 WB04-E199)
export function drawMappedShape(destination, width, height, database, block, frame, x, y, pane, colors) {
    if (!colors || colors.length !== 256) throw new RangeError('mapped RLE 색표는 256바이트여야 합니다.');
    return drawShapeWith(destination, width, height, database, block, frame, x, y, pane, colors);
}
