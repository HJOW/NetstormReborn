// 원본 Form 가상+54(감시 Form 13바이트 / 내용물 Form 18바이트)와 공통 삭제 가상+58(8바이트)의 실제 패킷 직렬화를 같은 raw 슬롯에서 옮긴 모듈이다(cpppj `o/RawFormPacket.cpp/.h`, 10.78: 004ae560 감시/내용물, 004aeb40 내용물 보조, 004ac340 삭제, 헤더 작성 004df590).
// 사용: const writer = new RawFormPacket(pool, headers, stamp); writer.writeData(formSid, packet) → 13 | 18; writer.writeDeletion(sid, flags, packet[, assertion]) → 8.
//   pool = 단일 실제 SID 풀(`SidPool`), headers = `createFormPacketHeaders()` 의 현재 선언 헤더(요청 중에도 직접 읽음), stamp = `createFormDeletionStamp()` 의 한 번 소비 삭제 표식(다른 삭제 요청과 공유)이다. 모두 빌리며 사본을 만들지 않는다.
// 실제 Depend/Content 생성자 가상 표로 두 몸체를 고르며 free/dead/void 필터를 추가하지 않는다. 반환 크기는 선언 길이와 별개이고 반환 크기 뒤의 옛 버퍼 바이트는 보존한다. 미복원 다른 Form 가상 표는 거부한다.
// 기준선 뒤 cpppj 변경(b8cadde14, 3.6절 ②): 삭제 writer 는 진단보다 먼저 현재 헤더를 실제 버퍼에 쓰고, 선택한 실제 Client 진단 장치가 있으면 client SID 진단 뒤 정상 복귀를 허용한다(독립 호출은 기존 명시 오류 유지). CD(10.72)는 옮기지 않았다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E312: 4-C RawFormPacket 이식. 앞서 Form 가상 표 4종 칸은 squid-vtables.js 에 있음)
import { SQUID_FORM_VTABLES } from './squid-vtables.js';

// 감시/내용물/삭제 패킷의 출력 크기(바이트)이다. 선언 길이와 반환 크기를 같게 강제하지 않는다. (2026-11-11 WB04-E312)
const DEPENDENCY_SIZE = 13;
const CONTENT_SIZE = 18;
const DELETION_SIZE = 8;

// 비정렬 원본 WORD/DWORD 를 실제 폭대로 읽는다(필드의 각 바이트만 읽고 인접 payload 는 섞지 않음). (2026-11-11 WB04-E312)
function read(raw, offset, width) {
    let value = 0;
    // little endian 으로 낮은 바이트부터 조립한다.
    for (let index = 0; index < width; index += 1) value |= raw[offset + index] << (8 * index);
    return value >>> 0;
}

// 원본 header/WORD 필드의 지정 폭만 낮은 바이트부터 덮어쓴다. 반환 크기 뒤의 옛 버퍼는 보존한다. (2026-11-11 WB04-E312)
function put(raw, offset, value, width) {
    // 필드의 각 바이트만 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 004df590 의 erase 0 경로이다: [protocol & 0x7f, message, 선언 길이 WORD(4 미만이면 4)]. 출력 폭이 부족하면 쓰기 전에 거부한다. 반환 크기와 선언 길이를 같게 강제하지 않는다. (2026-11-11 WB04-E312)
function writeHeader(packet, header, size) {
    if (packet.length < size) throw new RangeError('Form 패킷 버퍼 부족');
    packet[0] = (header.protocol & 0xff) & 0x7f;
    packet[1] = header.message & 0xff;
    put(packet, 2, Math.max(4, header.length & 0xffff), 2);
}

// 두 PE 의 초기 메시지/프로토콜/길이는 같다. 현재 값은 세션이 소유하며 writer 가 호출마다 읽는다: 감시 Form(SID/부모/상태/상대 Form 두 개), 내용물 Form(WORD 필드와 추가 WORD/순번 BYTE), 공통 삭제(SID/플래그/최근 표식). (2026-11-11 WB04-E312)
export function createFormPacketHeaders() {
    return {
        dependency: { message: 0x65, protocol: 1, length: 13 },
        content: { message: 0x2d, protocol: 1, length: 18 },
        deletion: { message: 0x2e, protocol: 1, length: 8 },
    };
}

// 삭제 writer 가 한 번 소비하는 원본 005c845c/60 의 전역 쌍이다: SID DWORD 가 일치하면 sid 만 0 으로 지우고 token 의 낮은 WORD 를 OR 한다. (2026-11-11 WB04-E312)
export function createFormDeletionStamp() {
    return { sid: 0, token: 0 };
}

// 순수 직렬화 몸체이다. pool/headers/stamp 는 이 객체보다 오래 살아야 한다. (2026-11-11 WB04-E312)
export class RawFormPacket {
    // 사용: 장치/송수신을 열지 않고 같은 풀과 현재 헤더/삭제 표식을 빌린다. (2026-11-11 WB04-E312)
    constructor(pool, headers, stamp) {
        if (!pool || typeof pool.slot !== 'function' || !pool.layout) throw new TypeError('Form 패킷의 SID 풀이 필요합니다.');
        if (!headers?.dependency || !headers.content || !headers.deletion) throw new TypeError('Form 패킷 헤더 묶음이 필요합니다.');
        if (!stamp || typeof stamp !== 'object') throw new TypeError('Form 삭제 표식이 필요합니다.');
        this.pool = pool;
        this.headers = headers;
        this.stamp = stamp;
    }

    // 원본은 0..4 를 이 assert 에서 client SID 로 분류하지 않는다: 5 이상 서버 영역 앞이면 client SID 로 보고 거부한다. (2026-11-11 WB04-E312)
    serverSid(sid) {
        if (sid >= 5 && sid < this.pool.layout.serverFirst) throw new Error('Form 패킷의 client SID 원본 assert');
    }

    // ProcessForm 은 타입 BYTE 가 프로세스 번호여도 같은 생성자 가상 표의 +0 getter 를 쓴다: 호출 시점의 현재 raw 가상 표가 Depend/Process/Gump/Content 네 실제 표 중 하나여야 하고, 값은 +16 WORD(부모)이다. 상태 필터는 추가하지 않는다. (2026-11-11 WB04-E312)
    parent(form) {
        const raw = this.pool.slot(form);
        const table = read(raw, 0, 4);
        if (!Object.values(SQUID_FORM_VTABLES).includes(table)) throw new Error('Form 부모 가상 getter 미복원');
        return read(raw, 16, 2);
    }

    // 실제 class 가상 표로 두 몸체를 고른다(free/dead/void 필터 없음). 감시 Form 은 13 바이트, 내용물 Form 은 18 바이트를 쓰고 그 크기를 돌려준다. (2026-11-11 WB04-E312)
    writeData(form, packet) {
        const raw = this.pool.slot(form);
        const table = read(raw, 0, 4);
        const dependency = table === SQUID_FORM_VTABLES.depend;
        if (!dependency && table !== SQUID_FORM_VTABLES.content) throw new Error('다른 Form 가상+54 writer 미복원');
        // 원본도 부모의 server 번호를 먼저 검사한다.
        const parent = this.parent(form);
        this.serverSid(parent);
        if (dependency) {
            if (!this.pool.isServer) throw new Error('감시 Form writer의 원본 server assert');
            // 상대 Form 과 그 부모도 server 번호여야 한다. 현재 0 인 상대는 getter 를 호출하지 않는다.
            for (const offset of [18, 20]) {
                const other = read(raw, offset, 2);
                if (other) {
                    this.serverSid(other);
                    this.serverSid(this.parent(other));
                }
            }
            writeHeader(packet, this.headers.dependency, DEPENDENCY_SIZE);
            put(packet, 4, form, 2);
            put(packet, 6, parent, 2);
            packet[8] = raw[11];
            put(packet, 9, read(raw, 18, 2), 2);
            put(packet, 11, read(raw, 20, 2), 2);
            return DEPENDENCY_SIZE;
        }
        writeHeader(packet, this.headers.content, CONTENT_SIZE);
        put(packet, 4, form, 2);
        put(packet, 6, read(raw, 12, 2), 2);
        put(packet, 8, read(raw, 18, 2), 2);
        put(packet, 10, read(raw, 20, 2), 2);
        put(packet, 12, read(raw, 24, 2), 2);
        put(packet, 14, read(raw, 16, 2), 2);
        packet[16] = raw[23];
        packet[17] = raw[11];
        return CONTENT_SIZE;
    }

    // 실제 공통 +58 에 도달한 SID 를 쓴다: 현재 선언 헤더를 진단보다 먼저 실제 버퍼에 쓰고, client SID 면 선택한 진단 장치(assertion)를 부르며(정상 복귀하면 계속, 없으면 기존 명시 오류), SID 와 flags 상위 WORD 의 fff0 비트에 최근 표식 WORD 를 합친다. 반환은 8 이며 호출자가 현재 +58 분배를 확인한다. (2026-11-11 WB04-E312)
    writeDeletion(sid, flags, packet, assertion = null) {
        // 004ac340 은 진단보다 먼저 현재 헤더를 실제 버퍼에 쓴다(버퍼 부족이면 표식 변경 전에 거부).
        writeHeader(packet, this.headers.deletion, DELETION_SIZE);
        if (sid >= 5 && sid < this.pool.layout.serverFirst) {
            if (assertion) assertion('!isClientSid()', '\\Ns\\O\\Squid.cpp', 0x2f6);
            else this.serverSid(sid);
        }
        put(packet, 4, sid, 2);
        // 현재 SID 와 일치하지 않으면 원본의 최근 표식 소비는 0 이다.
        let token = 0;
        if (this.stamp.sid === sid) {
            token = this.stamp.token & 0xffff;
            this.stamp.sid = 0;
        }
        put(packet, 6, ((flags >>> 16) & 0xfff0) | token, 2);
        return DELETION_SIZE;
    }
}
