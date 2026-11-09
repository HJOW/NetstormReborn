// 원본 VFX 셰이프의 8비트 평면 그리기(00401d92)를 옮긴 모듈이다(cpppj `client/VFXDraw.cpp` 의 DrawShape, 10.78).
// pane 원점 이동·양 끝을 포함하는 pane/화면 클리핑·투명 건너뛰기 유지를 보존하며, 색 번호 0/255 도 불투명 픽셀이면 그대로 쓴다.
//  - drawShape(destination, width, height, database, block, frame, x, y, pane): destination 은 width*height 이상의 Uint8Array(행 우선),
//    database 는 `assets/shapes.js` 의 ShapeDatabase, pane 은 { left, top, right, bottom }(양 끝 포함)이다.
//    반환은 원본과 같은 0 성공·-1 잘못된 화면 크기·-2 빈 pane·-3 화면 밖·-4 역전된 프레임 영역이다. 버퍼가 작거나 블록/프레임 번호가 범위 밖이면 예외다.
//  - drawShapeWith(…, map): 같은 몸체에서 불투명 소스 색만 map(색 번호 → 출력 색)으로 바꾸는 공유 경로이다. mapped 그리기(`vfx-mapped-draw.js`)가 쓴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E199: ShapeToBuffer 캡처의 선행 부품으로 추가)

// 원본 반환값이다. (2026-11-09 WB04-E199)
export const DRAW_OK = 0;
export const DRAW_BAD_SIZE = -1;
export const DRAW_EMPTY_PANE = -2;
export const DRAW_OUTSIDE = -3;
export const DRAW_INVERTED_FRAME = -4;

// plain/mapped 공통 몸체이다. map 이 null 이면 소스 색을 그대로 쓴다. 투명 픽셀은 기존 화면 값을 유지한다. (2026-11-09 WB04-E199)
export function drawShapeWith(destination, width, height, database, block, frame, x, y, pane, map) {
    if (width <= 0 || height <= 0) return DRAW_BAD_SIZE;
    if (width * height > destination.length) throw new RangeError('셰이프 출력 버퍼가 작습니다.');
    const left = Math.max(pane.left, 0);
    const top = Math.max(pane.top, 0);
    const right = Math.min(pane.right, width - 1);
    const bottom = Math.min(pane.bottom, height - 1);
    if (right < left || bottom < top) return DRAW_EMPTY_PANE;
    if (!Number.isInteger(block) || block < 0 || block >= database.blocks.length) throw new RangeError('셰이프 블록 범위 밖입니다.');
    const record = database.frame(block, frame);
    if (record.rect.right < record.rect.left || record.rect.bottom < record.rect.top) return DRAW_INVERTED_FRAME;
    // 원본은 64비트 넓이로 원점을 더하므로 정수 범위 안의 일반 수로 계산한다.
    const originX = x + pane.left;
    const originY = y + pane.top;
    const frameLeft = originX + record.rect.left;
    const frameTop = originY + record.rect.top;
    const frameRight = originX + record.rect.right;
    const frameBottom = originY + record.rect.bottom;
    if (frameRight < left || frameBottom < top || frameLeft > right || frameTop > bottom) return DRAW_OUTSIDE;
    const image = database.decode(block, frame);
    const rowEnd = Math.min(frameBottom, bottom);
    const columnStart = Math.max(frameLeft, left);
    const columnEnd = Math.min(frameRight, right);
    // 화면과 pane 에 모두 들어가는 행만 처리한다.
    for (let row = Math.max(frameTop, top); row <= rowEnd; row += 1) {
        const inputRow = (row - frameTop) * image.width - frameLeft;
        const outputRow = row * width;
        // 불투명 픽셀만 써서 투명 영역의 기존 화면 값을 유지한다.
        for (let column = columnStart; column <= columnEnd; column += 1) {
            const input = inputRow + column;
            if (image.opacity[input]) destination[outputRow + column] = map ? map[image.indices[input]] : image.indices[input];
        }
    }
    return DRAW_OK;
}

// 00401d92: plain RLE 그리기이다. (2026-11-09 WB04-E199)
export function drawShape(destination, width, height, database, block, frame, x, y, pane) {
    return drawShapeWith(destination, width, height, database, block, frame, x, y, pane, null);
}
