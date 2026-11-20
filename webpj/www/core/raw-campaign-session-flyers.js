// 생성자1669/ConnectFlyer2196..2260·Frame2274..2351·Target2353..2367의 결선이다(225553df, WB04-E611).
// 사용: ArcSpire 뒤 bind. 기존 비행 몸체를 조립하며 공중3종의 새 전용 실행 검사는 추가하지 않는다.
import { typeNumber, typeString } from '../assets/type-table.js';
import { RawFlyerProcess, FlyerProcessFields, FlyerProcess, RawFlyerPostPop } from './raw-flyer-process.js';
import { RawFlyerFrame } from './raw-flyer-frame.js';
import { RawFlyerTarget } from './raw-flyer-target.js';
import { RawSunFlyerFrame } from './raw-sun-flyer-frame.js';
import { RawWindFlyer } from './raw-wind-flyer.js';
import { SunAviaryProcess } from './raw-sun-aviary-process.js';
import { RainAviaryProcess } from './raw-rain-aviary-process.js';
import { findAttachedProcess } from './raw-wind-aviary-process.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { RegularProcess } from './squid-process.js';
import { SoundProcess } from './sound-process.js';
import { ShrapnelKind } from './raw-shrapnel-frame.js';
import { isSidRobust } from './raw-sid-robust.js';
import { writeFlyerCommandPacket } from './raw-battle-broadcast.js';
import { captureCampaignFlag } from './raw-campaign-session-state.js';

// 현재 원문 이미지의 비정렬 DWORD를 읽는다.
function read(raw, offset) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(offset, true); }
// 현재 원문 좌표의 float를 읽는다.
function coordinate(raw, offset) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true); }
// TYPE speed를 원문 float 비트로 보관한다.
function floatBits(value) { const raw = new DataView(new ArrayBuffer(4)); raw.setFloat32(0, value, true); return raw.getUint32(0, true); }
// 선택된 하위/Host가 없으면 그 위치에서 명시적으로 거부한다.
function need(value, message) { if (!value) throw new Error(message); return value; }

// 같은 현재 Host·프레임·표적/후처리를 기존 몸체에 연결하는 독립 typed 대여 수명이다.
export class RawCampaignSessionFlyers {
    // 부분 실패/반복/반납과 실제 postPop 등록 여부를 구별한다.
    #owner; #started = false; #ready = false; #disposed = false; #registered = false; #finder = null;
    // 각 원문 struct의 값형 훅과 현재 전역 getter를 보관한다.
    processHooks = null; frameHooks = null; targetHooks = null; postHooks = null; sunHooks = null; windHooks = null;
    processGlobals = null; frameGlobals = null; targetGlobals = null; sunGlobals = null; windGlobals = null;
    // 같은188TYPE의 추가 메타다. 기존 몸체가 사본을 요구하는 경계를 그대로 둔다.
    postTypes = null; frameTypes = null; bombTypes = null; targetRanges = null; windActiveSounds = null;

    // 실제 설치 전에는 자산/프로세스/난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // Process→Frame/Target→PostPop 순서로 구성한다. 첫 Pop/Kernel은 상위 World 게시 이후다.
    bind() {
        const o = this.#owner; const c = o.core; const s = o.state;
        if (this.#disposed || !o.arcSpireModules?.ready || this.#started || o.flyer) throw new Error('Session Flyer의 초기화 전/반복 결선'); this.#started = true;
        // 원문은 services.ai.assertion 함수 값을 생성 시 한 번 복사한다.
        const assertion = o.services.ai.assertion;
        const hooks = {
            assertion: (line) => { this.#live(); need(assertion, `실제 비행 프로세스 진단 장치 미연결 ${line}`); if (line === 0x2e3 || line === 0x41d) assertion('server'); else if (line === 0x904 || line === 0x8ee) assertion('flyer.getGenus() & gFLYER'); else if (line === 0x8f5) assertion('processSid'); else throw new RangeError('비행 프로세스의 다른 진단 위치'); }, // 확인한 위치의 원문 식만 허용한다.
            findProcess: (sid, type) => { this.#live(); return findAttachedProcess(c.pool, sid, type); }, // 현재 종속 체인의 첫 프로세스다.
            fields: (form) => { this.#live(); const process = c.processes.fromForm(form); if (!(process instanceof FlyerProcess)) throw new Error('비행 Form의 현재 PID/원문 미연결'); return process.fields; }, // 현재 Host의 같은 이미지다.
            attach: (fields, parent, type, flags) => { this.#live(); if (!o.pendingFlyer || o.pendingFlyer.fields !== fields) throw new Error('비행 생성자의 현재 원문/소유권 불일치'); const process = o.pendingFlyer; o.pendingFlyer = null; if (!c.processes.attach(process, type, parent, flags)) throw new Error('비행 생성자의 free/dead 부모 부착 거부'); }, // 같은 미부착 객체를 옮긴다.
            frame: (fields) => { this.#live(); const table = read(fields.bytes, 0); if (table === 0x509f24) o.sunFlyerFrame.frame(fields); else if (table === 0x509f98) { if (!read(fields.bytes, 84)) o.windFlyer.initialize(fields); o.flyerFrame.baseFrame(fields); } else o.flyerFrame.frame(fields); }, // 현재 외곽 VT에 따라 같은 기존 몸체를 고른다.
            notify: (fields, message, a, b) => { this.#live(); o.flyerFrame.notify(fields, message, a, b); }, // 공통 알림이다.
            parentDying: (fields) => { this.#live(); if (read(fields.bytes, 0) === 0x509f24) o.sunFlyerFrame.parentDying(fields); else o.flyerFrame.parentDying(fields); }, // 같은 파생 부모 소실이다.
            pieces: (parent) => { this.#live(); o.particles.addShrapnel(parent, ShrapnelKind.Explosion); }, // 실제 캡처/입자다.
            destroy: (parent, flags) => { this.#live(); o.world.destroy(parent, flags); }, // 같은 가상 삭제다.
        };
        this.processHooks = hooks; this.processGlobals = { server: () => c.processState.server, now: () => c.processState.now, processType: () => o.gameplayGlobals.gameAux().flyerProcessType, checking: () => o.gameplayGlobals.aiDebug().checking().value };
        o.flyer = new RawFlyerProcess(c.pool, c.types, this.processGlobals, hooks);
        o.broadcastFlyerCommand = (parent, command) => { this.#live(); const packet = new Uint8Array(20); writeFlyerCommandPacket(packet, parent, command); o.tables.broadcast.sendMutable(packet); }; // 미작성+8/+16은 JS 할당 바이트이며 별도 payload 의미를 보장하지 않는다.
        // 현재 부모에 붙은 비행 프로세스의 필드를 돌려준다(원본 `CurrentFlyerProcess`). 없으면 checking 과 상관없이 processSid 진단을 쓴 뒤 같은 0 슬롯을 조회한다. Rain 피격(ApplyFlyerHit 45fa39)과 86h 비행 명령이 쓴다.
        // 이 연결이 없으면 맨 오 워 비행이 맞는 순간 'Rain 피격의 실제 CurrentFlyerProcess 설치 전 호출' 오류로 프레임이 멈췄다. (2026-11-20 WB04-E1122)
        o.currentFlyerProcess = (parent, line) => {
            this.#live(); const type = o.gameplayGlobals.gameAux().flyerProcessType;
            if (!Number.isInteger(parent) || parent < 0 || parent > 65535 || parent >= c.pool.capacity) throw new RangeError('비행명령의 실제 DWORD 부모 SID 물리 범위');
            const form = findAttachedProcess(c.pool, parent, type);
            if (!form) o.world.storage.assertDisplay('processSid', '\\Ns\\O\\Flyer.cpp', line);
            const process = c.processes.fromForm(form); if (!(process instanceof FlyerProcess)) throw new Error('비행명령의 현재 Form PID에 대한 실제 Flyer outer 미연결');
            return process.fields;
        };
        this.#bindFrame();
        const post = {
            allocateRainProcess: () => this.#allocateFlyer(), allocateSunProcess: () => this.#allocateFlyer(), // JS 필드 확보는 각각 같은112/136BYTE 이미지의 수명을 소유한다.
            createRainProcess: (parent, home, speed, parameter) => { this.#live(); const current = need(o.pendingFlyer, '비행 생성자의 실제 할당 누락'); o.flyer.construct(current.fields, parent, home, speed, parameter); RawFlyerProcess.specializeRain(current.fields); }, // 기본 부착 뒤 전용 표를 쓴다.
            createSunProcess: (parent, home, speed, parameter) => { this.#live(); const current = need(o.pendingFlyer, '훨리기그 생성자의 실제 할당 누락'); o.flyer.construct(current.fields, parent, home, speed, parameter); RawFlyerProcess.specializeSun(current.fields); }, // 같은136BYTE 원문의 꼬리를 유지한다.
            repop: (sid, flags) => { this.#live(); o.world.repop(sid, flags); }, // 같은 공간 복원이다.
            allocateSound: () => { this.#live(); if (o.pendingSunFlyerSound) throw new Error('훨리기그 Sound47 중복 예약'); o.pendingSunFlyerSound = new SoundProcess(o.sounds, 0, 0, 0); return true; }, // 실제 미부착 Sound 객체를 예약한다.
            createSound: (parent, sound, alternate, flags) => { this.#live(); const process = need(o.pendingSunFlyerSound, '훨리기그 Sound47 실제 예약 누락'); o.pendingSunFlyerSound = null; process.initializeUnattached(sound, alternate, flags); c.processes.attach(process, s.soundState.currentType(), parent, 0x10); }, // 같은 확보를 초기화/부착한다.
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); }, // 실제 TYPE 생성자다.
            pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); }, unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); }, // 실제 공간 양 끝이다.
            initializeUnit: (sid) => { this.#live(); o.gunHit.setTime(sid, c.processState.now); }, // 현재 시계로 초기화한다.
            shadowOffset: (sid) => { this.#live(); const table = read(c.pool.slot(sid), 0); if (table === 0x50a050) return [0, 0]; if (table === 0x50a1e8) return [Math.fround(-0.6), 3.5]; if (table !== 0x50a388) throw new Error('다른 비행체 그림자 offset 미복원'); return [0.5, 0.5]; }, // 원문 세 가상 상수다.
            damageable: (sid, flags) => { this.#live(); o.carrier.damageablePostPop(sid, flags); }, // 기존 공통 부모다.
            hasRegular: (sid, event) => { this.#live(); return c.processes.findEvent(sid, event) !== null; }, // 같은 Host에서 중복을 조회한다.
            allocateWindRegular: () => { this.#live(); if (o.pendingWindRegular) throw new Error('더스트 데빌 Regular 중복 예약'); o.pendingWindRegular = new RegularProcess(0, 0.25); return true; }, // 같은 미부착 예약이다.
            createWindRegular: (sid, event, period) => { this.#live(); if (!o.pendingWindRegular || event !== 0 || period !== 0.25) throw new Error('더스트 데빌 Regular 확보/인수 불일치'); const process = o.pendingWindRegular; o.pendingWindRegular = null; c.processes.attachRegular(process, sid); }, // 확보한 객체의 수명과 인수를 유지한다.
        };
        this.postTypes = Array.from({ length: c.types.length }, () => ({ speed: 0, frames: 0, impactSound: 0, activeSound: 0 }));
        // 원문 자산 순서로 speed 비트·전체 프레임 수와 두 소리 이름표를 채운다.
        for (const asset of o.assets.game.types()) { const type = 70 + asset.block; const impact = typeString(asset.definition, 'impactSound'); const active = typeString(asset.definition, 'activeSound'); this.postTypes[type] = { speed: floatBits(typeNumber(asset.definition, 'speed') ?? 0), frames: c.frames[type].codes.length, impactSound: impact ? o.soundNames.lookup(impact) : 0, activeSound: active ? o.soundNames.lookup(active) : 0 }; }
        this.postHooks = post; o.flyerPostPop = new RawFlyerPostPop(c.pool, c.types, this.postTypes, () => c.processState.server, post); c.postPop.setBody('flyer', o.flyerPostPop); this.#registered = true; this.#ready = true;
    }

    // 112BYTE 기본 이미지와24BYTE Sun 꼬리를 같은 미부착 객체에 소유시킨다. CRT 미정 힙 바이트를 합성하지 않는다.
    #allocateFlyer() { this.#live(); const o = this.#owner; if (o.pendingFlyer) throw new Error('비행 생성자 부착의 예상 밖 재진입'); o.pendingFlyer = new FlyerProcess(o.flyer, new FlyerProcessFields()); return true; }

    // 가상 이동/검색의 서로 다른 원문 offset을 현재 VT로 선택한다.
    #offset(sid, home) { const table = read(this.#owner.core.pool.slot(sid), 0); if (table === 0x50a050) return [0, 0]; if (table === 0x50a1e8) return [2, home ? -2.5 : -4]; if (table !== 0x50a388) throw new Error('다른 플라이어 이동 offset 미복원'); return [1, 0]; }

    // 현재 좌표·중심·모드/소유자/프레임으로 같은 GunTarget 문맥을 만든다.
    #mayTarget(sid, target) { this.#live(); const o = this.#owner; const raw = o.core.pool.slot(sid); const center = o.gunTarget.center(sid); return o.gunTarget.mayTarget({ x: coordinate(raw, 14), y: coordinate(raw, 18), centerX: center[0], centerY: center[1], type: raw[10], direction: o.gunMode.get(sid) | 0, owner: raw[34], frame: read(raw, 36) | 0 }, target); }

    // 같은 현재 프레임 코드 글자의 run count다. 프레임 범위 오류를 사전 보정하지 않는다.
    #runCount(sid) { this.#live(); const o = this.#owner; const raw = o.core.pool.slot(sid); const frame = read(raw, 36) | 0; const table = o.core.frames[raw[10]]; if (frame < 0 || frame >= table.codes.length) throw new RangeError('플라이어 현재 글자 프레임 범위'); return table.run(String.fromCharCode(table.codes[frame].side)).count; }

    // CurrentSidRobust2262..2272: JS 풀의 상대 바이트 주소에서 같은 상세 진단/현재 전역을 사용한다.
    currentSidRobust(sid, live) {
        this.#live(); const o = this.#owner; const pool = o.core.pool;
        return isSidRobust(sid | 0, live ? 1 : 0, o.gameplayGlobals.sidRobustDiagnostics(), {
            addressMode: 'host', checking: () => o.state.gunState.checking, poolBase: () => 0, capacity: () => pool.capacity,
            memory: (token, width) => { if (!Number.isSafeInteger(token) || token < 0 || token + width > pool.capacity * 50) throw new RangeError('SID 현재 풀 물리 범위 밖'); const raw = pool.physical(Math.floor(token / 50)); const at = token % 50; if (at + width > 50) throw new RangeError('SID 현재 슬롯 접근 범위'); return raw.subarray(at, at + width); }, // 진단 TYPE 선행 읽기를 생략하지 않는다.
            assertion: (expression, line) => { need(o.services.ai.assertion, `SID 원문 진단 미연결 ${line}`)(expression); }, // 실제 진단이 정상 복귀하면 원문 true를 유지한다.
        });
    }

    // ConnectFlyerFrame의 현재 제어·의존/표적·음향과 Sun/Wind 전용 몸체를 구성한다.
    #bindFrame() {
        const o = this.#owner; const c = o.core; const s = o.state;
        const hooks = {
            assertion: (line) => { this.#live(); throw new Error(`Flyer Frame 실제 진단 미연결 ${line}`); }, // 원문 SessionMissing이다.
            robust: (fields) => { this.#live(); if ((fields.form && fields.form >= c.pool.capacity) || (fields.parent && fields.parent >= c.pool.capacity)) throw new Error('비행 Baseprocess0041be80의 Form/부모 원본 진단'); return true; }, // 원문 정상 범위 진단 뒤1이다.
            sidRobust: (sid, live) => this.currentSidRobust(sid, live), destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); }, // 같은 상세 진단/삭제다.
            finish: (fields) => { this.#live(); o.flyer.finish(fields); }, // 같은 실제 Process다.
            otherFrame: (fields) => { this.#live(); const table = read(fields.bytes, 0); if (table === 0x509f24) return o.sunFlyerFrame.animation(fields); if (table === 0x509f98) return o.windFlyer.animation(fields); throw new Error('다른 플라이어 outer+8 Frame 미복원'); }, // 현재 VT다.
            sunInitialize: (fields) => { this.#live(); o.sunFlyerFrame.initialize(fields); }, sunNotify: (fields, message, a, b) => { this.#live(); o.sunFlyerFrame.notify(fields, message, a, b); }, // 같은 Sun 전용 부모다.
            offset: (sid, home) => { this.#live(); return this.#offset(sid, home); }, scatter: (target, flyer) => { this.#live(); return o.flyerTarget.scatter(target, flyer); }, // 이동/검색의 가상 offset과 실제 할당 칸이다.
            refreshTime: (sid) => { this.#live(); o.gunHit.setTime(sid, c.processState.now); }, hit: (sid, target) => { this.#live(); o.gunHit.hit(sid, target, c.processState.now); }, // 같은 현재 HP 시계다.
            removeSound: (sid, type) => { this.#live(); const form = findAttachedProcess(c.pool, sid, type); if (form && (c.processState.server || (form > 4 && form < 15000))) o.world.destroy(form, 0); }, // 원문 client/server 권한으로 첫 일치 하나만 지운다.
            unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); }, pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); }, repop: (sid, flags) => { this.#live(); o.world.repop(sid, flags); }, // 같은 현재 가상 공간이다.
            movedCell: (sid) => { this.#live(); const raw = c.pool.slot(sid); const table = read(raw, 0); if (!((raw[10] === 73 && table === 0x50a1e8) || (raw[10] === 144 && table === 0x50a388) || (raw[10] === 138 && table === 0x50a050))) throw new Error('다른 비행체 칸 이동+d4 미복원'); }, // 원문 Flyer의 세 TYPE/VT 조합 관문과 실제 ret 몸체다.
            checkForms: (sid) => { this.#live(); return o.flyerFrame.checkForms(sid); }, mayTarget: (sid, target) => this.#mayTarget(sid, target), // 같은 Form과 현재 문맥이다.
            closerCount: (target, flyer) => { this.#live(); return o.flyerTarget.closerCount(target, flyer); }, capacity: (sid) => { this.#live(); return o.flyerTarget.capacity(sid); }, findTarget: (fields) => { this.#live(); o.flyerTarget.find(fields); }, // 실제 표적 경쟁이다.
            addDepend: (target, form) => { this.#live(); c.dependencies.create(target, form); }, removeDepend: (form, target) => { this.#live(); c.dependencies.remove(form, target, 0); }, // 같은 watcher 수명이다.
            refreshForm: (fields) => { this.#live(); const process = c.processes.fromForm(fields.form); if (!(process instanceof FlyerProcess) || process.fields !== fields) throw new Error('비행 Frame 현재 Host/raw 이미지 불일치'); c.processes.refreshForm(process); }, // 같은 부착 객체다.
            returnDelay: (fields) => { this.#live(); return RawFlyerProcess.returnDelay(fields); }, command: (parent, command) => { this.#live(); o.broadcastFlyerCommand(parent, command); }, // 현재 외곽 VT의 반환 지연과 실제 mutable 송신이다.
            sunProcess: (sid, type) => { this.#live(); const process = c.processes.fromForm(findAttachedProcess(c.pool, sid, type)); if (!(process instanceof SunAviaryProcess)) throw new Error('착륙 대상의 실제 훨리베이스 프로세스 미연결'); return process.fields; }, // 현재 같은48BYTE다.
            refreshSun: (fields) => { this.#live(); const process = c.processes.fromForm(fields.form); if (!(process instanceof SunAviaryProcess) || process.fields !== fields) throw new Error('착륙 훨리베이스 Host/raw 이미지 불일치'); c.processes.refreshForm(process); }, // 같은 생산기 갱신이다.
            homeNotice: (home, first, second) => { // 독립 typed의 실제 두 홈 가상34만 고른다. 두 DWORD 인자는 원문이 무시한다.
                this.#live(); if (!c.processState.server) return; const table = read(c.pool.slot(home), 0); if (table === 0x509930) { o.sunAviary.flyerDied(home); return; } if (table !== 0x5099e8) throw new Error('다른 실제 플라이어 홈 가상 통지 미복원');
                const process = c.processes.fromForm(findAttachedProcess(c.pool, home, o.rainAviaryType)); if (!(process instanceof RainAviaryProcess)) throw new Error('맨 오 워 홈 통지의 현재 프로세스 NULL 원본 경계');
                o.rainAviary.flyerDied(process.fields, o.random, (fields) => { const current = c.processes.fromForm(fields.form); if (!(current instanceof RainAviaryProcess) || current.fields !== fields) throw new Error('맨 오 워 홈 통지의 Host/raw 이미지 불일치'); c.processes.refreshForm(current); });
            },
            runCount: (sid) => this.#runCount(sid), launch: (source, target, filename) => { this.#live(); if (filename) throw new Error('맨 오 워 빔의 비영 파일명 주소 미복원'); o.particles.addBeam(source, target, null); }, // 같은 무음 빔이다.
            sound: (x, y, token) => { this.#live(); need(o.services.sound.playOnceAt, '맨 오 워 실제 발사 소리 미연결')(x, y, token, 0, 0); }, // 현재 단발 위치 소리다.
        };
        this.frameTypes = o.gunTypes.map((type, i) => ({ range: type.range, sound: o.missileTypes[i].firingSound }));
        const sun = { assertion: hooks.assertion, robust: hooks.sidRobust, destroy: hooks.destroy, unpop: hooks.unpop, pop: hooks.pop, repop: hooks.repop, mayTarget: hooks.mayTarget, sound: hooks.sound, runCount: hooks.runCount,
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); }, baseFrame: (fields) => { this.#live(); o.flyerFrame.baseFrame(fields); }, baseNotify: (fields, message, a, b) => { this.#live(); o.flyerFrame.baseNotify(fields, message, a, b); }, baseParentDying: (fields) => { this.#live(); o.flyerFrame.parentDying(fields); } }; // 같은 실제 가상/공통 부모를 공유한다.
        this.bombTypes = c.frames.map((table, i) => ({ fallingFrames: table.run('A').count, explosionFirst: table.run('B').first, explosionEnd: table.run('B').count, sound: o.missileTypes[i].firingSound })); // 원문+174는count이며first+count로 바꾸지 않는다.
        this.frameHooks = hooks; this.sunHooks = sun;
        this.frameGlobals = { server: () => c.processState.server, boss: () => s.assetState.readBoss(), checking: captureCampaignFlag(s.gunState, 'checking'), now: () => c.processState.now, frame: () => s.soundState.frame, sunType: () => o.sunFlyerType, windType: () => o.windFlyerType, rainType: () => o.rainFlyerType, sunAviaryProcess: () => o.sunAviaryType, soundProcess: () => s.soundState.currentType() };
        this.sunGlobals = { boss: () => s.assetState.readBoss(), paused: () => o.flyerPaused, now: () => c.processState.now, bombType: () => o.sunBombType };
        o.flyerFrame = new RawFlyerFrame(c.pool, c.types, this.frameTypes, s.soundState.typeFrames.subarray(0, c.types.length), this.frameGlobals, hooks); o.sunFlyerFrame = new RawSunFlyerFrame(c.pool, c.shapes, this.bombTypes, this.sunGlobals, sun);
        const damage = o.hits.damageHooks({}).damage;
        const wind = { runCount: (sid) => this.#runCount(sid), assertion: (line) => { this.#live(); throw new Error(`Wind Flyer 실제 진단 미연결 ${line}`); }, damageable: (sid, event, count, value) => { this.#live(); return o.assetBindings.regular.damageableHandle(sid, event, count, value); }, destroy: hooks.destroy, sound: hooks.sound, mayTarget: hooks.mayTarget,
            damageRate: (sid, target) => { this.#live(); const type = o.hitTypes[c.pool.slot(sid)[10]]; return type.specialFlyerDamage && (c.types[c.pool.slot(target)[10]].flags2 & 0x120000) ? type.flyerDamagePerSecond : type.damagePerSecond; }, // 같은 현재 표의 대공 피해다.
            damage: (target, amount, owner, source) => { this.#live(); need(damage, 'Wind Flyer 현재 피해 미연결')(target, amount | 0, owner & 0xff, source); }, weaken: (sid) => { this.#live(); o.damageableModules.preparePost(); o.damageablePost.weaken(sid); } }; // 현재 피해 함수 값과 후처리 준비다.
        this.windActiveSounds = new Uint32Array(c.types.length);
        // 같은 이름표로 원본TYPE+6c의 활성 소리를 읽는다.
        for (const asset of o.assets.game.types()) { const name = typeString(asset.definition, 'activeSound'); this.windActiveSounds[70 + asset.block] = name ? o.soundNames.lookup(name) : 0; }
        this.windHooks = wind; this.windGlobals = { now: () => c.processState.now, boss: () => s.assetState.readBoss(), frame: () => s.soundState.frame };
        o.windFlyer = new RawWindFlyer(c.pool, c.hash, c.types, s.soundState.typeFrames.subarray(0, c.types.length), this.windActiveSounds, this.windGlobals, wind);
        this.#bindTarget();
    }

    // 실제 일반 finder·watcher/Host 부모와 현재 같은 표적 문맥을 연결한다.
    #bindTarget() {
        const o = this.#owner; const c = o.core; const s = o.state;
        const hooks = { assertion: (line) => { this.#live(); throw new Error(`Flyer Target 실제 진단 미연결 ${line}`); }, checkForms: (sid) => { this.#live(); return o.flyerFrame.checkForms(sid); }, offset: (sid) => { this.#live(); return this.#offset(sid, false); },
            begin: (area, flags) => { this.#live(); this.#finder = new RawSquidFinder(c.pool, c.hash, c.types); return this.#finder.begin(area, flags); }, next: () => { this.#live(); return need(this.#finder, '비행 표적 일반 finder 커서 누락').next(); }, // 같은 단일 커서의 수명이다.
            mayTarget: (sid, target) => this.#mayTarget(sid, target), watcher: (form) => { this.#live(); return c.dependencies.watcher(form); }, processParent: (form) => { this.#live(); return need(c.processes.fromForm(form), '비행 경쟁의 현재 Host 프로세스 조회 실패').parent; }, // 같은 공간/의존/Host다.
            returnDelay: (fields) => { this.#live(); return RawFlyerProcess.returnDelay(fields); }, command: (parent, command) => { this.#live(); o.broadcastFlyerCommand(parent, command); } }; // 현재 전용 표와 방송을 공유한다.
        this.targetRanges = o.gunTypes.map((type) => type.range); this.targetHooks = hooks;
        this.targetGlobals = { server: () => c.processState.server, checking: captureCampaignFlag(s.gunState, 'checking'), now: () => c.processState.now, sunType: () => o.sunFlyerType, processEnd: () => o.gameplayGlobals.gameAux().processTypeEnd, dependType: () => o.flyerDependType };
        o.flyerTarget = new RawFlyerTarget(c.pool, c.types, this.targetRanges, this.targetGlobals, hooks);
    }

    // 반납한 이전 Session의 현재 World에 재진입하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Flyer'); }
    // 상위가 Host를 비운 뒤 등록/미부착 예약과 finder를 반납한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; const o = this.#owner; o.pendingFlyer = null; o.pendingSunFlyerSound = null; o.pendingWindRegular = null; this.#finder = null; if (this.#registered) { o.core.postPop.setBody('flyer', null); this.#registered = false; } }
    // 모든 내부 생성 구간의 정상 복귀이며 실제 World/Load 완료는 아니다.
    get ready() { return this.#ready; }
}
