// TYPE 의 요구 에너지(mana) 문자열을 원본 후처리와 같은 규칙으로 만드는 모듈이다(cpppj `o/RawTypeRequiredEnergy.cpp/.h`·`client/AiTypeRequiredEnergy.cpp/.h`, 10.78 전용, 원문 0049c3b0 mana 속성·0049b0d0 기본 보충과 소문자화).
// 규칙: 정의(AST)의 'mana' 속성(ASCII 대소문자 무시)을 순서대로 모두 읽어 마지막 정상 문자열을 Windows-1252 바이트로 보관한다(문자열이 아니면 TypeError, 첫 NUL 이후는 버리고 9바이트 초과는 RangeError).
//   그 값이 비어 있고 group 이 10 이 아니며 flags2 의 0x200000 비트가 없으면 기본값을 채운다: level>=0 이면 원소 글자(theme 번째), level==1 이면 's' 추가, level==2 이면 원소 글자 + 's'.
//   원소 글자표는 "wrtsWRTS.?>" 와 바로 뒤 NUL 까지 12칸이며 표 밖 theme 는 RangeError 이다. 마지막으로 첫 NUL 까지만 남기고 비ASCII 는 거부(원본 CRT tolower 경계 미복원)한 뒤 ASCII 소문자로 만든다.
// 정의가 없으면(null) 실제 내부/빈 TYPE 의 zero 초기 mana 입력이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E26: TYPE 요구 에너지 이식으로 추가)
import { asciiLower } from '../assets/config.js';
import { encodeWindows1252 } from '../assets/text-encoding.js';

// 원문 원소 글자표(00540ad8→0050c340)이며 바로 뒤 NUL 도 표의 일부이다. (2026-11-08 WB04-E26)
const ENERGY_LETTERS = 'wrtsWRTS.?>\0';
// mana 속성 문자열의 원본 최대 바이트 수와 제외 플래그 비트(flags2)이다. (2026-11-08 WB04-E26)
const MAX_MANA_BYTES = 9;
const EXCLUDED_FLAG = 0x200000;
// AI 몸체가 쓰는 보호 TYPE 번호 구간이다: 자산 70..185 만 실제 정의, 내부/186·187 은 빈 TYPE. (2026-11-08 WB04-E26)
const ASSET_FIRST = 70;
const TYPE_COUNT = 188;
const ASSET_COUNT = 116;

// 원소 글자표를 읽는다. 표 밖 theme 는 원본 메모리 접근이어서 명시 경계로 거부한다. (2026-11-08 WB04-E26)
function letter(theme) {
    if (!Number.isInteger(theme) || theme < 0 || theme >= ENERGY_LETTERS.length) throw new RangeError('TYPE 요구 에너지 실제 원소 문자표 범위 오류');
    return ENERGY_LETTERS[theme];
}

// 첫 NUL 까지만 남긴다. (2026-11-08 WB04-E26)
function untilNul(text) {
    const end = text.indexOf('\0');
    return end < 0 ? text : text.slice(0, end);
}

// type = { group, flags2, level, theme }, definition = { properties: [{ name, value }] } 또는 null 이다. 요구 에너지 문자열(ASCII 소문자)을 돌려준다. (2026-11-08 WB04-E26)
export function typeRequiredEnergy(type, definition) {
    let value = '';
    if (definition) {
        // 모든 지정값을 순서대로 검사하고 마지막 정상 문자열을 보존한다.
        for (const property of definition.properties) {
            if (asciiLower(property.name) !== 'mana') continue;
            if (typeof property.value !== 'string') throw new TypeError('원본 TYPE mana 는 문자열 AST 여야 합니다');
            value = untilNul(String.fromCharCode(...encodeWindows1252(property.value)));
            if (value.length > MAX_MANA_BYTES) throw new RangeError('원본 TYPE mana 는 9byte 이하이며 그 밖은 assert 후 손상 경계입니다');
        }
    }
    if (type.group !== 10 && ((type.flags2 >>> 0) & EXCLUDED_FLAG) === 0 && value === '') {
        if (type.level >= 0) value += letter(type.theme);
        if (type.level === 1) value += ENERGY_LETTERS[3];
        if (type.level === 2) { value += letter(type.theme); value += ENERGY_LETTERS[3]; }
    }
    value = untilNul(value);
    // 현재 실제 원본 mana 는 ASCII 원소 문자열이다. 임의 비ASCII CRT 로케일 동일성을 합성하지 않는다.
    for (const character of value) if (character.charCodeAt(0) >= 128) throw new TypeError('TYPE 비ASCII mana 는 원본 CRT tolower 경계 미복원');
    return asciiLower(value);
}

// definitions = 자산 70..185 의 정의 배열(116개), currentTypes = 현재 TYPE 188개의 살아 있는 배열이다. 반환 getter 는 매 조회마다 현재 TYPE 을 읽는다(복제·캐시 없음). (2026-11-08 WB04-E26)
export function bindAiTypeRequiredEnergy(definitions, currentTypes) {
    if (currentTypes.length !== TYPE_COUNT || definitions.length !== ASSET_COUNT) throw new TypeError('AI 에너지 실제 PATCH188TYPE/116자산 계약 오류');
    return (number) => {
        if (!Number.isInteger(number) || number < 0 || number >= currentTypes.length) throw new RangeError('AI 에너지 현재 TYPE 범위 오류');
        // 내부/빈 타입은 실제 zero 초기 입력이다.
        const definition = number >= ASSET_FIRST && number < ASSET_FIRST + definitions.length ? definitions[number - ASSET_FIRST] : null;
        return typeRequiredEnergy(currentTypes[number], definition);
    };
}
