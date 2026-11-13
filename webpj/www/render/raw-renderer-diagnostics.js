// 원문 Renderer 후단 진단 네 가지(Hash/Fire/Spot/Board, 10.78 436450)와 그 7/10/13 키 분배를 옮긴 모듈이다(cpppj `client/RendererDiagnostics.cpp/.h`, 10.78 전용).
// 기본 UI 나 임의 진단 활성화 없이 현재 플래그만 읽으며, 실제 SID 풀·네 단계 해시·TYPE 표·spot 65536 바이트만 빌려 읽고 표시용 월드나 거리표를 만들지 않는다(검색을 새로 실행하지 않음).
//  - RawRendererDiagnostics.draw(kind, renderer, snapshot, width, height, flag, fireMode, worldCells, background, hooks): flag 가 0 이면 카메라나 장치를 요구하지 않고 아무것도 하지 않는다. 종류별 후단:
//      Hash : 네 단계(1/2/4/16)마다 가시 영역을 칸 단위로 나눈 해시 칸 사각형을 snapshot 으로 투영해 clip → 사각형(색 0)으로 그린다. 즉시 Present 하지 않고 Renderer+60(background)에 1 을 쓴다.
//      Fire : 가시 영역을 finder 로 훑어 TYPE flags1 의 0x800 비트 객체마다 — 모드 0 은 graph BYTE 에 따라(0=색0, 무효 그래프=색7, 그 밖은 현재 팔레트 수의 나머지) 세 겹 사각형, 모드 비영은 8방향 연결선(같은 거리 WORD 만, 색7)을 그린 뒤 Present 한다.
//      Spot / Board : x 바깥/y 안쪽 순서(오른쪽·아래 포함)로 칸마다 spot BYTE 를 읽어 — Board 는 비트 8(색 5)/비트 6(색 1)의 세 겹, Spot 은 비트 0x40(색 7)·0x80(색 0, 둘 다면 진단 0xfe)의 3+3겹(뒤 세 겹은 표면 BYTE 비트 0 이 있을 때 회색 8)을 그린 뒤 Present 한다.
//    모든 정수 산술은 DWORD 감김이며(Add/Sub), 색은 매 조회마다 현재 9색 배열에서 다시 읽는다(미리 복사하거나 이름을 고정하지 않음). 물리 배열 접근이 범위 밖이면 RangeError 이다.
//  - dispatchRendererDiagnosticKey(event, special, specialAuxiliary, state, positionLog): 원문 일반 키 분기(Console/모달 뒤, 정지 앞)의 7(Fire 토글, 보조 특수가 켜져 있을 때)/0x10(Spot 토글, 특수 상태에서 Shift 없을 때 — Shift 이면 기존 역투영/좌표 로그를 지연 호출)/0x13(Board 토글, 특수 상태) 키만 고르고
//    뗌(release)은 소비만 한다. 토글로 꺼지면 redraw 를 1 로 쓴다. state = { fire, spot, board, redraw }(`{ value }` 상자). 반환은 이 키를 소비했는지이다.
//  - 인자: renderer/snapshot 은 `CameraView`(칸 크기 +8/+12), flag·fireMode·worldCells·background 는 `{ value }` 상자(현재 전역을 빌림), hooks = { screen: { clip(rect), rectangle(rect, color), present() }, line(first[x,y], second, color), colors()→9색 배열,
//    invalidGraph()→그래프 무효값, distances()→현재 거리 WORD 배열(미할당이면 빈 배열), assertion(행) }. 선택한 효과에만 장치를 요구한다(없으면 Error).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E384: 5-A 렌더러 진단으로 추가, 기준선 뒤 cpppj 변경 없음)
import { cameraFtolLow } from './camera-view.js';
import { InputCode } from '../core/input-queue.js';
import { SQUID_HASH_SCALES } from '../core/squid-hash.js';
import { RawSquidFinder } from '../core/raw-squid-finder.js';
import { drawWaterFinderArea, drawWaterVisibleArea } from '../core/draw-water.js';

// 진단 종류이다(436450 의 독립 후단 넷). (2026-11-13 WB04-E384)
export const RendererDiagnosticKind = Object.freeze({ hash: 0, fire: 1, spot: 2, board: 3 });
// 진단 단언이 알리는 원문 행(Spot 의 중복 표식 0xfe, Fire 의 거리표 좌표 0x1f9)이다. (2026-11-13 WB04-E384)
const SPOT_DUPLICATE_LINE = 0xfe;
const FIRE_DISTANCE_LINE = 0x1f9;
// TYPE flags1 의 Fire 대상 비트와 spot BYTE 의 비트들이다. (2026-11-13 WB04-E384)
const FIRE_TYPE_FLAG = 0x800;
const SPOT_PATH = 0x40;
const SPOT_BLOCK = 0x80;
const BOARD_STRONG = 8;
const BOARD_WEAK = 6;
// 8방향(원문 52f83c/85c)의 x/y 증분이다. (2026-11-13 WB04-E384)
const DIRECTION_X = [0, 1, 1, 1, 0, -1, -1, -1];
const DIRECTION_Y = [-1, -1, 0, 1, 1, 1, 0, -1];

// 선택한 효과에만 실제 장치를 요구한다. (2026-11-13 WB04-E384)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Renderer 진단 실제 경계 미연결');
    return hook;
}

// 하위 DWORD 감김의 덧셈/뺄셈이다(signed overflow 없이 원문 결과). (2026-11-13 WB04-E384)
const add = (value, delta) => (value + delta) | 0;
const sub = (value, delta) => (value - delta) | 0;

// 풀 슬롯의 비정렬 little endian float32 를 읽는다(+0e/+12). (2026-11-13 WB04-E384)
function coordinate(slot, offset) {
    return new DataView(slot.buffer, slot.byteOffset, slot.byteLength).getFloat32(offset, true);
}

// 현재 색 배열에서 index 번째 색을 매번 다시 읽는다(범위 밖은 물리 범위 오류). (2026-11-13 WB04-E384)
function color(hooks, index) {
    const colors = need(hooks.colors)();
    if (index >= colors.length) throw new RangeError('Renderer 진단 현재 색표 물리 범위');
    return colors[index] >>> 0;
}

// 원문 y*256+x 배열 칸 위치이다. 행 alias 는 유지하되 배열 밖이면 물리 범위 오류이다. (2026-11-13 WB04-E384)
function cell(x, y, size) {
    const offset = (Math.imul(y, 256) + x) | 0;
    if (offset < 0 || offset >= size) throw new RangeError('Renderer 진단 원문 배열 물리 범위');
    return offset;
}

// 사각형을 안쪽으로 한 칸씩 좁히며 count 번 그린다. 매 사각형 직전에 현재 색을 다시 읽는다. (2026-11-13 WB04-E384)
function inward(rect, count, rectangle, inkOf) {
    let current = rect;
    for (let index = 0; index < count; index += 1) {
        current = { left: add(current.left, 1), top: add(current.top, 1), right: add(current.right, -1), bottom: add(current.bottom, -1) };
        need(rectangle)(current, inkOf());
    }
    return current;
}

// 후단 네 진단의 그리기 본체이다. 사용: new RawRendererDiagnostics(pool, hash, types, spots).draw(kind, renderer, snapshot, 96, 80, flag, fireMode, worldCells, background, hooks). (2026-11-13 WB04-E384)
export class RawRendererDiagnostics {
    // 같은 현재 물리(SID 풀·네 해시·TYPE 표·65536 바이트 spot)를 동기 호출 동안 빌린다.
    #pool;
    #hash;
    #types;
    #spots;

    // 실제 표를 빌리기만 하며 생성 시 탐색/검색/픽셀을 바꾸지 않는다. (2026-11-13 WB04-E384)
    constructor(pool, hash, types, spots) {
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#spots = spots;
    }

    // 명시 DebugSwitch 가 기록한 현재 플래그를 읽어 종류별 후단을 그린다. flag 0 은 다른 의존을 요구하기 전에 끝난다. (2026-11-13 WB04-E384)
    draw(kind, renderer, snapshot, width, height, flag, fireMode, worldCells, background, hooks) {
        if (!flag.value) return;
        const visible = drawWaterVisibleArea(renderer, width, height);
        const cellX = snapshot.data.getInt32(8, true);
        const cellY = snapshot.data.getInt32(12, true);
        if (kind === RendererDiagnosticKind.hash) {
            this.#hashDiagnostic(visible, snapshot, worldCells, hooks);
            background.value = 1;
            return;
        }
        if (kind === RendererDiagnosticKind.fire) {
            this.#fireDiagnostic(visible, snapshot, fireMode, cellX, cellY, hooks);
            return;
        }
        this.#gridDiagnostic(kind, visible, snapshot, cellX, cellY, hooks);
    }

    // 네 단계(1/2/4/16) 해시 칸의 투영 사각형이다. 34380 은 아래 한계 0 을 추가하지 않는다. (2026-11-13 WB04-E384)
    #hashDiagnostic(visible, snapshot, worldCells, hooks) {
        for (const scale of SQUID_HASH_SCALES) {
            const clamp = (value) => Math.min(Math.trunc(value / scale) | 0, add(Math.trunc((worldCells.value | 0) / scale) | 0, -1));
            const left = clamp(visible.left);
            const top = clamp(visible.top);
            const right = clamp(visible.right);
            const bottom = clamp(visible.bottom);
            // 원문 현재 행의 양쪽 꼭짓점을 매 열 투영한다. imul 의 하위 값을 signed 로 읽은 뒤 float32 로 저장한다.
            for (let y = top; y <= bottom; y = add(y, 1)) {
                for (let x = left; x <= right; x = add(x, 1)) {
                    const product = (value) => Math.fround(Math.imul(value, scale));
                    const first = snapshot.project(product(x), product(y));
                    const second = snapshot.project(product(add(x, 1)), product(add(y, 1)));
                    const rect = { left: add(first.x, 1), top: add(first.y, 1), right: add(second.x, -1), bottom: add(second.y, -1) };
                    need(hooks.screen.clip)(rect);
                    need(hooks.screen.rectangle)(rect, color(hooks, 0));
                }
            }
        }
    }

    // Fire 대상 객체의 세 겹 사각형(모드 0) 또는 8방향 연결선(모드 비영)이며 마지막에 Present 한다. (2026-11-13 WB04-E384)
    #fireDiagnostic(visible, snapshot, fireMode, cellX, cellY, hooks) {
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 같은 네 단계 현재 체인을 순회하며 추가 free/dead 필터는 없다.
        for (let sid = finder.begin(drawWaterFinderArea(visible), 0); sid !== 0; sid = finder.next()) {
            const raw = this.#pool.slot(sid);
            if (raw[10] >= this.#types.length) throw new RangeError('Renderer 진단 TYPE 범위');
            if (!(this.#types[raw[10]].flags1 & FIRE_TYPE_FLAG)) continue;
            const x = coordinate(raw, 14);
            const y = coordinate(raw, 18);
            const point = snapshot.project(x, y);
            if (!fireMode.value) {
                const graph = raw[30];
                // 현재 graph BYTE 와 현재 팔레트 수의 나머지로 고른다.
                const choose = () => {
                    if (!graph) return color(hooks, 0);
                    if (graph === (need(hooks.invalidGraph)() >>> 0)) return color(hooks, 7);
                    const colors = need(hooks.colors)();
                    if (colors.length === 0) throw new RangeError('Renderer 진단 색 개수 0');
                    return colors[graph % colors.length] >>> 0;
                };
                const selected = choose();
                const rect = { left: add(sub(point.x, cellX), 1), top: add(sub(point.y, cellY), 1), right: add(point.x, -1), bottom: add(point.y, -1) };
                need(hooks.screen.clip)(rect);
                need(hooks.screen.rectangle)(rect, selected);
                // 선택 색은 clip 전에 한 번 캡처하고 세 겹 모두 같은 색이다.
                inward(rect, 2, hooks.screen.rectangle, () => selected);
            } else {
                const px = add(point.x, -8);
                const py = add(point.y, -6);
                need(hooks.screen.clip)({ left: add(sub(px, cellX), 1), top: add(sub(py, cellY), 1), right: add(point.x, -9), bottom: add(point.y, -7) });
                // 각 방향마다 현재 59514c 를 다시 읽고 같은 거리 WORD 만 연결선으로 그린다.
                for (let direction = 0; direction < 8; direction += 1) {
                    const distances = need(hooks.distances)();
                    if (distances.length === 0) continue;
                    const cx = cameraFtolLow(x);
                    const cy = cameraFtolLow(y);
                    if (cx < 0 || cy < 0 || cx > 256 || cy > 256) need(hooks.assertion)(FIRE_DISTANCE_LINE);
                    if (distances[cell(cx, cy, distances.length)] === distances[cell(add(cx, DIRECTION_X[direction]), add(cy, DIRECTION_Y[direction]), distances.length)]) {
                        need(hooks.line)([px, py], [add(px, DIRECTION_X[direction] * 7), add(py, DIRECTION_Y[direction] * 5)], color(hooks, 7));
                    }
                }
            }
        }
        need(hooks.screen.present)();
    }

    // Spot/Board 는 x 바깥/y 안쪽 순서와 포함 오른쪽/아래를 유지하며 칸마다 현재 spot·표면 슬롯을 다시 읽는다. (2026-11-13 WB04-E384)
    #gridDiagnostic(kind, visible, snapshot, cellX, cellY, hooks) {
        for (let x = visible.left; x <= visible.right; x = add(x, 1)) {
            for (let y = visible.top; y <= visible.bottom; y = add(y, 1)) {
                let surface = 0;
                // 원문은 표면 BYTE 를 spot/색/진단보다 먼저 읽는다(범위 밖 칸은 sid 0 슬롯).
                if (kind === RendererDiagnosticKind.spot) {
                    const sid = (x < 0 || y < 0 || x > 255 || y > 255) ? 0 : this.#hash.cell(0, x, y);
                    surface = this.#pool.slot(sid)[12];
                }
                const spot = this.#spots[cell(x, y, this.#spots.length)];
                let index = -1;
                let selected = 0;
                if (kind === RendererDiagnosticKind.board) {
                    if (spot & BOARD_STRONG) index = 5;
                    else if (spot & BOARD_WEAK) index = 1;
                } else {
                    if (spot & SPOT_PATH) { index = 7; selected = color(hooks, 7); }
                    if (spot & SPOT_BLOCK) {
                        if (index !== -1) need(hooks.assertion)(SPOT_DUPLICATE_LINE);
                        index = 0;
                        selected = color(hooks, 0);
                    }
                }
                if (index < 0) continue;
                const point = snapshot.project(Math.fround(x), Math.fround(y));
                let rect = { left: sub(point.x, cellX), top: sub(point.y, cellY), right: point.x, bottom: point.y };
                if (kind === RendererDiagnosticKind.board) {
                    inward(rect, 3, hooks.screen.rectangle, () => color(hooks, index));
                } else {
                    // 첫 3겹은 현재 선택 색, 뒤 3겹은 표면 BYTE 의 0 비트가 있을 때 현재 회색이다.
                    for (let count = 0; count < 6; count += 1) {
                        rect = { left: add(rect.left, 1), top: add(rect.top, 1), right: add(rect.right, -1), bottom: add(rect.bottom, -1) };
                        if (count < 3 || (surface & 1)) need(hooks.screen.rectangle)(rect, count < 3 ? selected : color(hooks, 8));
                    }
                }
            }
        }
        need(hooks.screen.present)();
    }
}

// 원문 일반 키 분기이다(Console/모달 뒤, 정지 앞; 새 Editor/mission 관문은 없다). event = { code }. state = { fire, spot, board, redraw }(`{ value }` 상자). 반환은 소비 여부이다. (2026-11-13 WB04-E384)
export function dispatchRendererDiagnosticKey(event, special, specialAuxiliary, state, positionLog = null) {
    const code = event.code & 0x03ffffff;
    if (code !== 7 && code !== 0x10 && code !== 0x13) return false;
    if (event.code & InputCode.release) return true;
    if (code === 7) {
        if (specialAuxiliary >>> 0) {
            state.fire.value = state.fire.value === 0 ? 1 : 0;
            if (!state.fire.value) state.redraw.value = 1;
        }
    } else if (special >>> 0) {
        if (code === 0x10) {
            if (!(event.code & InputCode.shift)) {
                const next = state.spot.value === 0 ? 1 : 0;
                state.redraw.value = 1;
                state.spot.value = next;
            } else need(positionLog)(event);
        } else {
            state.board.value = state.board.value === 0 ? 1 : 0;
            if (!state.board.value) state.redraw.value = 1;
        }
    }
    return true;
}
