// 캠페인 진행 값(`Done*`)을 브라우저 로컬 저장소에 보관하는 모듈이다.
// 원본은 미션을 끝내면 미션 스크립트의 `<$Config,Done{mission.fileName}=1>` 가 `DoneTheWarBegins = "1"` 같은 줄을 `d\options.cfg` 에 쓰고,
// 캠페인 목록(`$Checked`)은 이 값으로 체크 표시와 다음 미션 잠금 해제를 정한다. 원본은 설정 객체의 변경 표시를 메인 루프가 보고 저장한다(00441de0 → 00440f30).
// 웹 판본은 같은 키 이름을 쓰되 값을 버전이 있는 JSON 한 덩어리로 로컬 저장소에 쓰고, 값이 바뀔 때마다 바로 저장한다(탭이 언제 닫힐지 알 수 없어 종료 시점 저장에 기대지 않는다).
// 설정(platform/settings.js)과 저장 항목을 나눈 이유: 설정은 고정된 9개 키를 엄격히 검증하지만 진행 값은 미션 파일 이름에서 오는 열린 키 집합이라 검증 규칙이 다르다.
// 저장소를 쓸 수 없거나(개인 정보 보호 창·차단) 내용이 손상됐거나 버전이 다르면 진행 값 없이 시작하고, 값 변경은 메모리에서 계속 동작한다(저장 없는 플레이).
// 키는 대소문자를 가리지 않는다(원본 설정 조회가 대소문자를 무시한다): 저장 파일에는 `DoneThewarbegins` 로 적혀도 목록은 `{DoneTheWarBegins}` 로 읽는다.
// 이 모듈은 DOM 을 직접 쓰지 않는다(저장소 객체는 호출자가 넘긴다). 그래서 Node 에서도 불러와 검사할 수 있다.
// (2026-10-30 WB03-H: 진행 값 쓰기·저장을 위해 추가)
import { configParseLong } from '../assets/config.js';

// 저장소 항목 이름이다. 설정(netstorm.settings)과 다른 항목을 써서 한쪽이 깨져도 다른 쪽이 남는다. (2026-10-30 WB03-H)
export const PROGRESS_STORAGE_KEY = 'netstorm.progress';

// 저장 형식 버전이다. 형식을 바꾸면 올리고, 다른 버전의 저장 내용은 읽지 않는다. (2026-10-30 WB03-H)
export const PROGRESS_VERSION = 1;

// 진행 키의 최대 길이이다. 미션 파일 이름은 이보다 훨씬 짧으며, 저장소에 이상한 긴 키가 쌓이지 않게 막는다. (2026-10-30 WB03-H)
export const PROGRESS_KEY_MAX = 80;

// 진행 키의 모양이다: `Done` 다음에 영숫자·밑줄 한 글자 이상(원본 미션 파일 이름 `Tutorial1`·`BC3Acid_Bar` 등). 대소문자는 가리지 않는다. (2026-10-30 WB03-H)
const PROGRESS_KEY_PATTERN = /^Done[A-Za-z0-9_]+$/i;

// 진행 값의 모양이다: 부호 하나와 십진 숫자 10자리까지. 원본 미션 스크립트는 항상 1 을 쓴다. (2026-10-30 WB03-H)
const PROGRESS_VALUE_PATTERN = /^[+-]?\d{1,10}$/;

// 이 키가 진행 값(`Done*`)의 키인지 확인한다. 설정 키(sound 등)와 구분하는 데 쓴다. (2026-10-30 WB03-H)
export function isProgressKey(key) {
    return typeof key === 'string' && key.length <= PROGRESS_KEY_MAX && PROGRESS_KEY_PATTERN.test(key);
}

// 값을 저장 형식(십진 문자열)으로 정리한다. 정수 숫자나 숫자 문자열만 받고 원본 값 해석(ConfigParseLong)과 같은 32비트 정수로 맞춘다. 맞지 않으면 null 이다. (2026-10-30 WB03-H)
export function normalizeProgressValue(value) {
    if (typeof value === 'number') return Number.isInteger(value) && Math.abs(value) <= 0x7fffffff ? String(value) : null;
    if (typeof value !== 'string' || !PROGRESS_VALUE_PATTERN.test(value)) return null;
    return String(configParseLong(value));
}

// 진행 값 모음이다.
// 사용: const progress = new Progress({ storage: getLocalStorage(window) }); progress.set('DoneTheWarBegins', 1); progress.get('donethewarbegins') → '1'; progress.snapshot().
// set 은 값이 바뀌면 바로 저장하고 onChange(키, 새 값, 이전 값)를 부른다. 저장에 실패해도 값은 바뀐다(persisted 가 false 가 된다). (2026-10-30 WB03-H)
export class Progress {
    // storage 는 getItem/setItem/removeItem 을 가진 저장소(없으면 null: 저장 없음)이고, onChange 는 변경 통지 콜백이다.
    constructor({ storage = null, key = PROGRESS_STORAGE_KEY, onChange = null } = {}) {
        // 저장소 객체이다. 없으면 저장하지 않는다.
        this.storage = storage;
        // 저장소 항목 이름이다.
        this.key = key;
        // 변경 통지 콜백이다.
        this.onChange = onChange;
        // 소문자 키 → { key: 적은 대로의 키, value: 십진 문자열 } 이다. 같은 키를 대소문자만 달리 쓰면 한 항목으로 본다.
        this.entries = new Map();
        // 마지막 저장(또는 불러오기)이 성공했는지 여부이다. 저장소가 없으면 false 이다.
        this.persisted = false;
        // 불러오면서 버린 내용이 있는지(손상·다른 버전·잘못된 값) 알리는 사유이다. 없으면 null.
        this.loadProblem = null;
        this.load();
    }

    // 저장소에서 저장된 JSON 을 해독해 { values } 를 돌려준다. 읽을 수 없거나 형식·버전이 맞지 않으면 사유 문자열을 problem 에 담고 values 는 null 이다.
    // 잘못된 항목은 그 항목만 problem 에 사유를 남기고 버린다. 저장된 것이 없으면 problem 도 null 인 빈 결과이다. (2026-10-30 WB03-H)
    _readStored() {
        const result = { values: null, problem: null, readable: false };
        if (!this.storage) return result;
        let text = null;
        try {
            text = this.storage.getItem(this.key);
        } catch (error) {
            result.problem = '저장소를 읽을 수 없습니다';
            return result;
        }
        result.readable = true;
        if (text === null || text === undefined) {
            result.values = new Map();
            return result;
        }
        let parsed = null;
        try {
            parsed = JSON.parse(text);
        } catch (error) {
            result.problem = '저장된 진행 값이 손상되었습니다';
            return result;
        }
        if (!parsed || typeof parsed !== 'object' || parsed.version !== PROGRESS_VERSION || !parsed.values || typeof parsed.values !== 'object' || Array.isArray(parsed.values)) {
            result.problem = '저장된 진행 값의 버전이 맞지 않습니다';
            return result;
        }
        result.values = new Map();
        // 저장된 항목을 하나씩 검증해 받아들인다. 형식이 맞지 않는 항목은 버린다.
        for (const [key, rawValue] of Object.entries(parsed.values)) {
            const value = isProgressKey(key) ? normalizeProgressValue(rawValue) : null;
            if (value === null) result.problem = `저장된 '${key}' 값이 올바르지 않아 버립니다`;
            else result.values.set(key.toLowerCase(), { key, value });
        }
        return result;
    }

    // 저장소에서 값을 읽어 현재 값으로 삼는다. 읽을 수 없거나 형식·버전이 맞지 않으면 진행 값 없이 시작한다. (2026-10-30 WB03-H)
    load() {
        this.entries = new Map();
        const stored = this._readStored();
        this.loadProblem = stored.problem;
        this.persisted = stored.readable;
        if (stored.values) this.entries = stored.values;
    }

    // 현재 값을 저장소에 쓴다. 성공하면 true 이다(저장소가 없거나 쓰기가 막히면 false).
    // 다른 탭이 먼저 쓴 완료 값을 잃지 않도록 저장된 값을 다시 읽어 합친다(진행 값은 늘기만 하므로 합쳐도 안전하다). 같은 키는 이 탭의 값이 이긴다. (2026-10-30 WB03-H)
    save() {
        if (!this.storage) {
            this.persisted = false;
            return false;
        }
        const merged = new Map(this._readStored().values ?? []);
        // 이 탭의 값을 덮어 쓴다.
        for (const [lower, entry] of this.entries) merged.set(lower, entry);
        const values = {};
        // 저장 형식(적은 대로의 키 → 값)으로 바꾼다.
        for (const entry of merged.values()) values[entry.key] = entry.value;
        try {
            this.storage.setItem(this.key, JSON.stringify({ version: PROGRESS_VERSION, values }));
            this.persisted = true;
        } catch (error) {
            this.persisted = false;
        }
        return this.persisted;
    }

    // 키의 현재 값(십진 문자열)이다. 키는 대소문자를 가리지 않으며 없으면 undefined 이다.
    get(key) {
        return this.entries.get(String(key).toLowerCase())?.value;
    }

    // 값을 바꾼다. 진행 키가 아니거나 값이 올바르지 않으면 false, 같은 값이면 저장하지 않고 false, 바뀌면 저장하고 통지한 뒤 true 이다.
    // 키가 이미 다른 대소문자로 있으면 새로 적은 대로의 키로 바꾼다(원본이 줄을 지우고 새로 쓰는 것과 같다). persist 가 false 이면 저장하지 않고 메모리에서만 바꾼다. (2026-10-30 WB03-H)
    set(key, value, { persist = true } = {}) {
        const next = normalizeProgressValue(value);
        if (!isProgressKey(key) || next === null) return false;
        const lower = key.toLowerCase();
        const previous = this.entries.get(lower)?.value;
        if (previous === next) return false;
        this.entries.set(lower, { key, value: next });
        if (persist) this.save();
        try {
            this.onChange?.(key, next, previous);
        } catch (error) {
            console.error('Netstorm: 진행 값 변경 통지에서 예외가 발생했습니다.', error);
        }
        return true;
    }

    // 진행 값을 모두 지운다(처음 설치한 상태로 되돌린다). 저장된 항목도 지운다. 지운 값이 있었으면 true 이다. 개발·검사용이며 게임 화면에는 이 기능이 없다(원본도 options.cfg 를 지워야 한다). (2026-10-30 WB03-H)
    clear() {
        const had = this.entries.size > 0;
        this.entries = new Map();
        try {
            this.storage?.removeItem(this.key);
            if (this.storage) this.persisted = true;
        } catch (error) {
            this.persisted = false;
        }
        return had;
    }

    // 모든 값의 사본이다(적은 대로의 키 → 십진 문자열). 게임 데이터 설정 층에 넣을 때 쓴다. (2026-10-30 WB03-H)
    snapshot() {
        const values = {};
        // 항목마다 적은 대로의 키로 옮긴다.
        for (const entry of this.entries.values()) values[entry.key] = entry.value;
        return values;
    }
}
