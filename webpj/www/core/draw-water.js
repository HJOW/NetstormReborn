// DrawWater(원본 진단 사각형 표시) 명령·영역 계산·객체 순회와 경로 표식 사각형을 옮긴 모듈이다(cpppj `client/DrawWater.cpp/.h`, 10.78 전용: 00434dd0 표시·00497530 가시 영역·0041def0 finder 입력·004a2270 네 선·00434970 경로 표식).
//  - DRAW_WATER_COMMAND(0x0200001a)·drawWaterScriptCode(name): 명령 이름표다. 이름은 첫 NUL 까지 ASCII 소문자 비교(`drawwater`)이며 파생 별칭이나 기본 개발자 메뉴 항목을 만들지 않는다. 그 밖은 0xffffffff.
//  - dispatchDrawWater(event, enabled, worldRedraw): 일반 사건 분배 뒤 현재 DWORD 토글이다. 명령 코드(하위 26비트)가 아니면 false, 뗌(release) 사건은 true 로 끝내고 아무것도 쓰지 않는다. 그 밖은 현재 비영 전체 DWORD 를 0 으로,
//    0 이면 1 로 만들며 worldRedraw=1 을 flag 쓰기보다 먼저 쓴다. enabled·worldRedraw 는 `{ value }` 상자다(원문의 DWORD 참조).
//  - drawWaterVisibleArea(renderer, width, height): 00497530 은 snapshot 이 아니라 현재 Renderer 보기의 월드 float 와 정수 칸 크기, 전체 화면 폭/높이를 읽는다. 왼쪽/위는 float32 월드 좌표의 ftol 하위값이고
//    오른쪽/아래는 화면 크기를 칸 크기로 나눈(signed IDIV, 0 방향 절삭) 몫을 DWORD 로 더한다. 칸 크기 0 이나 INT_MIN / −1 은 원문의 나눗셈 예외라 RangeError 로 거부한다.
//  - drawWaterFinderArea(visible): 0041def0 의 입력 float32, lower/upper snap(위쪽은 +0.99999 상수 0x3f7fff58) → float32 → ftol 저장으로 finder 영역을 만든다. 큰 정수의 float32 반올림도 보존한다.
//  - RawDrawWater(pool, hash, types).draw(renderer, snapshot, width, height, enabled, color, hooks): flag 는 입구에서만 검사한다. 영역 finder 의 모든 현재 후보(free/dead/void 도 거르지 않음)마다 snapshot 투영점에서
//    (칸 크기/2) 만큼 왼쪽 위로 간 초기 사각형(가로 0 폭, 아래쪽은 원문대로 가로 반크기를 쓴다)을 clip 으로 알리고, 세 번 rectangle 을 부르되 매 호출 직전 현재 color 를 다시 읽고 네 변을 한 픽셀씩 확장한다.
//    후보가 없어도 flag 가 비영이면 마지막에 present 를 한 번 부른다. 장치(clip/rectangle/present)는 도달할 때만 요구한다. enabled·color 는 `{ value }` 상자다(rectangle 장치가 도중에 바꿀 수 있음).
//  - drawWaterRectangle(pixels, pitch, height, fullClip, rectangle, color): 004a2270 의 네 선(위·아래·왼쪽·오른쪽 순서, 뒤 선이 같은 모서리를 다시 씀)을 VFX 선으로 그린다. right/bottom −1 은 DWORD 감김을 보존한다.
//  - pathMarkerRectangles(point, cellPixels): 00434970 의 세 겹 사각형 좌표다. 투영 화면점을 오른쪽 아래로, 칸 크기만큼 왼쪽 위로 잡고 매 겹 먼저 한 픽셀씩 줄인 뒤 기록한다(signed DWORD 감김 보존).
// renderer/snapshot 은 render/camera-view.js 의 CameraView 이다. 이 모듈은 DOM 을 쓰지 않는다. 개발 진단 표시라 게임 규칙은 바꾸지 않는다.
// (2026-11-13 WB04-E380: 5-A DrawWater 이식과 함께 추가. PathMarkerRectangles 는 기준선 1f00a25a 뒤 cpppj 20261111-123 CP M7.4 에서 같은 파일에 더해졌고 원본 함수의 복원이라 함께 옮김)
import { cameraFtolLow } from '../render/camera-view.js';
import { drawVfxLinePixels } from '../render/vfx-line.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 00542760 의 실제 명령 코드이다. 기본 메뉴 항목을 추가하지 않는다. (2026-11-13 WB04-E380)
export const DRAW_WATER_COMMAND = 0x0200001a;
// 사건 코드의 명령 비트 마스크와 뗌(release) 비트이다. (2026-11-13 WB04-E380)
const COMMAND_MASK = 0x03ffffff;
const RELEASE = 0x80000000;
// 0 이름표 불일치 때 돌려주는 값(미등록 명령)이다. (2026-11-13 WB04-E380)
const NO_COMMAND = 0xffffffff;
// 위쪽/오른쪽 finder 좌표에 더하는 float32 상수 0x3f7fff58(0.99999...)의 값이다. (2026-11-13 WB04-E380)
const UPPER_SNAP = (() => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, 0x3f7fff58, true);
    return view.getFloat32(0, true);
})();
// signed 32비트 최솟값이다. INT_MIN / −1 나눗셈 예외 판정에 쓴다. (2026-11-13 WB04-E380)
const INT_MIN = -0x80000000;

// 부호 있는 좌표에 부호 없는 값을 더하는 32비트 감김이다(signed overflow 없이 하위 DWORD 유지). (2026-11-13 WB04-E380)
const add = (value, delta) => (value + delta) | 0;
// 투영 좌표/반크기를 빼는 32비트 감김이다. (2026-11-13 WB04-E380)
const sub = (value, delta) => (value - delta) | 0;

// 원문 signed IDIV 이다. 0 이나 INT_MIN / −1 은 정상 가시 영역으로 숨기지 않고 RangeError 로 거부한다. (2026-11-13 WB04-E380)
function divide(value, denominator) {
    if (denominator === 0 || (value === INT_MIN && denominator === -1)) throw new RangeError('DrawWater 의 원문 viewport IDIV 예외');
    return Math.trunc(value / denominator) | 0;
}

// 도달한 장치만 요구한다. flag 0·미선택 효과는 선행 장치를 요구하지 않는다. (2026-11-13 WB04-E380)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('DrawWater 현재 화면 효과 미연결');
    return hook;
}

// 슬롯의 정렬되지 않은 little endian float32 를 읽는다(풀 +0e/+12 좌표). (2026-11-13 WB04-E380)
function coordinate(slot, offset) {
    return new DataView(slot.buffer, slot.byteOffset, slot.byteLength).getFloat32(offset, true);
}

// 41d770/41d7a0 의 두 float32 저장과 최종 ftol 이다. upper 이면 snap 상수를 더한다. (2026-11-13 WB04-E380)
function finderCoordinate(value, upper) {
    const first = Math.fround(value);
    const snapped = Math.fround(cameraFtolLow(first + (upper ? UPPER_SNAP : 0)));
    return cameraFtolLow(snapped);
}

// 이름표 비교용 ASCII 대문자만 소문자로 바꾼다(비 ASCII 는 그대로). (2026-11-13 WB04-E380)
const lowerAscii = (text) => text.replace(/[A-Z]/g, (letter) => String.fromCharCode(letter.charCodeAt(0) + 32));

// 명령 이름 → 코드. 이름은 첫 NUL 까지이며 파생 별칭은 만들지 않는다. 사용: drawWaterScriptCode('DrawWater') === DRAW_WATER_COMMAND. (2026-11-13 WB04-E380)
export function drawWaterScriptCode(name) {
    const nul = name.indexOf('\0');
    return lowerAscii(nul < 0 ? name : name.slice(0, nul)) === 'drawwater' ? DRAW_WATER_COMMAND : NO_COMMAND;
}

// 일반 사건 분배의 DrawWater 분기이다. 명령이 아니면 false(사건을 소비하지 않음), release 이면 true 로 끝낸다. 그 밖은 enabled 를 토글하고 worldRedraw 를 1 로 쓴다(redraw 저장이 flag 저장보다 먼저).
// event = { code, x, y }, enabled/worldRedraw 는 `{ value }` 상자. 사용: dispatchDrawWater({ code: DRAW_WATER_COMMAND }, enabled, redraw). (2026-11-13 WB04-E380)
export function dispatchDrawWater(event, enabled, worldRedraw) {
    if ((event.code & COMMAND_MASK) !== DRAW_WATER_COMMAND) return false;
    if ((event.code & RELEASE) !== 0) return true;
    const next = enabled.value === 0 ? 1 : 0;
    worldRedraw.value = 1;
    enabled.value = next;
    return true;
}

// 00497530: 현재 Renderer 보기의 월드 float 와 정수 칸 크기, 전체 화면 폭/높이로 가시 사각형 { left, top, right, bottom } 을 만든다. ftol 은 분수 원점을 0 방향으로 절삭한 값이고
// 오른쪽/아래는 signed 나눗셈 몫을 DWORD 로 더한다. 위/아래 계산이 왼쪽/오른쪽보다 먼저다(예외 순서). (2026-11-13 WB04-E380)
export function drawWaterVisibleArea(renderer, width, height) {
    const view = renderer.data;
    const top = cameraFtolLow(view.getFloat32(4, true));
    const bottom = add(top, divide(height, view.getInt32(12, true)));
    const left = cameraFtolLow(view.getFloat32(0, true));
    return { left, top, right: add(left, divide(width, view.getInt32(8, true))), bottom };
}

// 0041def0: 가시 사각형의 왼쪽/위는 그대로, 오른쪽/아래는 +0.99999 snap 을 거쳐 finder 영역으로 바꾼다. 현재 보드 제한은 finder 의 begin 이 수행한다. (2026-11-13 WB04-E380)
export function drawWaterFinderArea(visible) {
    return {
        left: finderCoordinate(visible.left, false), top: finderCoordinate(visible.top, false),
        right: finderCoordinate(visible.right, true), bottom: finderCoordinate(visible.bottom, true),
    };
}

// 현재 실제 풀/해시/TYPE 표를 빌려 DrawWater 표시를 한다. 생성 자체는 순회·표시·상태 변경을 하지 않는다. (2026-11-13 WB04-E380)
export class RawDrawWater {
    // 실제 SID 풀·공간 해시·TYPE 발자국 표(finder 가 매 후보에서 다시 읽음)이다. 복사하지 않는다.
    #pool;
    #hash;
    #types;

    // 사용: new RawDrawWater(pool, hash, types). (2026-11-13 WB04-E380)
    constructor(pool, hash, types) {
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
    }

    // 00434dd0 표시이다. renderer 는 현재 가시 영역용 보기, snapshot 은 투영·칸 크기용 보기(CameraView), enabled/color 는 `{ value }` 상자, hooks = { clip(rect), rectangle(rect, color), present() }.
    // flag 는 입구에서만 검사해 반복 도중 꺼도 현재 호출은 끝까지 진행한다. 후보 순서는 finder(4단계 해시, 체인 순서)이며 객체별 bounds/지식/상태 필터를 추가하지 않는다. (2026-11-13 WB04-E380)
    draw(renderer, snapshot, width, height, enabled, color, hooks) {
        if (!enabled.value) return;
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        const area = drawWaterFinderArea(drawWaterVisibleArea(renderer, width, height));
        // 원문 current 의 다음 체인을 미리 저장하는 같은 네 단계 finder 이다.
        for (let sid = finder.begin(area, 0); sid !== 0; sid = finder.next()) {
            const slot = this.#pool.slot(sid);
            const point = snapshot.project(coordinate(slot, 14), coordinate(slot, 18));
            const halfX = Math.trunc(snapshot.data.getInt32(8, true) / 2);
            const halfY = Math.trunc(snapshot.data.getInt32(12, true) / 2);
            // 원문 left==right 와 bottom 의 halfX 사용을 일반 bounds 로 고치지 않는다.
            let rectangle = { left: sub(point.x, halfX), top: sub(point.y, halfY), right: sub(point.x, halfX), bottom: sub(point.y, halfX) };
            need(hooks.clip)(rectangle);
            // 각 호출 직전 현재 색(005ad128)을 다시 읽고 네 변을 한 픽셀씩 확장한다.
            for (let index = 0; index < 3; index += 1) {
                need(hooks.rectangle)(rectangle, color.value);
                rectangle = { left: add(rectangle.left, 0xffffffff), top: add(rectangle.top, 0xffffffff), right: add(rectangle.right, 1), bottom: add(rectangle.bottom, 1) };
            }
        }
        // 후보가 0개여도 flag 가 비영이면 전체 viewport 출력/clip 복구를 한 번 실행한다.
        need(hooks.present)();
    }
}

// 004a2270 의 네 선이다. right/bottom −1 을 감김으로 보존하고 역전/0폭 사각형을 임의로 생략하지 않는다(위·아래·왼쪽·오른쪽 순서). 사용: drawWaterRectangle(pixels, 100, 80, { left: 0, top: 0, right: 96, bottom: 80 }, rect, 45). (2026-11-13 WB04-E380)
export function drawWaterRectangle(pixels, pitch, height, fullClip, rectangle, color) {
    const right = add(rectangle.right, 0xffffffff);
    const bottom = add(rectangle.bottom, 0xffffffff);
    drawVfxLinePixels(pixels, pitch, height, fullClip, { x: rectangle.left, y: rectangle.top }, { x: right, y: rectangle.top }, color);
    drawVfxLinePixels(pixels, pitch, height, fullClip, { x: rectangle.left, y: bottom }, { x: right, y: bottom }, color);
    drawVfxLinePixels(pixels, pitch, height, fullClip, { x: rectangle.left, y: rectangle.top }, { x: rectangle.left, y: bottom }, color);
    drawVfxLinePixels(pixels, pitch, height, fullClip, { x: right, y: rectangle.top }, { x: right, y: bottom }, color);
}

// 00434970 의 do-while 세 번 반복을 좌표 계산만으로 옮긴다. point = [x, y](투영 화면점, 오른쪽 아래), cellPixels = [가로, 세로](현재 칸 크기). 먼저 줄이고 기록하는 세 겹이며 감김은 DWORD 연산으로 보존한다.
// 사용: pathMarkerRectangles([100, 80], [16, 11]) → [{ left: 85, top: 70, right: 99, bottom: 79 }, …]. (2026-11-13 WB04-E380)
export function pathMarkerRectangles(point, cellPixels) {
    let right = point[0];
    let bottom = point[1];
    let left = sub(point[0], cellPixels[0]);
    let top = sub(point[1], cellPixels[1]);
    const result = [];
    // 먼저 줄이고 기록하는 세 겹 반복이다.
    for (let index = 0; index < 3; index += 1) {
        bottom = sub(bottom, 1);
        right = sub(right, 1);
        top = add(top, 1);
        left = add(left, 1);
        result.push({ left, top, right, bottom });
    }
    return result;
}
