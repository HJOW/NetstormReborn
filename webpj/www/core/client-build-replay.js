// ClientBuildReplay의 장기 전역/현재 context·라벨 두 부모만 옮긴다(225553df, WB04-E601).
// FILE 판독/최신 rec 선택·실제 ProcessForm 설치/키 입력은 Client 연결 단계에 남는다.
import { BuildReplayState, isBuildReplayLabel, notifyBuildReplayLabel } from './raw-build-replay.js';

// Client의 실제 playing DWORD와 현재 context 약한 대여다. 반납이 playing을 임의로0으로 바꾸지 않는다.
export class ClientBuildReplayGlobals {
    // 최초 BSS0과 빈 weak 포인터다. 실제 생성 부모가 publish/playing1을 판독 전에 쓴다.
    #playing = 0; current = null;
    // 같은 DWORD 비트로 읽고 쓰며 Source의 다른 전역과 합치지 않는다.
    get playing() { return this.#playing; } set playing(value) { this.#playing = value >>> 0; }
}

// 실제 프로세스가 강하게 소유할 상태/장치다. 현재 전역은 WeakRef로만 빌린다.
export class ClientBuildReplayContext {
    // 원문 기본 초기 상태와 현재 Client 장치 표다. 생성 자체는 게임/파일 효과가 없다.
    state = new BuildReplayState(); hooks = {};
    // GC 시점과 무관하게 shared_ptr 소멸의 수명 종료를 구별하는 웹 관문이다.
    #disposed = false;
    // 실제 부모가 context의 마지막 소유를 반납할 때 호출한다. playing이나 이벤트를 초기화하지 않는다.
    dispose() { this.#disposed = true; }
    // 현재 weak 참조가 남아도 호스트 소유가 끝났는지 판단한다.
    get disposed() { return this.#disposed; }
}

// 매 호출의 현재 weak 포인터를 잠가 같은 호출 동안 강하게 보유한다. stale을 빈 이벤트로 대체하지 않는다.
function current(globals) {
    if (!globals) throw new Error('현재 건설 재생 전역 미연결');
    const context = globals.current?.deref(); if (!context || context.disposed) throw new Error('현재 건설 재생 객체의 호스트 수명 종료'); return context;
}
// 현재 객체의 실제 상태/장치로495710을 호출하며 이전 context를 값으로 고정하지 않는다.
export function currentBuildReplayIsLabel(globals) { const context = current(globals); return isBuildReplayLabel(context.state, context.hooks); }
// 현재 객체를 먼저 요구한 뒤495730의 callback/현재 이름/SID 처리를 실행한다.
export function notifyCurrentBuildReplay(globals, sid) { const context = current(globals); notifyBuildReplayLabel(context.state, sid, context.hooks); }
