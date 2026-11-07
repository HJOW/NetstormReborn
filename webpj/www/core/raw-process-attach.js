// 원본 10.78 의 프로세스 부착 두 부모를 옮긴 모듈이다. 각 부모는 원문이 소비하는 순서대로 진단·선택 장치를 부르고 **부분 실패를 되돌리지 않는다**.
//  - attachProcessForm(004b1440): ProcessForm 의 파생 가상 +74 접두. 부모 변경 진단(popFlags & pfPROCESS_DEBUG, Squid.cpp 0xa77) → 서버이면 현재 PID 조회(lookup) →
//    flags 에 0x50 이 없으면 실제 writer 로 전송 크기를 받고(0이면 transmitSize!=0 0xa7f, 양수 0x7ff 초과면 0xa81 진단 뒤) 송신 → flags|0x40 으로 공통 Form 부착(attach)을 넘긴다.
//    서버가 아니면 lookup/writer/송신을 부르지 않는다. 진단 장치가 정상 복귀하면 다음 위치부터 이어간다.
//  - attachBaseProcess(BaseProcess 생성자 부착): 프로세스 타입 진단(isProcessType, Baseprocess.cpp 0x1c) → 필드 쓰기(부모, 지정 Form) → 부모 상태(패치판은 free/dead 조기 반환) →
//    Kernel 등록(registerProcess) → 새 Form 이면 Create·타입 진단(Squid.cpp 0xab7/0xab8)·TYPE/PID 쓰기·공통 부착(flags|0x80)·가상 통지, 지정 Form 이면 free 일 때만 Take/타입·PID 쓰기/부착,
//    부모/PID 일치 진단(0x5e/0x5f) 뒤 가상 통지. 패치판의 비활성 부모 조기 반환만 false 이다(원문 전체 ABI 반환값은 아니다).
// SID 는 번호(숫자)이며 0 이 빈 값이다. flags 는 uint32 로 다룬다. CD 10.72 경로(진단 파일 이름 대소문자·행 번호)는 범위 밖이다.
// 근거: cpppj `o/RawProcessAttach.cpp/.h`, `o/RawBaseProcessAttach.cpp/.h`(기준선 1f00a25a). 원본 관측 fixture process-attach-current-local-x86.tsv·baseprocess-attach-current-local-x86.tsv
//       (tests/raw-process-attach.test.mjs). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-D5c: Process 부착 부모 이식으로 추가)

// 원문 진단 파일 이름이다(10.78). (2026-11-08 WB04-D5c)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const BASEPROCESS_CPP = '\\Ns\\O\\Baseprocess.cpp';

// Form 슬롯의 TYPE·상태 BYTE, 부모 WORD(+16), Kernel 번호 DWORD(+18) 위치이다. 비정렬 필드이므로 바이트 단위로만 읽고 쓴다. (2026-11-08 WB04-D5c)
const TYPE = 10, STATE = 11, PARENT = 16, PID = 18;
// 상태 free 비트와 부착 flags 이다: 0x10 client 번호로 할당, 0x40 전송 억제(서버 접두 뒤 덧붙임), 0x50 로컬 전용 마스크, 0x80 부모 변경 허용(pfPROCESS_DEBUG). (2026-11-08 WB04-D5c)
const FREE = 1, CLIENT_FORM = 0x10, SUPPRESS_TRANSMIT = 0x40, LOCAL_MASK = 0x50, PROCESS_DEBUG = 0x80;

// 선택된 실제 장치를 꺼낸다. 없으면 그 위치에서 오류이며 앞선 쓰기는 남는다. (2026-11-08 WB04-D5c)
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`Process 부착의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 슬롯의 비정렬 Kernel 번호 DWORD 전체를 읽는다. (2026-11-08 WB04-D5c)
function pidOf(raw) {
    return (raw[PID] | (raw[PID + 1] << 8) | (raw[PID + 2] << 16) | (raw[PID + 3] << 24)) >>> 0;
}

// Kernel 번호 DWORD 만 쓴다. 그 밖의 payload 는 보존한다. (2026-11-08 WB04-D5c)
function putPid(raw, pid) {
    // 낮은 바이트부터 네 바이트를 쓴다.
    for (let index = 0; index < 4; index += 1) raw[PID + index] = (pid >>> (index * 8)) & 0xff;
}

// 004b1440: ProcessForm 의 파생 +74 접두이다. hooks: { server() → boolean, lookup(form), writeData(parentChanged) → int32, transmit(size), attach(form, parent, flags), assertion(expression, file, line) }.
// 선택되지 않은 직렬화/송신 장치는 요구하지 않는다. 진단 장치가 정상 복귀하면 서버 판정부터 계속하고, 예외면 그 앞 효과만 남는다.
// 사용: attachProcessForm(pool, form, parent, flags, hooks). (2026-11-08 WB04-D5c)
export function attachProcessForm(pool, form, parent, flags, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('Process 부착은 10.78 SID 풀만 받습니다.');
    let current = flags >>> 0;
    // 첫 진단 전에 읽은 부모 변경 판정을 유지한다.
    const raw = pool.slot(form);
    const changed = (raw[PARENT] | (raw[PARENT + 1] << 8)) !== parent;
    const assertion = (expression, line) => need(hooks.assertion, 'assertion')(expression, SQUID_CPP, line);
    if (changed && !(current & PROCESS_DEBUG)) assertion('popFlags & pfPROCESS_DEBUG', 0xa77);
    if (need(hooks.server, 'server')()) {
        need(hooks.lookup, 'lookup')(form);
        if (!(current & LOCAL_MASK)) {
            const size = need(hooks.writeData, 'writeData')(changed) | 0;
            if (!size) assertion('transmitSize!=0', 0xa7f);
            if (size > 0) {
                if (size > 0x7ff) assertion('transmitSize < TRANSMIT_BUFFER_SIZE', 0xa81);
                need(hooks.transmit, 'transmit')(size);
            }
        }
        current = (current | SUPPRESS_TRANSMIT) >>> 0;
    }
    need(hooks.attach, 'attach')(form, parent, current);
}

// BaseProcess 생성자 부착이다. fields 는 프로세스의 실제 두 SID 필드({ form, parent } 읽기/쓰기 속성)이며 콜백 뒤에도 현재 값을 읽는다.
// hooks: { typeEnd() → DWORD(signed 상한), processFormType() → DWORD, registerProcess() → PID, create(할당 flags 0/2) → 새 Form, take(selected), parent(selected) → 현재 부모,
//          attach(form, into, popFlags), onAttached(), assertion(expression, file, line) }. 실패 시 이미 쓴 필드/등록/슬롯을 복구하지 않는다.
// 반환: 패치판의 비활성(free/dead) 부모 조기 반환이면 false, 그 밖에는 true. 사용: attachBaseProcess(pool, fields, 46, parent, 0, 0x50, hooks). (2026-11-08 WB04-D5c)
export function attachBaseProcess(pool, fields, processType, parent, form, flags, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('Process 부착은 10.78 SID 풀만 받습니다.');
    const type = processType >>> 0;
    const signedType = type | 0;
    const diagnose = (expression, line) => need(hooks.assertion, 'assertion')(expression, BASEPROCESS_CPP, line);
    if (signedType < 10 || (need(hooks.typeEnd, 'typeEnd')() | 0) <= signedType) diagnose('isProcessType(_processType)', 0x1c);
    fields.parent = parent;
    fields.form = form;
    // 패치판은 부모가 free 이거나 dead 이면 등록하지 않고 돌아온다.
    if (pool.slot(parent)[STATE] & 3) return false;
    const pid = need(hooks.registerProcess, 'registerProcess')() >>> 0;
    if (!fields.form) {
        const created = need(hooks.create, 'create')((flags & CLIENT_FORM) ? 2 : 0);
        fields.form = created;
        const raw = pool.physical(created);
        // 실제 setProcessType 의 두 진단도 정상 복귀하면 이어서 쓴다.
        const typeDiagnostic = (expression, line) => need(hooks.assertion, 'assertion')(expression, SQUID_CPP, line);
        if (signedType < 0 || signedType > 0xfe) typeDiagnostic('0 <= pType && pType < MAX_TYPES', 0xab7);
        if (raw[TYPE] !== (need(hooks.processFormType, 'processFormType')() >>> 0)) typeDiagnostic('typeNum == processFormType', 0xab8);
        raw[TYPE] = type & 0xff;
        putPid(raw, pid);
        need(hooks.attach, 'attach')(created, fields.parent, (flags | PROCESS_DEBUG) >>> 0);
        need(hooks.onAttached, 'onAttached')();
        return true;
    }
    // Take 전에 저장한 지정 객체는 이후 필드 변경에도 유지한다.
    const selected = fields.form;
    const raw = pool.physical(selected);
    if (raw[STATE] & FREE) {
        need(hooks.take, 'take')(selected);
        raw[TYPE] = type & 0xff;
        putPid(raw, pid);
        need(hooks.attach, 'attach')(selected, fields.parent, (flags | PROCESS_DEBUG) >>> 0);
    }
    // getter 가 복귀한 뒤 현재 프로세스 부모 필드를 읽는다.
    const currentParent = need(hooks.parent, 'parent')(selected);
    if (currentParent !== fields.parent) diagnose('s.getParentSid() == attachToSid', 0x5e);
    if (pidOf(raw) !== pid) diagnose('s.getPid() == pid', 0x5f);
    need(hooks.onAttached, 'onAttached')();
    return true;
}
