// 모든 화면·미리보기·이미지·픽셀 선택이 같은 주소로 빌리는 타입별 소유자 색 변환표의 현재 저장소와 그 수명 규칙(배너 옵션 43bc40, 최초 등록 43be30)을 옮긴 모듈이다
// (cpppj `client/OwnerColorTableState.cpp/.h`, 10.78 전용). 표 계산 자체(일반 띠·명도 전용)는 assets/owner-colors.js 가 맡고 이 모듈은 언제·무엇을 만들고 반납하는지를 정한다.
//  - setBannerOption(globals, requested, types, hooks): 준비 전이거나 같은 값이면 아무 것도 읽지 않고 돌아온다. 그 밖에는 설정 DWORD 와 다시 그리기 DWORD(+1, unsigned 감김)를 쓴 뒤,
//    0 이면 지면(97)·받침 윗면(94) 명도 전용 표 둘을 만들고(소스 목록 포함 hooks.special 호출 두 번), 0 이 아니면 배너 타입 다섯 개를 차례로 다시 읽어 존재하는 표를 반납(hooks.release)하고 0 으로 쓴다.
//  - initializeOwnerColorTables(globals, skip, types, hooks): 준비 1·설정 -1 강제 뒤 이전 설정으로 setBannerOption 을 부르고, skip 이 0 이면 일반 표 앞 둘, 그다음 전용 111·141(−0.15), 일반 중간 넷, 전용 137, 일반 나머지 여섯을 원본 순서로 등록한다.
//  - OwnerColorTableState: 장기 저장소 — initialize(primary, typeCount, bridgeType, redraw, skip) 가 최초 등록을 한 번만 하고, setEnabled(value, redraw) 가 설정 해석을 전달한다.
//    tokens 는 주소가 바뀌지 않는 타입별 현재 표 토큰 배열(0x100000 + 타입 × 2304)이고, color(type, 색번호)·colorToken(토큰)이 현재 9 × 256 표를 빌려 준다.
// globals = { ready, enabled, redraw, tables } 는 현재 DWORD 와 포인터 표를 복사하지 않고 읽고 쓰는 객체이다(독립 검사는 평범한 객체, 실제 저장소는 접근자). hooks = { standard(type), special(type, offset, sources), release(token) }.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E374: 5-A OwnerColorTableState 이식으로 추가, 기준선 뒤 cpppj 변경 없음)
import { buildSpecialOwnerColorMapsFromLightness, buildStandardOwnerColorMaps, specialOwnerColorSources } from '../assets/owner-colors.js';

// 타입 표의 토큰 기준값과 표 하나의 바이트 크기(9 × 256), 최초 등록이 요구하는 최소 타입 수, 색 번호 최댓값이다. (2026-11-13 WB04-E374)
const TOKEN_BASE = 0x100000;
const TABLE_BYTES = 2304;
const MIN_TYPE_COUNT = 171;
const MAX_PALETTE = 8;
// 전용 표 보정값(타입 141 의 −0.15000000596046448)과 강제 설정 값이다. (2026-11-13 WB04-E374)
const OFFSET_141 = -0.15000000596046448;
const FORCED = 0xffffffff;

// 보호 10.78 의 현재 TYPE 전역 값이다. banner 는 켤 때 반납할 다섯 표(끌 때는 마지막 두 표만 만든다), standard 는 최초 일반 표 등록 순서 열둘, special 은 최초 전용 명도 표 셋이다. (2026-11-13 WB04-E374)
export function defaultOwnerColorTableTypes() {
    return {
        banner: [85, 71, 128, 97, 94],
        standard: [82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126],
        special: [111, 141, 137],
    };
}

// 필요한 외부 효과가 없으면 빈 성공으로 넘기지 않고 거부한다. (2026-11-13 WB04-E374)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('공통 소유자 색표 장치 미연결');
    return hook;
}

// 타입 번호가 고정 별칭 토큰이 된다(같은 타입 9표의 고정 주소 별칭). (2026-11-13 WB04-E374)
const tableToken = (type) => TOKEN_BASE + type * TABLE_BYTES;

// 43bc40: 준비 전/같은 값이면 포인터/장치를 읽지 않는다. 설정 DWORD 와 다시 그리기 DWORD(+1)를 쓴 뒤 0 이면 97·94 전용 표, 아니면 배너 다섯 표를 반납한다. (2026-11-13 WB04-E374)
export function setBannerOption(globals, requested, types, hooks) {
    const value = requested >>> 0;
    if (!globals.ready || globals.enabled === value) return;
    const dirty = (globals.redraw + 1) >>> 0;
    globals.enabled = value;
    globals.redraw = dirty;
    if (!value) {
        need(hooks.special)(types.banner[3], 0.0, specialOwnerColorSources(97));
        need(hooks.special)(types.banner[4], 0.0, specialOwnerColorSources(94));
        return;
    }
    // 원문은 각 현재 TYPE/포인터를 차례로 다시 읽고 존재하는 표만 반납한 뒤 0 으로 쓴다.
    for (const type of types.banner) {
        if (type >= globals.tables.length) throw new RangeError('배너 현재 TYPE 색표 범위');
        const pointer = globals.tables[type];
        if (pointer) {
            need(hooks.release)(pointer);
            globals.tables[type] = 0;
        }
    }
}

// 43be30: 최초 owner 표 등록이다. 색표가 만들어질 때만 수행하며 일반 팔레트 교체에서 다시 부르지 않는다. skip 은 59ab28 의 현재 값이다. (2026-11-13 WB04-E374)
export function initializeOwnerColorTables(globals, skip, types, hooks) {
    const previous = globals.enabled;
    globals.ready = 1;
    globals.enabled = FORCED;
    setBannerOption(globals, previous, types, hooks);
    if (!skip) {
        need(hooks.standard)(types.standard[0]);
        need(hooks.standard)(types.standard[1]);
    }
    need(hooks.special)(types.special[0], 0.0, specialOwnerColorSources(111));
    need(hooks.special)(types.special[1], OFFSET_141, specialOwnerColorSources(141));
    // 중간 네 일반 표 다음 137 전용 표를 만드는 원본 순서를 유지한다.
    for (let index = 2; index < 6; index += 1) need(hooks.standard)(types.standard[index]);
    need(hooks.special)(types.special[2], 0.0, specialOwnerColorSources(137));
    // 나머지 여섯 일반 표도 원본 호출 순서이다.
    for (let index = 6; index < types.standard.length; index += 1) need(hooks.standard)(types.standard[index]);
}

// 두 숫자 배열이 같은 값 순서인지 본다. (2026-11-13 WB04-E374)
function sameNumbers(left, right) {
    return left.length === right.length && left.every((value, index) => value === right[index]);
}

// double 의 원시 비트가 같은지 본다(−0 과 0 을 구별한다). (2026-11-13 WB04-E374)
function sameBits(left, right) {
    const view = new DataView(new ArrayBuffer(16));
    view.setFloat64(0, left, true);
    view.setFloat64(8, right, true);
    return view.getBigUint64(0, true) === view.getBigUint64(8, true);
}

// 모든 화면·미리보기·이미지·픽셀 선택이 빌리는 단일 현재 소유자 색표 저장소이다. 사용: const owner = new OwnerColorTableState(); owner.initialize(primary, 188, 82, { value: 0 }); owner.color(97, 2). (2026-11-13 WB04-E374)
export class OwnerColorTableState {
    // 보호 PE 초기값(준비 0, 배너 사용 1)과 실제 다리 TYPE, 현재 TYPE 전역 값이다.
    #ready = 0;
    #enabled = 1;
    #bridgeType = 82;
    #types = defaultOwnerColorTableTypes();
    // 저장된 명도가 모든 표보다 오래 살아 있게 하는 primary, 타입별 현재 포인터 별칭 배열(F7 은 원소만 바꾼다), 각 비제로 토큰의 실제 9 × 256 표이다.
    #primary = null;
    #tokens = [];
    #maps = [];

    // 현재 DWORD 와 포인터 표를 복사하지 않고 읽고 쓰는 globals 를 만든다. redraw 는 호출자의 { value } 상자이다.
    #globals(redraw) {
        const state = this;
        return {
            get ready() { return state.#ready; },
            set ready(value) { state.#ready = value >>> 0; },
            get enabled() { return state.#enabled; },
            set enabled(value) { state.#enabled = value >>> 0; },
            get redraw() { return redraw.value >>> 0; },
            set redraw(value) { redraw.value = value >>> 0; },
            tables: state.#tokens,
        };
    }

    // 최초 Client 연결의 필수 물리를 검증하고 일반/전용 표를 원본 순서로 등록한다. redraw 는 { value } 상자이다.
    initialize(primary, typeCount, bridgeType, redraw, skip = 0) {
        if (this.#ready || !primary || !primary.constructed || typeCount < MIN_TYPE_COUNT) throw new TypeError('최초 공통 소유자 색표 준비 계약');
        this.#primary = primary;
        this.#bridgeType = bridgeType;
        this.#tokens.length = 0;
        this.#maps.length = 0;
        for (let type = 0; type < typeCount; type += 1) {
            this.#tokens.push(0);
            this.#maps.push(null);
        }
        initializeOwnerColorTables(this.#globals(redraw), skip, this.#types, this.#hooks());
    }

    // 배너 옵션 설정 해석을 전달한다. 준비 전이면 원문처럼 무시된다.
    setEnabled(value, redraw) {
        setBannerOption(this.#globals(redraw), value, this.#types, this.#hooks());
    }

    // 현재 배너 사용 DWORD 이다(출력 순간에 읽는다).
    get enabled() { return this.#enabled; }

    // 최초 등록이 끝났는지 읽는다(별도 화면/Session 생성 여부로 대체하지 않는다).
    get ready() { return this.#ready !== 0; }

    // 초기화 뒤 주소가 바뀌지 않는 실제 현재 TYPE 표 토큰 배열이다. 최초 등록 전에는 빈 표로 정상 렌더를 가장하지 않고 거부한다.
    get tokens() {
        if (!this.#ready) throw new Error('공통 소유자 색표 최초 등록 전 조회');
        return this.#tokens;
    }

    // 같은 소유자 색 번호 0..8 의 현재 표를 빌린다. 현재 비제로 표와 색 번호를 함께 검증한다.
    color(type, palette) {
        if (type >= this.#maps.length || !this.#tokens[type] || !this.#maps[type] || palette > MAX_PALETTE) throw new RangeError('현재 공통 소유자 색표 조회 계약');
        return this.#maps[type][palette];
    }

    // ImageGump 의 같은 base + 색번호 × 256 토큰을 판독한다. 행 오프셋은 256 바이트 배수여야 한다.
    colorToken(token) {
        if (token < TOKEN_BASE || (token - TOKEN_BASE) % 256) throw new RangeError('공통 색표 토큰 정렬');
        const offset = token - TOKEN_BASE;
        return this.color(Math.floor(offset / TABLE_BYTES), Math.floor((offset % TABLE_BYTES) / 256));
    }

    // 현재 자신의 표만 생성/반납하는 실제 장치이다. 네이티브 free 호출 뒤의 포인터 0 쓰기는 setBannerOption 이 수행한다.
    #hooks() {
        return {
            standard: (type) => {
                if (type >= this.#maps.length) throw new RangeError('공통 색표 TYPE 범위');
                this.#maps[type] = buildStandardOwnerColorMaps(type, this.#bridgeType);
                this.#tokens[type] = tableToken(type);
            },
            special: (type, offset, sources) => {
                if (type >= this.#maps.length) throw new RangeError('공통 색표 TYPE 범위');
                const expected = specialOwnerColorSources(type);
                const wanted = type === 141 ? OFFSET_141 : 0.0;
                if (!sameNumbers(sources, expected) || !sameBits(offset, wanted)) throw new Error('공통 전용 색표의 원본 소스/명도 계약');
                const light = Float64Array.from({ length: 256 }, (_, index) => this.#primary.lightness(index));
                this.#maps[type] = buildSpecialOwnerColorMapsFromLightness(light, type);
                this.#tokens[type] = tableToken(type);
            },
            release: (token) => {
                if (token < TOKEN_BASE || (token - TOKEN_BASE) % TABLE_BYTES) throw new RangeError('공통 색표 반납 토큰');
                const type = (token - TOKEN_BASE) / TABLE_BYTES;
                if (type >= this.#maps.length) throw new RangeError('공통 색표 반납 토큰');
                this.#maps[type] = null;
            },
        };
    }
}
