// 원본 Smackw32.dll 의 오디오 시계 없는 SmackWait 가지(SDK 00403000..00403137)를 옮긴 모듈이다(cpppj `client/MovieTimer.cpp/.h`, 10.78 전용). 영상의 원래 프레임 간격을 게임 시계/화면 FPS 와 구별한다.
// state = { fast, ready, due(최초 0xffffffff), interval }(SDK+39c/+408/+40c/+410 의 현재 DWORD, interval 은 10µs 단위), hooks = { milliseconds()(timeGetTime), yield()(SDK 406160 대기 서비스) } 이다.
// waitMovieTimer(state, hooks): fast 나 ready 가 있으면 시계를 읽지 않고 false(기다리지 않음). 아니면 밀리초×100(32비트 감김)을 now 로 보고, 최초(due 가 0xffffffff)이면 due = now + interval 로 두고 yield 한 뒤 true,
//   now 가 due 보다 작으면 yield 하고 true, 아니면 next = due + interval, late = now − next 로 due = next 를 쓰고 late 가 interval 보다 크면 due = now + interval 로 재설정하며 interval 이 0 이 아니면 ready = 1 을 쓰고 false 이다.
//   NextFrame 이 ready 를 0 으로 쓰는 것은 호출자 책임이다. 모든 비교는 unsigned DWORD 이고 선택되지 않은 시계/서비스는 요구하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E33: 영상 SDK 타이머 이식으로 추가)

// 선택된 시계/대기 서비스가 없으면 거부한다. (2026-11-08 WB04-E33)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('영상 SDK 시계의 선택된 장치 미연결');
    return callback;
}

// 대기해야 하면 true, 프레임을 진행할 시각이면 false 이다. (2026-11-08 WB04-E33)
export function waitMovieTimer(state, hooks) {
    if (state.fast || state.ready) return false;
    // SDK 는 밀리초를 10µs 단위로 바꾸며 32비트로 감는다.
    const now = Math.imul(need(hooks.milliseconds)(), 100) >>> 0;
    if (state.due === 0xffffffff) {
        state.due = (now + state.interval) >>> 0;
        need(hooks.yield)();
        return true;
    }
    if (now < state.due) { need(hooks.yield)(); return true; }
    const next = (state.due + state.interval) >>> 0;
    const late = (now - next) >>> 0;
    state.due = next;
    if (late > state.interval) state.due = (now + state.interval) >>> 0;
    if (state.interval) state.ready = 1;
    return false;
}
