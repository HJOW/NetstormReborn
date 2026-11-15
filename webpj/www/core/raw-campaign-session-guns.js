// Session 생성자1398..1426의 GunHit·투사체/배리어 기하와 기존 Gun 장애물 통지를 연결한다(225553df, WB04-E582).
// 사용: owner.connectGunBodies(). 자산 처리기/비행 및 전체 Load/Runtime 공개는 후속 구간이다.
import { RawGunHit } from './raw-gun-hit.js';
import { RawProjectileGeometry } from './raw-projectile-geometry.js';
import { RawFenceTarget } from './raw-fence-target.js';
import { RawGunBlocker } from './raw-gun-blocker.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { GunProcess } from './raw-gun-process.js';

// 원문00500fc8/00501908의 group1/0 검색 여유다. float32 덧셈 뒤 최대값을 비교한다.
const CANNON_SEARCH_MARGIN = 10;
const ARCHER_SEARCH_MARGIN = 3;
// 현재 raw VT를 같은 little endian DWORD로 읽는다. 호스트 함수 주소로 실행하지 않는다.
function dword(raw) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true); }

// 같은 실제 피해/표적/모드/표를 빌리는 생성 구간이다. 기존 Gun 조회 경계는 생성용 공장이 아니다.
export class RawCampaignSessionGuns {
    // 실제 기반 owner와 한 번 실행/정상 복귀/반납 상태다. 실패 접두를 유지한다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 생성 중 만든 원문 지역 callback 묶음이다. 실제 Blocker는 같은 함수를 값 인수로 보관한다.
    blockerHooks = null;
    // 기반 자료만 빌리며 실제 생성은 bind에서 수행한다.
    constructor(owner) { this.#owner = owner; }
    // 실제 Hit Bindings 뒤 원문 순서로 같은 Gun/투사체/배리어/장애물 몸체를 생성한다.
    bind() {
        const o = this.#owner; const c = o.core; const s = o.state;
        if (this.#disposed || !o.hitModules?.ready || this.#started) throw new Error('Session Gun 몸체의 초기화 전/반복 결선');
        this.#started = true;
        o.gunHit = new RawGunHit(c.pool, c.types, o.hitTypes, o.hits.damageHooks({
            // 미복원 가상 피해는 실제 두 TYPE를 남긴다. generic 본문/다른 파생을 성공으로 바꾸지 않는다.
            damage: (victim, _amount, _owner, attacker) => { throw new Error(`실제 세션 총 피해 미복원: 공격 TYPE${c.pool.slot(attacker)[10]} → 표적 TYPE${c.pool.slot(victim)[10]}`); },
            // 압축 시각 WORD 갱신과 같은 실제 공간 통지다.
            notifySurface: (sid) => { c.building.notifySurface(sid); },
            // Rain 전용 선행 경로를 선택하고 다른 발사자는 기존 generic 피해 본문을 계속한다.
            applyOverride: (source, target, amount) => this.applyFlyerHit(source, target, amount),
        }));
        o.projectileGeometry = new RawProjectileGeometry(c.pool, c.types, o.projectileTypes, 16, 11, o.gunMode);
        o.fenceTarget = new RawFenceTarget(c.pool, c.hash, c.types, o.projectileTypes, o.gunTarget, s.gunTargetState, s.gunState);
        // 원문 전체188TYPE 순서다. std::max의 a<b 비교로 NaN 후보가 이전 최대값을 오염시키지 않는다.
        for (let type = 0; type < c.types.length; type += 1) {
            if (c.types[type].group === 1) { const candidate = Math.fround(o.gunTypes[type].range + CANNON_SEARCH_MARGIN); if (s.gunBlockerState.cannonRange < candidate) s.gunBlockerState.cannonRange = candidate; }
            else if (c.types[type].group === 0) { const candidate = Math.fround(o.gunTypes[type].range + ARCHER_SEARCH_MARGIN); if (s.gunBlockerState.archerRange < candidate) s.gunBlockerState.archerRange = candidate; }
        }
        const h = {
            // 현재 첫 Form→PID를 매번 다시 읽는다. 총이 없다는 반환과 실제 표적0을 구별한다.
            currentTarget: (sid) => {
                const lookup = new RawContainedLookup(c.pool, (form, flags) => { o.world.destroy(form, flags); });
                const form = lookup.find(sid, s.gunState.processType); if (!form) return null;
                const current = c.processes.fromForm(form); if (!current) return null;
                if (!(current instanceof GunProcess)) throw new Error('장애물 통지의 현재 Gun 타입 불일치'); return current.fields.target;
            },
            // 같은 현재 모드/SHP 포구이며 별도 계산 표나 기본 방향을 만들지 않는다.
            mode: (sid) => o.gunMode.get(sid), muzzle: (sid) => o.fenceTarget.muzzle(sid),
            // 기존 실제 Gun의 상태 기계로만 표적을 바꾼다. 없으면 생성하지 않는다.
            setTarget: (sid, target) => {
                const lookup = new RawContainedLookup(c.pool, (form, flags) => { o.world.destroy(form, flags); });
                const form = lookup.find(sid, s.gunState.processType); if (!form) return;
                const gun = c.processes.fromForm(form); if (!(gun instanceof GunProcess)) throw new Error('장애물 통지의 기존 Gun 수명 불일치');
                if (!o.assetBindings) throw new Error('장애물 통지의 실제 Gun 처리기 미설치'); o.assetBindings.gun.setTarget(gun.fields, target);
            },
        }; this.blockerHooks = h;
        o.gunBlocker = new RawGunBlocker(c.pool, c.hash, c.types, o.gunTypes, o.gunTarget, s.gunState, s.gunBlockerState, h);
        this.#ready = true;
    }

    // 원문2374..2380 ApplyFlyerHit다. Rain VT 외에는 false이며 target0/free/dead 선관문을 앞당기지 않는다.
    // 비행 Form/기한/방송의 실제 부모 메서드는 후속 Session 연결에서 제공한다. 미설치는 선택 시 명시 오류다.
    applyFlyerHit(source, target, amount) {
        const o = this.#owner; const c = o.core;
        if (c.pool.edition !== 'patch1078' || dword(c.pool.slot(source)) !== 0x50a388) return false;
        if (o.hits.damageResult(target, amount, source) !== 0) return true;
        if (!(c.pool.slot(target)[11] & 2) || (c.pool.slot(source)[11] & 3) || !c.processState.server) return true;
        if (typeof o.currentFlyerProcess !== 'function') throw new Error('Rain 피격의 실제 CurrentFlyerProcess 설치 전 호출');
        const fields = o.currentFlyerProcess(source, 0x11e); if (!o.flyer) throw new Error('Rain피격의현재Flyer기한본체미연결');
        o.flyer.command(fields, 1);
        if (typeof o.broadcastFlyerCommand !== 'function') throw new Error('Rain 피격의 실제 BroadcastFlyerCommand 설치 전 호출');
        o.broadcastFlyerCommand(source, 1); return true;
    }
    // 부모가 현재 World 프로세스를 먼저 정리한 뒤 이 생성 단계의 재사용을 차단한다.
    dispose() { this.#disposed = true; }
    // 이 구간의 정상 복귀이며 전체 Session/교전/Load 완료는 아니다.
    get ready() { return this.#ready; }
}
