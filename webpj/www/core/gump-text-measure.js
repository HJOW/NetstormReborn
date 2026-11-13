// 원문4a3df0의 폭 표 합산·signed 길이·출력 쓰기/alias 순서를 이식한다(cpppj client/GumpText.cpp). (2026-11-14 WB04-E414)
// 사용: measureGumpText(bytes,widths256,fontHeight,context,out,length). out.width/height는 같은 저장소의 접근자로 alias할 수 있다.
// 웹 UTF16/D2Coding 입력은 폭 조회 함수를 공급하며 원문의4BYTE 확장 부호화는 기존 웹 정책대로 쓰지 않는다.
export function measureGumpText(bytes, widths, fontHeight, context, output, length = -1) {
    length |= 0; let count = 0;
    if (length === -1) { const end = bytes.indexOf(0); if (end === -1) throw new RangeError('Gump 글자 범위에 NUL 종료 필요'); count = end; }
    else if (length > 0) { count = length; if (count > bytes.length) throw new RangeError('Gump 글자 측정 길이 범위 초과'); }
    let total = 0;
    // 명시 길이는 NUL도 한 글자로 측정한다. 바이트 폭/Unicode 폭은 모두 DWORD 덧셈이다.
    for (let i = 0; i < count; i += 1) total = (total + (typeof widths === 'function' ? widths(bytes[i]) : widths[bytes[i] & 255])) | 0;
    output.width = total; output.height = fontHeight | 0;
    if (context.flags & 2) { output.width = (output.width + context.shadowX) | 0; output.height = (output.height + context.shadowY) | 0; }
    return output;
}
