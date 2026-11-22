// 생성자1659..1664/ConnectCarrierRegular2462..2564의 같은 운반·너겟·미리보기 결선이다(225553df, WB04-E608).
// 사용: connectAcidFence 뒤 bind. World 공개/Regular 분배 등록은 후속 생성 구간이 수행한다.
import { RawNuggetPostPop } from './raw-nugget-post-pop.js';
import { RawNuggetRegular } from './raw-nugget-regular.js';
import { RawCarrierRegular } from './raw-carrier-regular.js';
import { RawCarrierFalling } from './raw-carrier-falling.js';
import { RawCarrierTask } from './raw-carrier-task.js';
import { RawCarrierFallStart } from './raw-carrier-fall-start.js';
import { ContentPreviewFields, ContentPreviewProcess, RawContentPreviewProcess } from './raw-content-preview-process.js';
import { RawContainedFinder } from './raw-contained-finder.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RegularProcess } from './squid-process.js';
import { typeNumber } from '../assets/type-table.js';

// 선택된 현재 하위만 필수로 요구한다.
function need(value, message) { if (!value) throw new Error(message); return value; }

// 동일한 예약·시계·프레임 표와 현재 UI 약한 참조를 빌리는 생성 구간이다.
export class RawCampaignSessionCarrierRegular {
    // 한 번 설치/부분 실패/반납 상태다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 각 원문 몸체가 값으로 보관하는 훅이며 실제 공유 참조는 호출할 때 읽는다.
    nuggetHooks = null; nuggetEvents = null; previewHooks = null; hooks = null; fallingHooks = null; taskHooks = null; beginHooks = null;
    // 현재 A구간 수와 독립 typed의 TYPE별 기본 그림/시전 시간이다.
    aFrames = null; types = null;

    // 구성만으로 내용물·난수·프로세스를 만들지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 원문 다섯 runtime 훅 뒤 너겟→미리보기→낙하→task→Carrier 순서를 보존한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.spellModules?.ready || this.#started || o.carrierRegular) throw new Error('Session CarrierRegular의 초기화 전/반복 결선');
        this.#started = true;
        // 실제 현재 부모/프로세스를 같은 contained 조회로 찾고 선택된 가상 삭제만 실행한다.
        const lookup = (sid, type, remove = false) => { this.#live(); return new RawContainedLookup(c.pool, (target, flags) => o.world.destroy(target, flags)).find(sid, type, 0, remove); };
        // 회피 서비스의 반환값은 원문처럼 버린다.
        const escape = (sid, delay, event) => { this.#live(); need(o.services.motion?.carrier?.escape, '현재 Carrier 회피429010 미연결')(sid, delay, event); };
        // 현재 사제 월드의 실제 이동 타입/권한 처리기를 사용한다.
        const movementType = (type) => { this.#live(); return o.priests.movementProcessType(type); };
        // 경로 유무는 같은 현재 contained 프로세스의 첫 일치다.
        const hasPath = (sid, type) => Boolean(lookup(sid, type));
        // 후설치한 실제 낙하 몸체를 호출 시점에 읽는다.
        const falling = (sid, count) => { this.#live(); return need(o.carrierFalling, 'Carrier 낙하 몸체 미연결').fall(sid, count); };
        // 소유자 지정은 기존 실제 장부/표시 효과로 이어진다.
        const setOwner = (sid, owner) => { this.#live(); o.ownership.setOwner(sid, owner); };
        o.worldHooks.carrierRegular = { movementType, hasPath, escape, falling, setOwner };
        const nugget = {
            hasRegular: (sid, event) => { this.#live(); return c.processes.findEvent(sid, event) !== null; }, // 현재 Host의 첫 중복 예약이다.
            allocate: () => { this.#live(); if (o.pendingNuggetRegular) throw new Error('너겟 Regular 확보 메모리 중복'); o.pendingNuggetRegular = new RegularProcess(0, 0); return true; }, // PostPop/끝점이 같은 한 객체를 확보한다.
            random: (low, high) => { this.#live(); return Math.fround(o.random.next(65536) * (1 / 65536) * Math.abs(high - low) + low); }, // LCG상위16/65536과 넓은 차이 연산 뒤 한 번 float로 저장한다.
            construct: (sid, event, payload) => { // 같은 확보 객체를 생성/부착하고 성공했을 때만 효과 뒤 현재 시각으로 ResetTask한다.
                this.#live(); const reserved = need(o.pendingNuggetRegular, '너겟 Regular 생성 전 확보 메모리 없음'); o.pendingNuggetRegular = null; reserved.initializeDetached(event, payload);
                const attached = c.processes.attachRegular(reserved, sid); if (attached) attached.resetTask(event, c.processState.now);
            },
            damageable: (sid, flags) => { this.#live(); o.carrier.damageablePostPop(sid, flags); }, // 기존 공통 후처리의 장부/표시를 사용한다.
        };
        // 두 몸체가 같은 확보/생성 closure를 값으로 보관한다.
        const events = { allocate: nugget.allocate, construct: nugget.construct,
            randomInteger: (low, high) => { this.#live(); if (high <= low) throw new RangeError('너겟정수난수구간오류'); return low + o.random.next((high - low) >>> 0); }, // 상한 제외 정수 난수다.
            setFrame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame, flags); }, // 같은 실제 프레임 몸체다.
            refresh: (sid) => { this.#live(); c.display.update(c.pool.physical(sid)); }, // 같은 슬롯의 현재 표시를 갱신한다.
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); }, // 현재 가상 삭제다.
            unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); }, // 현재 공간에서 같은 객체를 분리한다.
            pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); }, // 같은 World에 현재 위치로 다시 붙인다.
            damageable: (sid, event, count, payload) => { this.#live(); return o.assetBindings.regular.damageableHandle(sid, event, count, payload); }, // 실제 기본 Handle에 복귀한다.
        };
        this.aFrames = new Int32Array(c.types.length);
        // 보호 자산에서 만든 같은188 프레임 표의 A구간 수만 원문 순서로 채운다.
        for (let type = 0; type < this.aFrames.length; type += 1) this.aFrames[type] = c.frames[type].run('A').count;
        o.nuggetRegular = new RawNuggetRegular(c.pool, this.aFrames, c.spots, () => o.state.assetState.readBoss(), events);
        o.nuggetPostPop = new RawNuggetPostPop(c.pool, nugget); this.nuggetHooks = nugget; this.nuggetEvents = events;
        this.types = Array.from({ length: c.types.length }, () => ({ defaultFrame: 0, castTime: 0 }));
        // 현재 정의의118/1b8을 같은 번호70부터 보충하며 정수 범위를 검사한 뒤0방향 절삭한다.
        for (const asset of o.assets.game.types()) {
            const cast = typeNumber(asset.definition, 'casttime') ?? 0;
            if (!Number.isFinite(cast) || cast < -2147483648 || cast > 2147483647) throw new RangeError('실제 자산 casttime의 정수 범위 오류');
            this.types[asset.block + 70] = { defaultFrame: asset.definition.defaultFrame, castTime: Math.trunc(cast) };
        }
        const preview = {
            attach: (fields, parent, type, flags) => { // 같은 확보 객체/필드의 소유권을 부착 전에 소비하며 NULL 결과도 원문처럼 그대로 둔다.
                this.#live(); const pending = o.pendingContentPreview; if (!pending || pending.fields !== fields) throw new Error('내용물 타이머의 실제 확보 메모리 불일치');
                o.pendingContentPreview = null; c.processes.attach(pending, type, parent, flags);
            },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); }, // 만료한 부모의 현재 가상 삭제다.
            // 원문(RawSurfaceCarrier Native 결선)처럼 부모의 머리 Form 부재 진단(Baseprocess.cpp 0xa4)을 표시하고 캡처한 부모를 계속 쓴다. (2026-11-22 WB04-E1211: 이전에는 오류를 던졌다)
            assertion: () => { this.#live(); o.world.storage.assertDisplay('parent->getHead() != 0', '\\Ns\\O\\Baseprocess.cpp', 0xa4); },
        };
        o.contentPreviewProcess = new RawContentPreviewProcess(c.pool, { now: () => c.processState.now, processType: () => o.state.soundState.currentType() }, preview); this.previewHooks = preview;
        const hooks = {
            find: (parent, mask, kind, index) => { this.#live(); return o.priests.carrier.findContained(parent, mask, kind, index); }, // 실제 현재 Form6 조회다.
            create: (type, flags) => { this.#live(); return c.factory.create(type, flags); }, // 같은 풀/VT 생성자다.
            setWord: (sid, value) => { this.#live(); const raw = c.pool.allocated(sid); raw[12] = value & 255; raw[13] = (value >>> 8) & 255; if (raw[10] === c.bridgeState.bridgeType) c.building.notifySurface(sid); }, // 하위WORD 저장 뒤 현재 다리만 통지한다.
            setOwner, pop: events.pop,
            allocateTimer: () => { this.#live(); if (o.pendingContentPreview) throw new Error('내용물 타이머 확보 메모리의 중복 소유권'); o.pendingContentPreview = new ContentPreviewProcess(o.contentPreviewProcess, new ContentPreviewFields()); return true; }, // 현재 미부착24BYTE 객체의 단일 소유권이다.
            constructTimer: (parent, duration) => { this.#live(); const pending = need(o.pendingContentPreview, '내용물 타이머 생성 전 실제 메모리 미확보'); o.contentPreviewProcess.construct(pending.fields, parent, duration); }, // 같은 실제 필드를 현재 시계/타입으로 생성한다.
            refresh: events.refresh,
        };
        // mask 없이 현재 kind WORD가 일치하는 첫 Form6을 찾는다.
        const findKind = (parent, kind) => {
            this.#live(); const finder = new RawContainedFinder(c.pool, o.contentFinder);
            // 후보 링크를 저장한 실제 Begin/Next 순서대로 kind만 비교한다.
            for (let sid = finder.begin(parent); sid; sid = finder.next()) { const raw = c.pool.slot(sid); if ((raw[18] | (raw[19] << 8)) === kind) return sid; }
            return 0;
        };
        const fall = { find: hooks.find, create: hooks.create, setWord: hooks.setWord, setOwner, pop: hooks.pop, findKind,
            destroy: events.destroy, unpop: events.unpop,
            fallWalker: (sid) => { this.#live(); return o.priests.fallWalker(sid); }, // 현재 사제의 실제 낙하로 이어진다.
            assertion: (line) => { this.#live(); throw new Error(`원본 Carrier.cpp 낙하 진단 ${line}`); }, // 선택된 원문 진단을 전파한다.
        };
        o.carrierFalling = new RawCarrierFalling(c.pool, c.types, c.frames, c.hash.view(0), o.players, () => o.state.assetState.readBoss(), () => o.gameplayGlobals.aiDebug().checking().value, () => o.priestState.movement.priest, () => o.cursorManaGlobals.mapLimit, fall); this.fallingHooks = fall;
        const task = {
            find: (sid) => { this.#live(); return c.processes.findEventMasked(sid, 0xfff0, 0x5100); }, // 실제 같은 task군의 첫 현재 예약이다.
            allocate: () => { this.#live(); if (o.pendingCarrierTask) throw new Error('캐리어 task 확보 메모리 중복'); o.pendingCarrierTask = new RegularProcess(0, 0); return true; }, // 크랩과 같은 pendingCarrierTask를 공유한다.
            construct: (sid, event, shared) => { this.#live(); const pending = need(o.pendingCarrierTask, '캐리어 task 생성 전 확보 메모리 없음'); o.pendingCarrierTask = null; pending.initializeDetached(event, 5); return c.processes.attachRegular(pending, sid, shared); }, // 같은 객체를 payload5로 생성하며 추가 ResetTask를 하지 않는다.
            assertion: () => { this.#live(); throw new Error('원본 Carrier.cpp407 낙하 task 교체 진단'); }, // 이미 낙하 중인 task 교체 진단이다.
        };
        o.carrierTask = new RawCarrierTask(c.pool, () => c.processState.now, task); this.taskHooks = task;
        const begin = { hasRegular: nugget.hasRegular, movementType,
            removeMovement: (sid, type) => { lookup(sid, type, true); }, // 현재 첫 이동 프로세스를 가상 삭제한다.
            clearSelection: () => { this.#live(); const selection = o.activeSelection?.deref()?.selection; need(selection, '낙하 선택 해제의 현재 UI 수명 미연결').select(0, 0, 1); }, // 살아 있는 실제 UI 토큰만 빌리고 반환값을 버린다.
            findKind, releasePriest: (sid) => { this.#live(); o.carrierFalling.releasePriest(sid); }, // 실제 같은 낙하 몸체를 사용한다.
            task: (sid, event, delay) => { this.#live(); o.carrierTask.set(sid, event, delay); }, // 현재 단일 task로 이어진다.
        };
        o.carrierFallStart = new RawCarrierFallStart(c.pool, o.players, () => o.state.assetState.readBoss(), () => o.priestState.movement.priest, begin); this.beginHooks = begin;
        Object.assign(hooks, { needsEscape: (x, y) => { this.#live(); return o.carrierFalling.needsEscape(x, y) !== 0; }, movementType, hasPath, escape, falling, damageable: events.damageable }); // 현재 내용물 그림과 같은 낙하/이동/기본 Handle이다.
        this.hooks = hooks; o.carrierRegular = new RawCarrierRegular(c.pool, c.types, this.types, () => o.state.assetState.readBoss(), hooks); this.#ready = true;
    }

    // 반납한 원소유자의 closure가 다시 World를 사용하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session CarrierRegular'); }
    // 상위가 실제 Host를 비운 뒤 미부착 두 예약만 정리한다. 공유 Carrier task는 앞 owner가 닫는다.
    dispose() { if (this.#disposed) return; this.#disposed = true; this.#owner.pendingNuggetRegular = null; this.#owner.pendingContentPreview = null; }
    // 이 생성 구간의 정상 복귀이며 World 공개/미션 시작 완료가 아니다.
    get ready() { return this.#ready; }
}
