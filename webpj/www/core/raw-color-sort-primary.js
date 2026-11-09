// ColorSort primary(439de0)의 raw 56바이트 × 256 기록을 소유하고 최초 명도 double 을 재계산 없이 빌려 주는 모듈이다(cpppj `o/RawColorSortPrimary.cpp/.h`, 10.78 기본 53비트/nearest).
//  - new RawColorSortPrimary(allocationBytes): 원본 malloc 직후의 실제 초기 0x3800 바이트를 그대로 복사해 소유한다. 미작성 padding 을 0 으로 채우지 않는다.
//  - construct(logicalRgbx, hooks): 첫 1024바이트 RGBX 를 현재 읽는다(같은 저장소 alias 가 가능하므로 사본을 만들지 않는다).
//    1) 첫 전체 순회는 항목마다 R(+24)→G(+32)→번호(+48 DWORD, B 바이트를 먼저 캡처)→B(+40) 순서로 RGB×(1/255 근사 상수)를 저장한다.
//    2) 둘째 전체 순회는 저장된 R/G/B double 에서 H(+0)·L(+8)·S(+16)를 binary64 원문 연산 순서로 계산해 저장한다(회색은 S=0·H=-1).
//    hooks.checkpoint(누적수, offset, 크기) 는 진입 때 (0,0,0), 각 store 완료 뒤 부른다. 콜백 예외는 이미 쓴 바이트를 되돌리지 않고 그대로 전파한다(부분 효과 관측).
//  - bytes: 같은 raw 저장소(Uint8Array, 주소 고정)이다. constructed: 부모가 성공 반환한 기록만 읽게 하는 host 표식이다.
//  - lightness(index): 완료한 +index*56+8 의 double 비트를 그대로 읽는다(RGB 재계산 아님). 미완료면 Error, 번호가 범위 밖이면 RangeError 다.
//  - x87 64비트(037f) 정밀도 결과는 지원하지 않는다(cpppj 와 같이 027f/53비트만).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E205: ColorSort primary 이식으로 추가)

// 실제 ColorSort 항목 수·항목 크기·전체 할당 바이트 수이다. (2026-11-09 WB04-E205)
export const COLOR_SORT_PRIMARY_COLORS = 256;
export const COLOR_SORT_PRIMARY_RECORD_BYTES = 56;
export const COLOR_SORT_PRIMARY_BYTES = COLOR_SORT_PRIMARY_COLORS * COLOR_SORT_PRIMARY_RECORD_BYTES;

// 원문 503608 의 실제 binary64 정규화 값(0x3f70101010101010)이다. (2026-11-09 WB04-E205)
const RGB_UNIT = (() => {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, 0x10101010, true);
    view.setUint32(4, 0x3f701010, true);
    return view.getFloat64(0, true);
})();

// 56바이트 기록의 H/L/S/R/G/B/번호 위치이다. (2026-11-09 WB04-E205)
const HUE_AT = 0;
const LIGHTNESS_AT = 8;
const SATURATION_AT = 16;
const RED_AT = 24;
const GREEN_AT = 32;
const BLUE_AT = 40;
const INDEX_AT = 48;

// raw 56바이트 × 256 기록의 소유자이다. 사용: const primary = new RawColorSortPrimary(initial); primary.construct(rgbx); primary.lightness(7). (2026-11-09 WB04-E205)
export class RawColorSortPrimary {
    // 정확 0x3800 payload 와 같은 버퍼의 little endian 보기, host 완료 표식이다.
    #bytes;
    #view;
    #constructed = false;

    // 실제 할당 초기값만 복사하며 이 단계에서 원문 생성자를 먼저 실행한 것으로 세지 않는다. (2026-11-09 WB04-E205)
    constructor(allocationBytes) {
        if (!allocationBytes || allocationBytes.length !== COLOR_SORT_PRIMARY_BYTES) throw new TypeError('ColorSort primary 초기 0x3800 BYTE 계약');
        this.#bytes = Uint8Array.from(allocationBytes);
        this.#view = new DataView(this.#bytes.buffer);
    }

    // 실제 초기 할당·alias·부분 효과를 위한 같은 raw 저장소이다(주소가 바뀌지 않는다). (2026-11-09 WB04-E205)
    get bytes() {
        return this.#bytes;
    }

    // host 표식이며 raw payload 에 추가 DWORD 를 쓰지 않는다. (2026-11-09 WB04-E205)
    get constructed() {
        return this.#constructed;
    }

    // 원문 8바이트 store 하나를 끝낸 뒤 현재 관측 경계를 부른다. (2026-11-09 WB04-E205)
    #storeDouble(offset, value, counter, hooks) {
        this.#view.setFloat64(offset, value, true);
        counter.writes += 1;
        hooks.checkpoint?.(counter.writes, offset, 8);
    }

    // +48 의 원래 4바이트 번호 store 이며 인접 +52 padding 은 건드리지 않는다. (2026-11-09 WB04-E205)
    #storeIndex(offset, value, counter, hooks) {
        this.#view.setUint32(offset, value >>> 0, true);
        counter.writes += 1;
        hooks.checkpoint?.(counter.writes, offset, 4);
    }

    // 기본 53비트/nearest 의 439de0 이다. 첫 1024바이트 RGBX 를 매번 현재 값으로 읽어 raw 와의 실제 alias 를 보존한다. (2026-11-09 WB04-E205)
    construct(logicalRgbx, hooks = {}) {
        if (!logicalRgbx || logicalRgbx.length < COLOR_SORT_PRIMARY_COLORS * 4) throw new TypeError('ColorSort primary 논리 RGBX 1024 BYTE 계약');
        this.#constructed = false;
        const counter = { writes: 0 };
        hooks.checkpoint?.(0, 0, 0);
        // 원문 첫 전체 순회: R → G → 번호 → B 의 실제 저장 순서다.
        for (let index = 0; index < COLOR_SORT_PRIMARY_COLORS; index += 1) {
            const base = index * COLOR_SORT_PRIMARY_RECORD_BYTES;
            this.#storeDouble(base + RED_AT, logicalRgbx[index * 4] * RGB_UNIT, counter, hooks);
            this.#storeDouble(base + GREEN_AT, logicalRgbx[index * 4 + 1] * RGB_UNIT, counter, hooks);
            // 원문은 번호를 쓰기 전에 이 바이트를 캡처한다.
            const blue = logicalRgbx[index * 4 + 2];
            this.#storeIndex(base + INDEX_AT, index, counter, hooks);
            this.#storeDouble(base + BLUE_AT, blue * RGB_UNIT, counter, hooks);
        }
        // 원문 둘째 전체 순회는 저장된 같은 R/G/B 에서 H/L/S 를 만든다.
        for (let index = 0; index < COLOR_SORT_PRIMARY_COLORS; index += 1) {
            const base = index * COLOR_SORT_PRIMARY_RECORD_BYTES;
            const red = this.#view.getFloat64(base + RED_AT, true);
            const green = this.#view.getFloat64(base + GREEN_AT, true);
            const blue = this.#view.getFloat64(base + BLUE_AT, true);
            const maximum = Math.max(red, green, blue);
            const minimum = Math.min(red, green, blue);
            // 원문 local_18 에 먼저 저장한 합이다.
            const sum = minimum + maximum;
            const lightness = sum * 0.5;
            let saturation = 0.0;
            let hue = -1.0;
            if (maximum !== minimum) {
                const delta = maximum - minimum;
                // 원문 high-lightness 뺄셈 순서를 보존한다.
                const denominator = lightness <= 0.5 ? sum : (2.0 - maximum) - minimum;
                saturation = delta / denominator;
                if (red === maximum) hue = (green - blue) / delta;
                else if (green === maximum) hue = (blue - red) / delta + 2.0;
                else hue = (red - green) / delta + 4.0;
                hue *= 60.0;
                if (hue < 0.0) hue += 360.0;
            }
            this.#storeDouble(base + HUE_AT, hue, counter, hooks);
            this.#storeDouble(base + LIGHTNESS_AT, lightness, counter, hooks);
            this.#storeDouble(base + SATURATION_AT, saturation, counter, hooks);
        }
        this.#constructed = true;
    }

    // 같은 저장 명도를 읽는 소비자는 owner 를 유지해야 하며 미완성 생성을 성공처럼 읽지 않는다. (2026-11-09 WB04-E205)
    lightness(index) {
        if (!this.#constructed) throw new Error('ColorSort primary 생성 미완료');
        if (!Number.isInteger(index) || index < 0 || index >= COLOR_SORT_PRIMARY_COLORS) throw new RangeError('ColorSort primary 명도 번호');
        return this.#view.getFloat64(index * COLOR_SORT_PRIMARY_RECORD_BYTES + LIGHTNESS_AT, true);
    }
}
