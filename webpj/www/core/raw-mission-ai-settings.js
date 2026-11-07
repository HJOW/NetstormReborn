// AI 생성자에 넘기는 600바이트 설정(0040ef80 기본값·00416860·004c2d40/004c2d90 공통·owner 덮어쓰기·0040f030 능력 비트)을 옮긴 모듈이다(cpppj `o/RawMissionAiSettings.cpp/.h`, 10.78 전용).
//  - initialize(storage): 실제 기본값이 쓰는 바이트만 바꾸고 나머지 caller 바이트는 보존한다(전체를 지우거나 문자열 공백을 보충하지 않음): +0 = 0, +0x1e = 기본 기술 문자열(NUL 종료), +0x220 = float 6.0, +0x224 = float(stuffRefreshRate), +0x228 = float 3.0,
//    +0x22c = startingSp + 4000(DWORD 감김), +0x230 = 3, +0x234 = 3, +0x238 = 4, +0x23c/0x240/0x244/0x24c/0x250/0x254 = 0, +0x248 = 0xffff.
//  - setAbility(storage, name, enabled): ASCII 대소문자를 무시하고 원본 13개 능력 이름의 비트(+0x248)를 켜고 끈다. 미등록 이름은 checking 이면 assertion 을 먼저, 그 뒤 항상 log 를 부른 다음 index 13 비트를 수정한다(임의로 무시하지 않음).
//  - apply(storage, owner): 키 접두는 owner>0 이면 'ai<owner>', 아니면 'ai'이며 매번 현재 설정을 조회한다. 값이 비어 있으면 이전 필드를 유지한다: Name(+0)·Tech(+0x1e) 문자열, 세 atof(BridgeDrawRate +0x220, StuffRefreshRate +0x224, TimeBetweenMoves +0x228),
//    일곱 atol(StartMoney +0x22c, GeyserAttachments +0x230, MaxInHand +0x238, Collectors +0x234, Enemy +0x240, NoTemple +0x244, Off +0x254), Color(1..8 색 이름이면 +0x250, none/미등록은 유지), AllyList(';' 구분 열 개, 양수면 비트 가산), Ability(';' 열 개, 선두 '!' 한 글자만 끄기).
//  - prepare(storage, owner): 기본값 → 공통(-1) → owner 순서로 같은 storage 에 누적한다. AI 생성 자체는 호출자 경계이다.
// 실제 설정 조회가 없으면 준비를 시작하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E32: AI 설정 이식으로 추가)
import { asciiLower, configParseLong } from '../assets/config.js';

// 설정 저장소 크기(0040ef80/00416860)와 기본 기술 목록(00501000), 능력 이름 13개(0052f3fc 포인터 순서), 색 이름(00542660 슬래시 목록 순서)이다. (2026-11-08 WB04-E32)
export const MISSION_AI_SETTINGS_SIZE = 600;
const DEFAULT_TECH = 'sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher';
const ABILITIES = Object.freeze([
    'FOCUS_ON_THINGS_THAT_ATTACK_BUILDINGS', 'BUILD_BLOCKERS_WHEN_BUILDINGS_HARMED', 'ATTACH_TO_ENEMY_ISLAND', 'STOP_COLLECTING_FROM_DANGEROUS_GEYSERS',
    'COLLECT_STORM_POWER', 'ENCIRCLE_ENEMY_ISLAND', 'CONCENTRATE_FIRE', 'REPEAT_SUCCESSFUL_DROPS', 'CAPTURE_ENEMY_PRIEST', 'BUILD_DAIS',
    'SACRIFICE_ENEMY_PRIEST', 'USE_PRIEST_TO_COLLECT', 'BUILD_BRIDGES',
]);
const COLORS = Object.freeze(['none', 'blue', 'red', 'white', 'green', 'purple', 'yellow', 'lightblue', 'orange']);

// 이 모듈의 실제 600바이트 계약을 확인한다. (2026-11-08 WB04-E32)
function check(raw) {
    if (!raw || raw.length !== MISSION_AI_SETTINGS_SIZE) throw new TypeError('AI 설정은 실제 600바이트여야 합니다');
}

// 실제 DWORD 를 리틀엔디언 순서로 읽고 쓴다. 주변의 미초기화 바이트는 보존한다. (2026-11-08 WB04-E32)
function read(raw, offset) {
    let value = 0;
    // 원본 필드의 네 바이트만 합친다.
    for (let index = 0; index < 4; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}
function put(raw, offset, value) {
    // 낮은 바이트부터 네 바이트만 기록한다.
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 단정도 float 의 비트이다. (2026-11-08 WB04-E32)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 원본 strcpy 처럼 NUL 까지 쓰며 저장소 밖의 손상은 명시적으로 거부한다. (2026-11-08 WB04-E32)
function copy(raw, offset, value) {
    const nul = value.indexOf('\0');
    const text = nul < 0 ? value : value.slice(0, nul);
    if (text.length >= raw.length - offset) throw new RangeError('AI 설정 문자열이 실제 저장소를 벗어납니다');
    // 글자 하나당 바이트 하나를 옮긴다.
    for (let index = 0; index < text.length; index += 1) raw[offset + index] = text.charCodeAt(index) & 0xff;
    raw[offset + text.length] = 0;
}

// 원본 __stricmp 의 ASCII 이름 비교이다. (2026-11-08 WB04-E32)
const equal = (left, right) => asciiLower(left) === asciiLower(right);

// ';' 구분 목록의 index 번째 조각이다(없으면 빈 문자열). (2026-11-08 WB04-E32)
export function configListItem(list, index) {
    let start = 0;
    // 앞 조각을 index 개 건너뛴다.
    for (let skip = index; skip > 0; skip -= 1) {
        const separator = list.indexOf(';', start);
        if (separator < 0) return '';
        start = separator + 1;
    }
    if (index < 0) return '';
    const end = list.indexOf(';', start);
    return end < 0 ? list.slice(start) : list.slice(start, end);
}

// state = { stuffRefreshRate(기본 3), startingSp(기본 2500), checkingAsserts(boolean/수 또는 getter) },
// hooks = { get(키) → 문자열(현재 미션 키 조회), parseFloat(문자열) → double, unknownAbilityAssertion(이름), unknownAbilityLog(이름) } 이다. (2026-11-08 WB04-E32)
export class RawMissionAiSettings {
    #state;
    #hooks;

    // 실제 설정 조회가 없으면 거부한다.
    constructor(state, hooks) {
        if (typeof hooks?.get !== 'function') throw new TypeError('MissionAI 는 실제 설정 조회가 필요합니다');
        this.#state = state;
        this.#hooks = hooks;
    }

    // 0040ef80: 실제 기본값이 쓰는 바이트만 바꾸고 나머지 caller 바이트를 보존한다.
    initialize(raw) {
        check(raw);
        raw[0] = 0;
        copy(raw, 0x1e, DEFAULT_TECH);
        put(raw, 0x220, floatBits(6));
        put(raw, 0x224, floatBits(this.#state.stuffRefreshRate));
        put(raw, 0x228, floatBits(3));
        put(raw, 0x22c, (this.#state.startingSp + 4000) >>> 0);
        put(raw, 0x230, 3);
        put(raw, 0x234, 3);
        put(raw, 0x238, 4);
        put(raw, 0x23c, 0);
        put(raw, 0x240, 0);
        put(raw, 0x244, 0);
        put(raw, 0x24c, 0);
        put(raw, 0x250, 0);
        put(raw, 0x254, 0);
        put(raw, 0x248, 0xffff);
    }

    // 0040f030: 원본은 미등록 이름을 보고한 뒤 index 13 비트도 수정한다. 임의로 무시하지 않는다.
    setAbility(raw, name, enabled) {
        check(raw);
        const nul = name.indexOf('\0');
        const text = nul < 0 ? name : name.slice(0, nul);
        if (text === '') return;
        let index = 0;
        // 실제 NULL 종료 이름 표에서 일치하거나 마지막 NULL 위치까지 찾는다.
        while (index < ABILITIES.length && !equal(text, ABILITIES[index])) index += 1;
        if (index === ABILITIES.length) {
            const checking = typeof this.#state.checkingAsserts === 'function' ? this.#state.checkingAsserts() : this.#state.checkingAsserts;
            if (checking) {
                if (typeof this.#hooks.unknownAbilityAssertion !== 'function') throw new Error('AI 능력 이름 assert 경계 누락');
                this.#hooks.unknownAbilityAssertion(text);
            }
            if (typeof this.#hooks.unknownAbilityLog !== 'function') throw new Error('AI 능력 이름 log 경계 누락');
            this.#hooks.unknownAbilityLog(text);
        }
        const bit = (1 << (index & 31)) >>> 0;
        put(raw, 0x248, enabled ? (read(raw, 0x248) | bit) >>> 0 : (read(raw, 0x248) & ~bit) >>> 0);
    }

    // 004c2d90: 공통과 owner 적용은 매번 현재 설정을 조회한다. 목록 열 개를 하나로 합치지 않는다.
    apply(raw, owner) {
        check(raw);
        const prefix = `ai${owner > 0 ? String(owner) : ''}`;
        // 원본 형식 문자열이 만든 현재 키로 조회한다.
        const get = (suffix) => this.#hooks.get(prefix + suffix);
        let value = get('Name');
        if (value !== '') copy(raw, 0, value);
        value = get('Tech');
        if (value !== '') copy(raw, 0x1e, value);
        // 세 atof → float 필드의 실제 순서이며 비어 있지 않은 값에만 원본 atof 경계를 호출한다.
        for (const [key, offset] of [['BridgeDrawRate', 0x220], ['StuffRefreshRate', 0x224], ['TimeBetweenMoves', 0x228]]) {
            value = get(key);
            if (value !== '') {
                if (typeof this.#hooks.parseFloat !== 'function') throw new Error('AI 설정 atof 경계 누락');
                put(raw, offset, floatBits(this.#hooks.parseFloat(value)));
            }
        }
        // 일곱 atol 필드의 실제 순서이다.
        for (const [key, offset] of [['StartMoney', 0x22c], ['GeyserAttachments', 0x230], ['MaxInHand', 0x238], ['Collectors', 0x234], ['Enemy', 0x240], ['NoTemple', 0x244], ['Off', 0x254]]) {
            value = get(key);
            if (value !== '') put(raw, offset, configParseLong(value) >>> 0);
        }
        value = get('Color');
        // none(index 0)과 미등록 값은 기존 색을 지우지 않는다.
        for (let color = 1; color < COLORS.length; color += 1) {
            if (equal(value, COLORS[color])) { put(raw, 0x250, color); break; }
        }
        // 공통/owner 목록은 원본대로 가산하며 8을 넘는 양수도 shift 의 하위 5비트로 처리한다.
        for (let index = 0; index < 10; index += 1) {
            const ally = configParseLong(configListItem(get('AllyList'), index));
            if (ally > 0) put(raw, 0x23c, (read(raw, 0x23c) | (1 << (ally & 31))) >>> 0);
        }
        // 느낌표는 선두 한 글자만 제거하며 빈 이름은 아무 것도 하지 않는다.
        for (let index = 0; index < 10; index += 1) {
            value = configListItem(get('Ability'), index);
            const enabled = value === '' || value[0] !== '!';
            this.setAbility(raw, enabled ? value : value.slice(1), enabled);
        }
    }

    // 004c2d40 의 실제 순서인 기본값 → 공통(-1) → owner 를 같은 storage 에 누적한다. AI 생성 자체는 호출자 경계이다.
    prepare(raw, owner) {
        this.initialize(raw);
        this.apply(raw, -1);
        this.apply(raw, owner);
    }
}
