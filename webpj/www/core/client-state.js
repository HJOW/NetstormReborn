// 원본 State.cpp(004b88c0)의 다음 루프 상태 전환 자리를 옮긴 모듈이다(cpppj `client/State.cpp/.h`, 10.78 전용). 현재는 단일 플레이 UI 전환만 연결한다.
// 사용:
//   const state = new ClientState(); state.post({ command: 'Tell', argument: 'ABORT' }); state.pending(); const action = state.take(); state.phase = ClientPhase.Dialog;
//   state.takeAfterPoll(poll) 은 Poll 진입 전의 같은 대기 요청만 꺼낸다. 새 요청/취소 뒤 교체 요청은 다음 Tick 까지 보존한다.
//   state.cancel('LoadMissionFort') 는 해당 소유자의 정확한 내부 명령(대기 큐의 맨 앞)만 취소하며 다른 대기 요청은 보존한다.
// 입력 큐와 상태 전환 큐를 분리해 버튼 처리가 끝난 뒤 장면/미션 자원을 교체한다. 입력 중에는 현재 화면을 삭제하지 않고 다음 루프에 명령을 보내며, 이미 대기 중인 전환이 있으면 한 번의 입력 묶음이
// 여러 화면을 건너뛰지 않도록 새 요청을 무시한다. 명령의 문자열 자원(DialogAction: `{ command, argument }`)도 다음 루프까지 소유한다(웹은 평범한 객체).
// pendingSequence 는 호스트 큐 항목의 수명만 구별하며 Native State 번호·phase·완료 표식과 별개이다(새 항목을 받을 때만 증가, 64비트 감김에서도 0 은 빈 입력으로 남긴다).
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E525: 4-D 구획 State 이식)

// 현재 UI/미션 단계 목록이다. 실제 미션 프로세스는 다음 월드 단계에서 연결한다. (2026-11-15 WB04-E525)
export const ClientPhase = Object.freeze({
    MainMenu: 0,
    Dialog: 1,
    LoadingMission: 2,
    Briefing: 3,
    Mission: 4,
});

// 메뉴·미션 전환에 사용하는 상태이다. (2026-11-15 WB04-E525)
export class ClientState {
    // 현재 UI/미션 단계이다.
    phase = ClientPhase.MainMenu;
    // 후속 처리 전까지 보류하는 요청이고, 호스트 큐 항목의 수명만 구별하는 64비트 일련번호이다.
    #pending = [];
    #pendingSequence = 0n;

    // 이미 대기 중인 전환이 있으면 한 번의 입력 묶음이 여러 화면을 건너뛰지 않는다. 빈 큐의 새 항목만 구별하며 unsigned 감김에서도 0 은 빈 입력으로 남긴다. (2026-11-15 WB04-E525)
    post(action) {
        if (this.#pending.length !== 0) return;
        this.#pending.push(action);
        this.#pendingSequence = BigInt.asUintN(64, this.#pendingSequence + 1n);
        if (this.#pendingSequence === 0n) this.#pendingSequence = 1n;
    }

    // 루프 시작 시 하나 꺼낸다. 빈 큐는 아무 전환도 만들지 않으며 null 이다. (2026-11-15 WB04-E525)
    take() {
        return this.#pending.length === 0 ? null : this.#pending.shift();
    }

    // 실제 전환 큐의 같은 저장소를 읽으며 조회만으로 전환을 꺼내지 않는다. 원본 Guide 의 pending 관문에 현재 다음 루프 전환 유무를 공급한다. (2026-11-15 WB04-E525)
    pending() {
        return this.#pending.length !== 0;
    }

    // Native Poll 이 예약한 다음 상태를 같은 Tick 에서 두 번째로 실행하지 않고 취소/부분 실패도 보존한다. Poll 진입 전의 같은 항목(일련번호 동일)일 때만 꺼낸다. (2026-11-15 WB04-E525)
    takeAfterPoll(poll) {
        if (typeof poll !== 'function') throw new TypeError('호스트 Tick의 실제 Poll 장치 누락');
        const previous = this.#pending.length === 0 ? 0n : this.#pendingSequence;
        poll();
        if (previous === 0n || this.#pending.length === 0 || previous !== this.#pendingSequence) return null;
        return this.take();
    }

    // 미션이 교체되기 전에 그 미션의 지연 요새 요청을 반납해 새 미션에 오래된 지도를 설치하지 않는다. 대기 큐 맨 앞의 정확한 명령만 취소하며 다른 요청은 보존한다. (2026-11-15 WB04-E525)
    cancel(command) {
        if (this.#pending.length === 0 || this.#pending[0].command !== command) return false;
        this.#pending.shift();
        return true;
    }
}
