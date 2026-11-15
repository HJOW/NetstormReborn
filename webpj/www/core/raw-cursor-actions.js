// 원본 커서의 중첩 잠금(446a10)·해제(446a40)·회전(446a70)을 같은 현재 전역과 기존 몸체에 연결한 모듈이다(cpppj `client/RawCursorActions.cpp/.h`, 10.78 전용).
// 사용:
//   const actions = new RawCursorActions(types, drop, lifecycle, specialType, rangeProcessType, hooks);
//     types = 타입 레코드 188개(`group`), drop = CursorDropState(raw-cursor-drop.js: lockDepth·direction·legalGump·type·cursorGump), lifecycle = CursorLifecycleState(raw-cursor-lifecycle.js: abstractType·previews),
//     specialType/rangeProcessType = 원본 541288(현재 Wind 타입)/54113c(현재 51 프로세스 타입)을 읽는 `{ value }` 상자 또는 함수, hooks = 아래.
//   actions.lock(skipCursor) → 처음 잠글 때만 Activate(0)하고 skipCursor 가 0 이면 커서 1 을 선택한 뒤 깊이를 올린다.   actions.unlock() → DWORD 깊이를 낮추고 0 이 되면 목록과 현재 Gump 커서(legalGump 이면 3, 아니면 2)를 다시 만든다.
//   actions.rotate(step) → 원본 signed 마스크 꼬리(0x80000007)와 `rotatepiece.wav`·잠금/그림/사정거리 재생성 순서를 보존한다.
//   hooks = { activate(force) — 445aa0, selectCursor(cursor) — 4a4410, createPreviews() — 445b30 의 현재 목록 생성/재사용, sound(이름), removeRange(parent, type) — 4b1a90(parent,type,0,1), createRange(parent, 0) → 474900 의 현재 프로세스 토큰 }.
// 현재 커서 상태의 사본이나 대체 계산을 만들지 않고 모든 callback 뒤 실제 저장소를 다시 읽는다. 미연결 경계는 그 위치에서 거부한다(Error '미연결').
// 원본 관측: `cursor-actions-x86.tsv` 289행(Lock·Unlock·Rotate 의 14 DWORD 독립 입력과 재진입 변경 조합: 호출 순서와 전체 전역)을 tests/raw-cursor-actions.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E547: 커서 잠금/해제/회전 RawCursorActions 이식)
import { cursorPreviewSid } from './raw-cursor-lifecycle.js';

// 도달한 필수 경계가 비었으면 계산을 대체하지 않고 해당 위치에서 거부한다. (2026-11-15 WB04-E547)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('실제 커서 잠금/회전 경계 미연결');
    return hook;
}

// 원본 전역을 `{ value }` 상자 또는 함수에서 호출 시점마다 읽는다. (2026-11-15 WB04-E547)
const current = (source) => (typeof source === 'function' ? source() : source.value) >>> 0;

// 세 원본 부모(잠금·해제·회전)만 복원하며 모든 callback 뒤 실제 저장소를 다시 읽는다. (2026-11-15 WB04-E547)
export class RawCursorActions {
    // 다른 타입 개수의 표를 10.78 레이아웃에 섞지 않는다. 현재 188 TYPE/커서/수명/특별 타입/프로세스 타입은 처리기보다 오래 살아야 한다.
    constructor(types, drop, lifecycle, specialType, rangeProcessType, hooks) {
        if (types.length !== 188) throw new TypeError('커서 동작 10.78 TYPE 계약 오류');
        this.types = types;
        this.drop = drop;
        this.lifecycle = lifecycle;
        this.specialType = specialType;
        this.rangeProcessType = rangeProcessType;
        this.hooks = hooks;
    }

    // 446a10: Activate 가 깊이를 변경해도 처음 분기의 select 조건과 마지막 증가를 유지한다.
    lock(skipCursor) {
        const drop = this.drop;
        if (!(drop.lockDepth >>> 0)) {
            need(this.hooks.activate)(0);
            if (!(skipCursor >>> 0)) need(this.hooks.selectCursor)(1);
        }
        drop.lockDepth = ((drop.lockDepth >>> 0) + 1) >>> 0;
    }

    // 446a40: Create 가 바꾼 legalGump 는 커서 번호를 고르는 다음 접근에서 읽는다.
    unlock() {
        const drop = this.drop;
        drop.lockDepth = ((drop.lockDepth >>> 0) - 1) >>> 0;
        if (!(drop.lockDepth >>> 0)) {
            need(this.hooks.createPreviews)();
            need(this.hooks.selectCursor)(drop.legalGump ? 3 : 2);
        }
    }

    // 446a70: signed remainder 를 unsigned modulo 8 로 바꾸지 않고 음수/큰 DWORD 입력도 원문과 대조한다.
    rotate(step) {
        const h = this.hooks;
        const drop = this.drop;
        const lifecycle = this.lifecycle;
        if (!(drop.lockDepth >>> 0)) {
            need(h.activate)(0);
            need(h.selectCursor)(1);
        }
        drop.lockDepth = ((drop.lockDepth >>> 0) + 1) >>> 0;
        let direction = (((drop.direction >>> 0) + 8 + (step >>> 0)) & 0x80000007) >>> 0;
        if ((direction | 0) < 0) direction = ((((direction - 1) >>> 0) | 0xfffffff8) + 1) >>> 0;
        drop.direction = direction;
        need(h.sound)('rotatepiece.wav');
        drop.lockDepth = ((drop.lockDepth >>> 0) - 1) >>> 0;
        if (!(drop.lockDepth >>> 0)) {
            need(h.createPreviews)();
            need(h.selectCursor)(drop.legalGump ? 3 : 2);
        }
        if (lifecycle.abstractType >= this.types.length) throw new RangeError('커서 회전 현재 TYPE 범위');
        if ((this.types[lifecycle.abstractType].group | 0) === 1 || (drop.type >>> 0) === current(this.specialType)) {
            need(h.removeRange)(cursorPreviewSid(lifecycle.previews[0]), current(this.rangeProcessType));
            drop.cursorGump = need(h.createRange)(cursorPreviewSid(lifecycle.previews[0]), 0);
        }
    }
}
