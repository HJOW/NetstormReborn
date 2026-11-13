// 공통 kind2 UI Draw의 현재 클립/가상 출력과8비트 픽셀 장치를 옮긴다(cpppj client/GumpDraw.cpp/.h, 10.78).
// 사용: new GumpDraw(collection,state,{draw(token)}).draw(token). 임시 클립은 예외에서도 복구하고 색/하위 상태는 유지한다.
// GumpPixelTarget은 같은 현재 VFX inclusive 클립으로 선/그림/원시 bitmap/배율/채우기를 출력한다. DOM을 쓰지 않는다. (2026-11-14 WB04-E405)
import { clipRect, drawIndexedImage } from './indexed-surface.js';
import { drawVfxLinePixels } from './vfx-line.js';
import { drawScaledVfxImage } from './vfx-transform.js';

// 이 플래그만 임시 클립을 요청한다. visible/dead/자식 재귀 조건을 추가하지 않는다.
export const GUMP_DRAW_CLIP = 0x00400000;
// 실제 signed64 소스 주소 범위다. bitmap 이동량은 BigInt로 계산한다.
const MIN_I64 = -(1n << 63n); const MAX_I64 = (1n << 63n) - 1n;

// UI의 half-open 논리 영역과 VFX의 양 끝 포함 영역이다.
export class GumpDrawState {
    // 두 영역의 주소를 유지하며 setter로 좌표만 바꾼다.
    clip = { left: 0, top: 0, right: 0, bottom: 0 }; vfxClip = { left: 0, top: 0, right: 0, bottom: 0 };
}

// 빈/역전 입력도 그대로 저장하고 right/bottom-1의 DWORD 감김을 유지한다.
export function setGumpDrawClip(state, clip) {
    Object.assign(state.clip, { left: clip.left | 0, top: clip.top | 0, right: clip.right | 0, bottom: clip.bottom | 0 });
    Object.assign(state.vfxClip, { left: clip.left | 0, top: clip.top | 0, right: (clip.right - 1) | 0, bottom: (clip.bottom - 1) | 0 });
}

// C++ ClipGump의 범위를 명시 dispose/try-finally로 복구한다. 한 보호 객체는 한 번만 복구한다.
export class GumpClipScope {
    // 현재 공유 상태와 bounds 조회 뒤 저장한 논리 영역이다.
    #state; #saved; #active = true;
    // 네 좌표를 독립 교차하며 빈 영역도 설정한다.
    constructor(state, bounds) { this.#state = state; this.#saved = { ...state.clip }; setGumpDrawClip(state, clipRect(bounds, this.#saved)); }
    // 출력 중 바뀐 현재 영역 대신 저장한 논리 영역으로 두 전역을 복구한다.
    dispose() { if (this.#active) { this.#active = false; setGumpDrawClip(this.#state, this.#saved); } }
}

// 가상 UI 출력과 현재 수집 트리를 같은 상태에 묶는다.
export class GumpDraw {
    // 현재 트리/논리·VFX 클립/가상 효과다.
    #collection; #state; #hooks;
    // 가상 출력 누락은 일부 클립 쓰기 전에 거부한다.
    constructor(collection, state, hooks) { if (typeof hooks?.draw !== 'function') throw new TypeError('Gump 가상 Draw 필요'); this.#collection = collection; this.#state = state; this.#hooks = hooks; }
    // 보호 없는 경로는 위치/캐시를 읽지 않는다. Bounds 투영 뒤 현재 클립을 저장한다.
    draw(token) {
        if (this.#collection.flags(token) & GUMP_DRAW_CLIP) {
            const bounds = this.#collection.bounds(token); const scope = new GumpClipScope(this.#state, bounds);
            try { this.#hooks.draw(token); } finally { scope.dispose(); }
            return;
        }
        this.#hooks.draw(token);
    }
}

// 공통 kind2 분배는 토큰을 같은 현재 트리의 실제 Draw에 전달한다.
export function bindGumpDraw(hooks, draw) { hooks.ui = (token) => draw.draw(token); }

// 버튼/글자/SHP가 같은 실제8비트 표면과 현재 VFX 클립을 공유하는 장치다.
export class GumpPixelTarget {
    // 출력 표면·행 간격·높이와 현재 클립 상태다.
    #pixels; #pitch; #height; #state;
    // 실제 표면 크기를 넘는 행 주소를 생성 전에 거부한다.
    constructor(pixels, pitch, height, state) {
        if (pitch < 1 || height < 1 || pitch * height > pixels.length) throw new TypeError('Gump 픽셀 표면 용량 오류');
        this.#pixels = pixels; this.#pitch = pitch; this.#height = height; this.#state = state;
    }
    // inclusive 끝점+1을 감기지 않는 수로 계산하고 각 좌표를 실제 표면 안으로 제한한다.
    #clip() {
        const c = this.#state.vfxClip; const clamp = (value, limit) => Math.max(0, Math.min(value, limit));
        return { left: clamp(c.left, this.#pitch), top: clamp(c.top, this.#height), right: clamp(c.right + 1, this.#pitch), bottom: clamp(c.bottom + 1, this.#height) };
    }
    // 직교 선은 양 끝 포함 BYTE 채우기, 대각선은 실제 VFX 분수/클립을 사용한다.
    lines(lines) {
        // 입력 순서를 유지하며 매 선에서 현재 클립을 다시 읽는다.
        for (const line of lines) {
            if (line.first.x !== line.last.x && line.first.y !== line.last.y) { drawVfxLinePixels(this.#pixels, this.#pitch, this.#height, this.#clip(), line.first, line.last, line.color); continue; }
            const clip = this.#clip(); const left = Math.max(clip.left, Math.min(line.first.x, line.last.x)); const top = Math.max(clip.top, Math.min(line.first.y, line.last.y));
            const right = Math.min(clip.right, Math.max(line.first.x, line.last.x) + 1); const bottom = Math.min(clip.bottom, Math.max(line.first.y, line.last.y) + 1);
            if (left >= right || top >= bottom) continue;
            // 유효 행의 연속 픽셀만 하위BYTE 색으로 채운다.
            for (let y = top; y < bottom; y += 1) this.#pixels.fill(line.color & 255, y * this.#pitch + left, y * this.#pitch + right);
        }
    }
    // 압축 해제한 그림의 투명도와 선택적 색 변환을 기존 합성기로 출력한다.
    image(image, position, colors = null) {
        // 기존 웹 합성기의 낮은 경계에 없는 Renderer의 원본 이미지 용량 검사를 먼저 적용한다.
        if (!Number.isInteger(image.width) || !Number.isInteger(image.height) || image.width < 0 || image.height < 0 || image.width > 0x7fffffff || image.height > 0x7fffffff
            || image.width * image.height !== image.indices.length || image.opacity.length !== image.indices.length) throw new TypeError('Invalid renderer image');
        drawIndexedImage(this.#pixels, this.#pitch, this.#height, image, position.x, position.y, this.#clip(), colors);
    }
    // 공유 전역은 바꾸지 않고 전체 Gump bounds와 현재 표면 클립의 교집합을 빌린다.
    linesClipped(lines, bounds) {
        const clipped = new GumpDrawState(); Object.assign(clipped.clip, this.#state.clip); Object.assign(clipped.vfxClip, this.#state.vfxClip);
        setGumpDrawClip(clipped, clipRect(bounds, this.#clip())); new GumpPixelTarget(this.#pixels, this.#pitch, this.#height, clipped).lines(lines);
    }
    // sourceBase는 signed64 BigInt/정확한 정수다. outside는 실제로 보이는 할당 밖 BYTE 주소(BigInt)만 공급한다.
    rawBitmap(source, stride, sourceBase, size, destination, bounds, outside = null) {
        if (stride <= 0) throw new TypeError('원시 bitmap의 양수 행 간격 필요');
        if (size.x <= 0 || size.y <= 0) return;
        if (typeof sourceBase !== 'bigint' && !Number.isSafeInteger(sourceBase)) throw new TypeError('원시 bitmap 소스 주소의 정확한 정수 필요');
        const base = BigInt(sourceBase); if (base < MIN_I64 || base > MAX_I64) throw new RangeError('원시 bitmap signed64 소스 주소 범위');
        const clip = clipRect(bounds, this.#clip()); const left = Math.max(clip.left, destination.x); const top = Math.max(clip.top, destination.y);
        const right = Math.min(clip.right, destination.x + size.x); const bottom = Math.min(clip.bottom, destination.y + size.y);
        // 대상이 보이는 행만 읽으며 숨은 소스 guard를 미리 읽지 않는다.
        for (let y = top; y < bottom; y += 1) {
            // 평면 X가 음수여도 최종 주소가 할당 안이면 읽는다.
            for (let x = left; x < right; x += 1) {
                const delta = BigInt(y - destination.y) * BigInt(stride) + BigInt(x - destination.x);
                if (base > MAX_I64 - delta) throw new RangeError('원시 bitmap 소스 주소 넘침');
                const offset = base + delta; let value;
                if (offset >= 0n && offset < BigInt(source.length)) value = source[Number(offset)];
                else {
                    if (typeof outside !== 'function') throw new RangeError(`원시 bitmap의 보이는 할당 밖 BYTE 공급 필요: 주소=${offset}, 할당=${source.length}, 시작=${base}, 크기=${size.x}x${size.y}, 목적=${destination.x},${destination.y}`);
                    value = outside(offset);
                }
                this.#pixels[y * this.#pitch + x] = value;
            }
        }
    }
    // signed16.16 직교 배율도 같은 현재 클립/대상 저장소로 출력한다.
    scaledImage(image, frame, position, scaleX, scaleY, colors = null) { drawScaledVfxImage(this.#pixels, this.#pitch, this.#height, image, frame, position, scaleX, scaleY, this.#clip(), colors); }
    // 반개방 사각형과 절대 좌표의 홀짝 체크 무늬를 현재 클립 안에 채운다.
    fill(rect, color, checker = false) {
        const v = clipRect(rect, this.#clip()); if (v.left >= v.right || v.top >= v.bottom) return;
        // 유효 행마다 연속 또는 체크 픽셀만 출력한다.
        for (let y = v.top; y < v.bottom; y += 1) {
            const row = y * this.#pitch;
            if (!checker) this.#pixels.fill(color & 255, row + v.left, row + v.right);
            else {
                // 잘린 영역에서도 원본 화면 위상을 유지한다.
                for (let x = v.left; x < v.right; x += 1) if ((x ^ y) & 1) this.#pixels[row + x] = color & 255;
            }
        }
    }
}
