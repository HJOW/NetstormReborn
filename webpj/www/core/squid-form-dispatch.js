// 원본 10.78 의 내장 Form 네 클래스(Depend 1·Process 2·Gump 3·Content 6) 가상 삭제를 같은 SID 풀·공통 삭제 깊이로 분배하는 모듈이다(cpppj SquidFormDispatch).
//  - 루트 삭제: 현재 TYPE BYTE 가 자산(70 이상)이고 자산 가상 삭제(예: 다리 +0x10 재정의)가 연결되면 그것, 아니면 공통 삭제 몸체(004af780)다.
//  - 공유 훅: 전송/선택 해제는 자산 경계, 종속 삭제는 루트 재진입, Form 의 pre/post/release 와 Unpop 은 효과 뒤 **현재 VT** 가 고른 클래스 호스트,
//    자산은 자산 경계로 보낸다. 변경 가능한 TYPE BYTE 를 클래스 대신 쓰지 않으며, 선택된 클래스 호스트가 없으면 다른 경로로 숨기지 않고 오류다.
//  - 각 호스트는 생성 때 fallback 에 이 분배기의 hooks() 를 넘긴 뒤 bind 로 등록한다. 등록 전에는 실제 삭제하지 않는다.
// Process 호스트(SquidProcessHost)는 WB04-D5c 에서 이식하며, 그 전에는 Process VT 가 선택되면 "미등록" 오류다.
// 근거: cpppj `o/SquidFormDispatch.cpp/.h`(기준선 1f00a25a). 검사(cpppj ContentLifetime_SharedMixedFormDispatch·FormClassLifetime_CurrentVtableKeepsGumpAndContentEffects·
//       DependCreateCurrent_UnregisteredClassDoesNotUseDifferentRawTypeRoute)는 tests/content-form-lifetime.test.mjs 에 있다. CD 10.72 VT 는 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-07 WB04-D5b: 혼합 종속 체인의 중첩 삭제 분배로 추가)
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { SQUID_FORM_VTABLES } from './squid-vtables.js';
import { DependFormHost } from './depend-form-host.js';
import { GumpFormHost } from './gump-form-host.js';
import { ContentFormLifetime } from './content-form-lifetime.js';

// 네 내장 Form 클래스의 등록 번호와 첫 자산 TYPE 번호이다. (2026-11-07 WB04-D5b)
const DEPEND = 1, PROCESS = 2, GUMP = 3, CONTENT = 6, FIRST_ASSET_TYPE = 70;
// 슬롯의 TYPE BYTE 위치이다. (2026-11-07 WB04-D5b)
const TYPE = 10;
// 현재 VT → 클래스 등록 번호이다(10.78 생성자 VT). (2026-11-07 WB04-D5b)
const CLASS_BY_VTABLE = new Map([
    [SQUID_FORM_VTABLES.depend, DEPEND], [SQUID_FORM_VTABLES.process, PROCESS], [SQUID_FORM_VTABLES.gump, GUMP], [SQUID_FORM_VTABLES.content, CONTENT],
]);

// 슬롯의 현재 VT DWORD 이다. (2026-11-07 WB04-D5b)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 자산 공통 경계({ emit, selected })가 완전한지 확인한다. (2026-11-07 WB04-D5b)
function checkAssets(assets) {
    if (typeof assets?.emit !== 'function' || typeof assets?.selected !== 'function') throw new TypeError('Form 분배 자산 공통 경계 필요');
}

// Form 분배기이다. assets 는 자산 기본 효과/선택 경계({ emit, selected }), assetDestroy(sid, flags, hooks) → boolean 은 자산 가상 +0x10 재정의(선택)다.
// 풀·공통 삭제기·등록한 호스트들은 이 객체보다 오래 살아야 한다.
// 사용: const dispatch = new SquidFormDispatch(pool, destroy, assets); const gumps = new GumpFormHost(pool, factory, types, destroy, { fallback: dispatch.hooks() });
//       dispatch.bind(gumps); dispatch.destroy(root, 0x10). (2026-11-07 WB04-D5b)
export class SquidFormDispatch {
    // 같은 풀·공통 삭제기·자산 경계·자산 가상 삭제이다.
    #pool;
    #destroy;
    #assets;
    #assetDestroy;
    // 클래스 번호 → 등록한 호스트 훅이다.
    #routes = new Map();

    // 다른 풀의 삭제기와 불완전한 자산 경계를 거부한다.
    constructor(pool, destroy, assets, assetDestroy = null) {
        if (pool?.edition !== 'patch1078' || !(destroy instanceof RawSquidDestroy)) throw new TypeError('Form 분배는 10.78 풀/공통 삭제기만 받습니다.');
        if (destroy.pool !== pool) throw new TypeError('Form 분배 공통 삭제 SID 풀 불일치');
        checkAssets(assets);
        if (assetDestroy !== null && typeof assetDestroy !== 'function') throw new TypeError('Form 분배 자산 가상 삭제는 함수여야 합니다.');
        this.#pool = pool;
        this.#destroy = destroy;
        this.#assets = assets;
        this.#assetDestroy = assetDestroy;
    }

    // 같은 실제 풀이다(월드/클라이언트 공유 확인용).
    get pool() { return this.#pool; }

    // 감시·Gump·Content 호스트를 현재 클래스 번호로 등록한다. 다른 풀·같은 클래스 재등록·불완전 훅·미지원 호스트는 거부한다.
    bind(host) {
        let number;
        if (host instanceof DependFormHost) number = DEPEND;
        else if (host instanceof GumpFormHost) number = GUMP;
        else if (host instanceof ContentFormLifetime) number = CONTENT;
        else throw new TypeError('Form 분배 등록 대상 클래스 오류(Process 호스트는 WB04-D5c 이식 대상)');
        this.#register(host.pool, number, host.hooks());
    }

    // 가상 몸체의 emit/Unpop 두 경계가 함께 있어야 완전한 Form 등록으로 인정한다.
    #register(pool, number, hooks) {
        if (pool !== this.#pool) throw new TypeError('Form 분배 등록 풀 오류');
        if (this.#routes.has(number)) throw new Error('Form 분배 타입 중복 등록');
        if (typeof hooks?.emit !== 'function' || typeof hooks?.unpopForm !== 'function') throw new TypeError('Form 분배 가상 경계 누락');
        this.#routes.set(number, hooks);
    }

    // 비표면 runtime 을 같은 풀에 구성한 뒤 자산 경계와 자산 가상 삭제를 교체한다. 등록된 Form 훅은 그대로 현재 경계를 읽는다.
    setAssets(assets, assetDestroy) {
        checkAssets(assets);
        if (typeof assetDestroy !== 'function') throw new TypeError('Form 분배 현재 자산 경계 필요');
        this.#assets = assets;
        this.#assetDestroy = assetDestroy;
    }

    // 효과 뒤 현재 VT 의 실제 Form 클래스 호스트를 고른다. TYPE 별칭을 다른 파생 효과로 보내지 않는다.
    #route(sid) {
        const number = CLASS_BY_VTABLE.get(vtableOf(this.#pool.slot(sid)));
        if (number === undefined) throw new Error('선택된 현재 Form 가상 처리기 미등록');
        const hooks = this.#routes.get(number);
        if (!hooks) throw new Error('선택된 현재 Form 클래스 처리기 미등록');
        return hooks;
    }

    // 현재 타입별 가상 루트 삭제다. 자산의 별도 재정의는 assetDestroy, 그 밖(Form 포함)은 공통 몸체다.
    destroy(sid, flags = 0) {
        if (this.#pool.slot(sid)[TYPE] >= FIRST_ASSET_TYPE && this.#assetDestroy) return this.#assetDestroy(sid, flags, this.hooks());
        this.#destroy.destroy(sid, flags, this.hooks());
        return true;
    }

    // 공유 분배 훅이다. 호스트 fallback 이 넘긴 공통 효과도 같은 최상위 자산 경계로 바로 보낸다(같은 타입의 순환 분배 방지).
    hooks() {
        return {
            // 호출 시점의 자산 선택 경계를 읽는다.
            selected: () => this.#assets.selected(),
            // 파생 Unpop 도 현재 VT 의 실제 호스트가 처리한다.
            unpopForm: (sid) => this.#route(sid).unpopForm(sid),
            // 전송·선택 해제는 자산, 종속 삭제는 루트 재진입, Form 효과는 현재 클래스, 자산 효과는 자산 경계다.
            emit: (event) => {
                if (event.effect === SQUID_DESTROY_EFFECT.transmit || event.effect === SQUID_DESTROY_EFFECT.clearSelection) {
                    this.#assets.emit(event);
                    return;
                }
                if (event.effect === SQUID_DESTROY_EFFECT.destroyDependent) {
                    this.destroy(event.sid, event.flags);
                    return;
                }
                if (this.#pool.slot(event.sid)[TYPE] < FIRST_ASSET_TYPE) {
                    this.#route(event.sid).emit(event);
                    return;
                }
                this.#assets.emit(event);
            },
        };
    }
}
