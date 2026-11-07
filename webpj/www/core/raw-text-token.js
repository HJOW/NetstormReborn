// 원본의 80 BYTE 임시 토큰 읽기와 다음 입력 위치(PATCH 00487620)를 옮긴 모듈이다(cpppj `o/RawTextToken.cpp/.h`, 10.78 경로).
// 규칙: NUL 은 입력/구분자 C 문자열의 끝이다 → 첫 구분자를 찾기 전에 앞쪽 space/tab 만 건너뛴다(개행과 나머지 공백은 보존) → 입력 78 바이트 한도 안에서 큰따옴표로 인용 상태를 토글하며(따옴표도 한도 하나를
//   소비하지만 출력에는 쓰지 않음) 인용 밖에서 구분자를 만나면 멈춘다 → 멈출 때(한도 때문이어도) 현재 연속 구분자를 인용 상태와 무관하게 건너뛴다 → 뒤쪽 space/tab 을 지우되 첫 출력 바이트는 남긴다.
// 반환은 { text(따옴표를 뺀 실제 바이트의 이진 문자열), next(원본 입력 포인터의 이동 결과 위치) } 이다. HTML/Config 의 실제 제한과 포인터 소비 순서를 공유한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E19: 원본 토큰 읽기 이식으로 추가)

// 원본 80 BYTE 지역 버퍼의 입력 바이트 상한이다. (2026-11-08 WB04-E19)
const INPUT_LIMIT = 78;

// source 는 원본 바이트(이진 문자열), delimiters 의 기본값은 공백·탭·개행·쉼표이다. (2026-11-08 WB04-E19)
export function readRawTextToken(source, offset = 0, delimiters = ' \t\n,') {
    if (offset > source.length) throw new RangeError('원본 토큰의 입력 위치 범위 오류');
    const nulSource = source.indexOf('\0');
    const text = nulSource < 0 ? source : source.slice(0, nulSource);
    const nulDelimiters = delimiters.indexOf('\0');
    const stops = nulDelimiters < 0 ? delimiters : delimiters.slice(0, nulDelimiters);
    if (offset > text.length) throw new RangeError('원본 토큰의 NUL 뒤 입력 위치');
    let position = offset;
    // 첫 구분자를 찾기 전에 space/tab 만 건너뛴다. 개행과 나머지 공백은 보존한다.
    while (position < text.length && (text[position] === ' ' || text[position] === '\t')) position += 1;
    let result = '';
    let quoted = false;
    let remaining = INPUT_LIMIT;
    // 인용부호도 원본 입력 제한을 하나 소비하지만 출력에는 쓰지 않는다.
    while (position < text.length) {
        const byte = text[position];
        if (!remaining || (!quoted && stops.includes(byte))) {
            // 제한 때문에 멈춘 경우에도 현재 연속 구분자는 인용 상태와 무관하게 건너뛴다.
            while (position < text.length && stops.includes(text[position])) position += 1;
            break;
        }
        if (byte === '"') quoted = !quoted;
        else result += byte;
        position += 1;
        remaining -= 1;
    }
    // 원본은 뒤 공백을 지워도 첫 출력 바이트는 남긴다. 인용한 공백 한 글자도 빈 문자열이 아니다.
    while (result.length > 1 && (result.at(-1) === ' ' || result.at(-1) === '\t')) result = result.slice(0, -1);
    return { text: result, next: position };
}
