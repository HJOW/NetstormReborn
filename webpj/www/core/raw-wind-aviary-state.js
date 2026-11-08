// 데빌메이커(윈드 아비어리) 전용 80바이트 본체의 생성자(0045d730/0045d7a0)와 변경 필드 패킷의 쓰기/읽기(0045b150/0045b210)를 원본 10.78 순서로 옮긴 모듈이다(cpppj `o/RawWindAviaryState.cpp/.h`, 10.78 전용).
// 원본은 전송 중 double 을 계산하지 않고 비트를 복사한다. NaN payload 와 음수 0 도 그대로 유지한다. 생성자가 쓰지 않는 필드와 padding 도 호출자 입력으로 보존한다.
//  - WindAviaryProcessFields: 원본 outer0/inner+38 의 80바이트 이미지(bytes). 후속 생산/비행 상태 기계와 전송이 공유할 유일한 원문 이미지이다.
//  - RawWindAviaryState.initialize(fields, server, now): +10/+20/+28 double 을 0 으로, dirty 0xff, 내장 표(0x509ddc/0x5019ec 순)·가상 표 0x509dfc·Form/부모 0 을 쓰고, 서버일 때만 시각(+10 double)·상태 1(+18)·dirty 비트 4 를 더한다.
//    비서버의 미정 상태와 두 예약 double 주변은 그대로 둔다(전체 0 초기화로 대신하지 않는다).
//  - RawWindAviaryState.writeProcess(fields, packet, includeParent, force, protocol = 1, message = 0x74): 부모는 includeParent 이거나 force 일 때만(비영이면) 삽입하고 dirty 1 을 켠다. force 이면 dirty 를 0xff 로 취급한다.
//    선택 필드(+10 double·+18 DWORD·+20 double·+28 double·+30 DWORD, dirty 비트 2/4/8/10/20)를 계산 없이 복사하고 길이 WORD 를 쓴 뒤 dirty 만 0 으로 바꾼다. 반환은 쓴 길이이며 버퍼가 모자라면 첫 쓰기 전에 거부한다.
//  - RawWindAviaryState.readProcess(fields | bytes, packet): 머리 9바이트·선택 필드 길이를 먼저 확인(모자라면 거부)하고 부모(dirty 1) DWORD 를 반환한다. 선택 필드만 덮고 선택되지 않은 상태·예약 시각·목표·padding 은 보존한다. 객체 저장이 없는 입력(빈 뷰)은 선택 필드가 없을 때만 반환한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E121: RawWindAviaryState 이식으로 추가)

// 실제 80바이트 본체의 독립 필드 위치(dirty, 시각, 단계, Form, 부모)와 dirty 비트 2/4/8/10/20 에 대응하는 원본 필드 순서·폭이다. (2026-11-08 WB04-E121)
const DIRTY = 8;
const TIME = 0x10;
const STAGE = 0x18;
const FORM = 0x3c;
const PARENT = 0x40;
const OFFSETS = Object.freeze([0x10, 0x18, 0x20, 0x28, 0x30]);
const SIZES = Object.freeze([8, 4, 8, 8, 4]);
const FIELD_SIZE = 0x50;
const doubleView = new DataView(new ArrayBuffer(8));

// 원본 80바이트 본체 이미지이다. 사용: new WindAviaryProcessFields() 의 bytes 를 직접 읽고 쓴다. (2026-11-08 WB04-E121)
export class WindAviaryProcessFields {
    // 후속 생산/비행 상태 기계와 전송이 공유할 유일한 원문 이미지이다.
    bytes = new Uint8Array(FIELD_SIZE);
}

// 정렬되지 않은 원문에서 DWORD 를 읽는다. (2026-11-08 WB04-E121)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 원본 WORD/DWORD/DOUBLE 저장의 낮은 바이트부터 쓴다(요청한 폭만, 인접 원문 유지). value 는 수 또는 BigInt 이다. (2026-11-08 WB04-E121)
function put(bytes, at, value, size = 4) {
    let remaining = BigInt.asUintN(64, BigInt(value));
    // 요청한 폭만 쓰고 호출자의 인접 원문을 유지한다.
    for (let index = 0; index < size; index += 1) {
        bytes[at + index] = Number(remaining & 0xffn);
        remaining >>= 8n;
    }
}

// 실제 선택 비트만 길이에 포함한다. 미사용 비트는 헤더에서 그대로 보존한다. (2026-11-08 WB04-E121)
function payloadSize(dirty) {
    let size = 0;
    // 다섯 필드의 원본 비트와 폭을 독립 순서로 더한다.
    for (let index = 0; index < SIZES.length; index += 1) if (dirty & (2 << index)) size += SIZES[index];
    return size;
}

// double 의 원본 비트(BigInt)이다. (2026-11-08 WB04-E121)
function doubleBits(value) {
    doubleView.setFloat64(0, value, true);
    return doubleView.getBigUint64(0, true);
}

// 데빌메이커 80바이트 본체의 저장/전송 규칙이다(실행 프레임은 별도 후속 몸체). 사용: RawWindAviaryState.initialize(fields, server, now). (2026-11-08 WB04-E121)
export class RawWindAviaryState {
    // 생성자는 0x1c/0x30/0x34/inner 꼬리를 초기화하지 않는다. 서버일 때만 상태 1/현재 시각을 쓴다. (2026-11-08 WB04-E121)
    static initialize(fields, server, now) {
        const raw = fields.bytes;
        put(raw, TIME, 0, 8);
        put(raw, 0x20, 0, 8);
        put(raw, 0x28, 0, 8);
        raw[DIRTY] = 0xff;
        put(raw, 0x38, 0x5019ec);
        put(raw, PARENT, 0);
        put(raw, FORM, 0);
        put(raw, 0, 0x509dfc);
        put(raw, 0x38, 0x509ddc);
        if (server) {
            put(raw, TIME, doubleBits(now), 8);
            raw[DIRTY] |= 4;
            put(raw, STAGE, 1);
        }
    }

    // 강제 전송의 부모 선택은 inner thunk 가 먼저 수행하고 본문은 실제 비제로 부모만 삽입한다. 반환은 실제 길이 WORD 이다. (2026-11-08 WB04-E121)
    static writeProcess(fields, packet, includeParent, force = false, protocol = 1, message = 0x74) {
        const raw = fields.bytes;
        const parent = includeParent || force ? read(raw, PARENT) : 0;
        const dirty = force ? 0xff : raw[DIRTY];
        const size = 9 + (parent ? 4 : 0) + payloadSize(dirty);
        if (packet.length < size) throw new RangeError('데빌메이커 상태 전송 버퍼 부족');
        if (force) raw[DIRTY] = 0xff;
        packet[0] = protocol;
        packet[1] = message;
        put(packet, 4, read(raw, FORM));
        let at = 9;
        if (parent) {
            raw[DIRTY] |= 1;
            put(packet, at, parent);
            at += 4;
        }
        // native writer 가 읽는 각 선택 필드를 계산 없이 현재 원문에서 복사한다.
        for (let index = 0; index < SIZES.length; index += 1) {
            if (!(raw[DIRTY] & (2 << index))) continue;
            packet.set(raw.subarray(OFFSETS[index], OFFSETS[index] + SIZES[index]), at);
            at += SIZES[index];
        }
        put(packet, 2, at, 2);
        packet[8] = raw[DIRTY];
        raw[DIRTY] = 0;
        return at;
    }

    // 잘린 host 버퍼는 첫 쓰기 전에 거부한다. 정상 원문의 부분 갱신과 부모 출력 0 은 그대로 따른다. fields 는 WindAviaryProcessFields 또는 바이트 뷰(PID NULL 이면 길이 0 뷰)이다. (2026-11-08 WB04-E121)
    static readProcess(fields, packet) {
        const target = fields instanceof WindAviaryProcessFields ? fields.bytes : fields;
        if (packet.length < 9) throw new RangeError('데빌메이커 상태 수신 머리 부족');
        const dirty = packet[8];
        const size = 9 + ((dirty & 1) ? 4 : 0) + payloadSize(dirty);
        if (packet.length < size) throw new RangeError('데빌메이커 상태 수신 필드 부족');
        let at = 9;
        let parent = 0;
        if (dirty & 1) {
            parent = read(packet, at);
            at += 4;
        }
        // 선택되지 않은 상태·예약 시각·목표·padding 은 호출자 원문 그대로 보존한다.
        for (let index = 0; index < SIZES.length; index += 1) {
            if (!(dirty & (2 << index))) continue;
            if (OFFSETS[index] > target.length || SIZES[index] > target.length - OFFSETS[index]) throw new RangeError('데빌메이커 수신의 실제 객체 필드 미연결');
            target.set(packet.subarray(at, at + SIZES[index]), OFFSETS[index]);
            at += SIZES[index];
        }
        return parent;
    }
}
