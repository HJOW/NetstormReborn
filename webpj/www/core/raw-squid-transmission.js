// 공통 Squid 전송의 같은 2048BYTE 버퍼·현재 헤더·압축 상태와 실제 가상 writer 분배를 옮긴 모듈이다(cpppj `o/RawSquidTransmission.cpp/.h`, 10.78: 004ac260 송신 선택, 004ad170 35BYTE 갱신 쓰기, 00432fe0 압축 갱신 쓰기, 00433100 같은 owner 모드).
// 사용: const state = createSquidTransmissionState(); const wire = new RawSquidTransmission(pool, state, { type, defaultType, maxHp, broadcast, trace, assertion, connected, send, processPacket, broadcastMutable, sendMutable }); wire.pop(sid, flags); wire.sendTo(sid, target, writerArgument);
//   pool = 현재 슬롯을 조회하는 SID 풀(10.78 전용), state = 월드 대여/반납에서 초기화하지 않는 공통 버퍼/헤더/압축 전역, hooks = 직렬화에 필요한 현재 조회와 송신 장치(선택한 장치는 도달한 위치에서만 요구하고 없으면 거부).
// 흐름(Transmit): 클라이언트 SID(5..14999) 진단(임시 허용 관문이 꺼져 있을 때) → 선택자 -2 이면 현재 +58(공통 삭제), 아니면 현재 +54(공통 35BYTE 갱신/압축/빈 writer/두 Form/ProcessForm) → 반환 크기가 2048 이상이면 진단 → 양수면 대상 -1 은 방송, 나머지는 연결 관문 뒤 대상 송신.
// 기준선 뒤 cpppj 변경(1cd2b05a5·b8cadde14·be567d46b, 3.6절 ②): 대상 지정 송신(SendTo)·ProcessForm writer·공통 삭제 선택자·mutable 방송/송신·같은 owner 모드를 반영했다. 압축 수신 복호기(RawSquidCompressedReceive)·월드 동기화(RawPeerWorldSync)는 대응표 밖 신규 파일이라 재기준선 때 다룬다.
// CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E313: 4-C RawSquidTransmission 이식)
import { RawFormPacket, createFormDeletionStamp, createFormPacketHeaders } from './raw-form-packet.js';
import { supportsConstructedVtable } from './squid-constructors.js';
import { SQUID_FORM_VTABLES } from './squid-vtables.js';

// 공통 송신 버퍼 크기(005c7c50, 2048BYTE)와 35BYTE 갱신 패킷 크기이다. (2026-11-11 WB04-E313)
export const TRANSMIT_BUFFER_SIZE = 2048;
const UPDATE_SIZE = 35;

// 시험 가능한 원문 주소 상수: ProcessForm 가상 표, 83개 공통 VT 중 +54 가 실제 0 을 반환하는 표(00455ae0)이다. (2026-11-11 WB04-E313)
const PROCESS_FORM_VTABLE = 0x512098;
const EMPTY_WRITER_VTABLE = 0x5092d0;

// 같은 owner 캐시의 "없음" 표시(signed -99)와 클라이언트 SID 상한(서버 영역 첫 번호)이다. (2026-11-11 WB04-E313)
export const CRAZY_PLAYER_ID = 0xffffff9d;
const SERVER_FIRST = 15000;

// 월드 대여/반납에서 초기화하지 않는 공통 전역을 만든다: 초기 PE 값과 BSS 0 을 보존한다. 압축 모드는 signed DWORD 이며 초기 1 이다(00531850, Config 가 현재값을 쓴다). (2026-11-11 WB04-E313)
export function createSquidTransmissionState() {
    return {
        // 005c7c50 의 공통 송신 버퍼이며 반환 크기 뒤를 지우지 않는다.
        buffer: new Uint8Array(TRANSMIT_BUFFER_SIZE),
        // 기존 감시/내용물/삭제 writer 의 현재 헤더와 삭제 writer 의 한 번 소비 표식이다.
        forms: createFormPacketHeaders(),
        deletion: createFormDeletionStamp(),
        // 0052edc0 의 갱신 헤더: 고정 WORD 선언 길이 27 은 35BYTE 반환과 별개이다.
        update: { message: 0x2c, protocol: 1, length: 27 },
        // 00544c90 의 압축 헤더: 가변 패킷이라 선언 길이는 DWORD 입력이며 WORD 로 미리 자르지 않는다.
        compressed: { message: 0x7a, protocol: 1, length: 0 },
        compress: 1,
        // 005c7a78 의 초기 BSS 0 이며 재동기화 부모의 임시 허용 관문이다.
        clientSidAllowed: 0,
        // 0054d8e8/e4/ec/f0 의 서로 다른 현재 DWORD 이다.
        sameOwnerEnabled: 0,
        lastOwner: 0,
        compressedCount: 0,
        compressedBytes: 0,
        // 005c7a7c 의 현재 동기화 대상이며 시계 동기화 token 과 별개이다.
        syncTarget: 0,
        // 0052ee9c/9d/a0 의 공유 Regular 수신 이미지 헤더이다.
        sharedRegular: { message: 0x7e, protocol: 1, length: 28 },
    };
}

// 선택한 실제 장치가 함수가 아니면 도달한 위치에서 오류를 낸다. (2026-11-11 WB04-E313)
function need(device) {
    if (typeof device !== 'function') throw new Error('Squid 전송의 선택된 실제 장치 미연결');
    return device;
}

// 비정렬 원본 WORD/DWORD 를 선택한 낮은 바이트부터 조립해 읽는다. (2026-11-11 WB04-E313)
function read(raw, offset, width) {
    let value = 0;
    // 필드의 각 바이트만 읽는다.
    for (let index = 0; index < width; index += 1) value |= raw[offset + index] << (index * 8);
    return value >>> 0;
}

// 원문 WORD/DWORD 필드만 낮은 바이트부터 덮어쓴다(밖은 보존). (2026-11-11 WB04-E313)
function put(raw, offset, value, width) {
    // 필드의 각 바이트만 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 004df590 의 erase 0 경로의 세 필드만 쓴다: [protocol & 0x7f, message, 선언 길이(DWORD 입력, 4 미만이면 4)의 낮은 WORD]. (2026-11-11 WB04-E313)
function writeHeader(packet, header) {
    packet[0] = (header.protocol & 0xff) & 0x7f;
    packet[1] = header.message & 0xff;
    put(packet, 2, Math.max(4, header.length >>> 0), 2);
}

// 4328c0 의 정수 변환/8비트/float 동일 비교이다: 유한하고 0..255 의 정수 값이어야 한다. 범위 밖 ftol 미정 비트는 통과하지 않는다. (2026-11-11 WB04-E313)
function byteCoordinate(value) {
    return Number.isFinite(value) && value >= 0 && value <= 255 && value === Math.trunc(value);
}

// 바이트 4개를 단정도 float 로 읽는다. (2026-11-11 WB04-E313)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(raw, offset, 4), true);
    return view.getFloat32(0, true);
}

// 공통 Squid 전송 몸체이다. 같은 물리(풀)와 장기 장치만 빌린다. (2026-11-11 WB04-E313)
export class RawSquidTransmission {
    // 사용: 풀은 10.78 전용이다(CD writer 를 패치 레이아웃으로 해석하지 않음). hooks 의 장치는 쓰는 위치에서만 요구한다. (2026-11-11 WB04-E313)
    constructor(pool, state, hooks = {}) {
        if (!pool || pool.edition !== 'patch1078') throw new TypeError('현재 Squid 전송은 10.78 전용');
        if (!state || !(state.buffer instanceof Uint8Array) || state.buffer.length !== TRANSMIT_BUFFER_SIZE) throw new TypeError('Squid 전송 전역은 2048BYTE 공통 버퍼가 필요합니다.');
        this.pool = pool;
        this.state = state;
        this.hooks = hooks;
    }

    // 004ac260 의 (-1, -1, 0, flags) 호출: 일반 +54 에는 원문 인자 0 을 넘기고 현재 1..8 의 실제 연결에 방송한다. (2026-11-11 WB04-E313)
    pop(sid, flags) {
        this.transmit(sid, -1, -1, 0, flags);
    }

    // 004ac260 의 선택자 -3 / 가상 +54 경로: 현재 writer 인자와 대상 연결을 사용하며 방송으로 대체하지 않는다. 동기화의 대상 번호/인자 1 을 보존한다. (2026-11-11 WB04-E313)
    sendTo(sid, target, writerArgument) {
        this.transmit(sid, target, -3, writerArgument, 1);
    }

    // 00433100: 모드를 쓰고 마지막 owner 를 signed -99 로 재설정한다(모드가 같아도 캐시를 항상 초기화). (2026-11-11 WB04-E313)
    sameOwnerMode(mode) {
        this.state.sameOwnerEnabled = mode >>> 0;
        this.state.lastOwner = CRAZY_PLAYER_ID;
    }

    // 004ac260 의 네 인자를 보존한다: 공통 진단 뒤 -2 만 현재 +58(buffer, flags), 나머지는 +54(buffer, writerArgument)를 실행하고 양수 반환이면 대상 -1 은 방송, 그 밖은 연결 관문 뒤 송신한다. (2026-11-11 WB04-E313)
    transmit(sid, target, selector, writerArgument, flags) {
        const state = this.state;
        if (!state.clientSidAllowed && sid >= 5 && sid < SERVER_FIRST) need(this.hooks.assertion)('!isClientSid()', '\\Ns\\O\\Squid.cpp', 0x2d8);
        // 공통 +58 은 writerArgument 를 쓰지 않으며 target 은 writer 반환 뒤에도 첫 인자이다.
        const size = (selector | 0) === -2 ? this.writeDeletion(sid, flags) : this.write(sid, writerArgument);
        if (size >= TRANSMIT_BUFFER_SIZE) need(this.hooks.assertion)('transmitSize < TRANSMIT_BUFFER_SIZE', '\\Ns\\O\\Squid.cpp', 0x2e3);
        if (size > 0) {
            if (size > state.buffer.length) throw new RangeError('Squid송신공통버퍼범위');
            const packet = state.buffer.subarray(0, size);
            if ((target | 0) === -1) {
                // Native 는 같은 전체 2048BYTE 를 대상별 observer 에 넘기고, 기존 const 경로는 방송 장치를 쓴다.
                if (this.hooks.broadcastMutable) need(this.hooks.broadcastMutable)(state.buffer);
                else need(this.hooks.broadcast)(packet);
            } else if (need(this.hooks.connected)(target)) {
                // 반환 크기는 양수 관문이며 Native 실제 observer/선언 WORD 재조회는 같은 전체 예약에서 실행한다.
                if (this.hooks.sendMutable) need(this.hooks.sendMutable)(target, state.buffer);
                else need(this.hooks.send)(target, packet);
            }
        }
    }

    // 현재 +54 의 공통/빈 writer·두 Form·실제 ProcessForm 을 분배해 원문 크기를 돌려준다. (2026-11-11 WB04-E313)
    write(sid, writerArgument) {
        const state = this.state;
        const raw = this.pool.slot(sid);
        const table = read(raw, 0, 4);
        // 004ae360 은 부모 진단 뒤 같은 PID 객체의 실제 writer 를 부른다.
        if (table === PROCESS_FORM_VTABLE) return need(this.hooks.processPacket)(sid, state.buffer, writerArgument) | 0;
        // 보호 83 VT 중 이 표의 +54 만 00455ae0 의 실제 0 반환이다.
        if (table === EMPTY_WRITER_VTABLE) return 0;
        // 다른 현재 VT 는 기존 Form writer 의 명시 실패를 유지한다.
        if (!supportsConstructedVtable(table)) return new RawFormPacket(this.pool, state.forms, state.deletion).writeData(sid, state.buffer);
        const packet = state.buffer;
        writeHeader(packet, state.update);
        put(packet, 4, sid, 2);
        packet.set(raw.subarray(14, 22), 6);
        packet.set(raw.subarray(12, 14), 14);
        packet[16] = raw[10];
        packet[17] = raw[11];
        packet[18] = raw[34];
        packet.set(raw.subarray(42, 50), 19);
        packet.set(raw.subarray(26, 28), 27);
        packet[29] = raw[35];
        packet.set(raw.subarray(36, 40), 30);
        packet[34] = raw[40];
        if (!state.compress) return UPDATE_SIZE;
        // 같은 버퍼의 압축 출력 전에 원문 35BYTE 를 보관한다.
        return this.compress(packet.slice(0, UPDATE_SIZE));
    }

    // 00432fe0 의 16필드 인코딩과 현재 owner/누적을 실행한다(원문 0..15 순서, 14/15 는 무효과). 출력은 공통 버퍼에 쓰고 길이(9 + 데이터)를 돌려준다. (2026-11-11 WB04-E313)
    compress(input) {
        const state = this.state;
        const type = need(this.hooks.type)(input[16]);
        const data = new Uint8Array(64);
        let count = 0;
        let mask = 0;
        // 선택 필드의 낮은 BYTE/WORD 만 기록한다.
        const append = (offset, width, field) => {
            if (count + width > data.length) throw new RangeError('압축Squid의원문64BYTE범위');
            data.set(input.subarray(offset, offset + width), count);
            count += width;
            mask |= 1 << field;
        };
        const x = floatAt(input, 6);
        const y = floatAt(input, 10);
        if (byteCoordinate(x) && byteCoordinate(y)) {
            data[count] = x;
            count += 1;
            data[count] = y;
            count += 1;
            mask |= 1;
        } else append(6, 8, 1);
        const quantity = read(input, 14, 2);
        if (quantity) append(14, quantity <= 255 ? 1 : 2, quantity <= 255 ? 2 : 3);
        if (input[16] !== (need(this.hooks.defaultType)() & 0xff)) append(16, 1, 4);
        if (input[17] & 0xef) append(17, 1, 5);
        if (input[17] & 0x10) mask |= 0x40;
        if (state.sameOwnerEnabled && state.lastOwner !== CRAZY_PLAYER_ID && input[18] === state.lastOwner) mask |= 0x80;
        else {
            append(18, 1, 8);
            state.lastOwner = input[18];
        }
        if (type.flags1 & 0x10) {
            const hp = read(input, 27, 2);
            if (((hp << 16) >> 16) !== need(this.hooks.maxHp)(input[16])) append(27, hp <= 255 ? 1 : 2, hp <= 255 ? 9 : 10);
        }
        if (((input[29] << 24) >> 24) !== type.zOrder) append(29, 1, 11);
        if (read(input, 30, 4)) append(30, 1, 12);
        append(34, 1, 13);
        const packet = state.buffer;
        writeHeader(packet, state.compressed);
        put(packet, 4, read(input, 4, 2), 2);
        put(packet, 6, mask, 2);
        packet.set(data.subarray(0, count), 9);
        packet[8] = count;
        const length = count + 9;
        state.compressedCount = (state.compressedCount + 1) >>> 0;
        state.compressedBytes = (state.compressedBytes + length) >>> 0;
        // 255 번째마다 누적 압축 비율(%)을 수준 4 로그로 남긴다(원문 idiv 범위 밖은 거부).
        if ((state.compressedCount & 0xff) === 0xff) {
            const numerator = Math.imul(state.compressedBytes, 100);
            const denominator = Math.imul(state.compressedCount, UPDATE_SIZE);
            if (!denominator || (numerator === -0x80000000 && denominator === -1)) throw new RangeError('압축Squid누적비율의원문idiv범위');
            need(this.hooks.trace)(4, `compressedUpdateSquid ${Math.trunc(numerator / denominator)}%\n`);
        }
        return length;
    }

    // 현재 +58 은 자산 83개/네 Form 모두 공통 004ac340 임을 보호 PE 에서 대조했다: 같은 헤더/삭제 표식을 소비한다. 미복원 가상 표는 거부한다. (2026-11-11 WB04-E313)
    writeDeletion(sid, flags) {
        const state = this.state;
        const table = read(this.pool.slot(sid), 0, 4);
        if (!supportsConstructedVtable(table) && !Object.values(SQUID_FORM_VTABLES).includes(table)) throw new Error('현재 Squid 삭제 가상+58 미복원');
        return new RawFormPacket(this.pool, state.forms, state.deletion).writeDeletion(sid, flags, state.buffer, this.hooks.assertion ?? null);
    }
}
