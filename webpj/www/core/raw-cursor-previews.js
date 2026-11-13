// 미리보기 생성445b30의 생성/재사용 인덱스·다리 연결 꼬리·좌표 보정을 옮긴다(cpppj client/RawCursorPreviews.cpp/.h, 10.78).
// 사용: new RawCursorPreviews(pool, types, drop, lifecycle, surfaces, hooks).create(). 같은30DWORD 목록과 실제 raw를 빌리며 공간 Pop은 하지 않는다.
// 생성 모드의 비활성 DWORD는 SID로 읽기 전에 실제 Append에서 덮어쓴다. 연결/가상 효과 뒤 현재 개수·TYPE·소유자를 다시 읽는다.
// 이 모듈은 DOM을 쓰지 않는다. (2026-11-14 WB04-E403: 5-A 커서 미리보기 생성)
import { cursorPreviewSid } from './raw-cursor-lifecycle.js';
import { cameraFtolLow } from '../render/camera-view.js';
import { setRawSquidHitPoints } from './raw-squid-word.js';

// 선택된 가상/호스트 경계의 누락은 성공으로 대체하지 않는다.
function need(effect) { if (typeof effect !== 'function') throw new Error('커서 미리보기 의존 동작 누락'); return effect; }
// 같은 raw의 float32 위치를 little endian으로 읽는다.
const readFloat = (raw, offset) => new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
// 지정 DWORD/WORD만 써 이웃 바이트를 보존한다.
function put(raw, offset, value, width = 4) {
    // 낮은 바이트부터 원래 폭을 유지한다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 255;
}

// 수명·내려놓기·배치와 같은 풀/TYPE/목록에서 실제 미리보기 raw를 생성 또는 재사용한다.
export class RawCursorPreviews {
    // 단일 월드의 실제 참조와 가상/Canon 경계다.
    #pool; #types; #drop; #life; #surfaces; #hooks;

    // 판본만 먼저 확인하고 선택된 장치는 소비할 때 요구한다.
    constructor(pool, types, drop, lifecycle, surfaces, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('커서 미리보기10.78 판본 계약 오류');
        this.#pool = pool; this.#types = types; this.#drop = drop; this.#life = lifecycle; this.#surfaces = surfaces; this.#hooks = hooks;
    }

    // 어댑터의 같은 풀 계약을 확인한다.
    get pool() { return this.#pool; }

    // 생성/가상 효과 뒤 현재 abstract TYPE를 다시 조회한다.
    #type() {
        const type = this.#life.abstractType >>> 0;
        if (type >= this.#types.length) throw new RangeError('커서 미리보기 abstract TYPE 범위');
        return this.#types[type];
    }

    // 목록 인덱스를 확인하되 기존 DWORD를 SID16으로 줄이지 않는다.
    #at(index) {
        if (index < 0 || index >= this.#life.previews.length) throw new RangeError('커서 미리보기 목록 인덱스');
        return this.#life.previews[index];
    }

    // 생성 효과 뒤 현재 count를 읽어 같은 목록에 쓴다. 비활성 칸의 큰 DWORD는 여기서 덮어쓴다.
    #append(sid) {
        const index = this.#life.previewCount | 0; this.#at(index); this.#life.previews[index] = sid;
        this.#life.previewCount = (this.#life.previewCount + 1) | 0;
    }

    // float32 위치와 원문 +0.5 뒤0 방향 절삭의 화면 WORD 캐시를 같은 raw에 쓴다.
    #position(sid, x, y) {
        x = Math.fround(x); y = Math.fround(y);
        const raw = this.#pool.allocated(sid); const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        view.setFloat32(14, x, true); view.setFloat32(18, y, true);
        put(raw, 22, cameraFtolLow(x * 16 + 0.5) & 0xffff, 2); put(raw, 24, cameraFtolLow(y * 11 + 0.5) & 0xffff, 2);
    }

    // 첫 미리보기 유무를 Query 뒤 한 번 캡처하고, 실제 Canon 칸/연결 꼬리/받침 순서로 처리한다.
    create() {
        const d = this.#drop; const l = this.#life; const h = this.#hooks;
        need(h.queryCursor)(); const create = !l.previews[0]; let ordinal = 0;
        need(h.walk)({ type: d.type >>> 0, argument: d.argument | 0, direction: d.direction | 0, x: Math.fround(d.x), y: Math.fround(d.y), explicitFrame: false }, (cell) => {
            const number = this.#at(ordinal++); let sid = create ? 0 : cursorPreviewSid(number);
            if (create) { sid = need(h.create)(d.type >>> 0, 2); this.#append(sid); }
            let raw = this.#pool.allocated(sid); raw[40] |= 8; put(raw, 36, cell.frame | 0);
            need(h.setWord)(sid, d.argument >>> 0); need(h.setOwner)(sid, d.localPlayer >>> 0);
            if (need(h.hasHitPoints)(sid)) need(h.setHitPoints)(sid, 0);
            this.#position(sid, cell.x, cell.y);
            if (this.#type().flags2 & 4) {
                const quality = this.#pool.slot(cursorPreviewSid(d.preview))[30]; const qualityFlags = quality === 1 ? 0x20 : quality === 3 ? 0x40 : 0;
                const frames = need(h.frames)(this.#surfaces.bridgeType >>> 0);
                if (cell.frame < 0 || cell.frame >= frames.codes.length) throw new RangeError('커서 다리 코드 프레임 범위');
                const code = frames.codes[cell.frame]; const frame = frames.findFlags(code.side, code.variant, ((code.flags << 24) >> 24) | qualityFlags);
                raw = this.#pool.allocated(sid); put(raw, 36, frame);
                const oldX = readFloat(raw, 14); const oldY = readFloat(raw, 18);
                const x = Math.fround(Math.floor(oldX + 0.5)); const y = Math.fround(Math.floor(oldY + 0.5));
                const deltaX = Math.fround(x - oldX); const deltaY = Math.fround(y - oldY); this.#position(sid, x, y);
                if ((l.previewCount | 0) < 0 || (l.previewCount | 0) > 30) throw new RangeError('커서 연결 객체 꼬리 범위');
                const count = need(h.connect)(sid, l.previews.subarray(l.previewCount | 0)) | 0;
                // 연결 객체는 원문 정수 맞춤 차이만 되돌린다. count 진단은 위치 복구 뒤에 수행한다.
                for (let index = 0; index < count; index += 1) {
                    const connector = cursorPreviewSid(this.#at(((l.previewCount | 0) + index) | 0)); const data = this.#pool.slot(connector);
                    const px = Math.fround(readFloat(data, 14) - deltaX); const py = Math.fround(readFloat(data, 18) - deltaY); this.#position(connector, px, py);
                }
                this.#position(sid, oldX, oldY);
                if (count > 6) throw new Error('sidCount <= NUM_CONNECTORS 원본 진단');
                l.previewCount = (l.previewCount + count) | 0;
            }
            if ((l.previewCount | 0) >= 30) throw new Error('cursorNumSids < MAX_CURSOR_SIDS 원본 진단');
            return true;
        });
        if (this.#type().flags1 & 0x400) {
            // 받침 재사용 인덱스에는 연결 객체 개수가 아니라 실제 Canon 칸 수만 더한다.
            for (let part = 0; part < 2; part += 1) {
                const number = this.#at(ordinal++); let sid = create ? 0 : cursorPreviewSid(number);
                if (create) { sid = need(h.create)(part ? this.#surfaces.stalagType >>> 0 : this.#surfaces.islandType >>> 0, 2); this.#append(sid); }
                const raw = this.#pool.allocated(sid); raw[40] |= 8; put(raw, 36, 8); this.#position(sid, d.x, d.y);
            }
        }
        need(h.prepareCursor)();
    }
}

// Take의 생성 경계만 같은 실제 몸체에 연결한다.
export function bindCursorPreviews(lifecycle, previews) { lifecycle.createPreviews = () => previews.create(); }

// 같은 위치 판정/준비와 이미 연결된 Canon 공급자를 공유한다.
export function bindCursorPreviewPlacement(hooks, pool, placement, placementHooks) {
    if (placement?.pool !== pool || typeof placementHooks.walk !== 'function') throw new TypeError('커서 미리보기/위치 단일 풀 또는 Canon 연결 오류');
    hooks.queryCursor = () => placement.query(); hooks.prepareCursor = () => placement.prepare(); hooks.walk = placementHooks.walk;
}

// 실제 Factory·HP·다리 연결과 현재 프레임 표를 연결한다. 가상 소유자 지정은 호출자 장치를 보존한다.
export function bindCursorPreviewWorld(hooks, pool, factory, reward, connect, frames, surfaces, notifySurface) {
    if (pool?.edition !== 'patch1078' || factory?.pool !== pool || reward?.pool !== pool || connect?.pool !== pool) throw new TypeError('커서 미리보기 실제 월드 단일 풀 계약 오류');
    hooks.create = (type, flags) => factory.create(type, flags);
    // 현재 다리 TYPE일 때만 실제 표면 통지 장치를 요구한다.
    hooks.setWord = (sid, value) => { const raw = pool.allocated(sid); put(raw, 12, value, 2); if (raw[10] === (surfaces.bridgeType >>> 0)) need(notifySurface)(sid); };
    hooks.hasHitPoints = (sid) => reward.hasHitPoints(sid); hooks.setHitPoints = (sid, value) => setRawSquidHitPoints(pool, sid, value);
    // 현재 bridge TYPE의 실제 FrameCodeTable만 반환한다.
    hooks.frames = (type) => { if ((type >>> 0) >= frames.length) throw new RangeError('커서 실제 bridge 프레임 표 범위'); return frames[type >>> 0].frames; };
    // 실제 연결기가 만든 SID를 같은32비트 꼬리에 바로 쓴다.
    hooks.connect = (sid, output) => {
        let count = 0;
        connect.connectPreview(sid, (born) => { if (count >= output.length) throw new RangeError('커서 실제 연결 객체 꼬리 넘침'); output[count++] = born; });
        return count;
    };
}
