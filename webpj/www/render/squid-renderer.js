// raw 공통 표시 계산(Squid 표시 출력 대상)과 실제 렌더러를 잇는 어댑터와, 원본 로딩 순서의 표시 그림 헤더 표를 만드는 모듈이다(cpppj `client/SquidRenderer.cpp/.h`, 10.78).
//  - new SquidRenderer(renderer): renderer 는 { fullRedrawPending(), invalidate(rect, flags) } 를 가진 실제 렌더러(변경 표 소유자)이다.
//    suppressed() 는 원본 displayOff 전역과 같은 전체 갱신 대기 여부, invalidate(rect, flags) 는 사각형을 viewport 로 다시 자르지 않고 그대로 전달한다.
//    `core/squid-display.js` 등 Squid 표시 계산의 sink 로 쓴다. 렌더러는 이 어댑터보다 오래 살아야 한다.
//  - squidRendererShapes(table, database): `assets/type-table.js` 의 TypeTable 과 `assets/shapes.js` 의 ShapeDatabase 로 188칸(타입 번호 인덱스) 표시 그림 헤더 표를 만든다.
//    각 칸은 { frameCount(클러스터 수), loaded, frames:[{ width, height, hotspotX, hotspotY, cellWidth, cellHeight, cellHotspotX, cellHotspotY, muzzleX, muzzleY, walkStepX, walkStepY }] } 이며
//    frames 는 그림자/abstract 레이어와 공유 프레임을 포함한 물리 프레임 순서이다. 70 미만 내장 타입과 186·187 은 빈 칸(frameCount 0·loaded false)이다.
//    정의를 읽지 못한 자산 타입이 있으면 원본 자산 적재 실패처럼 거부한다(빈 그림으로 대체하지 않는다).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E199: ShapeToBuffer·Burning 캡처의 그림 헤더 선행 부품으로 추가)
import { FIRST_ASSET_TYPE_NUMBER, PATCH_TYPE_COUNT, TYPE_LOAD_ORDER } from '../assets/type-table.js';

// Squid 표시 요청을 실제 렌더러 변경 표에 전달하는 출력 대상이다. (2026-11-09 WB04-E199)
export class SquidRenderer {
    // 실제 변경 표의 소유자이다.
    #renderer;

    // 두 메서드가 없는 렌더러는 거부한다. (2026-11-09 WB04-E199)
    constructor(renderer) {
        if (typeof renderer?.fullRedrawPending !== 'function' || typeof renderer?.invalidate !== 'function') throw new TypeError('SquidRenderer 실제 렌더러 미연결');
        this.#renderer = renderer;
    }

    // 첫 화면/전체 변경이면 원본처럼 표시 추가를 생략하도록 알린다. (2026-11-09 WB04-E199)
    suppressed() { return this.#renderer.fullRedrawPending(); }

    // main/shadow 변경을 실제 변경 표에 전달한다. 선택 표식 확장을 잃지 않도록 다시 자르지 않는다. (2026-11-09 WB04-E199)
    invalidate(rect, flags) {
        this.#renderer.invalidate({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }, flags >>> 0);
    }
}

// SHP 추가 헤더 +32/+34 의 비정렬 little endian signed WORD 포구 좌표이다. (2026-11-09 WB04-E199)
function headerWord(header, offset) {
    const value = header[offset] | (header[offset + 1] << 8);
    return value >= 0x8000 ? value - 0x10000 : value;
}

// 원본 로딩 순서의 .type 클러스터 수와 SHP 추가 헤더를 raw 타입 번호에 대응시킨다(SquidRenderer::Shapes). (2026-11-09 WB04-E199)
export function squidRendererShapes(table, database) {
    if (table.definitions.length !== TYPE_LOAD_ORDER.length) throw new RangeError('표시 자산 판본 불일치');
    const shapes = Array.from({ length: PATCH_TYPE_COUNT }, () => ({ frameCount: 0, loaded: false, frames: [] }));
    // 번호 70부터 원본 타입 로딩 순서와 같은 블록을 연결한다.
    for (let index = 0; index < table.definitions.length; index += 1) {
        const definition = table.definitions[index];
        if (!definition) throw new Error(`표시 자산 타입 정의 없음: ${TYPE_LOAD_ORDER[index]}`);
        const type = FIRST_ASSET_TYPE_NUMBER + index;
        const block = table.view(type).block;
        if (block < 0 || block >= database.blocks.length) throw new RangeError('표시 자산 SHP 블록 범위 오류');
        const shape = shapes[type];
        shape.frameCount = definition.clusters.length;
        shape.loaded = true;
        // 그림자/abstract 레이어와 공유 프레임도 물리 테이블 순서를 보존한다.
        for (let frame = 0; frame < database.blocks[block].frames.length; frame += 1) {
            const metrics = database.squidMetrics(block, frame);
            // 같은 36바이트의 마지막 두 WORD 가 포구이며 표시 기준점과 다르다.
            const header = database.squidHeader(block, frame);
            shape.frames.push({
                width: metrics.width,
                height: metrics.height,
                hotspotX: metrics.hotspotX,
                hotspotY: metrics.hotspotY,
                cellWidth: metrics.cellWidth,
                cellHeight: metrics.cellHeight,
                cellHotspotX: metrics.cellHotspotX,
                cellHotspotY: metrics.cellHotspotY,
                muzzleX: headerWord(header, 32),
                muzzleY: headerWord(header, 34),
                walkStepX: metrics.walkStepX,
                walkStepY: metrics.walkStepY,
            });
        }
    }
    return shapes;
}
