// 원본 10.78 객체 등록 처리기(cpppj `o/SquidPop.cpp/.h`)를 옮긴 모듈이다. 같은 SID 풀·공간 해시·spot 점유 지도에 객체를 등록하는 공통 Pop 부모(raw-squid-pop-parent.js)에
// 현재 TYPE 표·SHP 프레임 크기·월드 한 변·진단/로그·미니맵 dirty 큐·건물 옆면·표시 갱신·firstPop·현재 가상 Activate(raw-squid-activate.js)를 연결한다.
//  - 독립 입력: pop(sid, type, 프레임 폭, 높이, x, y, flags) 는 인자 TYPE 레코드와 치수를 쓴다. 실제 World: popCurrent(sid, x, y, flags) 는 사전 TYPE/SHP 조회 없이
//    bindDiagnostics 로 빌린 전체 현재 표와 표시 장치를 소비 위치마다 조회한다.
//  - Supports: 공통 firstPop/postPop 을 확인한 가상 표만 허용한다(PopVtables.inc 26칸 + 마나·볼트 표시, 134 의 실제 7c/80/84 쌍). 호스트 포인터로 호출하지 않는다.
//  - 표시(display)와 공통 postPop(postPop)은 선택 연결이다: null 이면 표시 갱신을 억제하되 현재 VT 의 지원 여부는 확인하고(미복원 파생은 오류), 공통 깊이는 호출마다의 지역 장부를 쓴다.
//    연결할 객체의 계약 — display: { frameSize(raw, frameNumber) → [폭, 높이], update(raw, flags) }, postPop: { pool, activationDepth: { value }, postPopVirtual(sid, flags), validateSpace?(hash, spots) }.
// 근거: cpppj `o/SquidPop.cpp/.h`, `o/PopVtables.inc`(기준선 1f00a25a), 원본 관측 pop-x86.tsv(10.78 4블록 2456단계)·pop-vtables.tsv(10.78 83행)·pop-world-bounds-x86.tsv(260행)(tests/squid-pop.test.mjs).
// CD 10.72 경로(사전 검사하는 기존 구현)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E1: 공통 Pop 이식으로 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';
import { SquidFactory } from './squid-factory.js';
import { SquidHash } from './squid-hash.js';
import { supportsUnpopDisplay } from './squid-unpop.js';
import { runSquidActivate } from './raw-squid-activate.js';
import { RAW_POP_RESULT, runSquidPopParent } from './raw-squid-pop-parent.js';

// PE 에서 공통 firstPop/postPop 을 확인한 10.78 가상 표 주소 26칸이다. 호스트 포인터가 아니다. (2026-11-08 WB04-E1)
export const PATCH_POP_VTABLES = Object.freeze([
    0x00501dd0, 0x00502370, 0x00502448, 0x00502510, 0x005025d8, 0x005026a0, 0x00502778, 0x00502840, 0x00502908, 0x005029d0,
    0x00502aa8, 0x00502b70, 0x00502e68, 0x00502f30, 0x00502ff8, 0x005030c0, 0x00503188, 0x00503360, 0x00506e50, 0x005092d0,
    0x00509388, 0x005096a0, 0x00509758, 0x00509b98, 0x0050db78, 0x0050eb98,
]);

// 마나(103)·볼트(105) 표시와 134 의 실제 7c/80/84 쌍은 공통 공간 Pop 을 쓰는 별도 확인 표이다. (2026-11-08 WB04-E1)
const EXTRA_POP_VTABLES = new Set([0x50c3b0, 0x509810, 0x5070c0]);

// 슬롯의 VT DWORD 를 읽는다. (2026-11-08 WB04-E1)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 객체를 공간 해시와 점유 지도에 등록하는 Pop 처리기이다.
// 사용: const pop = new SquidPop(pool, hash, spots); pop.pop(sid, types[number], 1, 1, 20.5, 31.25, 0) === RAW_POP_RESULT.registered. (2026-11-08 WB04-E1)
export class SquidPop {
    // 같은 풀·해시·spot 지도이다.
    #pool;
    #hash;
    #spots;
    // 선택한 표시 효과와 공통 postPop 이다. null 은 표시 억제 계약이다.
    #display;
    #postPop;
    // 건물 옆면의 실제 표면 WORD/진단 몸체와 좌표를 쓴 직후 등록할 미니맵 dirty 큐이다.
    #building = null;
    #metadisplay = null;
    // 현재 가상 writer 와 방송이다(전송을 생략하는 flags 에서는 요구하지 않는다).
    #transmission = null;
    // 현재 진단 묶음: checking() 은 005e4794 의 현재 DWORD, types 는 188칸 전체 현재 표, 나머지는 선택 장치이다.
    #diagnostics = { checking: () => 0, types: null, frameSize: null, worldCells: null, metamap: null, trace: null, assertion: null };

    // 공유 spot 의 크기와 풀 소유자를 확인한다. 표시와 공통 postPop 일부 효과를 선택 연결한다.
    constructor(pool, hash, spots, { display = null, postPop = null } = {}) {
        if (pool?.edition !== 'patch1078' || !(hash instanceof SquidHash)) throw new TypeError('Pop 처리기의 10.78 풀/해시 오류');
        if (!(spots instanceof Uint8Array) || spots.length !== WORLD_CELLS * WORLD_CELLS) throw new RangeError('Pop spot 지도 크기 오류');
        if (postPop !== null && postPop.pool !== pool) throw new TypeError('Pop/postPop SID 풀이 다릅니다');
        this.#pool = pool;
        this.#hash = hash;
        this.#spots = spots;
        this.#display = display;
        this.#postPop = postPop;
        if (postPop !== null && typeof postPop.validateSpace === 'function') postPop.validateSpace(hash, spots);
    }

    // 같은 풀인지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 건물 옆면 효과를 같은 실제 공간에 연결한다. null 은 미연결 건물의 기존 거부 경로이다.
    setBuildingSpace(space) {
        if (space !== null) space.validateSpace(this.#pool, this.#hash);
        this.#building = space;
    }

    // 원본 표면/다리 통지를 같은 풀의 실제 미니맵 큐에 연결한다. null 은 표시 객체가 없는 기존 경로이다.
    setMetadisplayDirty(dirty) {
        if (dirty !== null && dirty?.pool !== this.#pool) throw new TypeError('Pop/미니맵 큐 SID 풀 불일치');
        this.#metadisplay = dirty;
    }

    // 같은 관측자 대여만 끊는다. 이후 연결한 새 미니맵의 대여는 유지한다.
    releaseMetadisplayDirty(expected) {
        if (!expected || this.#metadisplay !== expected) return false;
        this.#metadisplay = null;
        return true;
    }

    // 프레임 변경의 제자리 갱신과 재등록이 같은 풀/표시 대상을 쓰는지 확인한다(표시를 생략한 Pop 으로 프레임을 재등록하면 새 영역이 누락되므로 미연결 구성도 거부).
    validateFrameBinding(pool, display, unpop) {
        if (pool !== this.#pool || display !== this.#display) throw new TypeError('프레임 Pop 풀/표시 연결 불일치');
        unpop.validateSpace(pool, this.#hash, this.#spots);
    }

    // 실제 현재 writer 와 방송을 연결한다(도달할 때만 요구). 함수 또는 null.
    bindTransmission(transmission) {
        if (transmission !== null && typeof transmission !== 'function') throw new TypeError('Pop 전송은 (sid, flags) 함수여야 합니다.');
        this.#transmission = transmission;
    }

    // 같은 현재 TYPE/Client 진단과 선택적 지연 SHP 조회를 연결한다. types 가 있으면 188칸 전체 표를 빌려 매 조회에 현재 값을 읽는다.
    bindDiagnostics({ checking = () => 0, types = null, frameSize = null, worldCells = null, metamap = null, trace = null, assertion = null } = {}) {
        if (typeof checking !== 'function') throw new TypeError('Pop 진단 DWORD 공급자는 함수여야 합니다.');
        if (types !== null && (!Array.isArray(types) || types.length !== 188)) throw new RangeError('Pop 현재 TYPE의 판본/길이 오류');
        this.#diagnostics = { checking, types, frameSize, worldCells, metamap, trace, assertion };
    }

    // 원본 가상 주소를 메타데이터로 비교한다. 호스트 포인터로 호출하지 않는다. 사용: SquidPop.supports(vtable, flags).
    static supports(vtable, flags) {
        const table = vtable >>> 0;
        if (EXTRA_POP_VTABLES.has(table)) return supportsUnpopDisplay(table, flags);
        // 선택적으로 켠 표시도 공통 가상 경로만 지원한다.
        return PATCH_POP_VTABLES.includes(table) && supportsUnpopDisplay(table, flags);
    }

    // 실제 PATCH World 의 Pop: 사전 TYPE/SHP 조회 없이 같은 현재 표와 실제 표시기를 쓴다.
    popCurrent(sid, x, y, flags = 0) {
        return this.#popPatch(sid, x, y, flags, null, null);
    }

    // 004b02d0: 현재 SHP 프레임 크기로 단계를 정하고 실제 raw 공간 필드를 갱신한다. PATCH 는 도달한 지점에서 현재 TYPE/프레임/효과를 조회하며 앞선 부분 변경을 유지한다.
    pop(sid, type, frameWidth, frameHeight, x, y, flags = 0) {
        return this.#popPatch(sid, x, y, flags, type, [frameWidth, frameHeight]);
    }

    // 독립 명시 입력(fallback 레코드/치수)과 실제 현재 World 를 같은 원문 부모로 실행한다.
    #popPatch(sid, x, y, flags, fallback, size) {
        const pool = this.#pool;
        const diagnostics = this.#diagnostics;
        const hooks = {
            type: (number) => {
                if (diagnostics.types === null) {
                    if (!fallback) throw new Error('Pop 현재 전체 TYPE 미연결');
                    return fallback;
                }
                if (number >= diagnostics.types.length) throw new RangeError('Pop 현재 TYPE 번호 범위');
                return diagnostics.types[number];
            },
            frameSize: (object) => {
                if (diagnostics.frameSize) return diagnostics.frameSize(object);
                if (this.#display) {
                    const raw = pool.slot(object);
                    return this.#display.frameSize(raw, (raw[36] | (raw[37] << 8) | (raw[38] << 16) | (raw[39] << 24)) | 0);
                }
                // 독립 입력의 치수는 실제 SHP 장치가 없는 경우에만 사용한다.
                if (!fallback) throw new Error('Pop 현재 SHP 조회 미연결');
                return size;
            },
            // 실제 장기 Client getter 또는 독립 원문 입력만 읽는다.
            worldCells: () => (diagnostics.worldCells ? diagnostics.worldCells() : WORLD_CELLS),
            // 기존 독립 원문 fixture 는 00594fc8=1 의 무로그 입력이다.
            metamap: () => (diagnostics.metamap ? diagnostics.metamap() : true),
            trace: (level, text) => {
                if (!diagnostics.trace) throw new Error('Pop 실제 원문 로그 미연결');
                diagnostics.trace(level, text);
            },
            assertion: (expression, file, line) => {
                if (!diagnostics.assertion) throw new Error(`Pop 실제 원문 진단 미연결 ${line}`);
                diagnostics.assertion(expression, file, line);
            },
            // 실제 통지 helper 도 활성 큐가 없으면 자연 반환한다.
            miniMap: (object) => { if (this.#metadisplay) this.#metadisplay.enqueueSid(object); },
            side: (cx, cy, buildingX, buildingY) => {
                if (!this.#building) throw new Error('Pop 실제 건물 옆면 미연결');
                this.#building.popSide(cx, cy, buildingX, buildingY);
            },
            update: (object, requested) => {
                const raw = pool.slot(object);
                if (!supportsUnpopDisplay(vtableOf(raw), requested)) throw new Error('Pop 현재 파생 표시 갱신 미복원');
                if (this.#display) this.#display.update(raw, requested);
            },
            firstPop: (object) => {
                const raw = pool.slot(object);
                if (!SquidFactory.supportsConstructedVtable(vtableOf(raw))) throw new Error('Pop 현재 가상 firstPop 미복원');
                const extra = raw[40];
                // 보호 83 VT 의 +30 은 모두 004ad470 이다. TYPE 생성자 쌍을 강제하지 않는다.
                return (extra & 9) === 0 ? 1 : (((extra & 1) ? 2 : 0) | ((extra & 8) ? 4 : 0));
            },
            activate: (object, requested) => {
                // 독립 fixture 의 명시 억제 모드만 지역 깊이를 쓴다.
                const depth = this.#postPop ? this.#postPop.activationDepth : { value: 0 };
                runSquidActivate(pool, object, requested, depth, {
                    server: () => pool.isServer,
                    assertion: hooks.assertion,
                    transmit: (target, sendFlags) => {
                        const send = this.#transmission;
                        if (!send) throw new Error('현재 Squid Pop의 실제 전송 미연결');
                        send(target, sendFlags);
                    },
                    postPop: (target, postFlags) => {
                        if (this.#postPop) this.#postPop.postPopVirtual(target, postFlags);
                        else {
                            // 표시/장부를 생략한 기존 독립 구성의 억제 계약이다.
                            if (!SquidPop.supports(vtableOf(pool.slot(target)), postFlags)) throw new Error('Pop 현재 가상 후처리 미복원');
                            depth.value = (depth.value - 1) >>> 0;
                        }
                    },
                });
            },
        };
        return runSquidPopParent(pool, this.#hash, this.#spots, sid, x, y, flags, diagnostics.checking, hooks);
    }
}

export { RAW_POP_RESULT };
