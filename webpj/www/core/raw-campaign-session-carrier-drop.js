// Cast/Drop의 공통 객체 명령과 현재 투하점 앞단을 같은 Session에 연결한다(225553df).
import { RawObjectCommand } from './raw-object-command.js';
import { RawCarrierDropPoint } from './raw-carrier-drop-point.js';
import { captureCampaignFlag } from './raw-campaign-session-state.js';
import { projectCameraCoordinates } from '../render/camera-view.js';

// SP DWORD의 현재 비트를 계산 없이 float로 읽는다.
function spFloat(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits, true); return view.getFloat32(0, true); }

// 실제 소유권·피해·배치 장치와 운반 투하 지역 상태를 관리한다.
export class RawCampaignSessionCarrierDrop {
    // 같은 Session과 설치 시작·완료·반납 관문이다.
    #owner; #started = false; #ready = false; #requestStarted = false; #requestReady = false; #disposed = false;
    // 공통 명령과 투하점이 빌리는 값형 상태 및 효과 표다.
    commandState = null; commandHooks = null; pointState = { mapLimit: 256, invalidPoint: [0, 0] }; pointHooks = null;
    // 실제 Cast/Drop이 완성될 때 값으로 빌릴 요청 효과 표다.
    dropHooks = null;

    // 생성은 현재 World를 바꾸지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 반납된 Session의 함수 값이 재진입하지 않도록 한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 운반 투하'); }

    // 요청 직전 현재 소유자·서버·진단 뷰와 지도 한계를 복사한다.
    prepare() {
        this.#live(); const o = this.#owner, c = o.core, state = o.state.carrierDropState;
        state.localOwner = o.playerState.battleOwner; state.server = c.processState.server ? 1 : 0;
        state.checking = captureCampaignFlag(o.state.gunState, 'checking'); state.numTypes = c.types.length;
        state.mapLimit = this.pointState.mapLimit = o.cursorManaGlobals.mapLimit | 0;
    }

    // 선행 파괴 주문 뒤 공통 객체 명령과 실제 투하점 계산기를 설치한다.
    bindFront() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !o.meltdownModules?.ready || !o.carrierPickup || this.#started || o.objectCommand || o.carrierDropPoint) throw new Error('Session 운반 투하 앞단 설치 순서 오류');
        this.#started = true; const damage = o.hits.damageHooks({}).damage;
        const state = {
            localOwner: () => { this.#live(); return o.playerState.battleOwner; },
            editor: () => { this.#live(); return Boolean(o.globals.editor); },
            mission: () => { this.#live(); return Boolean(o.globals.missionPost); },
        };
        const common = {
            owner: (sid, owner) => { this.#live(); o.ownership.setOwner(sid, owner); },
            hit: (sid, amount, owner, attacker) => { this.#live(); if (typeof damage !== 'function') throw new Error('공통 객체 명령의 실제 피해 미연결'); damage(sid, amount, owner & 255, attacker); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            describe: () => { this.#live(); throw new Error('공통 객체 명령의 실제 진단 출력 미연결'); },
        };
        o.objectCommand = new RawObjectCommand(c.pool, state, common);
        const point = {
            mayPlace: (query) => { this.#live(); o.assetModules.prepareConstruction(); return o.constructionModules.mayPlace(query); },
            center: (sid) => { this.#live(); return o.gunTarget.center(sid); },
        };
        o.carrierDropPoint = new RawCarrierDropPoint(c.pool, c.types, this.pointState, point);
        this.commandState = state; this.commandHooks = common; this.pointHooks = point; this.#ready = true;
    }

    // 현재 비용·프로세스·선택·요청 전송을 운반 투하 요청에 연결한다.
    bindRequest() {
        this.#live(); const o = this.#owner, c = o.core;
        if (this.#requestStarted || o.carrierDrop) throw new Error('Session 운반 투하 요청 설치 순서 오류');
        this.#requestStarted = true;
        const hooks = {
            immobile: (sid) => { this.#live(); return o.priests.priestState.groundImmobile(sid); },
            cost: (type) => { this.#live(); return o.hp.typeCostUnits(type); },
            afford: (cost) => { this.#live(); const money = spFloat(o.sp.get(o.playerState.battleOwner)); return Boolean(o.globals.editor) || cost <= money; },
            subtract: (owner, amount) => { this.#live(); o.hp.addSp(owner, amount); },
            project: (point) => { this.#live(); const p = o.gameplayGlobals.camera().snapshot().pixels; return projectCameraCoordinates(p.x, p.y, point[0], point[1]); },
            popup: (cost, point, reference) => { this.#live(); if (typeof o.services.carrierDropPopup !== 'function') throw new Error('Cast/Drop 실제 비용 Gump 미연결'); o.services.carrierDropPopup(cost, point[0], point[1], reference); },
            regular: (parent, delay) => { this.#live(); c.processes.addSharedRegular(parent, 0x493, delay); },
            attached: (parent, visit) => {
                this.#live(); const head = c.pool.slot(parent), visited = new Uint8Array(c.pool.capacity);
                let current = head[6] | (head[7] << 8);
                // 통지 전에 현재 다음 링크를 저장하고 원문처럼 프로세스 종류만 전달한다.
                while (current) {
                    if (current >= visited.length || visited[current]) throw new Error('Cast/Drop 부착 체인 손상');
                    visited[current] = 1; const raw = c.pool.slot(current), next = raw[4] | (raw[5] << 8);
                    if (raw[10] < 70) visit(current); current = next;
                }
            },
            notify: (form, event, info, value) => { this.#live(); const process = c.processes.fromForm(form); if (!process) throw new Error('Cast/Drop 현재 프로세스 누락'); process.notify(event, info, value); },
            select: (sid, force, activate) => { this.#live(); const selection = o.activeSelection?.deref()?.selection; if (!selection) throw new Error('Cast/Drop 실제 선택 수명 미연결'); selection.select(sid, force, activate); },
            send: (target, packet) => { this.#live(); if (typeof o.services.carrierRequest !== 'function') throw new Error('Cast/Drop 실제 전송 미설치'); o.services.carrierRequest(target, packet); },
            pickup: (parent) => { this.#live(); o.carrierPickup.request(parent); },
            baseMenu: (parent, command) => { this.#live(); return o.objectCommand.command(parent, command); },
            assertion: (line) => { throw new Error(`원본 Cast/Drop 진단 ${line}`); },
            dropPoint: (parent, type) => { this.#live(); return o.carrierDropPoint.find(parent, type); },
        };
        this.dropHooks = hooks; this.#requestReady = true;
    }

    // 실제 World를 닫을 때 기존 함수 값과 미완료 앞단을 함께 반납한다.
    dispose() { this.#disposed = true; }
    // 앞단 설치만 완료했으며 Cast/Drop 전체 수신은 후속이다.
    get ready() { return this.#ready; }
    // 요청 효과 표만 준비했으며 서버 수신과 전체 설치는 후속이다.
    get requestReady() { return this.#requestReady; }
}
