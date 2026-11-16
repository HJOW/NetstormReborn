// 생성자1655/ConnectCarrierSteal2419..2440의 실제 내용물/정지/Regular 결선이다(225553df, WB04-E606).
// 사용: connectIceTower 뒤 bind. 같은 Form6·현재 이동/방송과492 예약을 빌리며 실제 미션/60h 등록은 뒤 Session 단계다.
import { RawCarrierSteal } from './raw-carrier-steal.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { startSelectionContentPreview } from './raw-squid-selection.js';
import { RegularProcess } from './squid-process.js';

// 선택된 실제 후설치 부모만 요구하고 빈 성공으로 바꾸지 않는다.
function need(value, message) { if (!value) throw new Error(message); return value; }

// 같은 현재 헤더/공유 정지 카운터와 실제 PostPop 등록 수명이다.
export class RawCampaignSessionCarrierSteal {
    // 한 번 구성/부분 실패와 반납 뒤 closure 접근을 구별한다.
    #owner; #started = false; #ready = false; #disposed = false; #registered = false;
    // RawCarrierSteal이 빌리는 값형 훅과 같은 Session DWORD의 상자다.
    hooks = null; haltCounter = null;

    // 구성만으로 내용물 이동/예약/난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 원문의 현재 내용물→미리보기/송신→이동/정기 예약을 실제 기존 몸체에 연결한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.iceModules?.ready || this.#started || o.carrierSteal) throw new Error('Session CarrierSteal의 초기화 전/반복 결선');
        this.#started = true;
        // 선택 미리보기의 실제491 예약이며 전체 Kernel/현재 시각을 따로 만들지 않는다.
        const previewRegular = (sid, event, payload) => { this.#live(); c.processes.addRegular(sid, event, payload); };
        // 원문의 Form 진단 하위가 없는 선택만 이 위치에서 명시 오류로 남긴다.
        const previewAssertion = () => { this.#live(); throw new Error('선택 내용물 미리보기의 실제 Form 진단 미연결'); };
        const hooks = {
            find: (parent, mask, kind, index) => { this.#live(); return o.priests.carrier.findContained(parent, mask, kind, index); }, // 같은 현재 종속 내용물을 조회한다.
            detach: (form, flags) => { this.#live(); if (flags) throw new Error('포획 Unpop flags 오류'); o.contentForms.unpop(form); }, // Source의 flags0 관문 뒤 실제 Form6을 분리한다.
            attach: (form, parent, flags) => { this.#live(); o.contentForms.attach(form, parent, flags); }, // 같은 실제 Form6을 새 부모에 정렬 부착한다.
            preview: (parent) => { this.#live(); startSelectionContentPreview(c.pool, parent, o.objectState.loadingDepth, previewRegular, previewAssertion); }, // 현재 로딩/extra/TYPE 관문으로491을 요청한다.
            broadcast: (packet) => { this.#live(); o.tables.broadcast.send(packet); }, // 이미 끝난 로컬 효과 뒤 현재 기존 방송 표에 보낸다.
            movementType: (type) => { this.#live(); return o.priests.movementProcessType(type); }, // 현재 기증자의 실제 이동 종류를 고른다.
            removeMovement: (sid, type) => { this.#live(); new RawContainedLookup(c.pool, (target, flags) => { o.world.destroy(target, flags); }).find(sid, type, 0, true); }, // 같은 현재 경로 체인에서 해당 이동을 가상 삭제한다.
            goHome: (sid) => { this.#live(); need(o.carrierTarget, '이동 설치 전 크랩 귀환').goHome(sid); }, // 후설치한 현재 귀환 대상 몸체를 사용한다.
            sound: (name, loop, volume, pan, priority, limit) => { this.#live(); need(o.services.sound.play, '크리스탈 크랩 실제 전역 소리 미연결')(o.soundNames.lookup(name), loop, volume, pan, priority, limit); }, // 현재 이름표/서비스의 원문6인자를 유지한다.
            commonHalt: (sid) => { this.#live(); return need(o.motion, '이동 설치 전 크랩 정지').commonHalt(sid); }, // 현재 실제 이동의 공통 정지 판정을 사용한다.
            allocateRegular: () => { this.#live(); o.pendingCarrierTask = new RegularProcess(0, 0); return true; }, // 공유 Carrier task 한 개를 실제 미부착 객체로 확보한다.
            regular: (sid, event, delay) => { // 같은 예약과 원문float1 관문을 확인한 뒤 부착 효과 후 현재 시각으로 재설정한다.
                this.#live(); delay = Math.fround(delay); if (!o.pendingCarrierTask || delay !== 1) throw new Error('크랩 Regular 예약 누락');
                const reserved = o.pendingCarrierTask; o.pendingCarrierTask = null; reserved.initializeDetached(event, delay);
                const task = c.processes.attachRegular(reserved, sid); if (!task) throw new Error('크랩 Regular 실제 부착 실패'); task.resetTask(event, c.processState.now);
            },
            check: (sid) => { this.#live(); o.priests.carrierCheck.check(sid); }, // 비서버 재배치의 실제 운반체 검사를 실행한다.
            damageablePostPop: (sid, flags) => { this.#live(); o.carrier.damageablePostPop(sid, flags); }, // 같은 Carrier의 공통 피해 후처리/장부로 복귀한다.
        };
        this.hooks = hooks; this.haltCounter = { get value() { return o.stealHaltCounter >>> 0; }, set value(value) { o.stealHaltCounter = value >>> 0; } };
        o.carrierSteal = new RawCarrierSteal(c.pool, c.hash, c.types, () => o.state.assetState.readBoss(), () => o.globals.editor, () => o.playerState.battleOwner, o.alliances, () => o.state.motionState.halt.carrierType, () => o.state.motionState.carrier.priestType, o.stealHeader, this.haltCounter, hooks);
        c.postPop.setBody('carrierSteal', o.carrierSteal); this.#registered = true; this.#ready = true;
    }

    // 이전 Session의 closure가 반납한 World에 다시 진입하지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session CarrierSteal'); }
    // 상위가 Host를 비운 뒤 등록과 미부착 공유 예약만 닫고 정지 카운터/헤더는 쓰지 않는다.
    dispose() { if (this.#disposed) return; this.#disposed = true; this.#owner.pendingCarrierTask = null; if (this.#registered) { this.#owner.core.postPop.setBody('carrierSteal', null); this.#registered = false; } }
    // 이 생성 구간의 정상 복귀이며 실제 미션 포획/입력/World 게시는 아니다.
    get ready() { return this.#ready; }
}
