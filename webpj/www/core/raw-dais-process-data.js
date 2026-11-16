// 10.78의 DaisProcess 40BYTE·빈 실행과 6ah 선택 필드 decoder/writer다(225553df, WB04-E617).
// 사용: DaisProcess를 같은 SquidProcessHost에 부착하고 fields.bytes를 수신/writer에 빌린다.
// 기한 비트만 보관하며 RawDaisRegular/희생 예약을 시작하지 않는다. Native 수신/Session 송신 합성은 후속이다.
import { SquidProcess } from './squid-process.js';

// 원문 outer 이미지의 Form·부모·내장 가상 표·dirty·기한 오프셋과 전체 폭이다.
const FORM = 4, PARENT = 8, INNER = 0x10, DIRTY = 0x18, DEADLINE = 0x20, IMAGE_BYTES = 40;

// 선택한 필드만 물리 범위를 검사한다. 앞선 다른 필드의 쓰기를 되돌리지 않는다.
function selected(bytes, offset, width) {
    if (!(bytes instanceof Uint8Array) || offset > bytes.length || width > bytes.length - offset) throw new RangeError('DaisProcess의 선택된 field 물리 부족');
    return new DataView(bytes.buffer, bytes.byteOffset + offset, width);
}

// 현재 raw 필드를 리틀엔디언 비트로 읽는다. 기한은 double 변환 없이64비트를 보존한다.
function read(bytes, offset, width) {
    const view = selected(bytes, offset, width); // 선택된 현재 물리 영역이다.
    if (width === 8) return view.getBigUint64(0, true);
    if (width === 4) return view.getUint32(0, true);
    return view.getUint8(0);
}

// 선택된 폭을 한 번 검사한 뒤 같은 원문 비트만 쓴다. 정렬이나 JS 수치 NaN 정규화에 의존하지 않는다.
function put(bytes, offset, value, width) {
    const view = selected(bytes, offset, width); // 이번 저장에만 필요한 물리 영역이다.
    if (width === 8) view.setBigUint64(0, BigInt(value), true);
    else if (width === 4) view.setUint32(0, value, true);
    else if (width === 2) view.setUint16(0, value, true);
    else view.setUint8(0, value);
}

// 기록용40BYTE 이미지다. 기본0 padding은 호스트 값이며 원문 초기값으로 주장하지 않는다.
export class DaisProcessFields {
    // 실제 원문 이미지이며 이 객체 수명 동안 같은 배열을 대여한다.
    bytes;
    // C++ array 값 복사처럼 입력 padding을 복사한다. 생성자의 명시 쓰기는 별도 initialize가 수행한다.
    constructor(bytes = new Uint8Array(IMAGE_BYTES)) {
        if (!(bytes instanceof Uint8Array) || bytes.length !== IMAGE_BYTES) throw new RangeError('DaisProcess40BYTE 이미지 범위');
        this.bytes = Uint8Array.from(bytes);
    }
}

// 0044a2d4..fb의 명시 필드만 쓰고 나머지 padding을 보존한다.
export function initializeDaisProcessFields(fields) {
    put(fields.bytes, PARENT, 0, 4); put(fields.bytes, FORM, 0, 4);
    put(fields.bytes, INNER, 0x50782c, 4); put(fields.bytes, DEADLINE, 0n, 8);
    fields.bytes[DIRTY] = 0xff; put(fields.bytes, 0, 0x507834, 4); put(fields.bytes, INNER, 0x507830, 4);
}

// 468230→4ab2c0→41bd00의 base 소멸 표식만 쓴다. 실제 Kernel/Form 반납은 Host가 소유한다.
export function destructDaisProcessFields(fields) {
    put(fields.bytes, 0, 0x5019ec, 4); put(fields.bytes, FORM, 0xffffcfc7, 4); put(fields.bytes, PARENT, 0xffffcfc7, 4);
}

// 신규일 때만 선택된 parent DWORD를 읽어 반환한다. 기존 객체는 parent 물리를 건너뛰고 bit2 저장 때만 fields를 요구한다.
export function readDaisProcessData(fields, packet, newProcess) {
    const mask = read(packet, 8, 1); // 선택 필드 mask이며 다른 bit는 원문처럼 무시한다.
    let cursor = 9, parent = 0; // parent가 없을 때 원문 ESI의0이다.
    if (mask & 1) { if (newProcess) parent = read(packet, cursor, 4); cursor += 4; }
    if (mask & 2) {
        const deadline = read(packet, cursor, 8); // 원문처럼 입력을 먼저 읽고 현재 객체 저장을 요구한다.
        if (!fields) throw new Error('DaisProcess bit2 저장의 현재 NULL/타클래스 raw뷰 미지원');
        put(fields.bytes, DEADLINE, deadline, 8);
    }
    return parent;
}

// 004488c0의 부분 효과 순서다. force·부모 payload·dirty 상위bit를 정상화하지 않으며 마지막 성공 뒤에만 dirty를 지운다.
export function writeDaisProcessData(fields, packet, form, parentPayload, force, protocol = 1, message = 0x6a) {
    if (force) fields.bytes[DIRTY] = 0xff;
    put(packet, 0, protocol, 1); put(packet, 1, message, 1); put(packet, 4, form, 4);
    let size = 9, cursor = 9; // 선택 필드가 없어도 반환 길이는9다.
    if (parentPayload >>> 0) { fields.bytes[DIRTY] |= 1; put(packet, cursor, parentPayload, 4); cursor += 4; size += 4; }
    if (fields.bytes[DIRTY] & 2) { const deadline = read(fields.bytes, DEADLINE, 8); put(packet, cursor, deadline, 8); size += 8; }
    put(packet, 2, size, 2); put(packet, 8, fields.bytes[DIRTY], 1); fields.bytes[DIRTY] = 0;
    return size;
}

// 실제 Host/Kernel이 소유하는 DaisProcess다. 빈 runFrame/notify는 원문4437f0/480260의 복원이다.
export class DaisProcess extends SquidProcess {
    // C++ vptr/PID와 별개인 기록 이미지이며 decoder/writer가 같은 객체를 참조한다.
    fields;
    // 입력 이미지는 C++ 생성자 값 인자처럼 복사하며 원문의 명시 필드만 덮어쓴다.
    constructor(fields = new DaisProcessFields()) { super(); this.fields = new DaisProcessFields(fields.bytes); initializeDaisProcessFields(this.fields); }
    // 원문 base 표식만 쓴다. 새 Form 삭제나 Kernel 제거를 추가하지 않는다.
    destroy() { destructDaisProcessFields(this.fields); }
    // outerVT+18=4437f0은 빈 실행이다. 현재 기한·dirty로 사건/예약을 만들지 않는다.
    runFrame() {}
    // 실제 Host가 먼저 갱신한 현재 Form/부모를 기록 이미지에 반영한다.
    onAttached() { put(this.fields.bytes, FORM, this.form, 4); put(this.fields.bytes, PARENT, this.parent, 4); }
    // outer+1c/inner480260의 빈RET12 통지다.
    notify(event, info, value) {}
    // 4492f0의 parentChanged/force와 현재 Host 필드를 같은 writer에 전달한다.
    writePacket(packet, parentChanged, force = false, protocol = 1, message = 0x6a) {
        this.onAttached();
        const parent = parentChanged || force ? read(this.fields.bytes, PARENT, 4) : 0; // 비영 부모만 payload를 생성한다.
        return writeDaisProcessData(this.fields, packet, read(this.fields.bytes, FORM, 4), parent, force, protocol, message);
    }
}
