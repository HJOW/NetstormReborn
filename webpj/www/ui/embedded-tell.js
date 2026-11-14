// 원본 004c8f60의 내장 설명(Tell) 조회와 HtmlBody 생성/serial 반환을 옮긴다(cpppj client/EmbeddedTell.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 일반 004cf730 팝업과 별개의 내장 HtmlBody 경로다. 내장 설명은 빈 섹션도 실제 HtmlBody 를 생성하며 일반 팝업의 이름 기록/param/BLANK/등록 객체 조회를 하지 않는다.
// 현재 Mission → 매번 새로 읽는 tell 설정(fallback) → HtmlBody 생성 순서이며, 두 조회가 모두 실패하면 경고 뒤 0 을 돌려준다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다).
// 사용: showEmbeddedTell('Tips', { mission, fallback, missing, construct, setHtml, serial }) → serial|0, showEmbeddedTellInConfig('Tips', gameData, mission, hooks) 는 실제 tell 파일을 임시 Config 로 읽는 fallback 을 붙인다.
// (2026-11-14 WB04-E437: RawMenuControls 선행 내장 Tell 이식)
import { Config } from '../assets/config.js';
import { decodeOriginalText } from '../assets/text-encoding.js';

// 실제 선택한 장치만 연결을 요구한다. 도달하지 않는 fallback/경고는 요구하지 않는다(logic_error→Error).
function need(value) {
    if (typeof value !== 'function') throw new Error('내장 TELL의 실제 선택된 장치 미연결');
    return value;
}
// 값이 없음(nullopt)인지 판정한다. 빈 문자열은 존재하는 빈 섹션이다.
const absent = (value) => value === null || value === undefined;

// 원문 좁은 키를 실제 UTF-8 Config 경계에서만 해독한다(원문 DecodeDisplayText). 모든 글자가 0..255 이면 바이트로 보고 해독하며, 이미 해독된 문자열(255 초과 글자 포함)은 그대로 둔다.
// TellSection(E461)도 같은 경계 변환을 쓰므로 내보낸다.
export function decodeKey(key) {
    if (/[^\u0000-ÿ]/.test(key)) return key;
    return decodeOriginalText(Uint8Array.from(key, (character) => character.charCodeAt(0)));
}

// 0x4c8f60: name 은 첫 NUL 앞까지만 쓰며 hooks = { mission?(name) → 본문|null, fallback(name) → 본문|null, missing(name), construct() → 토큰(0 이면 확보 실패), setHtml(token, body), serial(token) → serial }.
// mission 은 선택 장치(없으면 건너뛴다). mission 이 null 이면 fallback 을 부르고, 둘 다 null 이면 missing(name) 뒤 0 을 돌려준다. 빈 문자열 본문은 있는 것이다.
// 본문이 있으면 construct → setHtml(token, body) → 본문 소유권 반납 뒤 serial(token) 이다(현재 부모의 serial 을 다시 읽는다). construct 가 0 이면 Error(원문 bad_alloc).
export function showEmbeddedTell(name, hooks) {
    const nul = name.indexOf('\0');
    name = nul < 0 ? name : name.slice(0, nul);
    let body = typeof hooks.mission === 'function' ? hooks.mission(name) : null;
    if (absent(body)) body = need(hooks.fallback)(name);
    if (absent(body)) {
        need(hooks.missing)(name);
        return 0;
    }
    const token = need(hooks.construct)() >>> 0;
    if (!token) throw new Error('내장 TELL HtmlBody 확보 실패');
    need(hooks.setHtml)(token, body);
    body = null;
    return need(hooks.serial)(token) >>> 0;
}

// 실제 파일 경로: fallback 은 현재 missionSpec 의 tell 키로 매번 임시 Config('tell')에 읽어 해독한 이름의 섹션을 돌려주고 그 임시 Config 는 UI 생성 전에 등록부에서 내린다
// (등록된 tell 캐시를 재사용하지 않는다). mission 이 있으면 키를 해독해 넘긴다. config 는 GameData 같은 설정 인터페이스({ registry, pathSpec, loadFile })이며 hooks 는 복사해서 쓴다.
// 호출자의 hooks.mission/fallback 은 이 함수가 덮어쓴다. 사용: showEmbeddedTellInConfig('Tips', data, (key) => mission.section(key), { missing, construct, setHtml, serial }).
export function showEmbeddedTellInConfig(name, config, mission, hooks) {
    const wrapped = { ...hooks };
    wrapped.mission = typeof mission === 'function' ? (key) => mission(decodeKey(key)) : undefined;
    wrapped.fallback = (key) => {
        const temporary = new Config(config.registry, 'tell');
        try {
            const path = config.pathSpec('missionSpec', 'tell');
            config.loadFile(temporary, path);
            const section = temporary.section(decodeKey(key));
            // 반환 전에 본문을 복사한 문자열로 확정한다(임시 Config 는 finally 에서 내려간다).
            return section === null ? null : String(section);
        } finally {
            temporary.dispose();
        }
    };
    return showEmbeddedTell(name, wrapped);
}
