// 원본 10.78 메인 루프 한 바퀴(FUN_00438dc0 의 do-while 본문)의 단계 순서를 옮긴 순서 실행기이다.
// 각 단계의 실제 몸체는 호출자가 처리기로 넘긴다. 처리기가 없는 단계는 실행하지 않고 `missing` 으로 보고한다(빈 성공 함수로 채우지 않는다).
// 원본 규칙: 1 시각 고정 → … → 9 입력·명령 처리(0 이 아닌 반환이면 종료 절차, 이 프레임의 나머지를 하지 않음) → 10 갱신 목록(Kernel 포함) → 11 그리기 → 13 플레이어 알림.
// 10 의 갱신 목록은 영상 갱신 정지 깊이가 1 미만일 때만 전부 실행하고, 정지 중이면 최상위 창 프레임(00464170)만 실행한다.
// 입력(9)이 갱신(10)보다 앞이므로 갱신 단계에서 예약한 명령(InputQueue.pushCommand)은 다음 프레임 입력에서 소비되고, 그 프레임에 내린 명령은 같은 프레임 Kernel 에 반영된다.
// 근거: docs/exe/main-loop.md 2절(패치판 호출 순서 표), cpppj `client/ClientMain.cpp` 메인 루프(옮긴 단계와 자리 표시). 웹에 해당하지 않는 OS/네트워크/디버그 단계는 excluded 로 표시한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-C3: 세션 장기 상태의 프레임 순서 이식으로 추가)

// 웹 제외 사유이다. (2026-11-05 WB04-C3)
const EXCLUDED = Object.freeze({
    os: 'OS 스레드 우선순위·커서 재배치·Sleep 은 브라우저에 없다(표시는 requestAnimationFrame 이 정한다)',
    network: '멀티플레이 송수신·시간 동기화·전투 생존 신호는 웹 제외 범위다',
    messages: 'Windows 메시지 펌프 대신 브라우저 사건 처리기가 프레임 사이에 입력 큐에 넣는다',
    cd: '패치판 CD 확인은 항상 1 을 돌려주는 빈 함수다',
    debug: '설정으로 켜는 디버그(상태 로그·격자 검증)이며 웹 제품에서 켜지 않는다',
});

// 메인 루프 단계 표이다: [id, 원본 순서, 패치판 주소(없으면 null), 단계, 웹 제외 사유 키(없으면 null), 설명]. 단계는 'frame'(바퀴 본문) 또는 'update'(10 의 갱신 목록)이다.
// 갱신 목록은 docs/exe/main-loop.md 의 패치판 호출 순서 그대로이다(빈 디버그 훅 004437f0 은 뺐다). (2026-11-05 WB04-C3)
export const MAIN_LOOP_STEPS = Object.freeze([
    ['priority', 0, null, 'frame', 'os', '창 활성 때 스레드 우선순위'],
    ['captureTime', 1, 0x00460e90, 'frame', null, '시각 고정: 게임 시각·벽 시각·프레임 번호(GameClock.capture)'],
    ['stateTick', 2, 0x004b88c0, 'frame', null, '화면 모드 변경 반영과 예약된 상태 전환'],
    ['soundCount', 3, 0x004a8e60, 'frame', null, '재생 중인 효과음 버퍼 수'],
    ['transportPoll', 4, 0x004e2920, 'frame', null, '송수신기 폴링 자리. 웹은 싱글/로컬 미션 패킷 진행(cpppj PollMissionPackets)만 이 자리에 둔다'],
    ['timeSync', 5, 0x004c2070, 'frame', 'network', '벽시계 표본 정리(네트워크 시간 동기화 추정)'],
    ['transportFlush', 6, 0x004e28b0, 'frame', 'network', '0.2초마다 송수신기 내보내기'],
    ['messages', 7, null, 'frame', 'messages', '쌓인 창 메시지 처리'],
    ['guide', 8, 0x00463f70, 'frame', null, '튜토리얼 안내 상태 기계'],
    ['input', 9, 0x004d62b0, 'frame', null, '입력·명령 처리. 참 반환이면 종료 절차'],
    ['cdCheck', 10, 0x00435060, 'update', 'cd', 'CD 확인(패치판은 mov eax,1; ret)'],
    ['configSave', 10, 0x00441de0, 'update', null, '변경 표시가 있으면 options.cfg 저장'],
    ['battleKeepAlive', 10, 0x004b9010, 'update', 'network', '전투 생존 신호·무응답 안내(벽시계)'],
    ['stateMachine', 10, 0x004b67c0, 'update', null, '게임 흐름 상태 기계(로그인·미션 시작 절차)'],
    ['ambientSound', 10, 0x004180f0, 'update', null, '먼 천둥 등 환경음(벽시계 타이머)'],
    ['windVector', 10, 0x004dd2b0, 'update', null, '바람·흔들림 벡터(매 프레임 0.9배와 10% 확률 난수)'],
    ['monitor', 10, 0x004182c0, 'update', 'debug', 'monitor 설정 시 상태 로그'],
    ['bridgeDecay', 10, 0x00422bc0, 'update', null, '다리 붕괴 스캔(10초 주기를 여러 프레임에 나눔)'],
    ['music', 10, 0x00469f60, 'update', null, '배경 음악 전환'],
    ['cursor', 10, 0x00446c50, 'update', null, '커서 모양·애니메이션'],
    ['minimap', 10, 0x00481320, 'update', null, '미니맵 갱신'],
    ['cleanupGumps', 10, 0x004662b0, 'update', null, '죽은 창 정리(여기서 예약한 입력은 다음 프레임 입력에서 소비)'],
    ['idleGumps', 10, 0x00464170, 'update', null, '최상위 창 프레임 처리(영상 갱신 정지 중에도 실행)'],
    ['validateGrid', 10, 0x004ab9a0, 'update', 'debug', 'validateGrid 설정 시 격자 검증'],
    ['aiRelease', 10, 0x00417de0, 'update', null, '조건이 풀린 AI 객체 해제(플레이어 9칸)'],
    ['kernel', 10, 0x00471a30, 'update', null, '모든 프로세스 실행(Kernel.runFrame)'],
    ['geysers', 10, 0x00486bc0, 'update', null, '전장 간헐천 생성 관리(게임 시각 타이머)'],
    ['stateTimer', 10, 0x004b6730, 'update', null, '10초마다 004b54e0(벽시계, 내용 미확인)'],
    ['dais', 10, 0x0044ac30, 'update', null, '플레이어별 제단·신전 목록 검증과 처리'],
    ['battleNotice', 10, 0x004d3090, 'update', null, 'inBattleMode 와 DAT_00594fc0 이 모두 비0 일 때만(패치판 전용, 내용 미확인)'],
    ['draw', 11, 0x00436450, 'frame', null, '프레임 제한과 그리기'],
    ['cursorSleep', 12, null, 'frame', 'os', 'GetCursorPos→SetCursorPos·sleepPerLoop'],
    ['playerNotices', 13, 0x00490da0, 'frame', null, '플레이어별 알림 확인'],
].map(([id, order, address, phase, excluded, note]) => Object.freeze({ id, order, address, phase, excluded: excluded === null ? null : EXCLUDED[excluded], note })));

// 영상 갱신 정지 중에도 실행하는 갱신 단계이다(원본 439b69). (2026-11-05 WB04-C3)
const IDLE_ONLY_STEP = 'idleGumps';

// 순서 실행기이다. handlers 는 단계 id → 함수(인자 없음)이며, input 처리기의 참 반환은 종료 절차이다.
// movieUpdatePauseDepth() 는 원본 DAT_0054dc58(signed)의 현재 값을 돌려주는 함수이다(기본 0).
// 사용: const loop = new MainLoop({ handlers: { captureTime: () => clock.capture(now()), input: () => drain(queue), kernel: () => kernel.runFrame() } }); const result = loop.runFrame(). (2026-11-05 WB04-C3)
export class MainLoop {
    // 단계 처리기 표이다.
    #handlers;
    // 영상 갱신 정지 깊이 공급 함수이다.
    #movieUpdatePauseDepth;

    // 처리기를 받는다. 표에 없는 id·제외 단계의 처리기·함수가 아닌 처리기는 거부한다.
    constructor({ handlers = {}, movieUpdatePauseDepth = () => 0 } = {}) {
        // 넘긴 처리기를 하나씩 확인한다.
        for (const [id, handler] of Object.entries(handlers)) {
            const step = MAIN_LOOP_STEPS.find((entry) => entry.id === id);
            if (!step) throw new RangeError(`메인 루프에 없는 단계입니다: ${id}`);
            if (step.excluded) throw new RangeError(`웹에서 제외한 단계입니다: ${id} (${step.excluded})`);
            if (typeof handler !== 'function') throw new TypeError(`단계 처리기는 함수여야 합니다: ${id}`);
        }
        if (typeof movieUpdatePauseDepth !== 'function') throw new TypeError('영상 갱신 정지 깊이 공급자는 함수여야 합니다.');
        this.#handlers = { ...handlers };
        this.#movieUpdatePauseDepth = movieUpdatePauseDepth;
    }

    // 한 바퀴를 원본 순서로 실행한다. 반환: { ran(실행한 단계 id), missing(처리기가 없어 실행하지 않은 단계 id), quit(입력의 종료 절차), updatesPaused }.
    runFrame() {
        const ran = [];
        const missing = [];
        const updatesPaused = this.#movieUpdatePauseDepth() >= 1;
        // 표 순서대로 단계를 처리한다.
        for (const step of MAIN_LOOP_STEPS) {
            if (step.excluded) continue;
            if (step.phase === 'update' && updatesPaused && step.id !== IDLE_ONLY_STEP) continue;
            const handler = this.#handlers[step.id];
            if (!handler) {
                missing.push(step.id);
                continue;
            }
            const value = handler();
            ran.push(step.id);
            // 입력이 종료 절차를 요청하면 이 프레임의 나머지(갱신·그리기·알림)를 하지 않는다. 원본은 int 반환이며 0 이 아니면 종료이다.
            // 실수로 다른 값(배열 길이 외의 객체 등)을 돌려주는 처리기를 잡기 위해 undefined·boolean·정수만 받는다.
            if (step.id === 'input') {
                if (value !== undefined && typeof value !== 'boolean' && !Number.isInteger(value)) throw new TypeError('입력 처리기는 undefined·boolean·정수만 돌려줘야 합니다.');
                if (value === true || (Number.isInteger(value) && value !== 0)) return { ran, missing, quit: true, updatesPaused };
            }
        }
        return { ran, missing, quit: false, updatesPaused };
    }
}
