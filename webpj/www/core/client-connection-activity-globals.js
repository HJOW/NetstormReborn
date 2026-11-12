// Client 수명의 두 연결 활동 표(마지막 수신 프레임·벽시각 초 각 22DWORD)와 그 사이의 실제 Zone 두 DWORD, 하트비트 기한·자동 재시작 DWORD 를 별개 필드로 보관하는 모듈이다
// (cpppj `client/ClientConnectionActivityGlobals.h`, 10.78 전용, 기준선 1f00a25a 뒤 37dcdfc2 에서 HeartbeatEnd·AutoRestart 추가).
// 원문 주소: 프레임 표 005c84e8..005c853c(88BYTE), Zone stamp 005c8540, Zone+10 복사 출력 005c8544, 벽시각 표 005c8548..005c859c(88BYTE), 하트비트 기한 005c8620(double), 자동 재시작 00531908.
// 두 표와 두 Zone DWORD 는 서로 다른 필드이며 표 정리(cleanup)는 프레임 22DWORD 만 지운다. 벽시각 표 바로 뒤 005c85a0(CentralCreated)은 표에 포함하지 않는다.
// Session 반납/연결 교체가 이 객체의 값을 다시 초기화하지 않는다: 소비자(`RawConnectionActivity`, 로그인 Zone 설치, 연결 종료 처리)는 `tables()` 와 상자들을 **빌린다**.
// 사용: const globals = new ClientConnectionActivityGlobals(); const activity = new RawConnectionActivity(globals.tables(), hooks); globals.activeStamp().value = 0x12345678;
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E325: 4-C ClientConnectionActivityGlobals 이식)
import { CONNECTION_ACTIVITY_ENTRIES } from './raw-connection-activity.js';
import { DwordBox } from './client-subscriber-globals.js';

// 보호 PE 의 자동 재시작 DWORD 초기값(6)이다. 실제 옵션 해석이 1 을 쓰기 전까지의 값이다. (2026-11-12 WB04-E325: 기준선 뒤 37dcdfc2 반영)
export const AUTO_RESTART_INITIAL = 6;

// 부동소수(double) 한 칸을 `{ value }` 상자로 노출하는 클래스이다. 하트비트 기한처럼 DWORD 로 줄이면 안 되는 벽시각 double 칸에 쓴다.
// 사용: const box = new DoubleBox(); box.value = 123.5. 쓰기는 JS 수(double)를 그대로 저장한다. (2026-11-12 WB04-E325)
export class DoubleBox {
    // 현재 double 값이다.
    #value;

    // 초기값(기본 0)을 저장한다. (2026-11-12 WB04-E325)
    constructor(initial = 0) {
        this.#value = Number(initial);
    }

    // 현재 값을 읽는다. (2026-11-12 WB04-E325)
    get value() { return this.#value; }

    // 새 값을 저장한다. 수가 아닌 값은 NaN 이 되므로 호출자가 수를 넘긴다. (2026-11-12 WB04-E325)
    set value(next) { this.#value = Number(next); }
}

// Client 수명의 활동 표와 Zone/하트비트 전역의 소유자이다. 각 접근자는 호출마다 같은 객체를 돌려준다(주소 고정). (2026-11-12 WB04-E325)
export class ClientConnectionActivityGlobals {
    // 005c84e8 부터 88BYTE 의 마지막 수신 프레임 표(최초 BSS 0)이다. cleanup 은 이 22DWORD 만 지운다.
    #frames = new Uint32Array(CONNECTION_ACTIVITY_ENTRIES);
    // 005c8548 부터 88BYTE 의 마지막 수신 벽시각 초 표(최초 BSS 0)이다. 이후 reader 는 각 DWORD 를 signed32 로 해석한다.
    #wallSeconds = new Uint32Array(CONNECTION_ACTIVITY_ENTRIES);
    // 005c8540 의 Zone 설치 stamp 이다. 프레임 표의 23번째 항목이 아니며 005c85e4 의 pendingStamp 원천과도 별개다.
    #activeStamp = new DwordBox(0);
    // 005c8544 의 Zone+10 복사 출력이다. 벽시각 표의 음수 인덱스 항목이 아니다.
    #activeZoneWord = new DwordBox(0);
    // 005c8620 의 독립 BSS 0 하트비트 기한(double)이다. Session 교체/수신 표 정리로 초기화하지 않는다.
    #heartbeatEnd = new DoubleBox(0);
    // 00531908 의 현재 DWORD 이다. 보호 PE 초기값 6 이며 실제 옵션 해석이 1 을 쓴다.
    #autoRestart = new DwordBox(AUTO_RESTART_INITIAL);

    // 두 고정 표를 `RawConnectionActivity` 에 빌려 준다. 간극(Zone 두 DWORD)을 포함하지 않는 서로 다른 두 배열이다. (2026-11-12 WB04-E325)
    tables() { return { frames: this.#frames, wallSeconds: this.#wallSeconds }; }

    // 005c84e8..005c853c 의 마지막 수신 프레임 22DWORD 이다. 같은 배열을 계속 돌려준다. (2026-11-12 WB04-E325)
    frames() { return this.#frames; }

    // 005c8548..005c859c 의 마지막 수신 벽시각 초 22DWORD 이다. frame cleanup 이나 Session 반납으로 암묵 0 이 되지 않는다. (2026-11-12 WB04-E325)
    wallSeconds() { return this.#wallSeconds; }

    // 005c8540 의 Zone 설치 stamp 상자이다. (2026-11-12 WB04-E325)
    activeStamp() { return this.#activeStamp; }

    // 005c8544 의 Zone+10 복사 출력 상자이다. 같은 실제 Zone 소비자가 현재 값을 빌린다. (2026-11-12 WB04-E325)
    activeZoneWord() { return this.#activeZoneWord; }

    // 005c8620 의 하트비트 기한(double) 상자이다. (2026-11-12 WB04-E325: 기준선 뒤 37dcdfc2 반영)
    heartbeatEnd() { return this.#heartbeatEnd; }

    // 00531908 의 자동 재시작 DWORD 상자이다(초기 6). (2026-11-12 WB04-E325: 기준선 뒤 37dcdfc2 반영)
    autoRestart() { return this.#autoRestart; }
}
