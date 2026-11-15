// 실제 미션 Start 와 600BYTE AI 설정을 같은 Session/Script/프레임 게이트에 연결하는 client 소비자이다(cpppj `client/RawCampaignMissionStartBindings.cpp/.h`, 10.78 전용).
// 사용:
//   const start = new RawCampaignMissionStartBindings(session, script, gate, startState, startGlobals, aiGlobals, { settingsBytes, gameSeconds, refreshSp, refreshCommandGump, randomGeysers, resetCamera,
//       prepareWorld?, unknownAbilityAssertion?, unknownAbilityLog?, encodeDisplayText?, hasExtendedDisplayText? });
//   start.start() 는 `prepareWorld`(12차 스커미시만) → 원본 004c2b20 한 객체를 실행하고, 마지막 실제 효과가 성공한 뒤 같은 gate.started 만 쓴다.
//   start.createAi(owner) 는 원본 004c2d40: 호출자가 채운 600BYTE 에 기본값 → 공통(-1) → owner 설정을 누적한 뒤 같은 Session 의 실제 AI 생성/초기 갱신을 실행한다.
//   start.startBody() · start.gate() · start.findLivePriest(owner)(원본 004921f0, 포획 참여 fallback 과 구분) · start.testPlay() (항상 null, 아래 참고).
// session 계약(실제 `RawCampaignSession` 이식 전의 최소 덕타입, 4-D 구획 D 에서 같은 이름으로 구현한다): { core() → { pool }, priests() → { priests() → 사제 목록 상태 }, localOwner(), playerKnown(), editor(),
//   setSp(owner, float), clearTechnology(owner), addTechnology(owner, 목록 문자열), createAi(owner, 600BYTE) → 토큰 }.
// script 계약: `MissionScript`(mission-script.js)의 { loaded, get(키) → 문자열|null, set(키, 값), sectionNames(접두) }.
// AI 설정의 문자열 값은 원본 BYTE 를 문자 하나로 담은 문자열이다(키가 `name` 으로 끝나면 표시 부호화, 그 밖은 원문 부호화). 표시 부호화(`DisplayTextInput`, 6·13차 한국어/일본어)는 순서 10 몫이라
//   `effects.encodeDisplayText` 로 주입한다. 없으면 원문 부호화를 쓰며 한국어 확장 이름의 29BYTE 제한(`hasExtendedDisplayText` 가 있을 때만)도 그 주입과 함께 켠다.
// **옮기지 않은 것:** cpppj 의 `CampaignTestPlay`(테스트 전용 자동 플레이 `--test-*` 옵션, PORT_MAP exclude-host)와 그 호출(`CreateLocalTestAi`·시작 SP 변경)이다. 일반 사용자 실행과 편집 Test Battle 에서는
//   원본 Start 와 한 비트도 다르지 않으며, `effects.testPlay` 를 주면 명시 오류로 거부한다.
// 기준선 뒤 cpppj 변경(+5줄)은 위 테스트 전용 자동 플레이 호출이라 웹 결과에 영향이 없다(3.6절 ①). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E520: 4-D C 구획 RawCampaignMissionStartBindings 이식)
import { asciiLower } from '../assets/config.js';
import { encodeOriginalText } from '../assets/text-encoding.js';
import { MISSION_AI_SETTINGS_SIZE, RawMissionAiSettings } from './raw-mission-ai-settings.js';
import { RawMissionStart } from './raw-mission-start.js';
import { RawPriestLookup } from './raw-priest-lookup.js';

// 미션 AI 소유자 범위(1..8)와 확장 표시 이름의 BYTE 상한이다. (2026-11-15 WB04-E520)
const FIRST_AI_OWNER = 1;
const LAST_AI_OWNER = 8;
const EXTENDED_NAME_LIMIT = 29;
// 필수 외부 소비자 이름이다. 누락은 자동 GUI 통합 때 빈 성공으로 남지 않도록 최초 등록 전에 거부한다. (2026-11-15 WB04-E520)
const REQUIRED_EFFECTS = Object.freeze(['settingsBytes', 'gameSeconds', 'refreshSp', 'refreshCommandGump', 'randomGeysers', 'resetCamera']);

// 원본 BYTE 배열을 문자 하나당 BYTE 하나인 문자열로 바꾼다(AI 설정 복사가 하위 BYTE 만 쓴다). (2026-11-15 WB04-E520)
function byteString(bytes) {
    return Array.from(bytes, (byte) => String.fromCharCode(byte)).join('');
}

// 같은 Session/Script/장기 상태는 이 연결보다 오래 살아야 한다. 생성만으로 Start/AI 를 실행하지 않는다. (2026-11-15 WB04-E520)
export class RawCampaignMissionStartBindings {
    // SP/기술/AI/현재 Player 9 의 유일한 실제 수명 소유자와 현재 mission Config 를 공유하는 Script 이다.
    #session;
    #script;
    // 실제 프레임/Guide 가 사용하는 같은 started/start/demo/tellAt 필드이다.
    #gate;
    // 실제 UI/게임 시계/세계 랜덤 생성 외부 경계, 같은 사제 목록의 조회, 기존 600BYTE 설정 몸체와 공통 Start 이다.
    #effects;
    #priests;
    #settings;
    #start;

    // 모든 외부 소비자를 명시 요구한다. aiGlobals 는 실제 stuffRefreshRate/startingSp/진단 전역 저장소이며 기본 3/2500 의 새 사본을 만들지 않는다. (2026-11-15 WB04-E520)
    constructor(session, script, gate, state, globals, aiGlobals, effects) {
        if (!script?.loaded || REQUIRED_EFFECTS.some((name) => typeof effects?.[name] !== 'function')) {
            throw new TypeError('실제 MissionStart의 Script/설정초기입력/시계/UI/랜덤간헐천 경계 누락');
        }
        // 테스트 전용 자동 플레이는 웹 범위 밖이다. 값이 오면 조용히 무시하지 않고 거부한다.
        if (effects.testPlay !== undefined && effects.testPlay !== null) throw new TypeError('테스트 전용 자동 플레이(CampaignTestPlay)는 웹 범위 밖입니다');
        this.#session = session;
        this.#script = script;
        this.#gate = gate;
        this.#effects = effects;
        this.#priests = new RawPriestLookup(session.core().pool, session.priests().priests());
        this.#settings = new RawMissionAiSettings(aiGlobals, this.#aiHooks());
        this.#start = new RawMissionStart(gate, state, globals, this.#startHooks());
        this.#start.bindCurrentInputs({ localOwner: () => session.localOwner(), playerKnown: () => session.playerKnown(), editor: () => session.editor() });
    }

    // 원본 atof 의 좁은 문자열 변환과 현재 미션 문자열의 부호화 경계를 사용한다. 미등록 능력 이름의 진단/기록은 외부 장치가 있어야 실행한다. (2026-11-15 WB04-E520)
    #aiHooks() {
        const effects = this.#effects;
        return {
            get: (key) => {
                const display = asciiLower(key).length >= 4 && asciiLower(key).endsWith('name');
                const text = this.#script.get(key) ?? '';
                const bytes = display && typeof effects.encodeDisplayText === 'function' ? effects.encodeDisplayText(text) : encodeOriginalText(text);
                // 새 확장 Name 만 제한하고 기존 CP1252 대본의 원문 복사 규칙은 보존한다.
                if (display && typeof effects.hasExtendedDisplayText === 'function' && effects.hasExtendedDisplayText(bytes) && bytes.length > EXTENDED_NAME_LIMIT) {
                    throw new RangeError('확장 AI 이름은 한글 표시토큰을 포함해29BYTE 이하여야 합니다');
                }
                return byteString(bytes);
            },
            parseFloat: (value) => {
                // C strtod 와 같은 접두 해석: 앞 공백 뒤 부호/숫자/소수점/지수만 읽고 나머지는 버린다. 읽을 수 없으면 0 이다.
                const match = /^\s*[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?/.exec(value);
                return match ? Number.parseFloat(match[0]) : 0;
            },
            unknownAbilityAssertion: (name) => {
                if (typeof effects.unknownAbilityAssertion !== 'function') throw new Error('미션 AI의 실제 능력 이름 진단 미연결');
                effects.unknownAbilityAssertion(name);
            },
            unknownAbilityLog: (name) => {
                if (typeof effects.unknownAbilityLog !== 'function') throw new Error('미션 AI의 실제 능력 이름 기록 미연결');
                effects.unknownAbilityLog(name);
            },
        };
    }

    // 같은 현재 Script/Session 과 원본 getter 를 읽기 위치 그대로 연결한다. (2026-11-15 WB04-E520)
    #startHooks() {
        const session = this.#session;
        const script = this.#script;
        const effects = this.#effects;
        return {
            get: (key) => script.get(key) ?? '',
            sectionNames: (prefix) => script.sectionNames(prefix),
            set: (key, value) => { script.set(key, value); },
            gameSeconds: () => effects.gameSeconds(),
            setSp: (owner, money) => { session.setSp(owner, money); },
            refreshSp: () => { effects.refreshSp(); },
            clearTechnology: (owner) => { session.clearTechnology(owner | 0); },
            addTechnology: (owner, list) => { session.addTechnology(owner | 0, list); },
            hasPriest: (owner) => this.findLivePriest(owner) !== 0,
            createAi: (owner) => { this.createAi(owner); },
            refreshCommandGump: () => { effects.refreshCommandGump(); },
            randomGeysers: () => { effects.randomGeysers(); },
            resetCamera: () => { effects.resetCamera(); },
        };
    }

    // 12차 스커미시: 일반 미션은 prepareWorld 가 없어 기존과 같고, 스커미시는 Start 가 사제를 찾기 전에 MP 규칙 섬 생성을 먼저 실행한다. (2026-11-15 WB04-E520)
    start() {
        if (typeof this.#effects.prepareWorld === 'function') this.#effects.prepareWorld();
        this.#start.start();
    }

    // 호출자가 공급한 미초기화 영역에 기존 600BYTE 설정 초기화 순서를 한 번 적용한 뒤 같은 Session 의 실제 AI 를 만든다. 반환은 AI 본체 토큰이다. (2026-11-15 WB04-E520)
    createAi(owner) {
        if (!(owner >= FIRST_AI_OWNER && owner <= LAST_AI_OWNER)) throw new RangeError('미션 AI 소유자 범위 오류');
        const bytes = new Uint8Array(MISSION_AI_SETTINGS_SIZE);
        this.#effects.settingsBytes(owner, bytes);
        this.#settings.prepare(bytes, owner | 0);
        return this.#session.createAi(owner, bytes);
    }

    // 테스트 전용 자동 플레이 설정이다. 웹에는 해당 기능이 없어 항상 null 이다. (2026-11-15 WB04-E520)
    testPlay() { return null; }

    // 프레임/안내 소비자가 별도 Start 를 만들지 않고 같은 처리기를 쓰도록 주소만 빌려준다. (2026-11-15 WB04-E520)
    startBody() { return this.#start; }

    // 현재 프레임/Guide 의 장기 상태를 복사하지 않고 같은 게이트를 돌려준다. (2026-11-15 WB04-E520)
    gate() { return this.#gate; }

    // 실제 사제 목록의 현재 owner/dead 를 읽어 포획 상태를 추정하지 않는다. 없으면 0 이다. (2026-11-15 WB04-E520)
    findLivePriest(owner) { return this.#priests.find(owner); }
}
