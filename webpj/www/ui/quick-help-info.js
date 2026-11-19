// 전용 도움말의 실제 요구 에너지 그림 문자열을 만든다.

// 원본 00540ad8 ASCII 문자 순서이며 점은 구분자로 소비한다.
const MANA_LETTERS = 'wrtsWRTS.?>';

// 004735c0: 비점 문자별 원본 그림 프레임과 앞쪽 강조를 순서대로 출력한다.
export function manaPictureText(mana, highlighted = -1, type = 103, suppliedFrames = null) {
    if (typeof mana !== 'string' || !Number.isInteger(highlighted) || !Number.isInteger(type) || (suppliedFrames !== null && (!(suppliedFrames instanceof Int32Array) || suppliedFrames.length !== 128))) throw new TypeError('도움말 마나 그림의 실제 입력 오류');
    const end = mana.indexOf('\0'); if (end >= 0) mana = mana.slice(0, end);
    if (mana.length >= 256) throw new RangeError('도움말 마나 원본 256바이트 초과');
    const frames = suppliedFrames ?? new Int32Array(128);
    // 기본 프레임은 실제 PE 문자표에서 한 번 배정하며 호출자가 제공한 현재 표는 보존한다.
    if (!suppliedFrames) for (let index = 0; index < MANA_LETTERS.length; index += 1) frames[MANA_LETTERS.charCodeAt(index)] = index;
    let output = '', count = 0;
    // 점은 아이콘으로 세지 않고 각 실제 비점 문자만 소비한다.
    for (let index = 0; index < mana.length; index += 1) {
        const code = mana.charCodeAt(index); if (code === 46) continue;
        if (code >= 128) throw new RangeError('도움말 마나 비ASCII 전역 조회 미복원');
        const frame = (frames[code] + (count < highlighted ? 11 : 0)) | 0; output += `~[I${type >>> 0}.${frame}]`; count += 1;
    }
    if (output.length >= 250) throw new RangeError('도움말 마나 원본 그림 250바이트 초과');
    return output;
}
