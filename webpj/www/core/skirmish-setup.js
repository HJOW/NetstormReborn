// 12차 스커미시의 순수 규칙을 옮긴 모듈이다(cpppj `client/SkirmishSetup.cpp`·`SkirmishStartIslands.inc`). Campaign 메뉴의 Skirmish 설정 화면(AI 수·AI별 난이도/동맹·금지 유닛·시작/취소)과 시작 대본·시작 섬 선택·빈 월드 fort 를 만든다.
// 원본 10.78 에는 스커미시가 없으므로 화면은 원본 HTML 창(Tell)과 같은 문법의 본문 문자열로 만들고, 시작은 생성한 미션 대본 + 기존 다인용 fort 의 영역으로 한다. 이 모듈은 DOM·설정 저장소·세션에 의존하지 않는다.
// 사용:
//   createSkirmishSettings() → 기본 설정(AI 1명·보통·적·금지 없음).   loadSkirmishSettings(get) / saveSkirmishSettings(set, settings) → get(key)→문자열, set(key, value) 로 왕복한다.
//   parseSkirmishTarget(target) → Tell 대상 `Patch1101.Skirmish…(a,b)` 를 { kind, args } 로 해석한다(대소문자 무시, 아니면 kind 'none').
//   applySkirmishAction(settings, args) → Set 대상의 동작(count/diff/ally/ban/clearbans)을 적용하고 다음에 보일 Tell 대상 문자열을 돌려준다.
//   buildSkirmishScreen(target, settings, language, translate) → 화면 한 장의 Tell 본문이다. language 는 원문 언어 번호(0/1 영어, 5 일본어, 7 한국어)이다.
//   buildSkirmishLaunch(settings, randomValue) → { script, islands, central, mapSize }: 같은 난수는 같은 자료이며 후보가 모자라면 islands 가 비어 있다.
//   makeSkirmishEmptyWorldFort(baseFort) → Chaff/Territory/Terr 를 비운 가상 fort 바이트(잘못된 입력은 길이 0).   isSkirmishMissionName(name) → 생성 미션 이름인지(대소문자 무시).
// 난수는 표준 mt19937 이다. 시작 섬 뽑기는 cpppj 의 std::shuffle 구현 의존 순서가 아니라 뒤에서부터의 Fisher–Yates 로 하므로 같은 씨앗의 정확한 결과는 cpppj 와 다를 수 있다(무작위 배치라 허용, 성질은 같은 시험으로 확인).
// (2026-11-20 WB04-E1101: 12차 스커미시 설정·화면·시작 자료 이식)
import { AiDifficulty, aiDifficultyFromValue, aiDifficultyToValue } from './ai-difficulty.js';
import { TERRITORY_PATTERNS } from '../assets/territory-patterns.js';

// BeginMission 이 이 이름이면 파일 대신 생성한 대본/fort 를 읽는 생성 미션 이름이다. (2026-11-20 WB04-E1101)
export const SKIRMISH_MISSION_NAME = 'Skirmish';
// 설정 화면이 열 수 있는 AI 수의 최대값이다(소유자 2..8). (2026-11-20 WB04-E1101)
export const SKIRMISH_MAX_AI = 7;
// 금지 유닛의 최대 개수이다(AGENTS 12차: 최대 5개). (2026-11-20 WB04-E1101)
export const SKIRMISH_MAX_BANS = 5;
// 한 화면에 보이는 금지 유닛 항목 수이다(창 높이 제한으로 쪽을 나눈다). (2026-11-20 WB04-E1101)
export const SKIRMISH_BANS_PER_PAGE = 12;
// 스커미시 Tell 대상의 소문자 접두(제품 전용 Patch1101 객체)이다. (2026-11-20 WB04-E1101)
export const SKIRMISH_TARGET_PREFIX = 'patch1101.skirmish';

// 금지 유닛 후보 표이다. type 은 미션 techAllowed 에 쓰는 내부 타입 이름이고 label 은 화면에 보이는 영어 이름(번역 키)이다(일부 짝은 이름 규칙에서 추정). (2026-11-20 WB04-E1101)
export const SKIRMISH_BANNABLE = Object.freeze([
    ['windBattery', 'Wind Generator'], ['rainBattery', 'Rain Generator'], ['thunderBattery', 'Thunder Generator'],
    ['sunArcher', 'Sun Disc Thrower'], ['windArcher', 'Crossbow'], ['thunderArcher', 'Vander Tower'],
    ['sunCannon', 'Sun Cannon'], ['rainCannon', 'Ice Cannon'], ['thunderCannon', 'Thunder Cannon'],
    ['sunBlocker', 'Stone Tower'], ['windBlocker', 'Wind Tower'], ['rainBlocker', 'Ice Tower'], ['thunderBlocker', 'Bulwark'],
    ['sunFence', 'Sun Barricade'], ['rainFence', 'Acid Barricade'], ['thunderFence', 'Arc Spire'],
    ['sunBalloon', 'Whirligig'], ['windBalloon', 'Dust Devil'], ['rainBalloon', "Man o'War"],
    ['sunAviary', 'Whirlibase'], ['windAviary', 'Devil Maker'], ['rainAviary', "Man o'War Pool"],
    ['sunWalker', 'Fire Ant'], ['windWalker', 'Sail Skater'], ['rainWalker', 'Crystal Crab'],
    ['bulf', 'Bulf'], ['outpost', 'Outpost'],
].map(([type, label]) => Object.freeze({ type, label })));

// 시작 섬 후보 표이다: 사제 1명만 있는 단일 소유자 영역 중 3x3 이하 패턴, 원본 d/*.fort 그대로(cpppj 자동 생성 표). 각 항목 = { fort 이름, 영역 번호 }이다. (2026-11-20 WB04-E1101)
export const SKIRMISH_START_ISLANDS = Object.freeze([
    ['FWOS9', 3], ['Imp1', 0], ['Imp1', 1], ['Imp1', 2], ['Imp1', 4], ['Imp1', 5], ['Imp1', 6], ['Imp1', 7], ['Imp3', 5],
    ['LCA', 0], ['LCA', 1], ['Razorianwar6', 5], ['Thundere', 1], ['Thundere', 2], ['TpVpath', 0],
    ['school5', 0], ['school5', 1], ['school5', 2], ['sh3_n_2', 4], ['sw_m2', 2], ['tnron42', 5], ['tnron51', 4],
].map(([fort, territory]) => Object.freeze({ fort, territory })));

// 언어별 고정 문구 묶음이다. {n}/{a}/{b}/{c} 는 호출자가 값을 채운다. 영어 문구이다. (2026-11-20 WB04-E1101)
const ENGLISH = Object.freeze({
    title: 'Skirmish', intro: 'Choose your opponents and press Start. Islands and geysers are random.', countLine: 'AI players: {n}',
    aiLine: 'AI {a}: {b} / {c}', bansLine: 'Banned units: {n}/5', start: 'Start', cancel: 'Cancel', back: 'Back', countTitle: 'Number of AI players',
    countIntro: 'Pick how many AI players join this game.', aiTitle: 'AI {a}', aiIntro: "Choose this AI's difficulty and side.", bansTitle: 'Banned units',
    bansIntro: 'Select up to 5 units or buildings that nobody may build. Selected items are marked [X].', easy: 'Easy', normal: 'Normal', hard: 'Hard',
    enemy: 'Enemy', ally: 'Ally', next: 'Next page', previous: 'Previous page', clear: 'Clear all', maxNote: 'Five units are banned. Unmark one to choose another.',
});
// 한국어 문구이다(건물·유닛 이름은 번역 hook 이 맡는다). (2026-11-20 WB04-E1101)
const KOREAN = Object.freeze({
    title: '스커미시', intro: '상대를 고른 뒤 시작을 누르세요. 섬과 간헐천은 무작위입니다.', countLine: '인공지능 수: {n}',
    aiLine: 'AI {a}: {b} / {c}', bansLine: '금지 유닛: {n}/5', start: '시작', cancel: '취소', back: '뒤로', countTitle: '인공지능 수',
    countIntro: '이 게임에 참가할 인공지능 수를 고르세요.', aiTitle: 'AI {a}', aiIntro: '이 인공지능의 난이도와 진영을 고르세요.', bansTitle: '금지 유닛',
    bansIntro: '아무도 지을 수 없는 건물·유닛을 최대 5개까지 고르세요. 고른 항목은 [X]로 표시됩니다.', easy: '쉬움', normal: '보통', hard: '어려움',
    enemy: '적', ally: '동맹', next: '다음 쪽', previous: '이전 쪽', clear: '모두 해제', maxNote: '금지 유닛이 5개입니다. 다른 것을 고르려면 하나를 해제하세요.',
});
// 일본어 문구이다(13차 일본어 지원 때 다시 점검한다). (2026-11-20 WB04-E1101)
const JAPANESE = Object.freeze({
    title: 'スカーミッシュ', intro: '対戦相手を選んで<br>開始を押してください。<br>島と間欠泉は<br>ランダムです。', countLine: 'AI人数: {n}',
    aiLine: 'AI {a}: {b} / {c}', bansLine: '禁止ユニット: {n}/5', start: '開始', cancel: 'キャンセル', back: '戻る', countTitle: 'AIの人数',
    countIntro: 'このゲームに参加する<br>AIの人数を選んでください。', aiTitle: 'AI {a}', aiIntro: 'このAIの難易度と<br>陣営を選んでください。', bansTitle: '禁止ユニット',
    bansIntro: '誰も建てられない建物や<br>ユニットを最大5つまで<br>選びます。選んだ項目は<br>[X]で表示されます。', easy: '易しい', normal: '通常', hard: '硬い',
    enemy: '敵', ally: '同盟', next: '次のページ', previous: '前のページ', clear: 'すべて解除', maxNote: '禁止ユニットが5つです。<br>別のものを選ぶには<br>1つ解除してください。',
});

// 원문 언어 번호에 맞는 문구 묶음이다(7 한국어, 5 일본어, 그 밖은 영어). (2026-11-20 WB04-E1101)
function wordsFor(language) { return language === 7 ? KOREAN : language === 5 ? JAPANESE : ENGLISH; }
// 문구의 {a}/{b}/{c}/{n} 자리를 이 순서대로 값으로 바꾼다. 같은 자리가 여러 번 나와도 모두 바꾸고 바뀐 값 안은 다시 찾지 않는다. (2026-11-20 WB04-E1101)
function fill(text, a = '', b = '', c = '', n = '') {
    let result = String(text);
    // 네 자리표시자를 차례로 치환한다.
    for (const [key, value] of [['{a}', a], ['{b}', b], ['{c}', c], ['{n}', n]]) result = result.split(key).join(String(value));
    return result;
}
// 난이도의 화면 이름이다. (2026-11-20 WB04-E1101)
function difficultyName(words, level) { return level === AiDifficulty.Easy ? words.easy : level === AiDifficulty.Hard ? words.hard : words.normal; }
// 소유자 번호(2..8)에 대응하는 설정 칸 번호(0..6)이다. 범위 밖은 끝으로 줄인다. (2026-11-20 WB04-E1101)
function slotOf(owner) { return Math.min(Math.max(owner, 2), 8) - 2; }
// 쉼표로 구분한 문자열을 나눈다(빈 칸은 버린다). (2026-11-20 WB04-E1101)
function splitCommas(text) { return String(text).split(',').filter((piece) => piece !== ''); }
// 10진 정수 전체가 맞을 때만 해석하고(부호는 '-' 만, 32비트 범위) 아니면 fallback 이다(C++ from_chars 와 같다). (2026-11-20 WB04-E1101)
function toInt(text, fallback) {
    if (!/^-?[0-9]+$/u.test(String(text))) return fallback;
    const value = Number(text);
    return value >= -2147483648 && value <= 2147483647 ? value : fallback;
}
// 내부 타입 이름이 금지 후보인지 판정한다. (2026-11-20 WB04-E1101)
function isBannable(type) { return SKIRMISH_BANNABLE.some((item) => item.type === type); }
// ASCII 대문자만 소문자로 바꾼다(C 로캘의 tolower 와 같다). (2026-11-20 WB04-E1101)
function lower(text) { return String(text).replace(/[A-Z]/gu, (letter) => letter.toLowerCase()); }
// 창 공통 머리(배경·제목·본문 시작)이다. (2026-11-20 WB04-E1101)
function head(title, intro) { return `<background="nsstart">\n<h2>{H2}${title}</h2>\n<p>{Text}\n${intro}<br>\n`; }

// 기본 설정이다: AI 1명, 모든 AI 보통·적, 금지 없음. (2026-11-20 WB04-E1101)
export function createSkirmishSettings() {
    return { aiCount: 1, ai: Array.from({ length: SKIRMISH_MAX_AI }, () => ({ difficulty: AiDifficulty.Normal, ally: false })), bans: [] };
}

// 설정에서 읽는다. 값이 없거나 범위 밖이면 기본값(AI 1명·보통·적·금지 없음)이다. get(key) 는 값 문자열(없으면 빈 문자열)을 돌려준다. (2026-11-20 WB04-E1101)
export function loadSkirmishSettings(get) {
    const settings = createSkirmishSettings();
    settings.aiCount = Math.min(Math.max(toInt(get('skirmishAiCount'), 1), 1), SKIRMISH_MAX_AI);
    // 소유자 2..8 의 난이도/동맹을 읽는다.
    for (let owner = 2; owner <= 8; owner += 1) {
        const ai = settings.ai[slotOf(owner)];
        ai.difficulty = aiDifficultyFromValue(toInt(get(`skirmishAi${owner}Diff`), 2));
        ai.ally = toInt(get(`skirmishAi${owner}Ally`), 0) !== 0;
    }
    // 저장된 금지 목록에서 후보에 있는 항목만(중복 없이) 최대 5개 받는다.
    for (const type of splitCommas(get('skirmishBans') ?? '')) {
        if (settings.bans.length < SKIRMISH_MAX_BANS && isBannable(type) && !settings.bans.includes(type)) settings.bans.push(type);
    }
    return settings;
}

// 설정에 쓴다. set(key, value) 는 값을 문자열로 받는다. (2026-11-20 WB04-E1101)
export function saveSkirmishSettings(set, settings) {
    set('skirmishAiCount', String(settings.aiCount));
    // 소유자 2..8 의 난이도/동맹을 쓴다.
    for (let owner = 2; owner <= 8; owner += 1) {
        const ai = settings.ai[slotOf(owner)];
        set(`skirmishAi${owner}Diff`, String(aiDifficultyToValue(ai.difficulty)));
        set(`skirmishAi${owner}Ally`, ai.ally ? '1' : '0');
    }
    set('skirmishBans', settings.bans.join(','));
}

// Tell 대상 문자열을 해석한다: "Patch1101.Skirmish"(Main)·Count·Ai(2)·Bans(0)·Set(ban,1,0)·Start. 스커미시 대상이 아니면 kind 가 'none' 이다. 인자는 소문자로 읽는다. (2026-11-20 WB04-E1101)
export function parseSkirmishTarget(target) {
    const result = { kind: 'none', args: [] };
    const text = lower(target);
    if (!text.startsWith(SKIRMISH_TARGET_PREFIX)) return result;
    let rest = text.slice(SKIRMISH_TARGET_PREFIX.length); // "", "count", "ai(2)", "set(ban,1,0)" 등이다.
    let args = '';
    const open = rest.indexOf('(');
    if (open !== -1) {
        const close = rest.lastIndexOf(')');
        if (close !== -1 && close > open) args = rest.slice(open + 1, close);
        rest = rest.slice(0, open);
    }
    result.args = splitCommas(args);
    const kinds = { '': 'main', count: 'count', ai: 'ai', bans: 'bans', set: 'set', start: 'start' };
    if (Object.hasOwn(kinds, rest)) result.kind = kinds[rest];
    return result;
}

// Set 동작을 적용하고 다음 화면 대상을 돌려준다. 알 수 없는 동작은 첫 화면(Main)이다. (2026-11-20 WB04-E1101)
export function applySkirmishAction(settings, args) {
    const main = 'Patch1101.Skirmish';
    if (!args.length) return main;
    const verb = args[0];
    if (verb === 'count' && args.length >= 2) {
        settings.aiCount = Math.min(Math.max(toInt(args[1], settings.aiCount), 1), SKIRMISH_MAX_AI);
        return main;
    }
    if ((verb === 'diff' || verb === 'ally') && args.length >= 3) {
        const owner = Math.min(Math.max(toInt(args[1], 2), 2), 8);
        const ai = settings.ai[slotOf(owner)];
        if (verb === 'diff') ai.difficulty = aiDifficultyFromValue(toInt(args[2], 2));
        else ai.ally = toInt(args[2], 0) !== 0;
        return `${main}Ai(${owner})`; // 같은 AI 화면에서 이어서 고른다.
    }
    if (verb === 'ban' && args.length >= 3) {
        const index = Math.max(0, toInt(args[1], 0));
        if (index < SKIRMISH_BANNABLE.length) {
            const type = SKIRMISH_BANNABLE[index].type, found = settings.bans.indexOf(type);
            if (found !== -1) settings.bans.splice(found, 1); // 이미 금지면 해제한다.
            else if (settings.bans.length < SKIRMISH_MAX_BANS) settings.bans.push(type); // 6번째 선택은 무시한다(최대 5개).
        }
        return `${main}Bans(${Math.max(0, toInt(args[2], 0))})`;
    }
    if (verb === 'clearbans') { settings.bans.length = 0; return `${main}Bans(0)`; }
    return main;
}

// 화면 한 장의 Tell 본문을 만든다. translate(label) 은 유닛 이름 번역(없으면 영어 이름 그대로)이다. (2026-11-20 WB04-E1101)
export function buildSkirmishScreen(target, settings, language, translate = null) {
    const words = wordsFor(language);
    const side = (ally) => (ally ? words.ally : words.enemy);
    let body;
    if (target.kind === 'count') { // AI 수 고르기
        body = head(words.countTitle, words.countIntro);
        // 1..7 항목을 만들고 현재 값에 표시한다.
        for (let count = 1; count <= SKIRMISH_MAX_AI; count += 1) body += `$Menu=${count === settings.aiCount ? '* ' : ''}${count},Tell,Patch1101.SkirmishSet(count,${count})\n`;
        body += `$Button=${words.back},Tell,Patch1101.Skirmish\n`;
    } else if (target.kind === 'ai') { // 한 AI 의 난이도/진영 고르기
        const owner = Math.min(Math.max(target.args.length ? toInt(target.args[0], 2) : 2, 2), 8);
        const ai = settings.ai[slotOf(owner)];
        body = head(fill(words.aiTitle, String(owner - 1)), words.aiIntro);
        // 세 난이도 항목이다.
        for (const level of [AiDifficulty.Easy, AiDifficulty.Normal, AiDifficulty.Hard]) {
            body += `$Menu=${level === ai.difficulty ? '* ' : ''}${difficultyName(words, level)},Tell,Patch1101.SkirmishSet(diff,${owner},${aiDifficultyToValue(level)})\n`;
        }
        // 두 진영 항목이다.
        for (const ally of [false, true]) body += `$Menu=${ally === ai.ally ? '* ' : ''}${side(ally)},Tell,Patch1101.SkirmishSet(ally,${owner},${ally ? 1 : 0})\n`;
        body += `$Button=${words.back},Tell,Patch1101.Skirmish\n`;
    } else if (target.kind === 'bans') { // 금지 유닛 고르기
        const pageCount = Math.ceil(SKIRMISH_BANNABLE.length / SKIRMISH_BANS_PER_PAGE);
        const page = Math.min(target.args.length ? Math.max(0, toInt(target.args[0], 0)) : 0, pageCount - 1);
        body = head(words.bansTitle, settings.bans.length >= SKIRMISH_MAX_BANS ? words.maxNote : words.bansIntro);
        // 이 쪽의 항목을 만든다. 금지 상태는 [X]/[ ]로 표시하고 내부 이름을 함께 보인다.
        for (let index = page * SKIRMISH_BANS_PER_PAGE; index < Math.min(SKIRMISH_BANNABLE.length, (page + 1) * SKIRMISH_BANS_PER_PAGE); index += 1) {
            const { type, label } = SKIRMISH_BANNABLE[index];
            const banned = settings.bans.includes(type), name = translate ? translate(label) : label;
            body += `$Menu=${banned ? '[X] ' : '[ ] '}${name} - ${type},Tell,Patch1101.SkirmishSet(ban,${index},${page})\n`;
        }
        if (page + 1 < pageCount) body += `$Menu=${words.next},Tell,Patch1101.SkirmishBans(${page + 1})\n`;
        if (page > 0) body += `$Menu=${words.previous},Tell,Patch1101.SkirmishBans(${page - 1})\n`;
        body += `$Menu=${words.clear},Tell,Patch1101.SkirmishSet(clearbans)\n$Button=${words.back},Tell,Patch1101.Skirmish\n`;
    } else { // 첫 설정 화면
        body = head(words.title, words.intro);
        body += `$Menu=${fill(words.countLine, '', '', '', String(settings.aiCount))},Tell,Patch1101.SkirmishCount\n`;
        // 선택한 수만큼 AI 줄을 만든다(소유자 2부터).
        for (let owner = 2; owner < 2 + settings.aiCount; owner += 1) {
            const ai = settings.ai[slotOf(owner)];
            body += `$Menu=${fill(words.aiLine, String(owner - 1), difficultyName(words, ai.difficulty), side(ai.ally))},Tell,Patch1101.SkirmishAi(${owner})\n`;
        }
        body += `$Menu=${fill(words.bansLine, '', '', '', String(settings.bans.length))},Tell,Patch1101.SkirmishBans(0)\n`;
        body += `$Button=${words.start},Tell,Patch1101.SkirmishStart\n$Button=${words.cancel},Tell,UCampaign\n`;
    }
    return body;
}

// 표준 mt19937(32비트 메르센 트위스터) 난수기이다. 씨앗 하나로 같은 수열을 만든다. next() 는 0..2^32-1 의 부호 없는 정수이다. (2026-11-20 WB04-E1101)
export class Mt19937 {
    // 씨앗 하나로 624칸 상태를 초기화한다(표준 init_genrand 와 같다). (2026-11-20 WB04-E1101)
    constructor(seed) {
        this.state = new Uint32Array(624);
        this.index = 624;
        this.state[0] = seed >>> 0;
        // 앞 칸에서 다음 칸을 만든다.
        for (let at = 1; at < 624; at += 1) this.state[at] = (Math.imul(1812433253, this.state[at - 1] ^ (this.state[at - 1] >>> 30)) + at) >>> 0;
    }

    // 다음 32비트 값을 돌려주며 상태를 다 쓰면 624칸을 다시 섞는다. (2026-11-20 WB04-E1101)
    next() {
        const state = this.state;
        if (this.index >= 624) {
            // 624칸을 한 번에 섞는다.
            for (let at = 0; at < 624; at += 1) {
                const mixed = (state[at] & 0x80000000) | (state[(at + 1) % 624] & 0x7fffffff);
                state[at] = (state[(at + 397) % 624] ^ (mixed >>> 1) ^ ((mixed & 1) ? 0x9908b0df : 0)) >>> 0;
            }
            this.index = 0;
        }
        let value = state[this.index];
        this.index += 1;
        value ^= value >>> 11; value = (value ^ ((value << 7) & 0x9d2c5680)) >>> 0;
        value = (value ^ ((value << 15) & 0xefc60000)) >>> 0; value ^= value >>> 18;
        return value >>> 0;
    }
}

// 설정과 난수 씨앗으로 시작 자료를 만든다. 시작 섬은 후보 표에서 서로 다른 것을 뽑고(소유자 1..N+1 순서), 중앙섬은 3x3 패턴 중 무작위 패턴/회전/씨앗이다. (2026-11-20 WB04-E1101)
export function buildSkirmishLaunch(settings, randomValue) {
    const launch = { script: '', islands: [], central: new Uint8Array(6), mapSize: 0 };
    const players = Math.min(Math.max(settings.aiCount, 1), SKIRMISH_MAX_AI) + 1;
    if (SKIRMISH_START_ISLANDS.length < players) return launch;
    const random = new Mt19937(randomValue);
    // 후보 표에서 참가자 수만큼 서로 다른 시작 섬을 뽑는다: 번호표를 뒤에서부터 섞는다(MP 에서 참가자마다 자기 fort 영역을 쓰는 것과 같다).
    const order = SKIRMISH_START_ISLANDS.map((_, index) => index);
    for (let at = order.length - 1; at > 0; at -= 1) {
        const pick = random.next() % (at + 1);
        [order[at], order[pick]] = [order[pick], order[at]];
    }
    for (let slot = 0; slot < players; slot += 1) launch.islands.push(SKIRMISH_START_ISLANDS[order[slot]]);
    // 참가자가 많을수록 링 간격이 넓은 맵 크기를 쓴다(2명 간격4, 3~4명 간격5, 5명 이상 간격6).
    launch.mapSize = players <= 2 ? 1 : players <= 4 ? 2 : 3;
    // 중앙섬은 3x3 패턴 가운데서 무작위로 고르고 회전/씨앗도 무작위다(MP 의 $Random 중앙섬 기록과 같은 형식).
    const bigPatterns = [];
    // 폭·높이가 모두 3 인 패턴 번호를 모은다.
    for (let index = 0; index < TERRITORY_PATTERNS.length; index += 1) if (TERRITORY_PATTERNS[index].width === 3 && TERRITORY_PATTERNS[index].height === 3) bigPatterns.push(index);
    const pattern = bigPatterns.length ? bigPatterns[random.next() % bigPatterns.length] : 0;
    const rotation = random.next() & 3, seed = random.next() & 0xff;
    launch.central.set([(pattern | (rotation << 6)) & 0xff, 0x09, 0x00, seed, 0x00, 0x00]);
    let script = '[Header]\nmissionType="Tutorial"\n\nmissionNumber=1\nmoreGeysers=1\nrandGeysers=1\n\nmyStartMoney=5000\nmyTech="all"\n';
    if (settings.bans.length) script += `techAllowed="deny${settings.bans.map((type) => `;${type}`).join('')}"\n`;
    const allies = [];
    // 플레이어와 같은 편인 AI 소유자를 모은다.
    for (let owner = 2; owner < 2 + settings.aiCount; owner += 1) if (settings.ai[slotOf(owner)].ally) allies.push(owner);
    script += `myAllyList="${allies.join(';')}"\ntitle="Skirmish"\n`;
    // 각 AI 의 기본 설정과 같은 편/적 편 동맹 목록을 만든다(같은 편끼리 동맹이다).
    for (let owner = 2; owner < 2 + settings.aiCount; owner += 1) {
        const ally = settings.ai[slotOf(owner)].ally, same = [];
        // 이 AI 와 같은 편인 모든 소유자(자신 포함, 플레이어 편이면 1 도 포함)를 나열한다.
        for (let other = 1; other < 2 + settings.aiCount; other += 1) {
            if (other === owner || (other === 1 ? ally : (other >= 2 && settings.ai[slotOf(other)].ally === ally))) same.push(other);
        }
        script += `\nai${owner}Name="Computer #${owner - 1}"\nai${owner}Tech="all"\nai${owner}StartMoney=5000\n`;
        script += `ai${owner}GeyserAttachments=3\nai${owner}Collectors=6\nai${owner}StuffRefreshRate=1\nai${owner}TimeBetweenMoves=2\n`;
        script += `ai${owner}BridgeDrawRate=1\nai${owner}AllyList="${same.join(';')}"\n`;
    }
    script += `\n[Options]\n\n[A.]\n<h2>Skirmish</h2>\n<i>${settings.aiCount} AI</i>\n\n$Button=Go!,DoNothing,0\n\n`
        + '[Succeeded][BadTeamDead]\n<h2>Victory!</h2>\n<p>\nEvery enemy has been defeated.\n$Button=Leave,MissionAbort,0\n\n'
        + '[Failed][GoodTeamDead]\n<h2>Defeat</h2>\n<p>\nYour team has fallen.\n$Button=Leave,MissionAbort,0\n\n[END]\n';
    launch.script = script;
    return launch;
}

// 스커미시용 빈 월드 fort 를 만든다: 섹션 35개 중 Chaff(4)/Territory(6)/Terr00..19(15..34)만 비우고 나머지(기술·덱·돈 등)는 기준 fort 그대로 둔다. 잘못된 입력은 길이 0 의 Uint8Array 이다. (2026-11-20 WB04-E1101)
export function makeSkirmishEmptyWorldFort(baseFort) {
    const SECTIONS = 35, CHAFF = 4, TERRITORY = 6, FIRST_TERR = 15, TERRITORY_BYTES = 120, CHUNK_RECORDS = 256, CHUNK_MARK = 99; // 섹션 수와 각 섹션 위치/크기(원본 저장 순서)와 청크 레코드 표지(0x63)이다.
    const empty = new Uint8Array(0);
    if (!baseFort || baseFort.length < 2 || baseFort[0] !== 0x46) return empty; // 서명 'F' 가 아니면 거부한다.
    const sections = [];
    let position = 2;
    // 각 섹션의 [길이(자기 2바이트 포함)][본문]을 읽는다.
    while (sections.length < SECTIONS) {
        if (position + 2 > baseFort.length) return empty;
        const length = baseFort[position] | (baseFort[position + 1] << 8);
        if (length < 2 || position + length > baseFort.length) return empty;
        sections.push(Array.from(baseFort.subarray(position + 2, position + length)));
        position += length;
    }
    sections[CHAFF] = [2]; // 버전 2 뒤에 빈 청크 레코드 256개(표지, 개수0)를 둔다.
    // 빈 청크 레코드 256개를 붙인다.
    for (let chunk = 0; chunk < CHUNK_RECORDS; chunk += 1) sections[CHAFF].push(CHUNK_MARK, 0, 0);
    sections[TERRITORY] = new Array(TERRITORY_BYTES).fill(0); // 영역 표를 모두 비운다(섬 없음).
    // Terr 섹션은 버전만 남긴다.
    for (let index = FIRST_TERR; index < SECTIONS; index += 1) sections[index] = [2];
    const result = [baseFort[0], baseFort[1]];
    // 각 섹션을 [길이][본문]으로 다시 쓴다.
    for (const body of sections) {
        const length = body.length + 2;
        result.push(length & 0xff, length >>> 8, ...body);
    }
    return Uint8Array.from(result);
}

// 이 이름이 생성 스커미시 미션인지 판정한다(대소문자 무시). (2026-11-20 WB04-E1101)
export function isSkirmishMissionName(name) { return lower(name) === lower(SKIRMISH_MISSION_NAME); }
