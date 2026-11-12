// Client 의 180초 시간 알림이 읽고 쓰는 장기 현재값 `MonitorTimeState` 만 옮긴 모듈이다(cpppj `client/RawMonitorTime.h` 의 구조체, 10.78 전용).
// 원문 00544d68/60 과 0054db9c/dba0 의 최초 BSS 0 이며 Session 교체로 초기화하지 않는다.
//  - initialized(DWORD): 첫 호출에서 bit0 만 켜며 다른 비트는 보존한다. deadline(double): 최초 guard 에서 0 을 쓰고 비활성 때 비영 기한을 지우지 않는다.
//  - monitor/alert(int32): 두 ReadInt 현재 DWORD 이며 비영 관문을 bool 로 정규화하지 않는다(쓰기는 signed 32비트로 감는다).
// 아직 옮기지 않은 것: `StepMonitorTime`(180초마다 달력 문자열을 채팅에 제출)·`FormatMonitorTimeLine`·`MonitorCalendarText`·`SubmitTemporaryChatFormat` 는 채팅 관리자(ChatManager, 5-A·6단계 UI)와 함께 옮긴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E335: 4-D ClientLocalStateGlobals 의 하위 소유자로 상태 구조체만 추가)

// 시간 알림의 장기 현재값이다. 사용: const state = new MonitorTimeState(); state.monitor = 1; 필드는 모두 원문 폭으로 감겨 저장된다. (2026-11-12 WB04-E335)
export class MonitorTimeState {
    // 첫 bit0 만 켜는 초기화 표식(DWORD)이다.
    #initialized = 0;
    // 다음 알림 벽시각 기한(double)이다. 0 은 예약 없음이다.
    deadline = 0;
    // monitor/alert 두 옵션의 signed DWORD 이다.
    #monitor = 0;
    #alert = 0;

    get initialized() { return this.#initialized; }

    set initialized(next) { this.#initialized = next >>> 0; }

    get monitor() { return this.#monitor; }

    set monitor(next) { this.#monitor = next | 0; }

    get alert() { return this.#alert; }

    set alert(next) { this.#alert = next | 0; }
}
