// 간헐천의 실제 485580 전처리(preDestroy)와 485540 후처리(postDestroy)를 기존 Damageable 수명에 연결해 옮긴 모듈이다(cpppj `o/RawGeyserLifetime.cpp/.h`, 10.78 전용).
//  - handles(sid): 부모의 가상 표 기록값이 0x50da88(간헐천)인 경우만 허용한다. 타입 번호가 같아도 다른 가상 표로 확장하지 않는다.
//  - preDestroy(sid, flags): 가상 +78 의 현재 기본 프레임을 raw +24(프레임 DWORD)에 직접 쓴 뒤(일반 SetFrame 의 추가 Unpop/두 표시 갱신 없음) 가상 +88 표시를 한 번 부르고, 효과 뒤 현재 extra 의 비트 9 가 꺼져 있으면 Player AI 제거(416720)를 요청하고
//    지면 목록에서 이 SID 의 모든 중복을 제거(비활성 물리 꼬리는 보존)한 뒤 Damageable 전처리(44b4b0)를 같은 flags 로 부른다.
//  - postDestroy(sid, flags): 현재 편집 모드가 켜져 있고 extra 비트 9 가 꺼져 있으면 현재 좌표/flags/소유자로 섬 정리(46e6c0)를 한 번 부른 뒤 Damageable 후처리(44b9e0)를 같은 flags 로 부른다.
// 현재 지면 목록은 PostPop/선택기와 같은 저장소이며 복사본이 아니다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E75: 간헐천 수명 이식으로 추가)

// 간헐천 가상 표 기록값, 슬롯의 프레임/extra/소유자/좌표 위치, extra 의 처리 생략 비트이다. (2026-11-08 WB04-E75)
const GEYSER_TABLE = 0x50da88;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
const OWNER_OFFSET = 34;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const SKIP_EXTRA = 9;

// 실제 선택된 큰 하위를 빈 성공으로 바꾸지 않는다. (2026-11-08 WB04-E75)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('간헐천 수명의 실제 하위 미연결');
    return hook;
}

// 비정렬 raw DWORD 를 리틀엔디언으로 읽는다. (2026-11-08 WB04-E75)
function dwordAt(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E75)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, geysers 는 현재 지면 목록(SquidPostPopList; 5950dc), editor 는 후처리 호출 시 읽는 편집 모드 getter,
//   hooks = { defaultFrame(sid)→프레임, update(sid), registerAi(sid, 0), damageablePre(sid, flags), damageablePost(sid, flags), dropIsland(x, y, flags, 소유자) } 이며 도달한 것만 필요하다. (2026-11-08 WB04-E75)
export class RawGeyserLifetime {
    // 죽은 상태도 읽을 실제 raw 저장소, 원래 현재 목록, 후처리 때 읽는 편집 모드, 선택된 효과이다.
    #pool;
    #geysers;
    #editor;
    #hooks;

    // 서로 다른 CD 수명은 이 PATCH 본문으로 실행하지 않는다. (2026-11-08 WB04-E75)
    constructor(pool, geysers, editor, hooks) {
        if (pool?.edition !== 'patch1078' || typeof editor !== 'function') throw new TypeError('간헐천 수명 판본 오류');
        this.#pool = pool;
        this.#geysers = geysers;
        this.#editor = editor;
        this.#hooks = hooks ?? {};
    }

    // 다른 실제 월드의 하위 혼합을 확인하는 풀이다. (2026-11-08 WB04-E75)
    get pool() {
        return this.#pool;
    }

    // 타입 번호가 같더라도 다른 가상 표면 파생으로 확장하지 않는다. (2026-11-08 WB04-E75)
    handles(sid) {
        return dwordAt(this.#pool.slot(sid), 0) === GEYSER_TABLE;
    }

    // getter 뒤 프레임 직접 저장이며 일반 SetFrame 의 추가 Unpop/두 표시 갱신을 넣지 않는다. (2026-11-08 WB04-E75)
    preDestroy(sid, flags) {
        const hooks = this.#hooks;
        const frame = need(hooks.defaultFrame)(sid) >>> 0;
        const raw = this.#pool.allocated(sid);
        // 현재 기본 프레임 DWORD 만 기록한다.
        for (let index = 0; index < 4; index += 1) raw[FRAME_OFFSET + index] = (frame >>> (index * 8)) & 0xff;
        need(hooks.update)(sid);
        if (!(this.#pool.slot(sid)[EXTRA_OFFSET] & SKIP_EXTRA)) {
            need(hooks.registerAi)(sid, 0);
            // 원본 40ea00 의 앞방향 압축과 남은 물리 꼬리를 그대로 보존한다(AI 제거 효과 뒤의 현재 목록으로 모든 중복 제거).
            this.#geysers.removeAll(sid);
        }
        need(hooks.damageablePre)(sid, flags);
    }

    // 원본의 진입 조건을 한 번 평가하고 섬 정리 뒤에는 동일 flags 로 공통 수명을 호출한다. (2026-11-08 WB04-E75)
    postDestroy(sid, flags) {
        const hooks = this.#hooks;
        const raw = this.#pool.slot(sid);
        if (this.#editor() && !(raw[EXTRA_OFFSET] & SKIP_EXTRA)) need(hooks.dropIsland)(floatOf(dwordAt(raw, X_OFFSET)), floatOf(dwordAt(raw, Y_OFFSET)), flags, raw[OWNER_OFFSET]);
        need(hooks.damageablePost)(sid, flags);
    }
}
