// Sun/Rain 아비어리가 공유하는 0045b0f0 의 선택 필드 decoder 를 옮긴 모듈이다(cpppj `o/RawAviaryProcessData.cpp/.h`, 10.78 전용).
// 사용: const parent = readAviaryProcessData(fields, packet) — fields 는 현재 outer 이미지(Uint8Array, 두 48BYTE 클래스의 공통 접두), packet 은 수신한 패킷이다.
//   packet[8] 의 dirty 비트로 선택 필드를 순서대로 읽는다(필드는 packet 의 9 번째 BYTE 부터 이어 붙는다): 비트 0 → 부모 DWORD(4BYTE, 반환값으로만 조립하며 outer 이미지에는 쓰지 않음),
//   비트 1 → fields[16..23] 8BYTE 복사, 비트 2 → fields[24..27] 4BYTE 복사. 계산 없는 8/4BYTE 복사와 원문 선택 순서를 유지한다.
//   현재 outer 이미지와 패킷만 빌리며 별도 예약/Kernel 객체를 만들지 않는다. 앞선 부분 쓰기는 되돌리지 않는다. 빈 fields 도 실제 저장이 없는 선택 필드가 요구될 때만 명시 오류이다(Native NULL this 와 같음).
// 패킷 접두(9BYTE)나 선택 필드가 부족하면 RangeError 이다. 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E530: 4-D 구획 RawAviaryProcessData 이식)

// dirty 비트와 필드 위치/폭(부모 DWORD 4BYTE, 8BYTE 복사 위치 16, 4BYTE 복사 위치 24)이다. (2026-11-15 WB04-E530)
const DIRTY_PARENT = 1;
const DIRTY_WIDE = 2;
const DIRTY_NARROW = 4;
const PREFIX_BYTES = 9;
const WIDE = { offset: 16, width: 8 };
const NARROW = { offset: 24, width: 4 };

// 아비어리 수신의 선택 필드를 읽는다. 반환은 부모 DWORD(비트 0 이 없으면 0)이다. (2026-11-15 WB04-E530)
export function readAviaryProcessData(fields, packet) {
    if (packet.length < PREFIX_BYTES) throw new RangeError('아비어리 수신의 실제 접두 물리 범위');
    const dirty = packet[8];
    let parent = 0;
    let offset = PREFIX_BYTES;
    // 앞선 부분 쓰기는 되돌리지 않는다.
    const require = (width) => {
        if (offset > packet.length || width > packet.length - offset) throw new RangeError('아비어리 수신의 선택 필드 부족');
    };
    // NULL this 는 실제 선택된 저장에서만 호스트 오류이다.
    const copy = ({ offset: destination, width }) => {
        require(width);
        if (destination > fields.length || width > fields.length - destination) throw new RangeError('아비어리 수신의 실제 객체 필드 미연결');
        fields.set(packet.subarray(offset, offset + width), destination);
        offset += width;
    };
    if (dirty & DIRTY_PARENT) {
        require(4);
        // 반복은 별도 부모 출력 DWORD 만 조립한다.
        for (let index = 0; index < 4; index += 1) parent = (parent | (packet[offset + index] << (8 * index))) >>> 0;
        offset += 4;
    }
    if (dirty & DIRTY_WIDE) copy(WIDE);
    if (dirty & DIRTY_NARROW) copy(NARROW);
    return parent >>> 0;
}
