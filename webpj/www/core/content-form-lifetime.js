// 원본 10.78 ContentForm(TYPE 6) 의 가상 삭제 수명(preDestroy 004aeeb0·postDestroy·빈 부모 release 통지·가상 +0x10 삭제)을 옮긴 모듈이다.
//  - preDestroy: 현재 커서 DWORD(0055a2a8)가 이 Form 이거나 부모 WORD 가 자기 번호면 커서 취소 → 내용물 kind(+18 WORD)가 현재 너겟 TYPE(005412c0)이면
//    부모(+16)가 dead 가 아닐 때 로컬 소유자면 수입 알림(00452fe0), 다른 소유자면 boss 일 때 원격 수입 패킷(004e2dc0) → 선지급 억제 BYTE(+22)가 0 이면
//    AI 지갑 정수 선가산(00413bb0, 절삭 cost) → 현재 소유자/비용/로컬 재조회 뒤 SP 가산 → 부모 중심 위치 소리("guy.wav") → 공통 pre 깊이 반납.
//  - 각 효과(커서 취소·알림·지갑·SP) 뒤에는 현재 슬롯·부모 owner BYTE·현재 너겟 번호/비용·localOwner·boss 를 소비 위치에서 다시 읽는다.
//  - postDestroy 는 공통 post 깊이 반납, 부모 release 통지는 빈 실제 가상 함수, 종속 삭제는 같은 공통 삭제기로 재진입한다.
//  - 클래스 판정은 사용 중/dead 슬롯은 현재 VT(Content 생성자 VT), 반납된 슬롯은 기존 TYPE 기록(6) 조회 계약이다. 다른 Form/자산은 fallback 훅으로 넘긴다.
// 근거: cpppj `o/ContentFormLifetime.cpp/.h`(기준선 1f00a25a). 원본 관측: `cpppj/tests/fixtures/contentlifetime-x86.tsv`(10.78 198행)·
//       `content-cursor-width-x86.tsv`(10.78 행). 검사는 tests/content-form-lifetime.test.mjs 에 있다. CD 10.72(너겟 효과 없음)는 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-07 WB04-D5b: Form 분배기와 함께 Content 삭제 수명 이식으로 추가)
import { ContentFormHost } from './content-form-host.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { SquidReward } from './squid-reward.js';
import { SQUID_FORM_VTABLES } from './squid-vtables.js';

// 슬롯의 TYPE·상태·부모(+16 WORD)·kind(+18 WORD)·선지급 억제(+22 BYTE)·부모 owner(+34 BYTE)·부모 좌표(+14/+18 float) 위치이다. (2026-11-07 WB04-D5b)
const TYPE = 10, STATE = 11, PARENT = 16, KIND = 18, PAID = 22, OWNER = 34, X = 14, Y = 18;
// 내장 Content TYPE 번호와 free·dead 비트이다. (2026-11-07 WB04-D5b)
const CONTENT_TYPE = 6, FREE = 1, DEAD = 2;
// CRT 의 유효한 signed64 변환 범위이다(지갑 절삭). (2026-11-07 WB04-D5b)
const INTEGER_LIMIT = 9.2e18;

// 선택된 장치를 꺼낸다(호출 순간의 장치를 끝까지 쓴다). 없으면 그 위치에서 오류이다. (2026-11-07 WB04-D5b)
function need(device, message) {
    if (typeof device !== 'function') throw new Error(message);
    return device;
}

// 슬롯 WORD·VT·float 읽기이다. (2026-11-07 WB04-D5b)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));
function floatAt(raw, offset) {
    FLOAT_VIEW.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return FLOAT_VIEW.getFloat32(0, true);
}

// 너겟의 선지급 AI 금액이다(SP float 금액과 구별). 0방향 절삭의 하위 DWORD(signed)이다. (2026-11-07 WB04-D5b)
function walletAmount(cost) {
    if (!Number.isFinite(cost) || Math.abs(cost) >= INTEGER_LIMIT) throw new RangeError('Content 너겟 지갑 금액 범위 오류');
    return cost | 0;
}

// 삭제 중 효과가 갱신할 수 있는 원본 전역이다(cpppj ContentLifetimeState). 매 분기/요청에서 최신 값을 읽는다.
//  - cursor: 0055a2a8 의 전체 DWORD(높은 WORD 도 비교), nuggetType: 005412c0, localOwner: 00540c70, notice: 005429c4 수입 알림 명령, boss: 00540bc4.
//  - currentCursor/currentBoss/currentNuggetType: 같은 Client/GameAuxState 의 현재값을 돌려주는 함수(있으면 필드 대신 매번 호출).
// 사용: const state = new ContentLifetimeState({ currentNuggetType: () => aux.nuggetType }). (2026-11-07 WB04-D5b)
export class ContentLifetimeState {
    // 독립 값형 입력 필드이다.
    cursor = 0;
    nuggetType = 154;
    localOwner = 1;
    notice = 0;
    boss = true;
    // 실제 소유자의 현재값 함수이다(없으면 위 필드).
    currentCursor = null;
    currentBoss = null;
    currentNuggetType = null;

    // 초기값을 덮어쓴다(알 수 없는 이름은 거부).
    constructor(values = {}) {
        // 지정한 필드만 덮어쓴다.
        for (const [name, value] of Object.entries(values ?? {})) {
            if (!Object.hasOwn(this, name)) throw new RangeError(`Content 수명 상태에 없는 칸: ${name}`);
            this[name] = value;
        }
    }

    // 현재 커서 DWORD 이다.
    readCursor() { return (typeof this.currentCursor === 'function' ? this.currentCursor() : this.cursor) >>> 0; }
    // 현재 boss 권한이다.
    readBoss() { return Boolean(typeof this.currentBoss === 'function' ? this.currentBoss() : this.boss); }
    // 현재 너겟 TYPE DWORD 이다.
    readNuggetType() { return (typeof this.currentNuggetType === 'function' ? this.currentNuggetType() : this.nuggetType) >>> 0; }
}

// ContentForm 의 가상 삭제 수명이다(cpppj ContentFormLifetime, 10.78). types 는 188칸 TYPE 레코드 배열이며 콜백이 바꾼 현재 비용/발자국을 읽도록 복사하지 않는다.
// hooks: { cancelCursor(), localNotice(command), remoteNotice(owner), wallet(owner, amount), addSp(owner, cost, local), sound(x, y) }.
// reward 가 있으면 지갑/SP 는 그 실제 저장소(creditWallet/addSp)로 가고 wallet/addSp 훅은 쓰지 않는다. fallback 은 다른 Form/자산 삭제 훅({ emit, selected, unpopForm })이다.
// 사용: const lifetime = new ContentFormLifetime(pool, types, content, destroy, state, hooks, dispatch.hooks(), reward); lifetime.destroy(form, 0x10). (2026-11-07 WB04-D5b)
export class ContentFormLifetime {
    // 같은 풀·빌린 TYPE 표·Content 호스트·삭제기·전역 상태·훅·fallback·보상이다.
    #pool;
    #types;
    #forms;
    #destroy;
    #state;
    #hooks;
    #fallback;
    #reward;

    // 다른 풀의 호스트/삭제기/보상 연결과 판본 크기 오류를 생성 때 거부한다.
    constructor(pool, types, forms, destroy, state, hooks = {}, fallback = {}, reward = null) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Content 수명은 10.78 SidPool 만 받습니다.');
        if (!(forms instanceof ContentFormHost) || !(destroy instanceof RawSquidDestroy) || (reward !== null && !(reward instanceof SquidReward))) {
            throw new TypeError('Content 수명 장치 형식 오류');
        }
        if (forms.pool !== pool || destroy.pool !== pool || (reward && reward.pool !== pool)) throw new TypeError('Content 수명 SID 풀 불일치');
        if (!Array.isArray(types) || types.length !== 188) throw new RangeError('Content 수명 타입 판본/크기 오류');
        if (!(state instanceof ContentLifetimeState)) throw new TypeError('Content 수명 상태는 ContentLifetimeState 여야 합니다.');
        this.#pool = pool;
        this.#types = types;
        this.#forms = forms;
        this.#destroy = destroy;
        this.#state = state;
        this.#hooks = hooks ?? {};
        this.#fallback = fallback ?? {};
        this.#reward = reward;
    }

    // 같은 실제 풀이다(분배기 연결 확인용).
    get pool() { return this.#pool; }

    // 사용 중/dead 는 현재 Content 생성자 VT, 반납된 슬롯은 기존 TYPE 기록(6)으로 판정한다. 예약/범위 밖은 false 다.
    isContentForm(form) {
        const pool = this.#pool;
        if (!Number.isInteger(form) || form < 5 || form >= pool.capacity || form === pool.layout.predictableFirst) return false;
        const raw = pool.slot(form);
        if (raw[STATE] & FREE) return raw[TYPE] === CONTENT_TYPE;
        return vtableOf(raw) === SQUID_FORM_VTABLES.content;
    }

    // 부모의 현재 owner BYTE 이다.
    #owner(parent) { return this.#pool.slot(parent)[OWNER]; }

    // 현재 너겟 번호로 현재 TYPE float 비용을 다시 읽는다.
    #cost() {
        const number = this.#state.readNuggetType();
        if (number >= this.#types.length) throw new RangeError('Content 너겟 타입 번호 오류');
        const cost = Math.fround(this.#types[number].cost ?? 0);
        if (!Number.isFinite(cost)) throw new RangeError('Content 너겟 비용 유한값 필요');
        return cost;
    }

    // 004adea0: 부모 현재 좌표에서 TYPE 발자국(footX/footY) 절반을 빼고 각 결과를 float 로 저장한다(footZ 미포함).
    #center(parent) {
        const raw = this.#pool.slot(parent);
        if (raw[TYPE] >= this.#types.length) throw new RangeError('Content 중심 부모 타입 오류');
        const type = this.#types[raw[TYPE]];
        return [Math.fround(floatAt(raw, X) - (type.footX ?? 0) * 0.5), Math.fround(floatAt(raw, Y) - (type.footY ?? 0) * 0.5)];
    }

    // 004aeeb0: 커서 취소 검사 → 너겟 효과 → 공통 pre 깊이 반납이다. 공통 삭제기가 이미 pre 깊이를 올린 상태에서 부른다.
    preDestroy(form) {
        const pool = this.#pool;
        if (!this.isContentForm(form) || (pool.slot(form)[STATE] & FREE)) throw new Error('Content preDestroy 슬롯 오류');
        const hooks = this.#hooks;
        const state = this.#state;
        // 커서 DWORD 전체와 비교한 뒤에만 부모 WORD 를 읽는다.
        if (state.readCursor() === form || word(pool.slot(form), PARENT) === form) need(hooks.cancelCursor, 'Content 커서 취소 경계 필요')();
        // 커서 취소 뒤 현재 kind 와 현재 너겟 번호를 비교한다.
        if (word(pool.slot(form), KIND) === state.readNuggetType()) {
            const parent = word(pool.slot(form), PARENT);
            if (!parent || parent >= pool.capacity) throw new Error('Content 너겟 부모 번호 오류');
            if (!(pool.slot(parent)[STATE] & DEAD)) {
                if (this.#owner(parent) === state.localOwner) need(hooks.localNotice, 'Content 로컬 수입 알림 경계 필요')(state.notice);
                else if (state.readBoss()) need(hooks.remoteNotice, 'Content 원격 수입 알림 경계 필요')(this.#owner(parent));
                if (!pool.slot(form)[PAID]) {
                    const recipient = this.#owner(parent);
                    const amount = walletAmount(this.#cost());
                    if (this.#reward) this.#reward.creditWallet(recipient, amount);
                    else need(hooks.wallet, 'Content 지갑 선가산 경계 필요')(recipient, amount);
                    // 지갑 뒤 현재 owner/cost/localOwner 를 다시 읽어 SP 가산 경로를 고른다.
                    const owner = this.#owner(parent);
                    const cost = this.#cost();
                    const local = owner === state.localOwner;
                    if (this.#reward) this.#reward.addSp(owner, cost);
                    else need(hooks.addSp, 'Content SP 가산 경계 필요')(owner, cost, local);
                    const sound = need(hooks.sound, 'Content 너겟 위치 소리 경계 필요');
                    const [x, y] = this.#center(parent);
                    sound(x, y);
                }
            }
        }
        this.#destroy.completePreDestroy();
    }

    // 가상 +0x10: 같은 공통 삭제기(현재 dead 를 읽음)로 공통 삭제 → 종속 재진입 → 실제 Unpop → 반납을 한다.
    destroy(form, flags = 0) {
        if (!this.isContentForm(form)) throw new Error('Content 삭제 타입 오류');
        this.#destroy.destroy(form, flags, this.hooks());
    }

    // Content 만 처리하고 다른 Form/자산은 fallback 으로 넘기는 삭제 훅이다(전체 월드 분배기와 내부 삭제가 같은 훅을 쓴다).
    hooks() {
        const fallback = this.#fallback;
        return {
            selected: typeof fallback.selected === 'function' ? fallback.selected : () => 0,
            // Content 는 앞서 검증한 호스트의 실제 Unpop(부모 제거 통지 2), 그 밖은 fallback 이다.
            unpopForm: (form) => {
                if (this.isContentForm(form)) {
                    this.#forms.unpop(form);
                    return;
                }
                need(fallback.unpopForm, 'Content 외 Unpop 경계 필요')(form);
            },
            // pre/post 깊이 반납·빈 release·종속 재진입이다.
            emit: (event) => {
                if (this.isContentForm(event.sid)) {
                    switch (event.effect) {
                    case SQUID_DESTROY_EFFECT.preDestroy:
                        this.preDestroy(event.sid);
                        return;
                    case SQUID_DESTROY_EFFECT.postDestroy:
                        this.#destroy.completePostDestroy();
                        return;
                    case SQUID_DESTROY_EFFECT.releaseDependent:
                        return;
                    case SQUID_DESTROY_EFFECT.destroyDependent:
                        this.destroy(event.sid, event.flags);
                        return;
                    default:
                        break;
                    }
                }
                need(fallback.emit, 'Content 외 삭제 효과 경계 필요')(event);
            },
        };
    }
}
