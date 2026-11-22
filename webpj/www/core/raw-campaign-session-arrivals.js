// ConnectMotion3782..3805의 typed 건설 도착·마나 도착/주기 조립이다(225553df, WB04-E640).
// 사용: ConnectBuilding 뒤 bind. 경로 본체와 전체 Load/Native 장기 장치는 후속이다.
import { RawConstructionArrival } from './raw-construction-arrival.js';
import { RawManaBoltArrival } from './raw-mana-bolt-arrival.js';
import { RawManaBoltRegular } from './raw-mana-bolt-regular.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { removeAttachedProcesses } from './raw-process-removal.js';
import { RawCursorPosition } from './raw-cursor-position.js';
import { RegularProcess } from './squid-process.js';
import { projectCameraCoordinates } from '../render/camera-view.js';
import { readRiftShapeFrame } from './raw-rift-shape-access.js';

// 같은 실제 World/Host와 도착 처리기 세 개의 지역 소유권을 묶는다.
export class RawCampaignSessionArrivals {
    // 부분 설치/반복/반납 관문과 기존 실제 Impl 참조다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문 값형 함수 표다. 실제 객체도 같은 표를 빌린다.
    buildHooks = null; manaHooks = null; regularHooks = null;
    // 생성 자체로 현재 시계/카메라/프로세스를 바꾸지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 반납한 수명의 callback이 같은 Client의 다음 World로 재진입하지 못한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 도착 처리기'); }

    // 같은 건설 준비/시계와 실제 부착·삭제·프레임·현재 카메라를 원문 순서로 연결한다.
    bind() {
        const o = this.#owner, c = o.core, s = o.state;
        if (this.#disposed || this.#started || o.motion || o.buildArrival || !o.buildingModules?.ready) throw new Error('Session 도착의 초기화 전/반복 결선');
        this.#started = true;
        const build = {
            start: (sid, duration) => { this.#live(); o.assetModules.prepareBuilding(c.processState.now); o.building.start(sid, duration); },
            removeContained: (sid, type) => { this.#live(); new RawContainedLookup(c.pool, (target, flags) => o.world.destroy(target, flags)).find(sid, type, 0, true); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
        };
        this.buildHooks = build; o.buildArrival = new RawConstructionArrival(c.pool, c.hash, c.types, s.buildArrivalState, build);
        const mana = {
            sound: (x, y, name) => { this.#live(); o.services.sound.playOnceAt(x, y, o.soundNames.lookup(name), 0, 0); },
            removeProcesses: (sid, type) => { this.#live(); removeAttachedProcesses(c.processes, sid, type); },
            reserveRegular: () => { this.#live(); o.manaPendingRegular = new RegularProcess(1, s.manaArrivalState.delay); return true; },
            addRegular: (sid, event, delay) => {
                this.#live(); if (!o.manaPendingRegular || delay !== s.manaArrivalState.delay) throw new Error('마나 볼트 Regular 예약 누락');
                const reserved = o.manaPendingRegular; o.manaPendingRegular = null;
                const regular = c.processes.attachRegular(reserved, sid); regular.resetTask(event, c.processState.now);
            },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
        };
        this.manaHooks = mana; o.manaArrival = o.localManaArrival = new RawManaBoltArrival(c.pool, c.hash, c.types, s.manaArrivalState, mana);
        const regular = {
            setFrame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame, flags); },
            sound: (x, y, name) => { this.#live(); o.services.sound.playOnceAt(x, y, o.soundNames.lookup(name), 0, 0); },
            project: (x, y) => { this.#live(); const p = o.gameplayGlobals.camera().snapshot().pixels; return projectCameraCoordinates(p.x, p.y, x, y); },
            inverse: (x, y) => { this.#live(); return RawCursorPosition.screenToWorld(x, y, o.contentCamera); },
            unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); },
            pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            // 원문(RawSurfaceMana Native 결선)처럼 저장한 TYPE 의 프레임 0 SHP 를 Rift SHP 접근(00419850)으로 읽는다: 범위 밖·미적재·해제/미초기화 표식이면 오류 상세와 frameCheck 진단을 표시한 뒤 현재 SHP 의 프레임 0 을 계속 쓰고, 그 SHP 가 실제로 없을 때만 호스트 오류이다. (2026-11-22 WB04-E1218: 이전에는 진단 없이 오류로 중단했다)
            frameZero: (type) => {
                this.#live(); const storage = o.world.storage; const shape = c.shapes[type]; const record = c.types[type];
                readRiftShapeFrame({ token: type, flags1: record.flags1, frameCount: shape.frameCount, loaded: shape.loaded, header: record.maxHitPoints }, 0, 0, {
                    typeNumber: (view) => view.token, frameError: (number, frame, abstract) => storage.recordDisplayFrameError(number, frame, abstract),
                    assertion: (expression, file, line) => storage.assertDisplay(expression, file, line),
                    shape: (selected, frame) => { const current = c.shapes[selected]; if (!current.loaded || frame < 0 || frame >= (current.frames?.length ?? 0)) throw new RangeError('Fountain 진단 복귀 뒤 실제 SHP 범위'); return [0, 0]; },
                });
                const header = c.shapes[type].frames?.[0]; if (!header) throw new RangeError('Fountain 진단 복귀 뒤 실제 SHP 프레임 0 없음'); return header;
            },
        };
        this.regularHooks = regular; o.manaRegular = o.localManaRegular = new RawManaBoltRegular(c.pool, c.types, c.frames, c.shapes, o.manaArrival, s.manaRegularState, regular);
        this.#ready = true;
    }

    // 부모가 실제 Host를 먼저 비우고 미부착 예약/훅의 대여만 반납한다.
    dispose() { this.#disposed = true; this.#owner.manaPendingRegular = null; }
    // 도착 세 장치의 설치 완료이며 전체 ConnectMotion/Load 성공이 아니다.
    get ready() { return this.#ready; }
}
