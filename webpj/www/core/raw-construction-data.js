// 건설 프로세스 상태 메시지 0x37의 쓰기·읽기·수신과 실제 프로세스의 전송 래퍼를 옮긴 모듈이다(cpppj `o/RawConstructionData.cpp/.h`, 10.78 004426d0/00442760/004427b0/00443bd0).
//  - writeConstructionData(fields, packet, form, parent, force, header): 현재 필드를 리틀엔디언으로 쓰고 dirty 를 0 으로 만든다. force 이면 먼저 dirty 를 0xff 로 둔다. 부모는 인자가 0 이 아닐 때만 쓰고(dirty bit0 도 켠다),
//    시작/끝 시각(double 비트 그대로, 산술 변환 없음)과 추가 바이트는 dirty 비트 1/2/3 에 따라 쓴다. 패킷은 헤더 9바이트(protocol, message, 선언 길이 WORD, form DWORD, dirty 바이트)로 시작하며 부족한 버퍼는
//    쓰기/dirty 변경 전에 거부한다. 반환은 패킷에 기록한 길이이다(최대 30).
//  - writeConstructionProcessData(process, packet, includeParent, force, header): 실제 프로세스의 부모(includeParent 또는 force 일 때)·form 으로 위 쓰기를 한다.
//  - readConstructionData(fields, packet): 선택된 시각/추가 바이트만 복원하고 부모 DWORD(없으면 0)를 반환한다. dirty/sound 는 보존한다. fields=null 은 시각/추가 비트가 없을 때만 허용한다. 선언 길이는 읽지 않으며
//    실제 버퍼 폭만 검사한다.
//  - RawConstructionData.receive(packet): form 이 free 이면(dirty 가 0xff 가 아니면 원문 진단 0x42) 새 객체를 만들어(hooks.allocate 또는 실제 logic 으로 ConstructionProcess) 필드를 0xff/0/0 으로 초기화하고 읽은 뒤
//    부착한다(hooks.attach 또는 host.attach(…, 현재 프로세스 타입, 부모, 0x10, form)). 살아 있는 form 이면 부모 비트가 있을 때 원문 진단 0x4a 를 낸 뒤 현재 프로세스를 host(또는 hooks.lookup 의 Kernel 번호)로 찾아 필드만 갱신한다.
//    진단이 복귀하면 원문처럼 같은 생성/읽기/부착을 계속한다(장치가 없으면 거부).
// 10.72(CD) 경로(진단 파일 이름 대소문자 차이)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E141: 건설 상태 메시지 이식으로 추가)
import { ConstructionProcess, createConstructionProcessFields } from './raw-construction-process.js';

// 헤더 길이, 부모/시작/끝/추가 바이트의 전송 비트, 최대 패킷 크기이다. (2026-11-09 WB04-E141)
const HEADER_SIZE = 9;
const PARENT = 1;
const START = 2;
const END = 4;
const ADDITIONAL = 8;
export const CONSTRUCTION_DATA_MAXIMUM = 30;
// 원본 전역의 두 헤더 바이트 기본값(세 PE 에 저장된 1/0x37), 쓰기 허용 form 상한(WORD 번호), 부착 flags, 진단 파일 이름(10.78)이다. (2026-11-09 WB04-E141)
const DEFAULT_PROTOCOL = 1;
const DEFAULT_MESSAGE = 0x37;
const ATTACH_FLAGS = 0x10;
const ASSERT_FILE = 'c:\\ns\\o\\_p_constructionprocessdata.cpp';

// 헤더의 두 바이트 기본값을 만든다(패치 005325b9 / 005325b8). 사용: writeConstructionData(…, createConstructionDataHeader()). (2026-11-09 WB04-E141)
export function createConstructionDataHeader() {
    return { protocol: DEFAULT_PROTOCOL, message: DEFAULT_MESSAGE };
}

// 선택된 실제 필드의 길이이다. 상위 네 비트는 보존하지만 추가 payload 를 만들지 않는다. (2026-11-09 WB04-E141)
function fieldSize(mask) {
    return ((mask & START) ? 8 : 0) + ((mask & END) ? 8 : 0) + ((mask & ADDITIONAL) ? 1 : 0);
}

// 원본의 범위 밖 읽기 대신 실제 버퍼 길이를 미리 요구한다. 선언 길이 WORD 는 별도 전송 계층의 책임이다. (2026-11-09 WB04-E141)
function require(actual, needed) {
    if (actual < needed) throw new RangeError('건설 상태 패킷 길이 부족');
}

// 실제 폭의 바이트만 low→high 순서로 읽는다(부호 없는 정수). 호출자가 전체 크기를 검사한 뒤 쓴다. (2026-11-09 WB04-E141)
function readBits(bytes, offset, width) {
    let result = 0;
    for (let index = 0; index < width; index += 1) result += bytes[offset + index] * 2 ** (8 * index);
    return result;
}

// 비트 값을 주어진 폭으로만 쓴다. 뒤의 패킷/주변 표식은 보존한다. (2026-11-09 WB04-E141)
function writeBits(bytes, offset, value, width) {
    for (let index = 0; index < width; index += 1) bytes[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// double 과 두 DWORD 사이를 산술 변환 없이 오간다. (2026-11-09 WB04-E141)
const view = new DataView(new ArrayBuffer(8));
function writeDouble(bytes, offset, value) {
    view.setFloat64(0, value, true);
    for (let index = 0; index < 8; index += 1) bytes[offset + index] = view.getUint8(index);
}
function readDouble(bytes, offset) {
    for (let index = 0; index < 8; index += 1) view.setUint8(index, bytes[offset + index]);
    return view.getFloat64(0, true);
}

// 004426d0: 현재 필드를 쓰고 dirty 를 0 으로 만든다. 부모는 0 이 아닐 때만 쓰고, 기존 dirty 의 bit0 과 부모 0 의 조합도 원본 그대로이다. (2026-11-09 WB04-E141)
export function writeConstructionData(fields, packet, form, parent, force, header = createConstructionDataHeader()) {
    const mask = force ? 0xff : fields.dirty;
    const size = HEADER_SIZE + (parent ? 4 : 0) + fieldSize(mask);
    require(packet.length, size);
    if (force) fields.dirty = 0xff;
    packet[0] = header.protocol & 0xff;
    packet[1] = header.message & 0xff;
    writeBits(packet, 4, form, 4);
    let cursor = HEADER_SIZE;
    if (parent) {
        fields.dirty |= PARENT;
        writeBits(packet, cursor, parent, 4);
        cursor += 4;
    }
    if (fields.dirty & START) { writeDouble(packet, cursor, fields.start); cursor += 8; }
    if (fields.dirty & END) { writeDouble(packet, cursor, fields.end); cursor += 8; }
    if (fields.dirty & ADDITIONAL) { packet[cursor] = fields.additional & 0xff; cursor += 1; }
    writeBits(packet, 2, cursor, 2);
    packet[8] = fields.dirty & 0xff;
    fields.dirty = 0;
    return cursor;
}

// 00443bd0: includeParent 또는 force 가 참일 때 실제 프로세스의 부모를 전송한다. (2026-11-09 WB04-E141)
export function writeConstructionProcessData(process, packet, includeParent, force, header = createConstructionDataHeader()) {
    const parent = (includeParent || force) ? process.parent : 0;
    return writeConstructionData(process.fields, packet, process.form, parent, force, header);
}

// 00442760: 선택된 시각/추가 바이트만 복원하고 부모 DWORD(없으면 0)를 반환한다. dirty/sound 는 보존한다. 선택된 실제 저장 직전만 검사해 앞선 부분 쓰기를 보존한다. (2026-11-09 WB04-E141)
export function readConstructionData(fields, packet) {
    require(packet.length, HEADER_SIZE);
    const mask = packet[8];
    let parent = 0;
    let cursor = HEADER_SIZE;
    const requireFields = (width) => {
        require(packet.length, cursor + width);
        if (!fields) throw new TypeError('건설 상태의 실제 필드가 없습니다');
    };
    if (mask & PARENT) { require(packet.length, cursor + 4); parent = readBits(packet, cursor, 4); cursor += 4; }
    if (mask & START) { requireFields(8); fields.start = readDouble(packet, cursor); cursor += 8; }
    if (mask & END) { requireFields(8); fields.end = readDouble(packet, cursor); cursor += 8; }
    if (mask & ADDITIONAL) { requireFields(1); fields.additional = packet[cursor]; }
    return parent;
}

// 건설 상태 수신기이다. pool(10.78 SidPool)·state(ConstructionProcessState)는 이 객체보다 오래 살아야 한다. 두 구성이 있다:
//  - { hooks: { allocate()→fields, attach(fields, type, parent, form, flags), lookup(pid)→fields }, host?, assertion? } — 새 form 부착을 명시 경계로 둔다(host 가 있으면 lookup 대신 실제 Kernel 조회).
//  - { logic: RawConstructionProcess, host: SquidProcessHost, assertion? } — 같은 풀의 실제 건설 몸체와 host 에 새/기존 수신을 직접 연결한다(새 객체의 소유권은 Kernel 에 넘기며 실패 시 자동 해제).
// 사용: new RawConstructionData(pool, state, { logic, host, assertion }).receive(packet). (2026-11-09 WB04-E141)
export class RawConstructionData {
    #pool;
    #state;
    #hooks;
    #host;
    #logic;
    #assertion;

    // 풀 일치와 필수 경계를 확인한다. (2026-11-09 WB04-E141)
    constructor(pool, state, options) {
        if (pool?.edition !== 'patch1078' || !state) throw new TypeError('건설 상태 수신의 저장소/효과 연결 오류');
        const { hooks = null, host = null, logic = null, assertion = null } = options ?? {};
        if (logic) {
            if (logic.pool !== pool || host?.pool !== pool) throw new TypeError('건설 상태 수신의 실제 풀이 다릅니다');
        } else if (typeof hooks?.allocate !== 'function' || typeof hooks?.attach !== 'function' || (!host && typeof hooks?.lookup !== 'function') || (host && host.pool !== pool)) {
            throw new TypeError('건설 상태 수신의 저장소/효과 연결 오류');
        }
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks;
        this.#host = host;
        this.#logic = logic;
        this.#assertion = assertion;
    }

    // 원문 파일/행을 같은 진단 장치에 전달한다. 진단이 정상 복귀하면 원문 다음 명령으로 진행한다. (2026-11-09 WB04-E141)
    #assert(expression, line) {
        if (typeof this.#assertion !== 'function') throw new Error('건설 상태의 실제 진단 장치 미연결');
        this.#assertion(expression, ASSERT_FILE, line);
    }

    // 004427b0: free 의 0xff / live 의 부모 bit1 진단 뒤 정상 복귀하면 같은 생성/읽기/부착을 계속한다. 현재 PID 가 없고 객체 필드가 선택되지 않으면 읽기만 끝낸다. (2026-11-09 WB04-E141)
    receive(packet) {
        require(packet.length, HEADER_SIZE);
        const number = readBits(packet, 4, 4);
        const mask = packet[8];
        if (!number || number >= this.#pool.capacity) throw new RangeError('건설 상태 form 번호 오류');
        const raw = this.#pool.slot(number);
        if (raw[11] & 1) {
            if (mask !== 0xff) this.#assert('*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF', 0x42);
            const owned = this.#logic ? new ConstructionProcess(this.#logic, createConstructionProcessFields()) : null;
            const fields = owned ? owned.fields : this.#hooks.allocate();
            if (!fields) throw new Error('건설 상태 새 객체 할당 실패');
            // 원본 수신 생성자가 쓰는 필드만 초기화한다. 추가/소리의 미초기화 영역은 저장소 공급자가 정한다.
            fields.dirty = 0xff;
            fields.start = 0;
            fields.end = 0;
            const parent = readConstructionData(fields, packet);
            if (owned) {
                if (parent > 65535 || parent >= this.#pool.capacity) throw new RangeError('수신한 건설 부모 DWORD 범위 오류');
                this.#host.attach(owned, this.#state.currentType(), parent, ATTACH_FLAGS, number);
            } else this.#hooks.attach(fields, this.#state.currentType(), parent, number, ATTACH_FLAGS);
            return;
        }
        if (mask & PARENT) this.#assert('(*(char *)(( (char *)zacket+8) ) & 1) == 0', 0x4a);
        let fields = null;
        if (this.#host) {
            const process = this.#host.fromForm(number);
            if (process instanceof ConstructionProcess) fields = process.fields;
        } else fields = this.#hooks.lookup(readBits(raw, 18, 4));
        readConstructionData(fields, packet);
    }
}
