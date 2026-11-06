// 원본 10.78 GumpForm(TYPE 3, UI 객체 부착용 Form)의 생성·수신·소유 ID·부착·해제·부모 삭제 통지·가상 삭제 호스트를 옮긴 모듈이다.
//  - 생성/Take/부착/해제는 같은 SquidFactory 의 현재 TYPE 3 생성자·공통 Form 부착·Form Unpop 으로 간다.
//  - 소유 ID 는 +18 의 비정렬 DWORD 전체(Gump 등록 ID)이며 WORD 로 줄이거나 호스트 포인터로 해석하지 않는다.
//  - 부모 삭제의 release 는 소유 ID 가 0 이 아닐 때만 UI 통지(owner, 현재 부모)를 부른다. Form 자신의 삭제에는 UI 통지가 없다.
//  - 클래스 판정은 사용 중/dead 슬롯은 현재 VT(Gump 생성자 VT), 반납된 슬롯은 기존 TYPE 기록 조회 계약(TYPE BYTE 3)이다.
// 근거: cpppj `o/GumpFormHost.cpp/.h`. 원본 관측 fixture 는 없고 cpppj 계약 검사(FormTake_GumpAttachmentAndDestroyIntegration 등)를 옮긴다.
// CD 10.72 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-06 WB04-D5a: Form 호스트 이식으로 추가)
import { SQUID_FORM_VTABLES } from './squid-vtables.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';

// 내장 GumpForm 타입 번호이다(Process 2·Content 6 과 별개). (2026-11-06 WB04-D5a)
export const GUMP_FORM_TYPE = 3;
// TYPE·상태·부모(+16)·소유 ID(+18 DWORD) 위치와 free 비트이다. (2026-11-06 WB04-D5a)
const TYPE = 10, STATE = 11, PARENT = 16, OWNER = 18, FREE = 1;

// GumpForm 호스트이다. options: { notice(owner, parent) — 소유 UI 알림, fallback: 다른 Form/자산의 삭제 훅({emit, selected, unpopForm}) }.
// 사용: const gumps = new GumpFormHost(pool, factory, types, destroy, { notice }); const form = gumps.create(2); gumps.setOwner(form, id); gumps.attach(form, parent). (2026-11-06 WB04-D5a)
export class GumpFormHost {
    // 같은 풀·공장·삭제기와 통지·fallback 훅이다.
    #pool;
    #factory;
    #destroy;
    #notice;
    #fallback;

    // 생성/삭제와 같은 풀, 판본 크기(188)의 TYPE 표만 받는다(표는 크기 확인용이며 현재 TYPE 은 공장이 읽는다).
    constructor(pool, factory, types, destroy, { notice = null, fallback = {} } = {}) {
        if (pool?.edition !== 'patch1078' || factory?.pool !== pool || destroy?.pool !== pool) throw new TypeError('GumpForm 생성/삭제 SID 풀이 다릅니다.');
        if (!Array.isArray(types) || types.length !== 188) throw new RangeError('GumpForm 타입 표의 판본/크기 오류');
        this.#pool = pool;
        this.#factory = factory;
        this.#destroy = destroy;
        this.#notice = notice;
        this.#fallback = fallback ?? {};
    }

    // 같은 풀인지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 메뉴의 Form(type, flags)을 같은 공장의 현재 TYPE 3 생성자로 만든다(부모/소유 ID 는 쓰지 않음).
    create(flags = 2) { return this.#factory.createGumpForm(flags); }

    // 서버/예측 번호를 수신한다. 사용 중이면 Form Unpop 뒤 소유 ID/payload 를 유지한다.
    take(form) { return this.#factory.take(GUMP_FORM_TYPE, form); }

    // 사용 중/dead 는 현재 Gump VT, 반납된 슬롯은 TYPE 기록(3)으로 판정한다.
    isGumpForm(form) {
        const pool = this.#pool;
        if (!Number.isInteger(form) || form < 5 || form >= pool.capacity || form === pool.layout.predictableFirst) return false;
        const raw = pool.slot(form);
        if (raw[STATE] & FREE) return raw[TYPE] === GUMP_FORM_TYPE;
        return ((raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0) === SQUID_FORM_VTABLES.gump;
    }

    // 삭제 중 dead 는 허용하고 반납된 슬롯의 필드 접근은 거부한다.
    #form(form) {
        if (!this.isGumpForm(form) || (this.#pool.slot(form)[STATE] & FREE)) throw new Error('GumpForm 슬롯 상태 오류');
        return this.#pool.slot(form);
    }

    // +18 소유 ID DWORD 전체를 읽는다.
    owner(form) {
        const raw = this.#form(form);
        return (raw[OWNER] | (raw[OWNER + 1] << 8) | (raw[OWNER + 2] << 16) | (raw[OWNER + 3] << 24)) >>> 0;
    }

    // Form +16 부모 WORD 를 읽는다.
    parent(form) {
        const raw = this.#form(form);
        return raw[PARENT] | (raw[PARENT + 1] << 8);
    }

    // 004644e0: 소유 ID DWORD 를 쓴다(부착 전에도 허용, 옆 필드 보존).
    setOwner(form, owner) {
        if (!Number.isInteger(owner) || owner < 0 || owner > 0xffffffff) throw new RangeError('Gump 소유 ID 는 DWORD 여야 합니다.');
        this.#form(form);
        const raw = this.#pool.allocated(form);
        // 네 바이트만 낮은 순서로 쓴다.
        for (let index = 0; index < 4; index += 1) raw[OWNER + index] = (owner >>> (index * 8)) & 0xff;
    }

    // 가상 +74 thunk → 같은 공장의 공통 Form 부착 부모이다.
    attach(form, parent, flags = 0) { this.#factory.attachForm(form, parent, flags); }

    // 004ae140: 공장의 현재 진단/VT/종속 분리로 Form Unpop 을 한다(부모 free/dead 의 링크 보존 순서 유지).
    unpop(form) { this.#factory.unpopForm(form); }

    // 가상 +0x1c/+0x78: 소유 ID 가 0 이 아니면 콜백 전에 현재 소유 ID 와 부모를 읽어 UI 에 알린다(삭제 부모 인자는 쓰지 않음).
    notify(form) {
        const owner = this.owner(form);
        if (!owner) return;
        const parent = this.parent(form);
        if (typeof this.#notice !== 'function') throw new Error('GumpForm 소유 UI 알림 경계 필요');
        this.#notice(owner, parent);
    }

    // 가상 +0x10: 공통 삭제→실제 Unpop→SID 반납이다.
    destroy(form, flags = 0) {
        if (!this.isGumpForm(form)) throw new Error('GumpForm 삭제 타입 오류');
        this.#destroy.destroy(form, flags, this.hooks());
    }

    // GumpForm 의 pre/post 깊이·release 통지·종속 삭제·Unpop 을 처리하고 다른 타입은 fallback 으로 넘기는 삭제 훅이다.
    hooks() {
        const fallback = this.#fallback;
        return {
            selected: typeof fallback.selected === 'function' ? fallback.selected : () => 0,
            // GumpForm 만 여기서 해제한다.
            unpopForm: (sid) => {
                if (this.isGumpForm(sid)) {
                    this.unpop(sid);
                    return;
                }
                if (typeof fallback.unpopForm !== 'function') throw new Error('GumpForm 외 Unpop 경계 필요');
                fallback.unpopForm(sid);
            },
            // release 는 부모 알림이고 종속 destroy 는 같은 가상 분배로 재진입한다.
            emit: (event) => {
                if (this.isGumpForm(event.sid)) {
                    switch (event.effect) {
                    case SQUID_DESTROY_EFFECT.preDestroy:
                        this.#destroy.completePreDestroy();
                        return;
                    case SQUID_DESTROY_EFFECT.postDestroy:
                        this.#destroy.completePostDestroy();
                        return;
                    case SQUID_DESTROY_EFFECT.releaseDependent:
                        this.notify(event.sid);
                        return;
                    case SQUID_DESTROY_EFFECT.destroyDependent:
                        this.destroy(event.sid, event.flags);
                        return;
                    default:
                        break;
                    }
                }
                if (typeof fallback.emit !== 'function') throw new Error('GumpForm 외 삭제 효과 경계 필요');
                fallback.emit(event);
            },
        };
    }
}
