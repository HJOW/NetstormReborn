// 사용자 설정(옵션 메뉴가 바꾸는 값)을 브라우저 로컬 저장소에 보관하는 모듈이다.
// 원본은 `d\options.cfg` 에 같은 이름의 키로 저장한다(sound·music·playWind·swapLeftRightSpeakers·soundVolume·musicVolume·edgeScroll·autoDemo·tellTips).
// 웹 판본은 같은 키 이름을 쓰되 값을 버전이 있는 JSON 한 덩어리로 로컬 저장소에 쓴다. 저장소를 쓸 수 없거나(개인 정보 보호 창·차단) 내용이 손상됐거나
// 버전이 다르면 기본값으로 시작하고, 값 변경은 메모리에서 계속 동작한다(저장 없는 플레이). 해상도·전체화면·설치 경로는 웹에서 의미가 없어 저장하지 않는다.
// 이 모듈은 DOM 을 직접 쓰지 않는다(저장소 객체는 호출자가 넘긴다). 그래서 Node 에서도 불러와 검사할 수 있다.
// (2026-10-29 WB03-F: Options 펼침 메뉴의 설정 값을 보관하기 위해 추가)

// 저장소 항목 이름이다. 같은 도메인에서 다른 앱과 겹치지 않게 접두어를 둔다. (2026-10-29 WB03-F)
export const SETTINGS_STORAGE_KEY = 'netstorm.settings';

// 저장 형식 버전이다. 형식을 바꾸면 올리고, 다른 버전의 저장 내용은 읽지 않는다. (2026-10-29 WB03-F)
export const SETTINGS_VERSION = 1;

// 음량 단계의 범위이다(원본 옵션 메뉴 Volume 1~5). (2026-10-29 WB03-F)
export const VOLUME_STEP_MIN = 1;
export const VOLUME_STEP_MAX = 5;

// 설정 키 정의이다. kind 는 'toggle'(0/1) 또는 'volume'(1~5), fallback 은 기본값이다.
// 기본값은 원본 옵션 해석(Interpret Options)의 기본 변수와 스크린샷의 기본 체크 상태를 따른다:
//  - 소리·음악·바람 소리·가장자리 스크롤·자동 데모·시작 팁은 켜짐, 좌우 스피커 교환은 꺼짐, 효과음 음량 3, 음악 음량 2. (2026-10-29 WB03-F)
export const SETTING_DEFINITIONS = Object.freeze({
    sound: Object.freeze({ kind: 'toggle', fallback: 1 }),
    music: Object.freeze({ kind: 'toggle', fallback: 1 }),
    playWind: Object.freeze({ kind: 'toggle', fallback: 1 }),
    swapLeftRightSpeakers: Object.freeze({ kind: 'toggle', fallback: 0 }),
    soundVolume: Object.freeze({ kind: 'volume', fallback: 3 }),
    musicVolume: Object.freeze({ kind: 'volume', fallback: 2 }),
    edgeScroll: Object.freeze({ kind: 'toggle', fallback: 1 }),
    autoDemo: Object.freeze({ kind: 'toggle', fallback: 1 }),
    tellTips: Object.freeze({ kind: 'toggle', fallback: 1 }),
});

// 값을 키의 종류에 맞게 정리한다. toggle 은 참/거짓을 1/0 으로, volume 은 1~5 의 정수만 받는다. 맞지 않으면 null 이다. (2026-10-29 WB03-F)
export function normalizeSetting(key, value) {
    const definition = SETTING_DEFINITIONS[key];
    if (!definition) return null;
    if (definition.kind === 'toggle') {
        if (value === true || value === 1 || value === '1') return 1;
        if (value === false || value === 0 || value === '0') return 0;
        return null;
    }
    const number = typeof value === 'string' && /^\d+$/.test(value) ? Number(value) : value;
    return Number.isInteger(number) && number >= VOLUME_STEP_MIN && number <= VOLUME_STEP_MAX ? number : null;
}

// 브라우저 문서의 로컬 저장소를 안전하게 얻는다. 접근만 해도 예외가 나는 환경(차단·개인 창)에서는 null 이다. view 는 문서의 window 이다. (2026-10-29 WB03-F)
export function getLocalStorage(view) {
    try {
        return view?.localStorage ?? null;
    } catch (error) {
        return null;
    }
}

// 설정 모음이다. 모든 키는 항상 값을 가진다(저장된 값이 없으면 기본값).
// 사용: const settings = new Settings({ storage: getLocalStorage(window) }); settings.get('sound'); settings.toggle('music'); settings.set('soundVolume', 4).
// set·toggle 은 값이 바뀌면 저장하고 onChange(키, 새 값, 이전 값)를 부른다. 저장에 실패해도 값은 바뀐다(persisted 가 false 가 된다). (2026-10-29 WB03-F)
export class Settings {
    // storage 는 getItem/setItem 을 가진 저장소(없으면 null: 저장 없음)이고, onChange 는 변경 통지 콜백이다.
    constructor({ storage = null, key = SETTINGS_STORAGE_KEY, onChange = null } = {}) {
        // 저장소 객체이다. 없으면 저장하지 않는다.
        this.storage = storage;
        // 저장소 항목 이름이다.
        this.key = key;
        // 변경 통지 콜백이다.
        this.onChange = onChange;
        // 키 → 현재 값이다.
        this.values = {};
        // 마지막 저장(또는 불러오기)이 성공했는지 여부이다. 저장소가 없으면 false 이다.
        this.persisted = false;
        // 불러오면서 버린 내용이 있는지(손상·다른 버전·잘못된 값) 알리는 사유이다. 없으면 null.
        this.loadProblem = null;
        this._reset();
        this.load();
    }

    // 모든 값을 기본값으로 되돌린다(저장하지 않음). (2026-10-29 WB03-F)
    _reset() {
        for (const [key, definition] of Object.entries(SETTING_DEFINITIONS)) this.values[key] = definition.fallback;
    }

    // 저장소에서 값을 읽는다. 읽을 수 없거나 형식·버전이 맞지 않으면 기본값으로 시작한다. 알 수 없는 키·잘못된 값은 그 항목만 버린다. (2026-10-29 WB03-F)
    load() {
        this.loadProblem = null;
        if (!this.storage) return;
        let text = null;
        try {
            text = this.storage.getItem(this.key);
        } catch (error) {
            this.loadProblem = '저장소를 읽을 수 없습니다';
            return;
        }
        this.persisted = true;
        if (text === null || text === undefined) return;
        let parsed = null;
        try {
            parsed = JSON.parse(text);
        } catch (error) {
            this.loadProblem = '저장된 설정이 손상되었습니다';
            return;
        }
        if (!parsed || typeof parsed !== 'object' || parsed.version !== SETTINGS_VERSION || !parsed.values || typeof parsed.values !== 'object') {
            this.loadProblem = '저장된 설정의 버전이 맞지 않습니다';
            return;
        }
        // 알려진 키만 정리해 받아들인다.
        for (const key of Object.keys(SETTING_DEFINITIONS)) {
            const value = normalizeSetting(key, parsed.values[key]);
            if (value !== null) this.values[key] = value;
            else if (key in parsed.values) this.loadProblem = `저장된 '${key}' 값이 올바르지 않아 기본값을 씁니다`;
        }
    }

    // 현재 값을 저장소에 쓴다. 성공하면 true 이다(저장소가 없거나 쓰기가 막히면 false). (2026-10-29 WB03-F)
    save() {
        if (!this.storage) {
            this.persisted = false;
            return false;
        }
        try {
            this.storage.setItem(this.key, JSON.stringify({ version: SETTINGS_VERSION, values: this.values }));
            this.persisted = true;
        } catch (error) {
            this.persisted = false;
        }
        return this.persisted;
    }

    // 키의 현재 값이다. 모르는 키는 undefined 이다.
    get(key) {
        return this.values[key];
    }

    // 값을 바꾼다. 알 수 없는 키·잘못된 값이면 false, 같은 값이면 저장하지 않고 false, 바뀌면 저장하고 통지한 뒤 true 이다.
    // persist 가 false 이면 저장하지 않고 메모리에서만 바꾼다(호스트가 준 임시 값). (2026-10-29 WB03-F)
    set(key, value, { persist = true } = {}) {
        const next = normalizeSetting(key, value);
        if (next === null) return false;
        const previous = this.values[key];
        if (next === previous) return false;
        this.values[key] = next;
        if (persist) this.save();
        try {
            this.onChange?.(key, next, previous);
        } catch (error) {
            console.error('Netstorm: 설정 변경 통지에서 예외가 발생했습니다.', error);
        }
        return true;
    }

    // toggle 키를 뒤집는다. 바뀐 새 값(0/1)을 돌려주며, toggle 이 아니면 null 이다. (2026-10-29 WB03-F)
    toggle(key) {
        if (SETTING_DEFINITIONS[key]?.kind !== 'toggle') return null;
        this.set(key, this.values[key] ? 0 : 1);
        return this.values[key];
    }

    // 모든 값의 사본이다(원본 options.cfg 의 키 이름 그대로). 게임 데이터 설정 층에 넣을 때 쓴다. (2026-10-29 WB03-F)
    snapshot() {
        return { ...this.values };
    }
}
