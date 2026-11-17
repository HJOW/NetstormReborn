// 같은 stream16BYTE/공유 난수로 원본267BYTE SP writer를 실행한다(225553df, WB04-E634).
// 사용: writeFortSp(stream, value, { nextRandom, stream: { buffer, assertion } }). 일반 writer/새 난수는 만들지 않는다.
// masked _ftol의 signed64 변환 한계다. 입력은 먼저 caller의 float32로 저장한다.
const SIGNED64_LIMIT = 2 ** 63;
// 실제 선택한 하위만 그 위치에서 요구한다.
function need(device) { if (typeof device !== 'function') throw new Error('Fort SP writer의 실제 선택장치 누락'); return device; }
// 같은 실제 물리의 지정 폭만 빌리고 범위 밖 접근은 무시하지 않는다.
function range(bytes, offset, width) { if (!(bytes instanceof Uint8Array) || offset > bytes.length || width > bytes.length - offset) throw new RangeError('Fort SP writer의 실제 물리 범위'); return bytes.subarray(offset, offset + width); }
// 현재 비정렬 DWORD만 little endian으로 읽는다.
function read(bytes, offset) { const field = range(bytes, offset, 4); return new DataView(field.buffer, field.byteOffset, 4).getUint32(0, true); }
// 고정된 값의 지정 BYTE/WORD/DWORD만 쓰며 인접 필드를 보존한다.
function put(bytes, offset, value, width = 4) { const field = range(bytes, offset, width); /* 원문 낮은 BYTE부터 기록하고 같은 할당 별칭도 유지한다. */ for (let byte = 0; byte < width; byte++) field[byte] = (value >>> (byte * 8)) & 255; }
// 004e49c0의 signed64 0방향 절삭 후 EAX 하위 DWORD다. 비정상 masked 결과의 하위 DWORD는0이다.
function integer(input) { const value = Math.fround(input); if (!Number.isFinite(value) || value >= SIGNED64_LIMIT || value < -SIGNED64_LIMIT) return 0; return Number(BigInt.asUintN(32, BigInt(Math.trunc(value)))); }
// 각 bit의16잡음을 모두 소비하고8BYTE·8WORD·16DWORD, 총88BYTE를 현재 stream에 쓴다.
export function writeFortSp(stream, value, hooks) {
    const original = integer(value);
    // 원문 EBP0..31의 순서이며 bit별 일반 writer 호출을 추가하지 않는다.
    for (let bit = 0; bit < 32; bit++) {
        let word = original & (1 << bit);
        // 원래 bit와 같은 난수도 소비하지만 잡음 OR에는 넣지 않는다.
        for (let noise = 0; noise < 16; noise++) { const selected = need(hooks.nextRandom)(32) >>> 0; if (selected !== bit) word |= 1 << (selected & 31); }
        const width = bit < 8 ? 1 : bit < 16 ? 2 : 4; let capacity, sum;
        if (bit < 8) { sum = (read(stream, 4) + width) >>> 0; capacity = read(stream, 12); }
        else { capacity = read(stream, 12); sum = (read(stream, 4) + width) >>> 0; }
        if ((sum | 0) > (capacity | 0)) need(hooks.stream?.assertion)('WriteCapacity');
        const position = read(stream, 4), pointer = read(stream, 0); if (!pointer) throw new Error('Fort SP writer의 현재 버퍼 포인터 누락');
        const bytes = need(hooks.stream?.buffer)(pointer); put(bytes, position, word, width);
        const next = (read(stream, 4) + width) >>> 0, end = read(stream, 8); put(stream, 4, next); if ((next | 0) > (end | 0)) put(stream, 8, next);
    }
}
