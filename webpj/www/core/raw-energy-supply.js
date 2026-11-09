// Mana.cpp 의 에너지 공급원 수집·요구 문자 대응·정렬·소비를 단일 월드에 적용하는 모듈이다(cpppj `o/RawEnergySupply.cpp/.h`, 10.78 전용, 원문 4730c0·473330·4734d0·473570·474970과 두 공급 가상 몸체 41c0f0/41c170).
// 문자 규칙: 공급원 TYPE 의 theme(0..7)이 원소이고 문자표는 "wrtsWRTS.?>"(바로 뒤 NUL 도 11번 칸)이다. 수집 문자열은 0..5 원소만 최대 254자 출력하며 6/7 공급원은 선택 목록에는 남아도 문구에는 없다.
//  - collect(x, y, type, owner, theme=-1): 대상 사각형(40e880/40e8f0: 들어온 모서리만 [1,255] 클램프, 양 끝점이 유효하면 min/max)에서 공급 목록을 순회해 소유자(편집기이거나 같은 소유자이거나 동맹 표 [공급 소유자*9+요청 소유자])와 공급 가상(+a0)이 허용하는 공급원만 세고 선택 목록(selected)에 최대 용량까지 다시 쓴다.
//    theme 이 -1 이 아니면 그 원소를 먼저 쓰고 나머지 0..5 원소는 범용 's' 로 쓴다. 문자열을 돌려준다.
//  - match(output, required, theme=-1): 요구 문자마다(s/w/r/t 외는 진단 TypeError) 같은 문자 → 's' 는 s → 점이 아니고 대체 원소가 아닌 아무 문자 → 대체 원소 순서로 소비해 점으로 바꾼다. 성공하면 대문자, 실패하면 소문자가 결과에 쓰이고 하나라도 실패하면 0 이다(Override 면 1).
//    { value, output, remainder } 를 돌려준다.
//  - evaluate(output, x, y, type, owner): Override(특수 또는 미션+사용자 치트, 그리고 ignore)이면 출력을 건드리지 않고 1, 아니면 수집 뒤 TYPE 요구 문자열이 비면 출력을 비우고 1, 아니면 대응 성공 시 수집 문자열의 원래 길이, 실패 시 0. { value, output } 을 돌려준다.
//  - covers(provider, bounds, centerX, centerY, sizeSquared, flags): 기본 공급(0x41c0f0)은 false, 공급형(0x41c170)은 발자국 절반을 뺀 raw 좌표와의 거리 제곱이 현재 제곱 반경 + 대상 크기 항보다 작으면 true 이다. 표 밖 가상 표는 거부한다.
//  - takeSource(): 선택 목록의 첫 비영 DWORD 만 0 으로 바꿔 돌려주고 count 는 줄이지 않는다. sortSources(demand, x, y): 요구 문자열 길이만큼 가까운 공급원을 최대 10개 골라 같은 목록 앞에 다시 쓴다(진단 두 위치 321/336).
// 계산은 원본 x87 의 중간 float 저장 위치를 따라 Math.fround 를 쓴다(두 정밀도 관측이 같은 행만 존재). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E29: 에너지 공급 이식으로 추가)

import { ENERGY_PROVIDER_ENTRIES } from './energy-provider-vtables.js';

// 공급 문자표(00540ad8)와 원문 상수들이다: 수집 문자 상한 254, 정렬 보관 수 10, 공급원/공급 장부 용량(004fcb80/004fcb60), 단정도 대각 배율(0050c36c), 두 공급 가상 진입점. (2026-11-08 WB04-E29)
const LETTERS = 'wrtsWRTS.?>\0';
const MAX_OUTPUT = 254;
const KEPT_SOURCES = 10;
export const ENERGY_SOURCE_CAPACITY = 30;
export const ENERGY_PROVIDER_CAPACITY = 256;
const DIAGONAL = Math.fround(0.7071067690849304);
const BASE_PROVIDER = 0x41c0f0;
const SUPPLYING_PROVIDER = 0x41c170;

// 실제 보호 PE 83개 가상 표의 +a0 주소 [가상 표, +a0]이다(decomp_energy_supply 가 현재 원본과 다시 확인). 기본 0x41c0f0 과 다섯 공급형 0x41c170 이외의 함수는 허용하지 않는다. (2026-11-08 WB04-E29)
const PROVIDER_ENTRIES = new Map(ENERGY_PROVIDER_ENTRIES);

// 동일 함수 주소를 공유하는 가상 표만 현재 원본 공급 몸체에 연결한다. 표 밖의 파생이나 파괴 중 표를 임의로 허용하지 않는다. (2026-11-08 WB04-E29)
export function energyProviderEntry(vtable) {
    const entry = PROVIDER_ENTRIES.get(vtable >>> 0);
    if (entry === undefined) throw new Error('에너지 공급의 현재 가상 표 미복원');
    return entry;
}

// 원본 표 밖을 호스트 메모리로 해석하지 않는 원소 문자 조회이다(NUL 칸 11 까지 읽을 수 있음). (2026-11-08 WB04-E29)
function letter(theme) {
    if (!Number.isInteger(theme) || theme < 0 || theme >= LETTERS.length) throw new RangeError('에너지 원소 문자표 범위 오류');
    return LETTERS[theme];
}

// raw 슬롯의 float 를 비정렬 little endian DWORD 에서 읽는다. (2026-11-08 WB04-E29)
function coordinate(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset + offset, 4).getFloat32(0, true);
}

// 첫 NUL 앞까지의 C 문자열이다. (2026-11-08 WB04-E29)
function cString(text) {
    const end = text.indexOf('\0');
    return end < 0 ? text : text.slice(0, end);
}

// std::clamp / std::min / std::max 와 같은 비교 순서(NaN 포함)이다. (2026-11-08 WB04-E29)
const clamp = (value, low, high) => (value < low ? low : high < value ? high : value);
const minOf = (a, b) => (b < a ? b : a);
const maxOf = (a, b) => (a < b ? b : a);

// pool, types(188개 { footX, footY, flags2, theme }), providers(SquidPostPopList), selected(독립 목록 entries/count), alliances(81 DWORD, 공급 소유자*9+요청 소유자), mapLimit() 정수 getter,
// state = { radiusSquared, editor, special, mission, userCheat, ignore, allied }, hooks = { providerEntry(표) → +a0, requirement(type) → 문자열, sortDiagnostic(kind) }, reads = { editor, special, mission } 현재 전역 getter 이다. (2026-11-08 WB04-E29)
export class RawEnergySupply {
    // 연결된 단일 월드의 읽기 대상들이다.
    #pool;
    #types;
    #providers;
    #selected;
    #alliances;
    #mapLimit;
    #state;
    #hooks;
    #reads;

    // 판본/188 TYPE/81 동맹/필수 조회를 검사한다.
    constructor(pool, types, providers, selected, alliances, mapLimit, state, hooks, reads = {}) {
        if (pool.edition !== 'patch1078' || types.length !== 188 || alliances.length !== 81 || typeof hooks?.providerEntry !== 'function' || typeof hooks.requirement !== 'function' || typeof mapLimit !== 'function') {
            throw new TypeError('에너지 실제 월드/TYPE/조회 계약 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#providers = providers;
        this.#selected = selected;
        this.#alliances = alliances;
        this.#mapLimit = mapLimit;
        this.#state = state;
        this.#hooks = hooks;
        this.#reads = reads;
    }

    // 다른 월드의 커서/AI/Builder 가 잘못 연결되는 것을 막는 풀 확인이다.
    get pool() { return this.#pool; }

    // 가상 효과가 바꿀 수 있는 전역은 각 원본 읽기 위치에서 조회한다(연결된 현재 전역이 우선).
    #read(fallback, current) { return current ? current() >>> 0 : fallback >>> 0; }

    // 이전 호출의 우회 결과를 저장하지 않는다: (특수 또는 미션+사용자 치트) 그리고 ignore.
    #override() {
        const state = this.#state;
        return Boolean((this.#read(state.special, this.#reads.special) || (this.#read(state.mission, this.#reads.mission) && state.userCheat)) && state.ignore);
    }

    // 현재 TYPE 번호를 검증한다(다른 번호/그림 블록으로 치환하지 않음).
    #type(number) {
        if (number >= this.#types.length) throw new RangeError('에너지 TYPE 범위 오류');
        return this.#types[number];
    }

    // 예약 슬롯도 원본처럼 읽을 수 있으나 SID 0/풀 밖은 거부한다.
    #raw(sid) {
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('에너지 공급 SID 범위 오류');
        return this.#pool.slot(sid);
    }

    // 목록의 현재 count/항목 하나를 읽는다. 손상된 count(용량 초과)는 거부한다.
    #item(list, index) {
        if (list.readCount() > list.capacity()) throw new RangeError('postPop 목록 count 범위 오류');
        return list.storage()[index] >>> 0;
    }

    // 40e880/40e8f0: 들어온 모서리만 클램프한 뒤 현재 양 끝점의 유효 여부로 min/max 를 선택한다. 반환은 [x0, y0, x1, y1] 단정도이다.
    #bounds(number, x, y) {
        const type = this.#type(number);
        let lowX = Math.fround(x - (type.footX - 1));
        let lowY = Math.fround(y - (type.footY - 1));
        const limit = Math.fround(this.#mapLimit());
        // 실제 41d0d0 은 가변 유효 경계와 별도로 고정 [1,255]에 제한한다.
        lowX = clamp(lowX, 1, 255);
        lowY = clamp(lowY, 1, 255);
        const valid = x > 0 && y > 0 && x < limit && y < limit;
        return valid ? [minOf(x, lowX), minOf(y, lowY), maxOf(x, lowX), maxOf(y, lowY)] : [lowX, lowY, lowX, lowY];
    }

    // 41c0f0/41c170: 대상 사각형과 flags 는 공급 가상 몸체가 읽지 않는 인자지만 ABI 의미를 보존한다.
    covers(provider, bounds, centerX, centerY, sizeSquared, flags) {
        const raw = this.#raw(provider);
        // 가상 표 DWORD 를 호스트 주소로 실행하지 않는다.
        const table = new DataView(raw.buffer, raw.byteOffset, 4).getUint32(0, true);
        const entry = this.#hooks.providerEntry(table);
        if (entry === BASE_PROVIDER) return false;
        if (entry !== SUPPLYING_PROVIDER) throw new Error('에너지 공급 가상+a0 미복원');
        const type = this.#type(raw[10]);
        const dx = Math.fround((coordinate(raw, 14) - type.footX * 0.5) - centerX);
        const dy = (coordinate(raw, 18) - type.footY * 0.5) - centerY;
        const distance = dy * dy + dx * dx;
        return distance < Math.fround(this.#state.radiusSquared) + sizeSquared;
    }

    // 4730c0: 공급량은 선택 목록의 용량과 무관하게 집계하며 문자열은 0..5 원소만 254개까지 출력한다.
    collect(x, y, type, owner, theme = -1) {
        const bounds = this.#bounds(type, x, y);
        const centerX = Math.fround((bounds[0] + bounds[2]) * 0.5);
        const centerY = Math.fround((bounds[1] + bounds[3]) * 0.5);
        const size = Math.fround((bounds[2] - bounds[0]) * DIAGONAL);
        const square = Math.fround(size * size);
        const flags = this.#type(type).flags2;
        const counts = new Array(8).fill(0);
        const selected = this.#selected;
        selected.count = 0;
        // 현재 공급 목록의 count/항목과 현재 소유자/동맹을 각 반복에서 읽는다.
        for (let index = 0; index < this.#providers.readCount(); index += 1) {
            const value = this.#item(this.#providers, index);
            if (!value || value >= this.#pool.capacity || value > 65535) throw new RangeError('에너지 공급 목록 DWORD 범위 오류');
            const raw = this.#raw(value);
            const sourceOwner = raw[34];
            let allowed = Boolean(this.#read(this.#state.editor, this.#reads.editor)) || sourceOwner === owner;
            if (!allowed && this.#state.allied) {
                if (sourceOwner >= 9 || owner >= 9) throw new RangeError('에너지 동맹 소유자 범위 오류');
                allowed = this.#alliances[sourceOwner * 9 + owner] !== 0;
            }
            if (!allowed || !this.covers(value, bounds, centerX, centerY, square, flags)) continue;
            const current = this.#raw(value);
            const kind = this.#type(current[10]).theme;
            if (kind < 0 || kind >= 8) throw new RangeError('에너지 공급 원소 카운터 범위 오류');
            counts[kind] += 1;
            if (selected.count < selected.entries.length) { selected.entries[selected.count] = value; selected.count += 1; }
        }
        let output = '';
        // 특정 원소 요청은 그것을 먼저 쓰고 다른 모든 0..5 원소를 범용 s 로 출력한다.
        if (theme !== -1) {
            if (theme < 0 || theme >= 8) throw new RangeError('에너지 수집 원소 범위 오류');
            output += letter(theme).repeat(Math.min(counts[theme], MAX_OUTPUT));
            counts[theme] = 0;
        }
        // 원본 반복 상한은 6 이며 6/7 공급원은 목록에는 남지만 문구에는 없다.
        for (let index = 0; index < 6; index += 1) output += (theme === -1 ? letter(index) : 's').repeat(Math.min(counts[index], MAX_OUTPUT - output.length));
        return output;
    }

    // 473330: 일반 원소는 같은 문자만, 범용 s 는 s → 다른 비점 원소 → 대체 원소 순서로 소비한다.
    match(output, required, theme = -1) {
        let available = cString(output);
        const requirements = cString(required);
        if (available.length > 255) throw new RangeError('에너지 원본 256바이트 임시 문자열 범위 오류');
        const substitute = theme === -1 ? 's' : letter(theme);
        let result = '';
        let success = 1;
        // 이미 사용한 문자는 점으로 바꾸어 다음 요구가 같은 공급원을 재사용하지 못하게 한다.
        for (const wanted of requirements) {
            if (wanted !== 's' && wanted !== 'w' && wanted !== 'r' && wanted !== 't') throw new TypeError('에너지 원본 요구 문자 진단');
            let found = available.indexOf(wanted);
            if (wanted === 's' && found < 0) {
                // 점도 대체 원소도 아닌 첫 문자, 없으면 대체 원소 자리이다.
                found = [...available].findIndex((character) => character !== '.' && character !== substitute);
                if (found < 0) found = available.indexOf(substitute);
            }
            if (found < 0) { result += wanted; success = 0; }
            else {
                available = `${available.slice(0, found)}.${available.slice(found + 1)}`;
                result += String.fromCharCode(wanted.charCodeAt(0) - 0x61 + 0x41);
            }
        }
        if (this.#override()) success = 1;
        return { value: success, output: result, remainder: available };
    }

    // 4734d0: 우회 경로는 출력/공급 목록을 건드리지 않는 원본 조기 반환이다. 빈 요구는 1, 나머지는 대응 성공 시 수집 문자열의 원래 길이를 반환한다(bool 로 축약하지 않음).
    evaluate(output, x, y, type, owner) {
        if (this.#override()) return { value: 1, output };
        const collected = this.collect(x, y, type, owner);
        const length = collected.length;
        const required = this.#hooks.requirement(type);
        if (required.length === 0 || required[0] === '\0') return { value: 1, output: '' };
        const matched = this.match(collected, required);
        return { value: matched.value ? length : 0, output: matched.output };
    }

    // 473570: Builder 가 같은 목록을 순서대로 소비하게 한다. 첫 비영 DWORD 만 0 으로 바꿔 돌려주며 count 는 줄이지 않는다.
    takeSource() {
        const selected = this.#selected;
        if ((selected.count | 0) <= 0) return 0;
        // 앞에서부터 첫 비영 항목을 찾는다.
        for (let index = 0; index < selected.count; index += 1) {
            if (index >= selected.entries.length) throw new RangeError('에너지 선택 목록 범위 오류');
            const value = selected.entries[index];
            if (value) { selected.entries[index] = 0; return value; }
        }
        return 0;
    }

    // 474970: 41cfc0 의 제곱근 거리와 float 임계값 저장을 사용한다. 원소/중심으로 후보를 다시 거르지 않는다.
    sortSources(demand, x, y) {
        const selected = this.#selected;
        const kept = [];
        // 원본 보고 뒤 현재 조건을 다시 검사한다. 진단 소비자가 없으면 거부한다.
        const report = (kind) => {
            if (typeof this.#hooks.sortDiagnostic !== 'function') throw new Error('에너지 정렬 원본 진단');
            this.#hooks.sortDiagnostic(kind);
        };
        // 문자 내용은 사용하지 않으며 첫 NUL 또는 첫 미선택에서 끝난다.
        for (const character of demand) {
            if (character === '\0') break;
            let best = -1;
            let distanceLimit = 999;
            // 선택/진단 뒤에도 현재 signed count 와 현재 슬롯을 읽는다.
            for (let index = 0; index < (selected.count | 0); index += 1) {
                if (index >= selected.entries.length) throw new RangeError('에너지 선택 목록 범위 오류');
                const value = selected.entries[index];
                if (!value) continue;
                // 예약 슬롯 1..4 도 원본 범위라면 허용한다.
                const valid = () => value >= 1 && value < this.#pool.capacity && value <= 65535 && (this.#pool.slot(value)[11] & 1) === 0;
                if (!valid()) report('InvalidSource');
                if (!valid()) continue;
                const raw = this.#pool.slot(value);
                const dx = x - coordinate(raw, 14);
                const dy = y - coordinate(raw, 18);
                const distance = Math.sqrt(dx * dx + dy * dy);
                if (distance < distanceLimit) { distanceLimit = Math.fround(distance); best = index; }
            }
            if (best === -1) break;
            if (best < 0 || best > (selected.count | 0)) report('InvalidBestIndex');
            if (best >= 0 && best <= (selected.count | 0)) {
                if (best >= selected.entries.length) throw new RangeError('에너지 선택 목록 범위 오류');
                const value = selected.entries[best];
                if (kept.length < KEPT_SOURCES) kept.push(value);
                selected.entries[best] = 0;
            }
        }
        selected.count = 0;
        // 임시 목록을 현재 용량까지만 앞에 쓰고 선택하지 않은 꼬리는 지우지 않는다.
        for (const value of kept) {
            if (selected.count < selected.entries.length) { selected.entries[selected.count] = value; selected.count += 1; }
        }
    }
}
