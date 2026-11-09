// 복원된 섬·다리·noIsland 삭제 훅을 공통 삭제와 연결하는 호스트 분배기이다(cpppj `o/RawSurfaceLifecycle.cpp/.h`, 10.78 전용). 원본에 새 게임 규칙을 더하지 않는다.
// 프로세스 호스트의 asset 훅으로 공급하면 공통 삭제기의 preDestroy/postDestroy 사건을 슬롯 +0 의 실제 가상 표 기록값으로 분배한다.
//  - 다리 가상 표(0x005034c8): preDestroy → RawBridgeLifecycle.preDestroy(또는 현재 연결 TYPE 가 연결됐으면 preDestroyNative), postDestroy → postDestroy.
//    다리 훅의 효과는 EmitBridge 로 옮긴다: 연결 객체 삭제(destroyLink)는 hooks.destroy 로 재진입, 공통 pre/post 는 common.emit, walker 낙하는 hooks.fallWalker, 파편 요청/소리는 hooks.bridgeEffect.
//  - 섬 받침 가상 표(0x00506d98): preDestroy 만 RawIslandLifecycle.islandPreDestroy 로. noIsland 가상 표(0x00506f08): postDestroy 만 surfacePostDestroy 로. 지연 낙하 예약과 walker 낙하는 hooks 로 잇는다.
//  - 타입 번호만 같고 가상 표가 다르면, 그 밖의 사건(releaseDependent 등)은 모두 공통 훅 common.emit 으로 보낸다. 선택 조회(selected)와 form 해제(unpopForm)는 common 에서 그대로 쓴다.
// 사용: const lifecycle = new RawSurfaceLifecycle(pool, neighbors, connectorType, common, hooks); host 의 asset 훅 = lifecycle.hooks(). 같은 인스턴스의 다리 탐색은 중첩 사용하지 않는다(연결 객체 삭제는 공통 훅만 쓴다).
// common = { emit(event), selected(), unpopForm?(sid) }, hooks = { scheduleFall(sid, x, y), destroy(sid, flags), fallWalker(sid), bridgeEffect(event) } 가 모두 필수이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E248: 3-B RawSurfaceLifecycle 이식으로 추가)
import { BRIDGE_VTABLE } from './raw-bridge-events.js';
import { ISLAND_VTABLE } from './raw-bridge-connect.js';
import { BRIDGE_LIFECYCLE_EFFECT, RawBridgeLifecycle, makeBridgeLifecycleHooks } from './bridge-lifecycle.js';
import { RawIslandLifecycle } from './raw-island-lifecycle.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';

// noIsland(표면) 가상 표 기록값(10.78)이다. 섬 받침/다리 값은 각 모듈의 상수를 쓴다. (2026-11-10 WB04-E248)
export const NO_ISLAND_VTABLE = 0x00506f08;

// 첫 자산 타입 번호이다. 연결 객체 타입은 이 번호 이상이고 TYPE 표 안이어야 한다. (2026-11-10 WB04-E248)
const FIRST_ASSET_TYPE = 70;

// 슬롯 첫 DWORD 의 원본 가상 표 기록값을 읽는다. 원본 주소를 호스트 함수로 실행하지 않는다. (2026-11-10 WB04-E248)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

export class RawSurfaceLifecycle {
    #pool;
    #connectorType;
    // Native 의 현재 연결 TYPE getter 이다(없으면 typed 값 사본 connectorType 을 쓴다). 값을 쓰거나 별도 World 를 만들지 않고 같은 전역만 읽는다.
    #currentConnectorType = null;
    #common;
    #hooks;
    #bridge;
    #island;
    #bridgeHooks;

    // 풀·이웃 탐색기·공통 훅의 대상은 이 객체보다 오래 살아야 한다. 같은 풀/타입으로 내부 삭제 탐색기를 만든다. 훅 누락·연결 TYPE 범위 오류·다른 풀의 탐색기는 구성 때 거부한다. (2026-11-10 WB04-E248)
    constructor(pool, neighbors, connectorType, common, hooks) {
        if (pool?.edition !== 'patch1078' || neighbors?.pool !== pool) throw new TypeError('표면 삭제 훅의 이웃 탐색기 풀이 다릅니다');
        const types = neighbors.types();
        if (!Number.isInteger(connectorType) || connectorType < FIRST_ASSET_TYPE || connectorType >= types.length) throw new RangeError('표면 삭제 연결 객체 타입 범위 오류');
        if (typeof common?.emit !== 'function' || typeof common.selected !== 'function'
            || ['scheduleFall', 'destroy', 'fallWalker', 'bridgeEffect'].some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('표면 삭제 공통/외부 훅 누락');
        }
        this.#pool = pool;
        this.#connectorType = connectorType;
        this.#common = common;
        this.#hooks = hooks;
        this.#bridge = new RawBridgeLifecycle(pool, types);
        // 섬 삭제가 예약하는 실제 지연 낙하·noIsland 칸 위 walker 낙하·파생 접두 뒤 공통 pre/post 를 각각 잇는다.
        this.#island = new RawIslandLifecycle(pool, neighbors, {
            scheduleFall: (sid, x, y) => this.#hooks.scheduleFall(sid, x, y),
            fallWalker: (sid) => this.#hooks.fallWalker(sid),
            basePreDestroy: (sid, flags) => this.#common.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid, parent: 0, flags }),
            basePostDestroy: (sid, flags) => this.#common.emit({ effect: SQUID_DESTROY_EFFECT.postDestroy, sid, parent: 0, flags }),
        });
        this.#bridgeHooks = makeBridgeLifecycleHooks(new RawSquidFinder(pool, neighbors.hash, types), (event) => this.#emitBridge(event));
    }

    // Native 첫 구성 때 같은 장기 연결 TYPE 전역을 getter 로 빌린다. null 은 typed 값 사본으로 되돌린다. (2026-11-10 WB04-E248)
    bindCurrentConnectorType(getter) {
        if (getter !== null && typeof getter !== 'function') throw new TypeError('Native 표면 연결 TYPE getter 형식 오류');
        this.#currentConnectorType = getter;
    }

    // 선택 조회와 form 해제 계약은 공통 훅에서 유지하고 파생 삭제 사건만 분배하는 훅 묶음이다. 프로세스 호스트의 asset 훅으로 공급한다. (2026-11-10 WB04-E248)
    hooks() {
        return { ...this.#common, emit: (event) => this.#emit(event) };
    }

    // 다리 훅이 내는 효과를 옮긴다: 연결 객체는 실제 삭제, Base 는 공통 훅, walker 는 낙하 훅, 파편/소리는 외부 효과이다. (2026-11-10 WB04-E248)
    #emitBridge(event) {
        switch (event.effect) {
        case BRIDGE_LIFECYCLE_EFFECT.destroyLink: this.#hooks.destroy(event.sid, event.flags); break;
        case BRIDGE_LIFECYCLE_EFFECT.basePreDestroy: this.#common.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid: event.sid, parent: 0, flags: event.flags }); break;
        case BRIDGE_LIFECYCLE_EFFECT.basePostDestroy: this.#common.emit({ effect: SQUID_DESTROY_EFFECT.postDestroy, sid: event.sid, parent: 0, flags: event.flags }); break;
        case BRIDGE_LIFECYCLE_EFFECT.fallWalker: this.#hooks.fallWalker(event.sid); break;
        default: this.#hooks.bridgeEffect(event);
        }
    }

    // 타입 번호만 같아도 다른 가상 표이면 공통 훅으로 보낸다. 실제 Factory 가 기록한 파생 표를 기준으로 한다. (2026-11-10 WB04-E248)
    #emit(event) {
        const pre = event.effect === SQUID_DESTROY_EFFECT.preDestroy;
        const post = event.effect === SQUID_DESTROY_EFFECT.postDestroy;
        if (pre || post) {
            const vtable = vtableOf(this.#pool.slot(event.sid));
            if (vtable === BRIDGE_VTABLE) {
                if (pre) {
                    if (this.#currentConnectorType) this.#bridge.preDestroyNative(event.sid, event.flags, this.#currentConnectorType, this.#bridgeHooks);
                    else this.#bridge.preDestroy(event.sid, event.flags, this.#connectorType, this.#bridgeHooks);
                } else this.#bridge.postDestroy(event.sid, event.flags, this.#bridgeHooks);
                return;
            }
            if (pre && vtable === ISLAND_VTABLE) {
                this.#island.islandPreDestroy(event.sid, event.flags);
                return;
            }
            if (post && vtable === NO_ISLAND_VTABLE) {
                this.#island.surfacePostDestroy(event.sid, event.flags);
                return;
            }
        }
        this.#common.emit(event);
    }
}
