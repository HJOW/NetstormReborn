// 수신 활동 기록과 실제 0x64 등록을 옮긴 모듈이다(cpppj `o/RawConnectionActivity.cpp/.h`, 10.78: 004b6180 수신 몸체, 0040be79..0040be97 등록).
// 사용: const activity = new RawConnectionActivity({ frames, wallSeconds }, { wallSeconds, frameNumber, server, eraseStatus }); activity.receive(sender); registerConnectionActivity(dispatch, activity);
//   tables 는 같은 Client owner 의 두 실제 22DWORD 표(마지막 수신 프레임·벽시각 초)를 **빌린다**(복사본을 동기화하지 않음). 두 표는 서로 다른 메모리여야 하고 사이의 Zone 두 DWORD(005c8540/544)는 표에 포함하지 않는다.
//   hooks 는 원문 위치에서 현재값을 매번 새로 조회하는 실제 장치이다: wallSeconds()=00460d70(분배기의 앞선 now 를 재사용하지 않음), frameNumber()=0055b4a8 의 하위 DWORD, server()=00540bc0, eraseStatus(kind, player)=004d3090 상태 삭제.
// 순서: 벽시각 조회 → signed64 절삭 하위 DWORD → 프레임 조회 → 벽시각 표 store → server 조회 → 프레임 표 store → 상태 삭제(server 면 (2, sender) 아니면 (1, 0)). 호출 뒤 표를 다시 동기화하거나 값을 복구하지 않는다.
// 정상 연결 인덱스 0..21 만 처리한다. 범위 밖은 원문의 메모리 덮어쓰기를 흉내 내지 않고 첫 효과 전에 명시 거부한다. 패킷 본문·연결 객체는 원문처럼 읽지 않는다.
// 기준선 뒤 cpppj 변경(eeae45c69, 3.6절 ③ 4-D): 등록 때 실제 Native 고정 수신기(nativeEntry 0x004b6180)가 이미 있으면 같은 값을 대여하는 분기는 옮기지 않는다(고정 Native 수신기 승격은 E306 의 ③ 과 같은 묶음).
// CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E309: 4-C RawConnectionActivity 이식)

// 005c84e8 의 cleanup 22DWORD 와 같은 정상 연결 인덱스(0..21) 개수이다. (2026-11-11 WB04-E309)
export const CONNECTION_ACTIVITY_ENTRIES = 22;

// 0x64(수신 활동) 패킷 타입 번호이다. 길이 5 는 기존 PE 길이 표의 현재값이며 이 모듈이 덮어쓰지 않는다. (2026-11-11 WB04-E309)
const ACTIVITY_TYPE = 0x64;

// 004e49c0: double 을 signed64 로 0 방향 절삭한 뒤의 EAX(하위 DWORD)이다. masked x87 의 비유한/범위 밖 "integer indefinite" 하위 DWORD 는 0 이다. (2026-11-11 WB04-E309)
function wallLowDword(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 두 표가 같은 메모리 시작(같은 Native 주소)인지 본다. 같은 배열이거나 같은 버퍼의 같은 오프셋이면 같은 주소로 본다. (2026-11-11 WB04-E309)
function sameStart(first, second) {
    return first === second || (first.buffer === second.buffer && first.byteOffset === second.byteOffset);
}

// 수신 활동 몸체이다. 표와 장치는 이 객체보다 오래 살아야 하며 모든 효과를 실제 장치로 제공한다. (2026-11-11 WB04-E309)
export class RawConnectionActivity {
    // 사용: 두 Native 주소를 실수로 같은 표에 연결하거나 필수 장치를 생략한 구성을 거부한다. 표는 정확히 22칸 Uint32Array 여야 한다. (2026-11-11 WB04-E309)
    constructor(tables, hooks) {
        const frames = tables?.frames;
        const wallSeconds = tables?.wallSeconds;
        for (const table of [frames, wallSeconds]) {
            if (!(table instanceof Uint32Array) || table.length !== CONNECTION_ACTIVITY_ENTRIES) throw new TypeError('연결 활동의 표는 22칸 Uint32Array 여야 합니다.');
        }
        // 선택 장치가 아니라 모두 필수이다.
        const missing = ['wallSeconds', 'frameNumber', 'server', 'eraseStatus'].some((name) => typeof hooks?.[name] !== 'function');
        if (sameStart(frames, wallSeconds) || missing) throw new TypeError('연결 활동의 별개 두 표/실제 필수 장치 누락');
        this.tables = tables;
        this.hooks = hooks;
    }

    // 004b6180: sender(정상 0..21)의 마지막 수신 벽시각 초/프레임을 기록하고 상태를 삭제한다. server 는 앞선 벽시각 효과 뒤 다시 읽고, 프레임은 벽시각 store 전에 캡처한 같은 DWORD 를 쓴다. (2026-11-11 WB04-E309)
    receive(sender) {
        if (!Number.isInteger(sender) || sender < 0 || sender >= CONNECTION_ACTIVITY_ENTRIES) throw new RangeError('연결 활동의 정상 Native 인덱스0..21 범위밖');
        // 004b6188/6196/619b: 벽시각 → 절삭 → 프레임 순으로 선택된 현재 입력을 읽는다.
        const wall = wallLowDword(this.hooks.wallSeconds());
        const frame = this.hooks.frameNumber() >>> 0;
        // 004b61a6/61ad/61b6: 두 store 를 한꺼번에 앞당기지 않고 사이에서 server 를 조회한다.
        this.tables.wallSeconds[sender] = wall;
        const server = Boolean(this.hooks.server());
        this.tables.frames[sender] = frame;
        // 원문 호출 뒤 표를 다시 동기화하거나 값을 복구하지 않는다.
        this.hooks.eraseStatus(server ? 2 : 1, server ? sender : 0);
    }
}

// 실제 0040be79..0040be97 의 세 등록(관찰기 없음·버전 1/정책 0·수신 본문)을 분배기에 한다. 길이 5 는 기존 PE 길이 표의 현재값이며 암묵 초기화하지 않는다. activity 는 등록을 바꾸기 전까지 살아 있어야 한다. (2026-11-11 WB04-E309)
export function registerConnectionActivity(dispatch, activity) {
    // 0040b380 은 송신/수신 선택과 관계없이 아무 효과 없이 반환한다.
    dispatch.observe(ACTIVITY_TYPE, () => {});
    dispatch.version(ACTIVITY_TYPE, 1, 0);
    // 인접 connection/payload 는 원문처럼 소비하지 않으며, sender 는 signed 32비트 비트열로 읽는다.
    dispatch.register(ACTIVITY_TYPE, (sender) => { activity.receive(sender | 0); });
}
