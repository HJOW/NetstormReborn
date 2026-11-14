// 원본 4cf730 의 파라미터 처리 뒤 현재 이름 기록과 세 단계 섹션 조회·실제 Config 연결을 옮긴다(cpppj client/TellSection.cpp/.h, 기준선 1f00a25a, 10.78 경로).
// 현재 Tell 이름은 Client 수명의 첫 NUL 문자열 256BYTE(005cab40)이며 조회가 실패해도 새 이름을 기록하고 NUL 뒤 바이트를 지우지 않는다. 이름이 대소문자 무시로 달라질 때만 이전 이름을 global.priorTell 에 쓴다.
// 조회 순서는 ① 이름에 점이 있으면 등록된 접두 객체의 섹션(파일을 새로 적재하지 않음) → ② 현재 미션 → ③ 매번 새로 적재하는 임시 Config('tell')이다. 없는 섹션(null)과 존재하는 빈 본문('')을 구별한다.
// 표시 부호화(6차 한국어 4BYTE 확장)는 E269 정책 ① 웹 제외라 설정 경계의 Decode/Encode 는 원본 Windows-1252/UTF-8 변환(`decodeOriginalText`/`encodeOriginalText`)만 쓴다.
// 사용: const state = createTellSectionState(); findTellSectionInConfig(state, 'About', data, mission) → 본문|null, showTellInConfig(state, 'Foo(1,2)', 0, data, mission, show) → { found, serial }. (2026-11-14 WB04-E461)
import { Config, asciiLower } from '../assets/config.js';
import { encodeOriginalText } from '../assets/text-encoding.js';
import { decodeKey } from './embedded-tell.js';
import { parseTellParameters } from './tell-parameters.js';

// 현재 Tell 이름 저장소의 BYTE 수다(NUL 포함). 이름은 이 크기 미만이어야 한다.
export const TELL_CURRENT_BYTES = 256;

// 현재 Tell 이름 저장소를 만든다. 같은 Client 수명의 장기 상태이며 창을 바꿔도 다시 만들지 않는다(ClientHtmlWindowGlobals.tell).
export function createTellSectionState() {
    return { current: new Uint8Array(TELL_CURRENT_BYTES) };
}

// 선택된 실제 조회만 연결을 요구한다. 도달하지 않는 조회는 요구하지 않는다(C++ std::logic_error → Error).
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Tell 섹션의 선택된 실제 조회 미연결');
    return hook;
}
// 값이 없음(nullopt)인지 판정한다. 빈 문자열은 존재하는 빈 섹션이다.
const absent = (value) => value === null || value === undefined;
// 첫 NUL 앞까지의 원문 BYTE 문자열을 돌려준다(C 문자열 경계).
function cString(value) { const end = value.indexOf('\0'); return end < 0 ? value : value.slice(0, end); }

// 이름을 현재 이름에 기록하고 이름/현재 미션/fallback 세 단계로 섹션 본문을 찾는다. name 은 파라미터를 제거한 좁은 이름(BYTE 문자열)이다.
// hooks = { prior(이전 이름), named(접두, 섹션)→본문|null, mission(이름)→본문|null, fallback(이름)→본문|null }. 반환은 본문(빈 문자열 포함) 또는 null 이다.
// 이름 복사는 콜백이 원래 입력 저장소를 바꿔도 지역 256BYTE 사본을 쓴다. 지역 이름이 256BYTE 이상이거나 현재 이름 저장소에 NUL 이 없으면 아무것도 쓰기 전에 RangeError 이다(원본 지역 256BYTE 범위).
export function findTellSection(state, name, hooks) {
    const local = cString(name);
    if (local.length >= state.current.length) throw new RangeError('Tell 이름의 원본256BYTE 지역 범위 초과');
    // 이름은 원문 BYTE 문자열이다. 255 를 넘는 글자는 Uint8Array 가 조용히 잘라 쓰므로 아무것도 쓰기 전에 거부한다(웹 경계 보호).
    for (let index = 0; index < local.length; index += 1) if (local.charCodeAt(index) > 255) throw new RangeError('Tell 이름은 원문 BYTE 문자열이어야 합니다');
    const end = state.current.indexOf(0);
    if (end < 0) throw new RangeError('Tell 현재 이름의 원본 NUL 범위 초과');
    // 현재 이름 저장소의 첫 NUL 앞 BYTE 를 이전 이름으로 읽는다.
    let previous = '';
    for (let index = 0; index < end; index += 1) previous += String.fromCharCode(state.current[index]);
    if (asciiLower(previous) !== asciiLower(local)) need(hooks.prior)(previous);
    // 새 이름을 NUL 까지 기록한다. NUL 뒤의 기존 바이트는 지우지 않는다.
    for (let index = 0; index < local.length; index += 1) state.current[index] = local.charCodeAt(index);
    state.current[local.length] = 0;
    const dot = local.indexOf('.');
    if (dot >= 0) {
        const body = need(hooks.named)(local.slice(0, dot), local.slice(dot + 1));
        if (!absent(body)) return body;
    }
    const mission = need(hooks.mission)(local);
    if (!absent(mission)) return mission;
    const fallback = need(hooks.fallback)(local);
    return absent(fallback) ? null : fallback;
}

// 원문 좁은 키/이름을 실제 UTF-8 Config 경계에서만 해독한다(Windows-1252 바이트 → 문자열, 이미 해독된 문자열은 그대로).
const decode = decodeKey;
// 현재 Config 의 문자열 치환 결과를 원본 좁은 BYTE 문자열로 되돌린다. 나타낼 수 없는 글자는 RangeError 이다.
function encode(text) { return Array.from(encodeOriginalText(text), (byte) => String.fromCharCode(byte)).join(''); }

// findTellSection 을 실제 설정 인터페이스(config = GameData 같은 { registry, set, pathSpec, loadFile })에 연결한다. mission(이름)→본문|null 은 현재 미션 조회(없으면 건너뜀)다.
// prior 는 global.priorTell 에 기존 원문 이름을 쓰고(저장 통지 없이), named 는 등록된 접두 객체만 조회하며 없는 접두어의 파일 오류를 새로 만들지 않는다.
// fallback 은 같은 이름의 기존 tell 객체와 구별되는 새 임시 Config('tell')에 missionSpec 의 tell 파일을 매번 적재해 섹션을 복사한 뒤 반환 전에 등록부에서 내린다.
export function findTellSectionInConfig(state, name, config, mission) {
    const hooks = {
        prior: (previous) => { config.set('global.priorTell', decode(previous), { silent: true }); },
        named: (prefix, section) => {
            const object = config.registry.findByName(decode(prefix));
            if (!object) return null;
            const body = object.section(decode(section));
            return body === null ? null : String(body);
        },
        mission: (target) => (typeof mission === 'function' ? mission(decode(target)) : null),
        fallback: (target) => {
            const temporary = new Config(config.registry, 'tell');
            try {
                config.loadFile(temporary, config.pathSpec('missionSpec', 'tell'));
                const body = temporary.section(decode(target));
                // 반환 전에 본문을 복사한 문자열로 확정한다(임시 Config 는 finally 에서 내려간다).
                return body === null ? null : String(body);
            } finally {
                temporary.dispose();
            }
        },
    };
    return findTellSection(state, name, hooks);
}

// 원문 4cf730 의 지역 param 생성/파싱/조회/전체 공장 호출/반납을 같은 실제 등록부에서 수행한다. original 은 `이름(인자,…)` 형식의 원문, suppress 는 전체 공장의 억제 인자다.
// show(본문, suppress, 0, 0) → serial 은 실제 전체 공장 호출이다. 섹션이 없으면 공장을 부르지 않고 { found: false, serial: 0 } 이다. 빈 본문도 존재하는 섹션이다.
// param 객체는 두 번의 HTML 변환과 메타 처리 동안 같은 등록부에 남아 있어야 하므로 show 가 돌아온 뒤(오류 포함)에야 내린다.
export function showTellInConfig(state, original, suppress, config, mission, show) {
    const parameters = new Config(config.registry, 'param');
    try {
        const name = parseTellParameters(original, {
            substitute: (value) => encode(config.substitute(decode(value))),
            append: (line) => parameters.append(decode(line)),
        });
        const body = findTellSectionInConfig(state, name, config, mission);
        if (absent(body)) return { found: false, serial: 0 };
        return { found: true, serial: need(show)(body, suppress >>> 0, 0, 0) >>> 0 };
    } finally {
        parameters.dispose();
    }
}
