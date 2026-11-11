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
export function productCreditsText(language) {
    return PRODUCT_CREDITS_SECTIONS[productLanguage(language)];
}

// 언어의 세 번째 Credits 목록 행 이름을 돌려준다. (2026-11-05 WB03-AC)
export function productCreditsLabel(language) {
    return PRODUCT_CREDITS_LABELS[productLanguage(language)];
}

// 메인 Credits 섹션 본문에 세 번째 행 하나만 덧붙인 본문을 돌려준다. 다른 섹션이거나, 본문에 이미 제품 대상이 있으면(중복 확장 방지) 원문 그대로이다.
// target 은 지역 인자를 뗀 Tell 섹션 이름이며 대소문자를 무시해 비교한다(원본 Tell 이름 비교 규칙). 원본 두 행·Cancel 버튼은 그대로 둔다.
// 사용: productTellBody('Credits', 원본본문, 'english') → 원본본문 + "\n$Menu=Netstorm 11.01 Patch Credits,Tell,Patch1101.Credits\n". (2026-11-05 WB03-AC)
export function productTellBody(target, body, language) {
    if (asciiLower(String(target)) !== CREDITS_SECTION || body.includes(PRODUCT_CREDITS_TARGET)) return body;
    return `${body}\n$Menu=${productCreditsLabel(language)},Tell,${PRODUCT_CREDITS_TARGET}\n`;
}
