// 생성자1665..1667의 Sun/Wind/RainAviary 결선이다(225553df, WB04-E609).
// 사용: connectCarrierRegular 뒤 bindSun → bindWind → bindRain. 공중3종 전용 실행 없이 같은 현재 Host/상태를 구성한다.
import { RawSunAviaryProcess, SunAviaryProcessFields, SunAviaryProcess } from './raw-sun-aviary-process.js';
import { RawWindAviaryProcess, WindAviaryProcess, findAttachedProcess } from './raw-wind-aviary-process.js';
import { WindAviaryProcessFields } from './raw-wind-aviary-state.js';
import { RawRainAviaryProcess, RainAviaryProcessFields, RainAviaryProcess } from './raw-rain-aviary-process.js';
import { FlyerProcessFields, FlyerProcess, specializeWind } from './raw-flyer-process.js';
import { windCirclePosition } from './raw-wind-circle.js';

// 선택된 실제 하위만 요구한다.
function need(value, message) { if (!value) throw new Error(message); return value; }
// 현재 speed의 float 비트를 수치 정수 변환 없이 보낸다.
function floatBits(value) { const raw = new DataView(new ArrayBuffer(4)); raw.setFloat32(0, value, true); return raw.getUint32(0, true); }

// 세 생산기가 빌리는 값형 훅·현재 전역과 원문 생성 순서를 보관한다.
export class RawCampaignSessionAviaries {
    // 부분 실패/중복/반납과 각 원문 호출의 정상 복귀를 구별한다.
    #owner; #sunStarted = false; #sunReady = false; #windStarted = false; #windReady = false; #rainStarted = false; #ready = false; #disposed = false;
    // 현재 owner를 읽는 전역 표와 세 값형 효과 표다.
    sunGlobals = null; windGlobals = null; rainGlobals = null; sunHooks = null; windHooks = null; rainHooks = null;

    // 생성 자체로 자산/생산 프로세스·난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 각 원문 struct의 참조 필드를 같은 owner의 현재 값으로 보충한다.
    #globals(process, flyer) {
        const o = this.#owner; const c = o.core;
        return {
            fort: () => c.ownerMode.fort, friendlyFire: () => o.gameplayGlobals.gameAux().missionSettings()[3], // RawCheckingFlag 사본도 같은 owner다.
            server: () => c.processState.server, boss: () => o.state.assetState.readBoss(), editor: () => o.globals.editor, // 서로 다른 현재 권한을 유지한다.
            checking: () => o.gameplayGlobals.aiDebug().checking().value, now: () => c.processState.now, // 같은 시계/진단 owner다.
            processType: () => o[process], flyerType: () => o[flyer], aviaryType: () => o.windAviaryAsset, // 후속 변경도 읽는 같은 역할 전역이다.
        };
    }

    // 세 원문이 공유하는 현재 World/표시/소유권/위치 소리를 값형 closure로 구성한다.
    #hooks(kind, duplicateLine, duplicateExpression) {
        const o = this.#owner; const c = o.core;
        return {
            findProcess: (parent, type) => { this.#live(); return findAttachedProcess(c.pool, parent, type); }, // 현재 종속 체인의 첫 실제 프로세스다.
            damageable: (parent, flags) => { this.#live(); o.carrier.damageablePostPop(parent, flags); }, // 기존 실제 공통 후처리다.
            assertion: (line) => { this.#live(); if (line !== duplicateLine) throw new Error(`${kind} 실제 진단 미연결 ${line}`); const assertion = need(o.services.ai.assertion, `${kind} PostPop 중복 진단 장치 미연결`); assertion(duplicateExpression); }, // 원문 중복 진단의 정상 복귀 뒤 계속할 수 있다.
            setFrame: (sid, frame) => { this.#live(); c.frame.set(sid, frame, 0); }, // 실제 프레임/표시 효과다.
            sound: (x, y, name) => { this.#live(); need(o.services.sound.playOnceAt, `${kind} 실제 위치 음향 미연결`)(x, y, o.soundNames.lookup(name), 0, 0); }, // 현재5인자 단발 위치 음향이다.
            checkFrame: (sid, frame, line) => { // 현재 TYPE·코드 수와 복합 프레임 배수로 선택된 개발자 진단을 판정한다.
                this.#live(); const type = c.pool.slot(sid)[10]; if (type >= c.frames.length || type >= c.types.length) throw new RangeError(`${kind} 현재 프레임 타입 오류`);
                const count = c.frames[type].codes.length; const multiplier = (c.types[type].flags1 & 0x440000) ? 2 : 1;
                if (frame < 0 || frame >= count * multiplier) throw new Error(`${kind} 실제 프레임 진단 미연결 ${line}`);
            },
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); }, // 같은 가상 생성자를 사용한다.
            setOwner: (sid, owner) => { this.#live(); o.ownership.setOwner(sid, owner); }, // 같은 장부/표시를 사용한다.
            pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); }, // 같은 현재 공간이다.
            bindFlyer: (sid, home) => { this.#live(); need(o.flyer, '현재 실제 Flyer 결선 전 호출').bind(sid, home); }, // 뒤1669에서 설치하는 같은 몸체다.
        };
    }

    // ConnectSunAviary2074..2103의 실제48BYTE/Host·현재 홈 통지 결선이다.
    bindSun() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.carrierRegularModules?.ready || this.#sunStarted || o.sunAviary) throw new Error('Session SunAviary의 초기화 전/반복 결선'); this.#sunStarted = true;
        const hooks = this.#hooks('SunAviary', 0x1e2, '!findProcess( getSid(), sunAviaryProcessType, 0 )');
        hooks.allocate = (size) => { this.#live(); if (size !== 0x30) throw new RangeError('훨리베이스 실제 할당 크기 오류'); return new SunAviaryProcessFields(); }; // JS 신규 메모리를 만들며 원문 생성자가 미정 칸을 덮지 않는다.
        hooks.attach = (fields, parent, type, flags) => { this.#live(); if (!fields) throw new Error('SunAviary 실제 할당 실패'); c.processes.attach(new SunAviaryProcess(o.sunAviary, fields), type, parent, flags); }; // 같은 실제 Kernel 래퍼다.
        hooks.refreshForm = (fields) => { this.#live(); const process = c.processes.fromForm(fields.form); if (!(process instanceof SunAviaryProcess) || process.fields !== fields) throw new Error('훨리베이스 현재 Host/raw 이미지 불일치'); c.processes.refreshForm(process); }; // 같은 객체의 현재 Form만 갱신한다.
        hooks.processFields = (form) => { this.#live(); const process = c.processes.fromForm(form); return process instanceof SunAviaryProcess ? process.fields : null; }; // 같은 Host의48BYTE만 빌린다.
        hooks.unpop = (sid, flags) => { this.#live(); o.world.unpop(sid, flags); }; // 직접 프레임 쓰기 전 분리다.
        hooks.repop = (sid, flags) => { this.#live(); o.world.repop(sid, flags); }; // 직접 프레임 쓰기 후 같은 위치 복원이다.
        this.sunHooks = hooks; this.sunGlobals = this.#globals('sunAviaryType', 'sunFlyerType'); o.sunAviary = new RawSunAviaryProcess(c.pool, this.sunGlobals, hooks); this.#sunReady = true;
    }

    // ConnectWindAviary2034..2073의 실제80BYTE·원주 헤더와 후설치 비행 몸체 결선이다.
    bindWind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !this.#sunReady || this.#windStarted || o.windAviary) throw new Error('Session WindAviary의 초기화 전/반복 결선'); this.#windStarted = true;
        const hooks = this.#hooks('WindAviary', 0x275, '!findProcess( getSid(), windAviaryProcessType, 0 )');
        delete hooks.sound; // 원문 Wind 훅에는 위치 소리가 없다.
        hooks.allocate = () => { this.#live(); return new WindAviaryProcessFields(); }; // 같은80BYTE 원문 이미지다.
        hooks.attach = (fields, parent, type) => { this.#live(); if (!fields) throw new Error('WindAviary 실제 할당 실패'); c.processes.attach(new WindAviaryProcess(o.windAviary, fields), type, parent, 8); }; // 원문flags8의 실제 Host 부착이다.
        hooks.create = (type) => { this.#live(); return o.world.create(type, 0); }; // Wind의 생성 flags는 고정0이다.
        hooks.pop = (sid, x, y) => { this.#live(); o.world.pop(sid, x, y, 0); }; // Wind의 Pop flags도 고정0이다.
        hooks.height = (sid, frame) => { this.#live(); const type = c.pool.slot(sid)[10]; return need(c.shapes[type]?.frames[frame], 'Wind 현재 프레임 SHP 범위 오류').walkStepY; }; // 현재 SHP+14의float다.
        hooks.footY = (sid) => { this.#live(); return Math.fround(c.types[c.pool.slot(sid)[10]].footY); }; // 현재 발자국을float로 바꾼다.
        hooks.createFlyerProcess = (sid, home) => { // 같은104BYTE를 확보한 뒤 후설치 몸체의 생성/부착과 전용표 쓰기 순서를 유지한다.
            this.#live(); const fields = new FlyerProcessFields(); if (o.pendingFlyer) throw new Error('더스트 데빌비행생성부착재진입');
            const body = need(o.flyer, '현재 실제 Flyer 결선 전 호출'); o.pendingFlyer = new FlyerProcess(body, fields); const current = o.pendingFlyer;
            body.construct(current.fields, sid, home, floatBits(need(o.dudeSpeeds[c.pool.slot(sid)[10]], 'Wind 현재 speed TYPE 범위').speed), 0.5); specializeWind(current.fields);
        };
        hooks.circlePosition = (parent, flyer, fields) => { // 같은 현재13번 SHP 헤더의 float와 signedWORD 두 개를 읽는다.
            this.#live(); return windCirclePosition(c.pool, c.types, parent, flyer, fields, (sid, unused, frame) => {
                const type = c.pool.slot(sid)[10]; if (type < 70 || frame < 0) throw new RangeError('Wind 원주 실제 헤더 범위');
                const bytes = o.assets.game.shapes.squidHeader(type - 70, frame); const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
                const shape = need(c.shapes[type]?.frames[frame], 'Wind 원주 현재 SHP 범위'); return { walkStepY: shape.walkStepY, height: view.getInt16(26, true), hotspotY: view.getInt16(30, true) };
            });
        };
        hooks.destroy = (sid, flags) => { this.#live(); o.world.destroy(sid, flags); }; // 현재 가상표의 실제 삭제다.
        hooks.finish = (fields) => { this.#live(); const bytes = fields.bytes; const form = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(60, true) & 0xffff; const process = c.processes.fromForm(form); if (!(process instanceof WindAviaryProcess) || process.fields !== fields) throw new Error('데빌메이커종료Host/raw 불일치'); c.processes.kill(process, 0); }; // 같은 현재80BYTE의 Host만 종료한다.
        this.windHooks = hooks; this.windGlobals = this.#globals('windAviaryType', 'windFlyerType'); o.windAviary = new RawWindAviaryProcess(c.pool, this.windGlobals, hooks); this.#windReady = true;
    }

    // ConnectRainAviary2105..2126의 별도48BYTE·프레임/73h 부모를 같은 Host에 구성한다.
    bindRain() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !this.#windReady || this.#rainStarted || o.rainAviary) throw new Error('Session RainAviary의 초기화 전/반복 결선'); this.#rainStarted = true;
        const hooks = this.#hooks('RainAviary', 0x257, '!findProcess( getSid(), rainAviaryProcessType, 0 )');
        hooks.allocate = (size) => { this.#live(); if (size !== 0x30) throw new RangeError('맨 오 워 풀 실제 할당 크기 오류'); return new RainAviaryProcessFields(); }; // 현재 별도48BYTE 이미지다.
        hooks.attach = (fields, parent, type, flags) => { this.#live(); if (!fields) throw new Error('RainAviary 실제 할당 실패'); c.processes.attach(new RainAviaryProcess(o.rainAviary, fields), type, parent, flags); }; // 같은 Kernel의 별도 래퍼다.
        this.rainHooks = hooks; this.rainGlobals = this.#globals('rainAviaryType', 'rainFlyerType'); o.rainAviary = new RawRainAviaryProcess(c.pool, this.rainGlobals, hooks); this.#ready = true;
    }

    // 이전 Session이 반납한 현재 World로 재진입하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Aviaries'); }
    // 실제 Host가 먼저 비워진 뒤 closure와 미부착 공유 Flyer 예약을 닫는다.
    dispose() { this.#disposed = true; this.#owner.pendingFlyer = null; }
    // Sun 구간의 정상 복귀다.
    get sunReady() { return this.#sunReady; }
    // Wind 구간의 정상 복귀다.
    get windReady() { return this.#windReady; }
    // 세 생성 구간을 모두 마쳤는지 확인한다.
    get ready() { return this.#ready; }
}
