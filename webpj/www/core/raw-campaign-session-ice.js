// 생성자1654/ConnectIceTower2145..2173의 독립 typed 성장형/완성형 아이스 타워 결선이다(225553df, WB04-E605).
// 사용: connectDamageable 뒤 bind. 같은 현재 TYPE/시계/Host·공통 후처리/삭제를 빌리며 World 게시는 뒤 구간이다.
import { RawIceTower } from './raw-ice-tower.js';
import { RegularProcess } from './squid-process.js';

// Source의 float 인자→signed64 절삭→하위 signedDWORD를 보존한다. 비유한/범위 밖은0이다.
function cell(value) {
    value = Math.fround(value); if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}
// 호출 위치의 실제 후설치 부모를 요구한다.
function need(value, message) { if (!value) throw new Error(message); return value; }

// 현재 역할 TYPE와 실제 미부착 Regular 예약을 소유하는 Session 구간이다.
export class RawCampaignSessionIce {
    // 한 번 구성/부분 실패·반납 이후 접근 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 독립 typed의 Source 초기 TYPE와 값형 훅이다. Native 장기 역할은 실제 Client 단계에 남는다.
    types = { ice: 147, growing: 150, noIsland: 157 }; hooks = null;

    // 구성은 아이스 자산/예약/난수를 생성하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 같은 현재 World/Host/전후처리와 원문 시간/WORD/소유자 경계를 연결한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.damageableModules?.ready || this.#started || o.iceTower) throw new Error('Session IceTower의 초기화 전/반복 결선');
        this.#started = true;
        const hooks = {
            reserveRegular: () => { this.#live(); o.pendingIceRegular = new RegularProcess(0, 0); return true; }, // 성장 후처리가 실제 미부착 예약 한 개를 확보한다.
            regular: (sid, event, period) => { // 같은 예약을 소비하고 부착 효과 뒤 현재 시각으로 task를 재설정한다.
                this.#live(); const reserved = need(o.pendingIceRegular, '성장형 아이스 Regular 예약 누락'); o.pendingIceRegular = null; reserved.initializeDetached(event, period);
                const task = c.processes.attachRegular(reserved, sid); if (!task) throw new Error('성장형 아이스 Regular 부착 실패'); task.resetTask(event, c.processState.now);
            },
            basePop: (sid, flags) => { this.#live(); c.postPop.postPopBase(sid, flags); }, // 최초 성장형 등록 뒤 같은 공통 장부 후처리를 실행한다.
            grid: (x, y) => { this.#live(); const cx = cell(x); const cy = cell(y); return cx < 0 || cy < 0 || cx > 255 || cy > 255 ? 0 : c.hash.cell(0, cx, cy); }, // 원문 절삭/보드 관문 뒤 현재 표면 SID를 조회한다.
            genusAt: (x, y, genus) => { this.#live(); return o.damageablePost.genusAt(x, y, genus); }, // 성장형 삭제의 받침 후보를 같은 일반 Finder에서 찾는다.
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); }, // 재성장/완성 자산을 같은 가상 생성기로 만든다.
            unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); }, // 재성장 생성 전에 원래 자산의 공간을 가상 해제한다.
            word: (sid, value) => { this.#live(); const raw = c.pool.allocated(sid); raw[12] = value; raw[13] = value >>> 8; if (raw[10] === c.bridgeState.bridgeType) c.building.notifySurface(sid); }, // 이전 WORD를 복사하고 현재 다리면 표면을 통지한다.
            owner: (sid, owner) => { this.#live(); o.ownership.setOwner(sid, owner & 0xff); }, // 원래 소유자 BYTE를 현재 가상 소유권 분배에 전달한다.
            pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); }, // 새 성장형/완성형을 현재 좌표에 가상 배치한다.
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); }, // 성장 완료 뒤 이전 성장형의 전체 수명을 반납한다.
            dropIsland: (x, y, flags, owner) => { this.#live(); o.assetModules.prepareBuilding(c.processState.now); need(o.buildingEffects, '아이스 타워 실제 받침 정리 미설치').dropIsland(x, y, flags, owner); }, // 현재 시각을 준비하고 실제 받침을 정리한다.
            damageablePost: (sid, flags) => { this.#live(); o.damageableModules.preparePost(); o.damageablePost.postDestroy(sid, flags); }, // 현재 전투/population을 준비한 뒤 피해 후처리를 실행한다.
            basePost: (sid, flags) => { this.#live(); o.world.commonDeletion.postDestroy(sid, flags); }, // 성장형의 별도 공통 삭제 후처리를 실행한다.
            setFrame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame, flags); }, // 현재 성장 프레임과 표시 갱신을 같은 몸체에 전달한다.
        };
        this.hooks = hooks;
        o.iceTower = new RawIceTower(c.pool, c.frames, this.types, () => o.state.assetState.readServer(), () => o.state.assetState.readBoss(), () => o.objectState.loadingDepth, hooks);
        this.#ready = true;
    }

    // 반납한 구간의 closure는 현재 World에 다시 들어갈 수 없다.
    #live() { if (this.#disposed) throw new Error('반납한 Session IceTower'); }
    // 상위가 실제 Host를 비운 뒤 미부착 예약만 반납하며 TYPE/장기 상태를 재설정하지 않는다.
    dispose() { this.#disposed = true; this.#owner.pendingIceRegular = null; }
    // ConnectIceTower의 정상 복귀이며 실제 첫 Pop/World 게시의 완료는 아니다.
    get ready() { return this.#ready; }
}
