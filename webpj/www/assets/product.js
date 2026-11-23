// 웹 판본의 제품 표기(버전 11.01)와 세 번째 Credits 항목을 원본 데이터 판본(10.78)과 분리해 제공하는 모듈이다.
// 원본 자료(tell·config·setup.cfg)는 바꾸지 않는다. 대신 다음 두 경로로 같은 Config/Tell/HTML 창 장치를 그대로 쓴다(cpppj `client/ProductPresentation.cpp` 와 같은 경계).
//  1) 설정 층 앞머리의 MAJOR_VERSION·MINOR_VERSION·version 줄을 제품 값(11·1·"v11.01")으로 쓴다. 원문 Startup 의 정수 연결은 "v11.1" 이 되므로 선행 0 을 명시한다.
//  2) 등록 설정 객체 `Patch1101` 에 `[Credits]` 섹션을 두고, 메인 Credits 본문 끝에 `$Menu=…,Tell,Patch1101.Credits` 한 줄만 덧붙인다.
//     Tell 의 `이름.섹션` 조회(원본 004cf730 → 0043fb90)가 이 객체의 섹션을 찾으므로 별도 창·파서를 만들지 않는다.
// 판독기·fixture·타입 표 등 원본 의미의 "10.78" 은 ORIGINAL_DATA_EDITION 으로 남기며, 제품 표기와 섞지 않는다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-05 WB03-AC: AGENTS.md 의 11.01 표기·세 번째 Patch Credits 요구를 webpj 에 적용하려고 추가)
import { asciiLower } from './config.js';

// 원본 자료와 판독 규칙이 따르는 판본(패치 10.78)이다. 제품 표기가 아니라 복원 기준을 나타낼 때만 쓴다. (2026-11-05 WB03-AC)
export const ORIGINAL_DATA_EDITION = '10.78';

// 제품 주 버전 번호이다. 설정 키 MAJOR_VERSION 의 값이 된다. (2026-11-05 WB03-AC)
export const PRODUCT_MAJOR_VERSION = 11;

// 제품 부 버전 번호이다. 설정 키 MINOR_VERSION 의 값이 된다(본문 `{MAJOR_VERSION}.{MINOR_VERSION}` 은 원본처럼 "11.1" 로 읽힌다). (2026-11-05 WB03-AC)
export const PRODUCT_MINOR_VERSION = 1;

// 사용자에게 보이는 제품 버전 표기이다(부 번호의 선행 0 을 보존). (2026-11-05 WB03-AC)
export const PRODUCT_VERSION = '11.01';

// 설정 키 `version` 의 값이다. About 창의 `Version {version}`·용어표 copyright 의 `{version}` 이 이 값으로 바뀐다. (2026-11-05 WB03-AC)
export const PRODUCT_VERSION_VALUE = `v${PRODUCT_VERSION}`;

// 추가 Credits 를 담는 등록 설정 객체 이름이다. cpppj kProductCreditsObject 와 같은 이름이다. (2026-11-05 WB03-AC)
export const PRODUCT_CREDITS_OBJECT = 'Patch1101';

// 추가 Credits 행이 Tell 명령으로 넘기는 대상이다(`객체.섹션`). cpppj kProductCreditsTarget 과 같다. (2026-11-05 WB03-AC)
export const PRODUCT_CREDITS_TARGET = `${PRODUCT_CREDITS_OBJECT}.Credits`;

// 세 번째 행을 덧붙일 원본 Tell 섹션 이름이다(메인 메뉴 Credits 버튼의 대상, 대소문자 무시 비교). (2026-11-05 WB03-AC)
const CREDITS_SECTION = 'credits';

// 정보(About) ▸ 버전 창의 Tell 섹션 이름이다(Help 메뉴 Version·게임 메뉴 About ▸ Version 의 대상, 대소문자 무시 비교). (2026-11-23 WB04-E1289)
const ABOUT_SECTION = 'about';
// 11.01 카피라이트 줄 본문이다. AGENTS.md 의 `Copyright 2026 HJOW` 표기이며 Patch Credits 본문·버전 창이 같은 줄을 쓴다(중복 삽입 검사에도 쓴다). (2026-11-23 WB04-E1289)
export const PRODUCT_COPYRIGHT_LINE = 'Copyright 2026 HJOW';
// 언어별 버전 창의 11.01 개발자 줄이다. cpppj ProductTellBody 의 About 확장과 같은 문구이며 기존 10.78 작성자(Ticonderoga) 줄은 바꾸지 않는다. (2026-11-23 WB04-E1289)
const ABOUT_PATCH_LINES = Object.freeze({
    english: 'NetStorm 11.01 Patch by HJOW.<br>',
    korean: 'NetStorm 11.01 패치: HJOW.<br>',
});

// 12차 스커미시: 설정 화면의 첫 Tell 대상(`Patch1101.Skirmish`)과 Campaign 목록(UCampaign)의 항목 이름이다. cpppj ProductTellBody 와 같다. (2026-11-20 WB04-E1102)
export const PRODUCT_SKIRMISH_TARGET = `${PRODUCT_CREDITS_OBJECT}.Skirmish`;
export const PRODUCT_SKIRMISH_LABELS = Object.freeze({ english: 'Skirmish', korean: '스커미시' });
// Skirmish 항목을 덧붙일 원본 Tell 섹션 이름이다(메인 메뉴 Campaign 버튼의 대상, 대소문자 무시 비교). (2026-11-20 WB04-E1102)
const CAMPAIGN_SECTION = 'ucampaign';
// 스커미시 화면 Tell 섹션의 자리 틀이다. 실제 본문은 앱이 설정으로 만들지만 Tell 이름 조회(`Patch1101.SkirmishCount` 등)가 섹션을 찾도록 같은 이름의 섹션이 있어야 한다(cpppj kPatchCredits). (2026-11-20 WB04-E1102)
const SKIRMISH_SECTIONS = ['[Skirmish]', '<h2>Skirmish</h2>', '[SkirmishCount]', '<h2>Skirmish</h2>', '[SkirmishAi]', '<h2>Skirmish</h2>', '[SkirmishBans]', '<h2>Skirmish</h2>', ''].join('\n');

// 15차 Reset All: 옵션 메뉴 마지막 행이 여는 확인 창의 Tell 대상과, 그 창의 확정 단추가 보내는 대상이다. cpppj ProductPresentation 의 `[ResetAll]` 섹션·`Patch1101.ResetAllConfirmed` 와 같은 이름이다.
// 확정 대상은 확인 창이 열린 직후 한 번만 효력이 있다(netstorm.js `_openTell`, 원본 UberGump 의 `resetAllSubmission_` 가드에 대응). (2026-11-23 WB04-E1251)
export const PRODUCT_RESET_ALL_TARGET = `${PRODUCT_CREDITS_OBJECT}.ResetAll`;
export const PRODUCT_RESET_ALL_CONFIRMED_TARGET = `${PRODUCT_CREDITS_OBJECT}.ResetAllConfirmed`;

// 언어별 Reset All 확인 창 섹션 원문이다. 영어는 cpppj 의 `[ResetAll]` 본문과 같고(웹은 게임을 종료하는 대신 다시 불러오므로 마지막 줄만 reload 로 적는다),
// 한국어는 cpppj ProductTellBody 의 한국어 본문과 같은 구성이다. 단추는 확정(`Patch1101.ResetAllConfirmed`)과 취소(`Blank`) 두 개이다. (2026-11-23 WB04-E1251)
const RESET_ALL_SECTIONS = Object.freeze({
    english: ['[ResetAll]', '<h2>Reset All</h2>', '<p>Reset all settings and campaign progress?<br>', 'The game will reload after resetting.', `$Button=Reset All,Tell,${PRODUCT_RESET_ALL_CONFIRMED_TARGET}`, '$Button=Cancel,Tell,Blank', ''].join('\n'),
    korean: ['[ResetAll]', '<h2>설정 초기화</h2>', '<p>모든 설정과 캠페인 진행 상황을 초기화하시겠습니까?<br>', '초기화 후 게임을 다시 불러옵니다.', `$Button=초기화,Tell,${PRODUCT_RESET_ALL_CONFIRMED_TARGET}`, '$Button=취소,Tell,Blank', ''].join('\n'),
});

// 제품 Credits 를 제공하는 언어(파일 확장자 이름)별 목록 행 이름이다. 영어는 AGENTS.md 가 정한 표기 그대로이고,
// 한국어는 기존 한국어 Tell 의 "NetStorm 10.72 패치 크레딧" 번역 규칙을 따른다. 없는 언어는 영어를 쓴다. (2026-11-05 WB03-AC)
export const PRODUCT_CREDITS_LABELS = Object.freeze({
    english: 'Netstorm 11.01 Patch Credits',
    korean: 'NetStorm 11.01 패치 크레딧',
});

// 언어별 제품 Credits 창 제목이다. 영어는 AGENTS.md 가 정한 대소문자 그대로이다. (2026-11-05 WB03-AC)
export const PRODUCT_CREDITS_TITLES = Object.freeze({
    english: 'NetStorm 11.01 Patch Credits',
    korean: 'NetStorm 11.01 패치 크레딧',
});

// 언어별 제품 Credits 섹션 원문이다. 원본 Credits 창들과 같은 배경·제목·{CText} 소제목 형식이며, Back 은 Credits 목록으로, Cancel 은 메인 메뉴로 돌아간다.
// 본문은 HJOW 의 개발·패치와 AI 도움(분석·프로그래밍·문서화·검증)을 알리고, 원작 제작진은 기존 두 Credits 항목에 있음을 밝힌다. `{version}` 은 제품 표기로 치환된다. (2026-11-05 WB03-AC)
// 개발 줄 다음에 11.01 카피라이트 줄 `Copyright 2026 HJOW` 를 영어·한국어 본문에 둔다. 사용자 커밋 60226ea9(AGENTS.md)의 요구이며 cpppj client/ProductPresentation.cpp(제품 1291)와 같은 위치다. (2026-11-11 WB)
const PRODUCT_CREDITS_SECTIONS = Object.freeze({
    english: [
        '[Credits]',
        '<background="nsstart">',
        `<h2>{H2}${PRODUCT_CREDITS_TITLES.english}</h2>`,
        '<p>{Text}',
        'Version {version}',
        '<p>',
        '{CText}Development{Text}<br>',
        'HJOW, NetStorm 11.01 patch development.<br>',
        'Copyright 2026 HJOW<br>',
        '<br>',
        '{CText}AI Assistance{Text}<br>',
        'Developed with AI assistance using OpenAI Codex, ChatGPT and Anthropic Claude Code.<br>',
        'AI was used to assist analysis, programming, documentation and verification.<br>',
        '<br>',
        'The original game creators are acknowledged in the original Credits entries.',
        '$Button=Back,Tell,Credits',
        '$Button=Cancel,Tell,Blank',
        '',
    ].join('\n'),
    korean: [
        '[Credits]',
        '<background="nsstart">',
        `<h2>{H2}${PRODUCT_CREDITS_TITLES.korean}</h2>`,
        '<p>{Text}',
        '버전 {version}',
        '<p>',
        '{CText}개발{Text}<br>',
        'HJOW, NetStorm 11.01 패치 개발.<br>',
        'Copyright 2026 HJOW<br>',
        '<br>',
        '{CText}AI 사용{Text}<br>',
        'OpenAI Codex, ChatGPT, Anthropic Claude Code 등 AI의 도움을 받아 개발했습니다.<br>',
        'AI는 분석, 프로그래밍, 문서화, 검증을 돕는 데 사용했습니다.<br>',
        '<br>',
        '원작 게임 제작진은 기존 크레딧 항목에 소개되어 있습니다.',
        '$Button=뒤로,Tell,Credits',
        '$Button=취소,Tell,Blank',
        '',
    ].join('\n'),
});

// 제품 표기를 제공하는 언어 이름으로 바꾼다. 목록에 없는 언어는 영어이다. (2026-11-05 WB03-AC)
function productLanguage(language) {
    return Object.hasOwn(PRODUCT_CREDITS_SECTIONS, language) ? language : 'english';
}

// 언어의 제품 Credits 섹션 원문(`[Credits]` 머리 포함)을 돌려준다. 등록 설정 객체 Patch1101 의 버퍼로 쓴다.
// 사용: new Config(registry, PRODUCT_CREDITS_OBJECT).append(productCreditsText('korean')). (2026-11-05 WB03-AC)
// 15차: 같은 등록 설정 객체에 Reset All 확인 창 섹션(`[ResetAll]`)도 둔다. Tell `Patch1101.ResetAll` 이 이 섹션을 찾는다. (2026-11-23 WB04-E1251)
export function productCreditsText(language) {
    return PRODUCT_CREDITS_SECTIONS[productLanguage(language)] + SKIRMISH_SECTIONS + RESET_ALL_SECTIONS[productLanguage(language)];
}

// 언어의 세 번째 Credits 목록 행 이름을 돌려준다. (2026-11-05 WB03-AC)
export function productCreditsLabel(language) {
    return PRODUCT_CREDITS_LABELS[productLanguage(language)];
}

// 메인 Credits 섹션 본문에 세 번째 행 하나만 덧붙인 본문을 돌려준다. 다른 섹션이거나, 본문에 이미 제품 대상이 있으면(중복 확장 방지) 원문 그대로이다.
// target 은 지역 인자를 뗀 Tell 섹션 이름이며 대소문자를 무시해 비교한다(원본 Tell 이름 비교 규칙). 원본 두 행·Cancel 버튼은 그대로 둔다.
// 사용: productTellBody('Credits', 원본본문, 'english') → 원본본문 + "\n$Menu=Netstorm 11.01 Patch Credits,Tell,Patch1101.Credits\n". (2026-11-05 WB03-AC)
// 12차 스커미시: skirmish 가 참이면 Campaign 목록(UCampaign)에도 Skirmish 항목을 원문 목록과 첫 버튼 사이에 한 번만 넣는다(cpppj ProductTellBody). 스커미시를 켜지 않은 판본은 원문 그대로이다. (2026-11-20 WB04-E1102)
// 버전 창(About): 첫 `$Button=` 앞에 "NetStorm 11.01 Patch by HJOW." 와 "Copyright 2026 HJOW" 두 줄을 한 번만 넣는다(버튼이 없으면 끝). 한국어는 "NetStorm 11.01 패치: HJOW." 이다. (2026-11-23 WB04-E1289, 사용자 요구)
export function productTellBody(target, body, language, { skirmish = false } = {}) {
    const lower = asciiLower(String(target));
    if (lower === ABOUT_SECTION) { // 버전 창은 원작 저작권·10.78 패치 줄을 그대로 두고 확인 단추 앞에 11.01 개발자와 저작권 줄만 더한다(cpppj ProductTellBody 의 About 확장).
        if (body.includes(PRODUCT_COPYRIGHT_LINE)) return body; // 같은 줄이 이미 있으면 중복해서 넣지 않는다.
        const patch = `${ABOUT_PATCH_LINES[productLanguage(language)]}\n${PRODUCT_COPYRIGHT_LINE}<br>\n`, button = body.indexOf('$Button=');
        return button === -1 ? `${body}${patch}` : `${body.slice(0, button)}${patch}${body.slice(button)}`;
    }
    if (skirmish && lower === CAMPAIGN_SECTION) {
        if (body.includes(PRODUCT_SKIRMISH_TARGET)) return body;
        const entry = `$Menu=${PRODUCT_SKIRMISH_LABELS[productLanguage(language)]},Tell,${PRODUCT_SKIRMISH_TARGET}\n`, back = body.indexOf('$Button=');
        return back === -1 ? `${body}${entry}` : `${body.slice(0, back)}${entry}${body.slice(back)}`;
    }
    if (lower !== CREDITS_SECTION || body.includes(PRODUCT_CREDITS_TARGET)) return body;
    return `${body}\n$Menu=${productCreditsLabel(language)},Tell,${PRODUCT_CREDITS_TARGET}\n`;
}
