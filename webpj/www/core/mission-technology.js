// 원본 Mission 의 techAllowed 목록(`;` 구분)을 현재 건설/생산이 함께 쓰는 허용 표에 적용하는 모듈이다(cpppj `client/MissionTechnology.cpp/.h`, 10.78, 원본 00482eb0).
//  - applyMissionTechnology(hooks): 허용 상태 allowed = 1 에서 시작해 인덱스 0 부터 `hooks.item(index)` 로 **매 항목마다 현재 목록을 다시 조회**한다. 첫 NUL 앞이 빈 항목이면 종료(그 뒤는 읽지 않는다).
//    항목을 ASCII 소문자로 낮춘 값이 `deny` 면 allowed = 0, `allow` 면 allowed = 1(둘 다 표를 쓰지 않음), `all` 이면 `hooks.allowAll(allowed)`(첫 255칸에 금지값), 그 밖이면 `hooks.allow(hooks.findType(원문 항목), allowed)`
//    로 타입 한 칸을 쓴다. 공백은 새로 다듬지 않는다. 미등록 이름의 찾기 결과 -1 도 그대로 쓰기 몸체에 넘겨 실제 저장소의 범위 거부를 성공으로 바꾸지 않는다.
//  - applyMissionTechnologyToTotalMade(script, types, totalMade): 실제 미션 스크립트(`script.get('techAllowed')` — 없으면 빈 목록)·보호 타입 표(`types.find(이름)` → 번호, 없으면 -1)·월드의 `RawTotalMade`(allowAll/allow)에 같은 목록 효과를 적용한다.
//  - applyMissionTechnologyToRestrictions(script, types, restrictions): 월드 설치 전 Init Header 가 쓰는 전역 금지 표(`Uint32Array`)에 `setTotalMadeAllowAll`/`setTotalMadeAllow` 로 같은 쓰기를 한다. 이후 Session 이 같은 배열을 빌린다.
//  - 목록은 원본 좁은 문자(Windows-1252)로 변환해 읽는다(`encodeOriginalText`). 첫 NUL 앞이 512BYTE 이상이면 원본 512BYTE 임시 버퍼 범위로 보고 `RangeError` 로 거부한다. 별도 허용 사본을 만들지 않는다.
//  C++ 오버로드 세 개를 이름으로 나눴다(JS 는 오버로드가 없음). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E338: 4-D B 구획 MissionTechnology 이식)
import { asciiLower } from '../assets/config.js';
import { encodeOriginalText } from '../assets/text-encoding.js';
import { configListItem } from './raw-mission-ai-settings.js';
import { setTotalMadeAllow, setTotalMadeAllowAll } from './raw-total-made.js';

// 원본 임시 버퍼 크기이다. 첫 NUL 앞 목록이 이 길이 이상이면 거부한다. (2026-11-12 WB04-E338)
const TEMPORARY_BYTES = 512;

// 선택된 실제 효과의 누락은 도달한 위치에서 즉시 드러낸다. 도달하지 않은 효과는 요구하지 않는다. (2026-11-12 WB04-E338)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('미션 기술 허용의 실제 조회/쓰기 연결 누락');
    return effect;
}

// 첫 NUL 앞까지의 C 문자열 부분이다. (2026-11-12 WB04-E338)
function visibleText(text) {
    const nul = text.indexOf('\0');
    return nul < 0 ? text : text.slice(0, nul);
}

// 00482eb0 본문이다: 현재 목록을 매 항목마다 다시 얻고 첫 빈 항목에서 끝낸다. (2026-11-12 WB04-E338)
//  hooks = { item(index) → 문자열, findType(이름) → 번호(-1 은 미등록), allowAll(allowed), allow(type, allowed) }.
export function applyMissionTechnology(hooks) {
    let allowed = 1;
    // 같은 현재 Config 에서 인덱스별 항목을 다시 얻으며 첫 빈 항목 뒤는 읽지 않는다.
    for (let index = 0; ; index += 1) {
        const item = need(hooks?.item)(index | 0);
        const visible = visibleText(String(item));
        if (visible === '') return;
        const lower = asciiLower(visible);
        if (lower === 'deny') allowed = 0;
        else if (lower === 'allow') allowed = 1;
        else if (lower === 'all') need(hooks.allowAll)(allowed);
        else need(hooks.allow)(need(hooks.findType)(visible), allowed);
    }
}

// UTF-16 문자열 목록을 원본 좁은 문자 바이트 문자열로 바꾸고 512BYTE 임시 버퍼 범위를 검사한 뒤 index 번째 조각을 돌려주는 조회기를 만든다. (2026-11-12 WB04-E338)
function scriptHooks(script, types) {
    return {
        item(index) {
            const bytes = encodeOriginalText(script.get('techAllowed') ?? '');
            // 바이트마다 한 글자로 옮겨(인자 수 제한을 피해) 좁은 문자 목록을 만든다.
            const list = Array.from(bytes, (byte) => String.fromCharCode(byte)).join('');
            const visible = visibleText(list);
            if (visible.length >= TEMPORARY_BYTES) throw new RangeError('미션 기술 허용의 원본512BYTE 임시 버퍼 범위');
            return configListItem(visible, index);
        },
        findType: (name) => types.find(name),
    };
}

// 설치한 월드의 실제 RawTotalMade 에 같은 목록 효과를 쓴다. (2026-11-12 WB04-E338)
export function applyMissionTechnologyToTotalMade(script, types, totalMade) {
    const hooks = scriptHooks(script, types);
    hooks.allowAll = (value) => totalMade.allowAll(value);
    hooks.allow = (type, value) => totalMade.allow(type, value);
    applyMissionTechnology(hooks);
}

// Init Header 는 설치 전 전역 금지 표를 쓰고 이후 Session 은 이 같은 배열을 빌린다. (2026-11-12 WB04-E338)
export function applyMissionTechnologyToRestrictions(script, types, restrictions) {
    const hooks = scriptHooks(script, types);
    hooks.allowAll = (value) => setTotalMadeAllowAll(restrictions, value);
    hooks.allow = (type, value) => setTotalMadeAllow(restrictions, type, value);
    applyMissionTechnology(hooks);
}
