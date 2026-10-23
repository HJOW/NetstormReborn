// 원본 게임 글꼴(.chfnt)이 쓰는 Windows-1252 바이트와 JavaScript 문자열 사이의 변환 모듈이다.
// 원본 글꼴은 바이트 코드 0~255 로 글리프를 찾으므로, 화면에 그릴 문자열은 먼저 이 모듈로 바이트 배열로 바꾼다.
// (2026-10-23 WB03-M1: 메인 메뉴 버튼 라벨을 원본 글리프로 그리기 위해 추가)

// 유니코드 코드 포인트 → Windows-1252 의 0x80~0x9F 구간 바이트 대응표이다.
// 0x00~0x7F 와 0xA0~0xFF 는 코드 포인트와 바이트 값이 같으므로 이 표에 없다.
// 사용: encodeWindows1252 가 이 구간의 특수 문자(유로 기호, 따옴표 등)를 바꿀 때 조회한다. (2026-10-23 WB03-M1)
const CP1252_EXTRA = new Map([
    [0x20ac, 0x80], [0x201a, 0x82], [0x0192, 0x83], [0x201e, 0x84], [0x2026, 0x85], [0x2020, 0x86],
    [0x2021, 0x87], [0x02c6, 0x88], [0x2030, 0x89], [0x0160, 0x8a], [0x2039, 0x8b], [0x0152, 0x8c],
    [0x017d, 0x8e], [0x2018, 0x91], [0x2019, 0x92], [0x201c, 0x93], [0x201d, 0x94], [0x2022, 0x95],
    [0x2013, 0x96], [0x2014, 0x97], [0x02dc, 0x98], [0x2122, 0x99], [0x0161, 0x9a], [0x203a, 0x9b],
    [0x0153, 0x9c], [0x017e, 0x9e], [0x0178, 0x9f],
]);

// Windows-1252 로 나타낼 수 없는 문자를 대신할 바이트('?')이다. 원본 글꼴에 없는 문자를 빈칸 없이 표시하기 위해 쓴다. (2026-10-23 WB03-M1)
const REPLACEMENT_BYTE = 0x3f;

// 문자열을 Windows-1252 바이트 배열(Uint8Array)로 바꾼다. 나타낼 수 없는 문자는 '?' 로 바꾼다.
// 사용: 원본 글꼴로 그리거나 폭을 잴 라벨을 넘기기 전에 호출한다. 예) encodeWindows1252('Campaign')
// (2026-10-23 WB03-M1)
export function encodeWindows1252(text) {
    const source = String(text ?? '');
    const output = [];
    // 서로게이트 쌍을 하나의 코드 포인트로 읽도록 for...of 로 문자 단위 순회한다.
    for (const character of source) {
        const code = character.codePointAt(0);
        if (code < 0x80 || (code >= 0xa0 && code <= 0xff)) output.push(code);
        else output.push(CP1252_EXTRA.get(code) ?? REPLACEMENT_BYTE);
    }
    return Uint8Array.from(output);
}
