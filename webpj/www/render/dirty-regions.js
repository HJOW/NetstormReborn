// 원본 변경 사각형 표(Renderer 00497580 Add·00499fe0 Clear 계열)를 옮긴 모듈이다(cpppj `client/Renderer.cpp/.h` 의 DirtyRegions 부분, 10.78 전용).
// 부분 갱신 사각형을 모아 화면 경계 안의 항목만 받고, 겹치거나 가까운 항목은 합집합으로 합친다(합친 면적의 1/8 허용치). 100항목이 넘으면 전체 다시 그리기로 넘긴다.
// 원본은 병합 전에 사각형을 화면에 자르지 않는다(그리기·표시 때 자른다). 항목 플래그 1(버려짐)·4(배경을 다시 지우지 않는 불투명 영역)를 보존한다.
// 이 모듈은 DOM 을 쓰지 않으며, 화면 합성·실제 그리기(Renderer 의 나머지)는 월드 표시 단위에서 옮긴다. (2026-11-08 WB04-E59: Squid 표시 이식의 변경 표 수집을 위해 추가)

// 원본 변경 사각형 표의 최대 개수(this+0x68 의 사각형, this+0x6a8 의 플래그)이다. (2026-11-08 WB04-E59)
export const DIRTY_CAPACITY = 100;

// 원본 변경 표 플래그: 1 은 병합으로 버려진 항목, 4 는 배경을 다시 지우지 않는 불투명 영역이다. (2026-11-08 WB04-E59)
export const DIRTY_DISCARDED = 1;
export const DIRTY_OPAQUE = 4;

// 양 끝을 포함하지 않는 사각형 면적이다(큰 좌표에도 안전하도록 BigInt 가 아니라 2^53 안의 수로 계산한다). (2026-11-08 WB04-E59)
function area(rect) {
    return (rect.right - rect.left) * (rect.bottom - rect.top);
}

// 두 변경 영역의 합집합 상자이다. (2026-11-08 WB04-E59)
function union(a, b) {
    return {
        left: Math.min(a.left, b.left),
        top: Math.min(a.top, b.top),
        right: Math.max(a.right, b.right),
        bottom: Math.max(a.bottom, b.bottom)
    };
}

// 부분 갱신 사각형을 수집하고 전체 갱신 여부를 관리한다. 사용: const dirty = new DirtyRegions(640, 480); dirty.add({left, top, right, bottom}, flags);
//   dirty.entries → [{rect, flags}] 원본 순서, dirty.fullRedraw → 넘침/전체 변화 여부. (2026-11-08 WB04-E59)
export class DirtyRegions {
    // 화면 가로/세로와 전체 다시 그리기 표시, 원본 순서의 항목 표, Clear 가 남기는 첫 항목 플래그이다.
    #width;
    #height;
    #full = false;
    #entries = [];
    #clearedFirstFlags = 0;

    // 양수 화면 크기만 받는다. (2026-11-08 WB04-E59)
    constructor(width, height) {
        if (!(width > 0) || !(height > 0)) throw new RangeError('변경 사각형 화면 크기는 양수여야 합니다.');
        this.#width = width;
        this.#height = height;
    }

    // 00497580: 화면 밖 사각형은 무시하고, 100항목이 차면 전체 갱신으로 바꾼다. 버려지지 않은 첫 병합 대상에서 끝낸다(뒤 항목과 다시 합치지 않는다). (2026-11-08 WB04-E59)
    add(rect, flags = 0) {
        if (!(rect.left < this.#width && rect.top < this.#height && rect.right > 0 && rect.bottom > 0)) return;
        if (this.#entries.length >= DIRTY_CAPACITY) {
            this.#full = true;
            return;
        }
        const newArea = area(rect);
        // 원본 순서대로 버려지지 않은 항목과 병합을 시도한다.
        for (const entry of this.#entries) {
            if ((entry.flags & DIRTY_DISCARDED) !== 0) continue;
            const oldArea = area(entry.rect);
            const united = union(entry.rect, rect);
            const unitedArea = area(united);
            if ((flags & DIRTY_OPAQUE) !== 0 && unitedArea === newArea) {
                entry.flags = (entry.flags | DIRTY_DISCARDED) >>> 0;
            } else {
                if ((entry.flags & DIRTY_OPAQUE) !== 0 && unitedArea === oldArea) return;
                if (unitedArea - Math.trunc(unitedArea / 8) < oldArea + newArea) {
                    entry.rect = united;
                    return;
                }
            }
        }
        this.#entries.push({ rect: { left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }, flags: (flags & ~DIRTY_DISCARDED) >>> 0 });
    }

    // 화면 모드 변경 등 전체 다시 그리기를 요청한다. (2026-11-08 WB04-E59)
    invalidateAll() {
        this.#full = true;
    }

    // 00499fe0 의 출력 뒤 표 초기화이다: 첫 항목의 플래그를 남기고 전체 표시와 항목을 지운다. 화면 크기는 유지한다. (2026-11-08 WB04-E59)
    clear() {
        if (this.#entries.length > 0) this.#clearedFirstFlags = this.#entries[0].flags;
        this.#full = false;
        // 497960 같은 호출자가 빌린 표(cameraEntries)의 배열 소유자를 유지하도록 새 배열로 바꾸지 않고 비운다. (2026-11-13 WB04-E391: C++ vector clear 의미)
        this.#entries.length = 0;
    }

    // 원본 순서의 항목 표이다(읽기 전용 목적). (2026-11-08 WB04-E59)
    get entries() {
        return this.#entries;
    }

    // 497960(카메라 이동)·Renderer.draw 가 현재 표를 직접 이동/확장하도록 빌려 주는 살아 있는 배열이다. add/clear 뒤에도 같은 배열이다. (2026-11-13 WB04-E391: Renderer 이식을 위해 추가)
    cameraEntries() {
        return this.#entries;
    }

    // 전체 화면 변화나 100항목 넘침이 등록됐는지 읽는다. (2026-11-08 WB04-E59)
    get fullRedraw() {
        return this.#full;
    }

    // 항목이 없어도 남는 첫 flags DWORD 이다(원문 전체 출력의 무시 비트 보존). (2026-11-08 WB04-E59)
    get firstFlags() {
        return this.#entries.length === 0 ? this.#clearedFirstFlags : this.#entries[0].flags;
    }
}
