// 같은 SNART 물리 이름표를 제자리 변환하고 현재 이름의 시작 위치를 빌리는 도구(원문 004a8800/004a8850)를 옮긴 모듈이다(cpppj `o/RawSnart.cpp/.h`, 10.78).
// selectSnartName(names, index, assertion): 표의 첫 BYTE 가 비0 이면 첫 BYTE 가 '/' 인지 확인한다(아니면 원문 진단 Snart.cpp 0x46 후 계속, 장치가 없으면 거부) → 첫 NUL 을 0xff 로 바꾸고 첫 0xff 전의 '/' 만 NUL 로 바꾼다
//   (나머지 원시 BYTE 는 보존; 한 번 변환한 첫 BYTE 0 과 마지막 0xff 는 다음 호출에도 남음). index 0 은 첫 separator 바로 다음 위치이고, index>0 이면 현재 NUL 개수만 지나며 다음 0xff 를 만나면 멈춘다.
//   반환은 현재 이름 시작부터 같은 물리 끝까지의 뷰이다(길이를 미리 읽지 않음). readSnartName 은 기존 파일 reader 처럼 현재 첫 NUL 까지 읽는 이진 문자열을 돌려준다.
// 범위 밖/누락은 RangeError 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E19: SNART 이름표 선택 이식으로 추가)

// 같은 표를 제자리에서 변환하고 index 번째 이름의 시작 뷰를 돌려준다. assertion() 은 잘못된 첫 separator 의 선택 진단이다. (2026-11-08 WB04-E19)
export function selectSnartName(names, index, assertion = null) {
    if (names.length === 0) throw new RangeError('SNART 실제 이름표 누락');
    if (names[0]) {
        if (names[0] !== 0x2f) {
            if (typeof assertion !== 'function') throw new Error('SNART separator 실제 진단 미연결');
            assertion();
        }
        const end = names.indexOf(0);
        if (end < 0) throw new RangeError('SNART 입력 NUL 누락');
        names[end] = 0xff;
        // 원문 첫 0xff 전의 slash 만 NUL 로 바꾸며 나머지 원시 BYTE 는 보존한다.
        for (let at = 0; at < names.length && names[at] !== 0xff; at += 1) if (names[at] === 0x2f) names[at] = 0;
    }
    // index 0 은 같은 표의 첫 separator 바로 다음 포인터다.
    let at = 1;
    let remaining = index >>> 0;
    // 기존 파일 reader 와 같은 현재 NUL 개수만 지나고 다음 0xff 를 만나면 멈춘다.
    while (remaining) {
        if (at >= names.length) throw new RangeError('SNART 이름 번호 범위');
        if (!names[at]) {
            remaining -= 1;
            at += 1;
            if (at >= names.length) throw new RangeError('SNART 끝 누락');
            if (names[at] === 0xff) break;
        } else at += 1;
    }
    if (at >= names.length) throw new RangeError('SNART 이름 시작 범위');
    return names.subarray(at);
}

// 선택한 이름을 현재 첫 NUL 까지 읽은 이진 문자열로 돌려준다. (2026-11-08 WB04-E19)
export function readSnartName(names, index, assertion = null) {
    const tail = selectSnartName(names, index, assertion);
    const end = tail.indexOf(0);
    if (end < 0) throw new RangeError('SNART 이름 NUL 누락');
    return String.fromCharCode(...tail.subarray(0, end));
}
