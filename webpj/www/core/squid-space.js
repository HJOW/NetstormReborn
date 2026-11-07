// 원본 10.78 객체 공간(spot 점유 지도·표면 WORD 지도)의 공통 계산을 옮긴 모듈이다.
//  - effectiveGenus(004afd30): 타입 genus 에 발자국 안쪽 칸이면 비트 8 을 보정한다(지붕 가운데 열 윗부분 제외). 기준점은 +0.9999 후 0 방향 절삭이다.
//  - RawBuildingSpace: 표면 WORD 지도(공간 해시 0단계)를 조회하고 건물 Unpop 의 옆면 BYTE 를 지우며, 다리 표면 통지(004214a0)의 진단을 고른다.
// 근거: cpppj `o/Squid.cpp`(EffectiveGenus, 커밋 65927742), `o/RawBuildingSpace.cpp/.h`(edc7db85), 원본 관측 `cpppj/tests/fixtures/hash-x86.tsv` Genus 3216행.
// 건물 Pop 의 옆면 쓰기(PopSide·HasIsland)는 Pop 과 함께 WB04-E1 에서 옮겼다. CD 10.72 경로는 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-D1: 공통 공간 해제의 기반으로 추가, 2026-11-08 WB04-E1: popSide·hasIsland 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';

// 내부 spot 보정을 받는 건물군 genus 마스크이다(원본 DAT_005424b8). (2026-11-05 WB04-D1)
export const INTERIOR_GENUS_MASK = 0x50444200;

// 지붕 genus 비트이다. 가운데 열의 윗부분은 안쪽 비트 8 을 붙이지 않는다. (2026-11-05 WB04-D1)
const ROOF_GENUS = 0x400000;

// spot 좌표 보정값이다(원본 float 비트 3f7ff972 = 0.9999f). 해시 버킷의 단순 절삭과 다른 변환이다. (2026-11-05 WB04-D1)
const SPOT_COORDINATE_BIAS = Math.fround(0.9999);

// 표면 WORD 의 위치·타입 BYTE 위치·extra BYTE 위치(10.78)이다. (2026-11-05 WB04-D1)
const SURFACE_WORD = 12, TYPE = 10, EXTRA = 40;

// 원본 004afd30: 발자국(오른쪽 아래 기준 width×height) 안쪽 칸이고 건물군 genus 이면 genus|8, 아니면 genus 그대로이다.
// 지붕 genus 는 가운데 열((cellX-left)*2 == right-left)의 위쪽 절반((cellY-top)*2 <= bottom-top)을 안쪽으로 보지 않는다.
// 좌표는 float32 로 반올림한 뒤 double 로 0.9999f 를 더해 0 방향 절삭한다(x87 중간 덧셈 보존). 범위 밖 입력은 RangeError 이다.
// 사용: effectiveGenus(type.flags2, x, y, type.footX, type.footY, cellX, cellY). (2026-11-05 WB04-D1)
export function effectiveGenus(flags2, x, y, width, height, cellX, cellY) {
    const fx = Math.fround(x);
    const fy = Math.fround(y);
    const inWorld = (value) => Number.isInteger(value) && value >= 0 && value < WORLD_CELLS;
    if (!Number.isFinite(fx) || !Number.isFinite(fy) || fx < 0 || fy < 0 || fx >= WORLD_CELLS || fy >= WORLD_CELLS ||
        !Number.isInteger(width) || !Number.isInteger(height) || width < 1 || height < 1 || width > WORLD_CELLS || height > WORLD_CELLS ||
        !inWorld(cellX) || !inWorld(cellY)) throw new RangeError('Squid effective genus 발자국 범위 오류');
    const genus = flags2 >>> 0;
    const right = Math.trunc(fx + SPOT_COORDINATE_BIAS);
    const bottom = Math.trunc(fy + SPOT_COORDINATE_BIAS);
    const left = right - width + 1;
    const top = bottom - height + 1;
    const interior = cellX !== left && cellX !== right && cellY !== top && cellY !== bottom;
    const roofOpening = (genus & ROOF_GENUS) !== 0 && (cellX - left) * 2 === right - left && (cellY - top) * 2 <= bottom - top;
    return (genus & INTERIOR_GENUS_MASK) !== 0 && interior && !roofOpening ? (genus | 8) >>> 0 : genus;
}

// 건물 옆면·표면 통지 장치이다. pool 은 SidPool(같은 실제 풀), hash 는 SquidHash(0단계가 원본 표면 WORD 지도), types 는 188칸 타입 표,
// state 는 { islandType, bridgeType, checking: () => 현재 진단 DWORD } 이다. bindCurrentBridgeType 으로 현재 다리 TYPE 소유자를 빌릴 수 있다.
// 사용: const building = new RawBuildingSpace(pool, hash, types, { islandType: 83, bridgeType: 82, checking: () => 0 }); building.unpopSide(x, y). (2026-11-05 WB04-D1)
export class RawBuildingSpace {
    // 실제 SID 풀이다.
    #pool;
    // 실제 공간 해시이다.
    #hash;
    // 타입 표이다.
    #types;
    // 타입 전역과 진단 상태이다.
    #state;
    // 현재 다리 TYPE 을 돌려주는 함수이다. 없으면 state.bridgeType 이다.
    #currentBridgeType = null;
    // 다리 표면 통지의 원문 진단 장치이다(선택된 경우에만 요구).
    #notifyAssertion = null;

    // 10.78 타입 표(188칸)와 범위 안의 섬/다리 TYPE 만 받는다.
    constructor(pool, hash, types, state) {
        if (pool?.edition !== 'patch1078' || types?.length !== 188 || !(state?.islandType < types.length) || !(state.bridgeType < types.length) || typeof state.checking !== 'function') {
            throw new TypeError('건물 공간의 풀/타입 표/상태 오류');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#state = state;
    }

    // 같은 월드의 현재 다리 TYPE 공급 함수를 빌린다(GameAuxState 등). 함수가 아니면 오류이다.
    bindCurrentBridgeType(read) {
        if (typeof read !== 'function') throw new TypeError('현재 다리 TYPE 공급자는 함수여야 합니다.');
        this.#currentBridgeType = read;
    }

    // 다리 표면 통지의 현재 진단 함수와 원문 진단 장치를 함께 연결한다.
    bindNotifyDiagnostics(checking, assertion) {
        if (typeof checking !== 'function') throw new TypeError('진단 DWORD 공급자는 함수여야 합니다.');
        this.#state.checking = checking;
        this.#notifyAssertion = assertion;
    }

    // 0040da00: 지도 밖이면 0, 그 외 해당 칸의 표면 WORD SID 이다(상태로 거르지 않는다).
    surface(x, y) {
        if (x < 0 || y < 0 || x >= WORLD_CELLS || y >= WORLD_CELLS) return 0;
        return this.#hash.cell(0, x, y);
    }

    // 원본 CRT 절삭의 유효 정수 범위(int32)만 허용하며 음수 소수를 floor 로 바꾸지 않는다. 범위 밖·NaN·무한대는 RangeError 이다. (2026-11-08 WB04-E1)
    static #truncate(value) {
        const truncated = Math.trunc(Math.fround(value));
        if (!Number.isFinite(truncated) || truncated < -2147483648 || truncated > 2147483647) throw new RangeError('건물 지면 좌표 절삭 범위 오류');
        return truncated;
    }

    // 원본 boolean 섬 확인: 전달 좌표(Pop 은 건물 자체의 좌표)의 표면 SID 가 있고 그 현재 TYPE 이 섬 TYPE 이면 참이다. (2026-11-08 WB04-E1)
    hasIsland(x, y) {
        const sid = this.surface(RawBuildingSpace.#truncate(x), RawBuildingSpace.#truncate(y));
        return sid !== 0 && this.#pool.slot(sid)[TYPE] === this.#state.islandType;
    }

    // Pop 의 옆면 한 칸: 유효 SID 의 현재 TYPE 이 표면(flags1 0x800)일 때만, 건물 좌표가 섬이면 표면 WORD 낮은 BYTE 의 bit2 를 지우고 bit1 을 켜고
    // 아니면 두 비트(1·2)를 지운 뒤 통지한다. side 판정과 spot 쓰기는 호출자가 처리하고 이 몸체는 그 직후의 현재 후보를 다시 조회한다.
    // 타입 번호가 표 밖이면 RangeError 이다. 사용: building.popSide(cellX, cellY, buildingX, buildingY). (2026-11-08 WB04-E1)
    popSide(x, y, buildingX, buildingY) {
        const sid = this.surface(x, y);
        if (sid === 0 || sid >= this.#pool.capacity) return;
        const old = this.#pool.slot(sid);
        if (old[TYPE] >= this.#types.length) throw new RangeError('건물 옆면 표면 타입 오류');
        if (!(this.#types[old[TYPE]].flags1 & 0x800)) return;
        const island = this.hasIsland(buildingX, buildingY);
        const raw = this.#pool.physical(sid);
        // 섬이면 WORD 의 bit2 만 지우고 bit1 을 켜며, 그 외에는 BYTE 의 두 비트만 지운다.
        raw[SURFACE_WORD] = island ? (raw[SURFACE_WORD] & 0xfb) | 2 : raw[SURFACE_WORD] & 0xf9;
        this.notifySurface(sid);
    }

    // Unpop 의 옆면 한 칸: 표면 플래그·섬 확인 없이 유효 SID 의 표면 WORD 낮은 BYTE 비트 1/2 를 지우고 통지한다(높은 BYTE 유지).
    unpopSide(x, y) {
        const sid = this.surface(x, y);
        if (sid === 0 || sid >= this.#pool.capacity) return;
        this.#pool.physical(sid)[SURFACE_WORD] &= 0xf9;
        this.notifySurface(sid);
    }

    // 004214a0: 현재 다리 TYPE 이고 extra 비트 8 이 없으면 표면 WORD 의 번호((>>3)&15)를 읽고, 현재 진단이 비0 이며 번호가 7 을 넘으면 원문 진단을 부른다.
    // 진단이 정상 복귀하면 바로 반환한다. 선택된 진단 장치가 없으면 오류이다.
    notifySurface(sid) {
        const raw = this.#pool.slot(sid);
        const bridge = this.#currentBridgeType ? this.#currentBridgeType() : this.#state.bridgeType;
        if (raw[TYPE] !== bridge || (raw[EXTRA] & 8) !== 0) return;
        const number = (raw[SURFACE_WORD] >>> 3) & 15;
        if (this.#state.checking() !== 0 && number > 7) {
            if (typeof this.#notifyAssertion !== 'function') throw new Error('지면 통지의 선택된 원문 진단 장치가 연결되지 않았습니다.');
            this.#notifyAssertion('num >= 0 && num <= 7', '\\Ns\\O\\Bridge.cpp', 0xff);
        }
    }

    // 다른 풀/해시와 섞어 쓰지 않도록 같은 저장소인지 확인한다.
    validateSpace(pool, hash) {
        if (pool !== this.#pool || hash !== this.#hash) throw new TypeError('건물 공간 풀/해시 불일치');
    }
}
