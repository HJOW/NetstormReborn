// 원본 MessageGump의 TextGump 공통 생성과 종류별 메시지를 옮긴다(004d0600, 10.78 전용).
// 기준선 1f00a25a의 11/16/17과 후속 일반 전투 안내를 같은 현재 노드/이름/번역/StyleText에 연결한다. CD 분기는 웹 제외다.
// 사용: new MessageGump(createState, hooks).construct(token, kind, player, parameter, source|null). 이름/본문의 실제 BYTE 뷰는 서식 위치까지 빌린다.
// (2026-11-14 WB04-E452: RawMenuControls 선행 메시지 생성자와 기준선 뒤 공통 안내 본문 이식)
import { TextGumpNode } from './text-gump.js';

// 원본 메시지 VT·플레이어 색 두 표·시간/정지 번역 키다. 응답 지연만 네 번째 색이 u다.
const VTABLE = 0x517b60; const COLORS = 'brwgmylo'; const INACTIVE_COLORS = 'brwumylo';
const TIME_TEXT = '~E~yGame Time~r:~w %d:%02d:%02d'; const PAUSE_TEXT = '~E~wGame Paused!';
// 원본 10.78에서 복원한 종류다. 다른 종류는 공통 생성 전에 거부한다.
const SUPPORTED = new Set([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18]);
// 실제 첫 이름 포인터를 소비하는 종류다. 그 밖에는 이름 조회의 효과만 수행한다.
const NAMED = new Set([2, 3, 4, 5, 6, 7, 8]);

// 선택된 이름/시계/소리/Idle 경계만 현재 위치에서 요구한다.
function need(hook) { if (typeof hook !== 'function') throw new Error('메시지의 선택된 실제 장치 미연결'); return hook; }
// 원문 바이트 문자열 또는 BYTE 뷰의 첫 NUL까지 읽는다. 이름 포인터만 NUL이 반드시 있어야 한다.
function text(value, requireNul = false) {
    if (typeof value === 'string') { const end = value.indexOf('\0'); if (requireNul && end < 0) throw new RangeError('메시지 실제 이름 NUL 범위'); return end < 0 ? value : value.slice(0, end); }
    const end = value.indexOf(0); if (requireNul && end < 0) throw new RangeError('메시지 실제 이름 NUL 범위');
    const length = end < 0 ? value.length : end; let result = '';
    // 실제 BYTE 의미를 유지하며 UTF-8 코드포인트로 길이를 바꾸지 않는다.
    for (let index = 0; index < length; index += 1) result += String.fromCharCode(value[index]);
    return result;
}

// 고정 폭/정밀도의 문자열 또는 signed 정수 서식만 처리한다. 미공급 가변 인자·%n 등은 명시 거부한다.
function format(pattern, expected, values, Invalid = Error) {
    pattern = text(pattern); const tokens = []; let used = 0; let begin = 0;
    // 실제 서식을 먼저 검사해 미공급 인자 읽기나 NUL 범위 밖 이름 조회를 하지 않는다.
    for (let index = 0; index < pattern.length; index += 1) {
        if (pattern[index] !== '%') continue;
        tokens.push(pattern.slice(begin, index)); index += 1;
        if (pattern[index] === '%') { tokens.push('%'); begin = index + 1; continue; }
        let left = false; if (pattern[index] === '-') { left = true; index += 1; }
        const widthStart = index;
        // 폭의 숫자만 읽는다. 가변폭 *는 허용하지 않는다.
        while (index < pattern.length && /[0-9]/.test(pattern[index])) index += 1;
        const widthText = pattern.slice(widthStart, index); const width = Number(widthText || 0); let precision = null;
        if (pattern[index] === '.') {
            index += 1; const start = index;
            // 문자열 정밀도도 실제 BYTE 수이며 빈 정밀도는 0이다.
            while (index < pattern.length && /[0-9]/.test(pattern[index])) index += 1;
            precision = Number(pattern.slice(start, index) || 0);
        }
        const type = pattern[index]; const wanted = expected[used];
        if (used >= expected.length || (type !== wanted && !(wanted === 'd' && type === 'i'))) throw new Invalid('메시지의 실제 인자 형식/순서 범위');
        if (width >= 256) throw new RangeError('메시지 원문256BYTE 서식 범위');
        tokens.push({ index: used, type: wanted, left, width, precision, zero: widthText.startsWith('0') }); used += 1; begin = index + 1;
    }
    tokens.push(pattern.slice(begin)); const args = values(); let result = '';
    // 검사가 끝난 뒤 현재 인자를 실제 CRT 순서로 반영하고 최종 256BYTE를 넘지 않는다.
    for (const token of tokens) {
        if (typeof token === 'string') result += token;
        else {
            let value;
            if (token.type === 's') { value = args[token.index]; if (token.precision !== null) value = value.slice(0, token.precision); }
            else {
                const number = args[token.index] | 0; const negative = number < 0; let digits = String(Math.abs(number));
                if (token.precision === 0 && number === 0) digits = '';
                if (token.precision !== null) { if (token.precision >= 256) throw new RangeError('메시지 원문256BYTE 서식 범위'); digits = digits.padStart(token.precision, '0'); }
                if (token.zero && !token.left && token.precision === null) digits = digits.padStart(Math.max(0, token.width - (negative ? 1 : 0)), '0');
                value = (negative ? '-' : '') + digits;
            }
            result += token.left ? value.padEnd(token.width, ' ') : value.padStart(token.width, ' ');
        }
        if (result.length >= 256) throw new RangeError('메시지 원문256BYTE 서식 범위');
    }
    return result;
}

// VS2003 CRT의 NULL %s=(null), 빈 문자열, 명시 뷰의 첫 NUL을 구별한다.
export function formatWarningMessage(pattern, source) {
    const current = source === null || source === undefined ? '(null)' : text(source);
    return format(pattern, 's', () => [current]);
}

// 같은 기본 TextGump와 메시지 검색 키/기한을 보관한다. reserved/trailing은 생성자가 쓰지 않는다.
export class MessageGumpNode extends TextGumpNode {
    // +a4/+a8/+ac의 종류·플레이어·미작성 필드다.
    kind = 0; player = 0; reserved = 0;
    // +b0 double 기한과 +b8/+bc 매개변수/미작성 필드다.
    deadline = 0; parameter = 0; trailing = 0;
}

// 공통 생성자·현재 이름·실제 스타일/측정을 같은 노드에 적용한다.
export class MessageGump {
    // 호출 위치의 현재 상태를 조회하는 함수 목록이다. 생성 시 값으로 보관한다.
    #hooks;
    // 웹은 고정 10.78이다. create의 같은 collection과 일곱 필수 장치를 확인한다.
    constructor(create, hooks) {
        if (!create?.collection) throw new TypeError('메시지 실제 TextGump/생성 상태 연결 누락');
        // 미선택 Challenge 이름/소리/시계는 생성 시 요구하지 않는다.
        for (const name of ['node', 'constructText', 'challenge', 'playerName', 'translate', 'style', 'fit']) if (typeof hooks?.[name] !== 'function') throw new TypeError('메시지 실제 TextGump/이름/번역/측정 연결 누락');
        this.#hooks = { ...hooks };
    }
    // 경계 호출 뒤 실제 현재 노드를 다시 빌린다. 별도 기본 노드를 만들지 않는다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node) throw new Error('메시지의 현재 공유 노드 없음'); return node; }
    // 기한/Idle 공통 꼬리다. 이름·번역·Style 효과가 바꾼 현재 노드에 기한을 쓴다.
    #deadline(token, seconds, checkIdleFirst = false) { const h = this.#hooks; if (checkIdleFirst) { need(h.wallSeconds); need(h.idle); } const now = need(h.wallSeconds)(); this.#node(token).deadline = now + seconds; need(h.idle)(token, true); }

    // 원문 이름 조회 뒤의 현재 kind/player/parameter를 같은 분기 위치에서 다시 읽는다.
    construct(token, kind, player, parameter, source = null) {
        const h = this.#hooks; token >>>= 0; kind |= 0; player |= 0; parameter |= 0;
        if (!SUPPORTED.has(kind)) throw new Error('다른 원본 메시지 종류의 생성 분기 미복원');
        h.constructText(token); this.#node(token).vtable = VTABLE; this.#node(token).deadline = 0;
        this.#node(token).kind = kind; this.#node(token).player = player; this.#node(token).parameter = parameter;
        let firstName = new Uint8Array(0);
        if (NAMED.has(kind)) firstName = need(h.challenge() ? h.challengePlayerText : h.playerText)(player);
        else if (h.challenge()) need(h.challengePlayerName)(player);
        else h.playerName(player);
        // 현재 서식을 번역한 뒤 앞서 보관한 실제 이름 BYTE를 읽는 한 이름 안내다.
        const named = (pattern) => { const translated = h.translate(pattern); h.style(token, formatWarningMessage(translated, text(firstName, true))); };
        // 두 이름 안내는 번역 효과 뒤 현재 parameter의 두 번째 주소를 조회한다.
        const allied = (pattern) => { const translated = h.translate(pattern); const second = need(h.playerText)(this.#node(token).parameter); h.style(token, format(translated, 'ss', () => [text(firstName, true), text(second, true)], TypeError)); };
        if (this.#node(token).kind === 3 || this.#node(token).kind === 5) {
            if (this.#node(token).kind === 5) { need(h.warningSound)(); const owner = this.#node(token).player; if (owner >= 1 && owner <= 8) named(`~E~${COLORS[owner - 1]}%s~w has disconnected`); }
            else if (h.challenge()) named('~E~w%s is present');
            else if (need(h.battle)()) { const owner = this.#node(token).player; if (owner >= 1 && owner <= 8) named(`~E~${COLORS[owner - 1]}%s~w is present`); }
            this.#deadline(token, 10, true);
        } else if (this.#node(token).kind === 4 || this.#node(token).kind === 18) {
            if (this.#node(token).kind === 4) {
                need(h.warningSound)(); const owner = this.#node(token).player;
                if (owner >= 1 && owner <= 8) { const pattern = h.translate(`~E~${COLORS[owner - 1]}%s~w sent you %d Storm Power`); h.style(token, format(pattern, 'sd', () => [text(firstName, true), parameter], TypeError)); }
            } else { const pattern = h.translate('~E~wResource Injection of %d sp was received'); const amount = this.#node(token).parameter; h.style(token, format(pattern, 'd', () => [amount], TypeError)); }
            this.#deadline(token, 10, true);
        } else if (this.#node(token).kind === 1 || this.#node(token).kind === 2) {
            need(h.warningSound)();
            if (this.#node(token).kind === 1) h.style(token, h.translate('~wServer not responding'));
            else { const owner = this.#node(token).player; if (owner >= 1 && owner <= 8) named(`~E~${INACTIVE_COLORS[owner - 1]}%s~w not responding`); }
        } else if (this.#node(token).kind === 6 || this.#node(token).kind === 8) {
            const broken = this.#node(token).kind === 8; if (broken) need(h.warningSound)(); const row = this.#node(token).player;
            if (!broken) { if (row >= 1 && row <= 8) named(`~E~${COLORS[row - 1]}%s~w wishes to declare a Draw`); }
            else { const column = this.#node(token).parameter; if (row >= 1 && row <= 8 && column >= 1 && column <= 8) allied(`~E~${COLORS[row - 1]}%s~w and ~${COLORS[column - 1]}%s~w have broken their Alliance`); }
            this.#deadline(token, 10, true);
        } else if (this.#node(token).kind === 7 || this.#node(token).kind === 9 || this.#node(token).kind === 10) {
            if (this.#node(token).kind === 10) h.style(token, h.translate('~E~wGame completed!'));
            else if (this.#node(token).kind === 9) h.style(token, h.translate('~E~wGame started!'));
            else { need(h.warningSound)(); const row = this.#node(token).player; const column = this.#node(token).parameter; if (row >= 1 && row <= 8 && column >= 1 && column <= 8) allied(`~E~${COLORS[row - 1]}%s~w and ~${COLORS[column - 1]}%s~w are now Allies`); }
            this.#deadline(token, 10, true);
        } else if (this.#node(token).kind === 16) {
            const seconds = need(h.gameTimeSeconds)() | 0; const minutesTotal = (seconds / 60) | 0;
            const hours = (minutesTotal / 60) | 0; const minutes = minutesTotal % 60; const remainder = (seconds % 60) % 60;
            const pattern = h.translate(TIME_TEXT); h.style(token, format(pattern, 'ddd', () => [hours, minutes, remainder])); this.#deadline(token, 0.1);
        } else if (this.#node(token).kind === 17) {
            need(h.warningSound)(); const pattern = h.translate('%s'); h.style(token, formatWarningMessage(pattern, source)); this.#deadline(token, 10);
        } else h.style(token, h.translate(PAUSE_TEXT));
        h.fit(token, 0, 0); return token;
    }
}
