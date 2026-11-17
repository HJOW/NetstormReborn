// ConnectBuilding3668..3771·ReceiveGeyserMined3772..3776과 공통 신전 그림3659..3667이다(225553df, WB04-E639).
// 사용: 생성자 조립 뒤 bind. 같은 World/Host·현재 색표/프레임·HP·음향을 빌리고 Load 완료를 만들지 않는다.
import { buildStandardOwnerColorMaps, buildSpecialOwnerColorMaps, buildSpecialOwnerColorMapsFromLightness } from '../assets/owner-colors.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../render/raw-shape-to-buffer.js';
import { drawShape } from '../render/vfx-draw.js';
import { RawFactoryDraw } from './raw-factory-draw.js';
import { isPlainBuildingCapture } from './raw-campaign-session-particles.js';
import { isPlainUnitCapture } from './raw-plain-unit-capture.js';
import { isPatchDais, drawDaisAdditional, PATCH_DAIS_EXTRA_TYPE } from './raw-dais-draw.js';
import { isPatchTemple, drawWindThunderTemple } from './raw-temple-draw.js';
import { isRainVortex, drawRainVortex } from './raw-rain-vortex-draw.js';
import { isGeyserCapture, drawGeyserCapture } from './raw-geyser-capture.js';
import { isSunAviaryCapture, sunAviaryCaptureMainFrame, drawSunAviaryCapture } from './raw-sun-aviary-capture.js';
import { isThunderCannonCapture, thunderCannonCaptureMainFrame, drawThunderCannonCapture } from './raw-thunder-cannon-capture.js';
import { isBurningArcherCapture, drawBurningArcherCapture } from './raw-burning-lifetime-draw.js';
import { RawConstructionEffects } from './raw-construction-effects.js';
import { RawConstructionProcess } from './raw-construction-process.js';
import { RawConstructionData } from './raw-construction-data.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { checkRawFormLinked, readRawProcessFormParent } from './raw-form-linked.js';
import { writeDropIslandPacket } from './raw-battle-broadcast.js';
import { ShrapnelKind } from './raw-shrapnel-frame.js';
import { captureCampaignFlag } from './raw-campaign-session-state.js';

// 현재 프레임 DWORD는 unsigned로 읽고 선택한 실제 SHP 범위에서 검사한다.
function dword(raw, at) { return new DataView(raw.buffer, raw.byteOffset + at, 4).getUint32(0, true); }
// 원본 선택 위치에서만 필수 음향/UI를 요구한다.
function need(device, label) { if (typeof device !== 'function') throw new Error(label + ' 실제 장치 미연결'); return device; }
// 실제 TYPE의 첫 대소문자 무시 문자열 속성만 읽는다. 없는 속성은 원문 optional 부재다.
function typeString(owner, type, key) { return owner.assets.game.types()[type - 70].definition.properties.find((p) => p.name.toLowerCase() === key.toLowerCase())?.value; }

// 같은 전용 신전 몸체/현재 프레임과 조건부 상승 음향을 Renderer와 캡처가 공유한다.
export function drawCampaignTempleFrames(owner, sid, frame) {
    const c = owner.core, raw = c.pool.slot(sid);
    if (isRainVortex(raw)) {
        drawRainVortex(raw, c.frames[raw[10]], { frame, ascendSound: () => { need(owner.services.sound.play, '레인 신전 상승음')(owner.soundNames.lookup('templeAscend.wav'), 0, 0, 0, 0, 0); } }); return;
    }
    if (c.pool.edition !== 'patch1078' || !isPatchTemple(raw)) throw new Error('다른 판본/가상표 신전 Draw 미복원');
    drawWindThunderTemple(raw, c.frames[raw[10]], owner.assets.game.types()[raw[10] - 70].definition.baseFrame, frame);
}

// 실제 Impl의 생성 시0인 건설 색표/캡처 멤버들이다. bind 전에는 캡처 장치를 게시하지 않는다.
export function createCampaignBuildingTables() {
    return { buildingMapTokens: new Uint32Array(188), buildingOwnerColors: [], buildingSpecialOwnerColors: new Map(), buildingCaptureState: createShapeToBufferState() };
}

// 같은 기존 Host에 실제30/17 수신/효과와 건설·연소 캡처를 연결하는 짧은 Session 장치다.
export class RawCampaignSessionBuilding {
    // 원본 owner와 반복/반납 관문이다. 이미 실행한 실패 접두를 재사용하지 않는다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 생성 자체로 프로세스나 그림을 만들지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 반납한 callback이 다음 World의 현재 전역을 빌리지 못하게 한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 건설 처리기'); }

    // 기존 생성자의 조립 뒤 같은 실제 캡처/입자/효과/프로세스/decoder 순서로 설치한다.
    bind() {
        const o = this.#owner, c = o.core, s = o.state, capture = o.buildingCaptureState;
        if (this.#disposed || this.#started || o.building || !o.aiProductionModules?.ready) throw new Error('Session 건설의 초기화 전/반복 결선');
        this.#started = true; o.buildingOwnerColors = buildStandardOwnerColorMaps(0, c.bridgeState.bridgeType); capture.addTypeHotspot = true;
        // 원문 일반 색표와 전용 색표의 현재188칸 토큰이다. 뒤 전용 대상이 같은 칸을 덮는다.
        for (const type of [82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126]) o.buildingMapTokens[type] = 1;
        for (const type of [82, 155, 111, 141, 137, 94, 97]) o.buildingMapTokens[type] = 2;
        capture.ownerTypeMaps = o.services.ownerColorTables ? o.services.ownerColorTables.tokens : o.buildingMapTokens;
        capture.ownerColors = (type, owner) => { this.#live(); return this.#ownerColors(type, owner); };
        o.factoryDraw = new RawFactoryDraw(c.pool, c.frames);
        capture.mainFrame = (sid) => {
            this.#live(); const raw = c.pool.slot(sid);
            if (isPatchDais(raw)) return -1;
            if (isSunAviaryCapture(raw)) return sunAviaryCaptureMainFrame(raw);
            if (isThunderCannonCapture(raw)) return thunderCannonCaptureMainFrame(raw, c.frames[raw[10]]);
            if (isPatchTemple(raw) || isBurningArcherCapture(raw) || isPlainBuildingCapture(raw) || isPlainUnitCapture(raw) || isGeyserCapture(raw)) return o.assets.game.types()[raw[10] - 70].definition.baseFrame;
            return o.factoryDraw.mainFrame(sid);
        };
        capture.additional = (sid, image, x, y) => { this.#live(); this.#additional(sid, image, x, y); };
        o.buildingCapture = new RawShapeToBuffer(c.pool, c.types, c.shapes, o.assets.game.shapes, capture);
        o.particles.connectConstruction((sid) => { this.#live(); return o.buildingCapture.capture(sid); }, (sid) => o.hp.maxHitPoints(sid));
        const victim = { // RawCheckingFlag 복사는 현재 pointer/지역 입력을 캡처한다. 이후 원본 필드 대입과 구별한다.
            checking: captureCampaignFlag(s.gunState, 'checking'), robust: (sid, live) => o.flyerModules.currentSidRobust(sid, live),
            assertion: (condition, file, line) => { if (o.services.ai.assertion) o.services.ai.assertion(condition); else throw new Error(`원문 연소 진단 ${file}:${line}: ${condition}`); },
        };
        o.particles.connectBurning((sid) => { this.#live(); return o.buildingCapture.capture(sid); }, (sound) => { if (sound) need(o.services.sound.stop, 'Burning 소리 정지')(sound); }, victim);
        const effects = { // 현재 공간/경로/가상 낙하·방송을 실제 World/사제/입자와 연결한다.
            pathType: (type) => o.priests.movementProcessType(type),
            clearPath: (sid, type) => { new RawContainedLookup(c.pool, (target, flags) => o.world.destroy(target, flags)).find(sid, type, 0, true); },
            unpop: (sid, flags) => o.world.unpop(sid, flags), pop: (sid, x, y, flags) => o.world.pop(sid, x, y, flags),
            fall: (sid) => { o.particles.addShrapnel(sid, ShrapnelKind.Variant2); },
            sound: (x, y) => { need(o.services.sound.playOnceAt, '섬 낙하 위치 음향')(x, y, o.soundNames.lookup('islandFall.wav'), 0, 0); },
            destroy: (sid, flags) => { o.world.destroy(sid, flags); }, notify: (sid) => { o.priests.fallWalker(sid); },
            broadcast: (x, y, flags, owner) => { const packet = new Uint8Array(20); writeDropIslandPacket(packet, x, y, flags, owner); o.tables.broadcast.send(packet); },
            assertion: (expression, line) => o.world.storage.assertDisplay(expression, '\\Ns\\O\\Islanddropper.cpp', line),
        };
        o.buildingEffects = new RawConstructionEffects(c.pool, c.hash, c.types, c.spots, s.buildingEffectsState, effects);
        s.buildingState.checking = captureCampaignFlag(s.gunState, 'checking');
        const hooks = { // 실제 Host가 Form/PID를 확보한다. source의 reserve 두 장치는 정상 true다.
            formRobust: (form) => checkRawFormLinked(c.pool, form, (current) => readRawProcessFormParent(c.pool, current)),
            assertion: (condition, file, line) => { if (o.services.ai.assertion) o.services.ai.assertion(condition); else throw new Error(`원문 건설 Form 진단 ${file}:${line}: ${condition}`); },
            reserve: () => true, reserveSound: () => true,
            effect: (sid, instant) => { o.particles.addConstruction(sid, instant); }, loadSound: (name) => o.soundNames.lookup(name),
            attachSound: (sid, sound, alternate, flags) => { if (!o.sounds.add(sid, sound, alternate, flags)) throw new Error('건설 실제 Sound47 부착 실패'); },
            refresh: () => need(o.services.construction.refresh, '건설 화면 갱신')(), maxHitPoints: (sid) => o.hp.maxHitPoints(sid), updateDisplay: (sid) => c.display.update(c.pool.slot(sid)),
            removeContained: (sid, type) => { new RawContainedLookup(c.pool, (target, flags) => o.world.destroy(target, flags)).find(sid, type, 0, true); },
            playAt: (x, y, name, loop) => { o.services.sound.playOnceAt(x, y, o.soundNames.lookup(name), loop, 0); },
            readySound: (type) => { if (type < 70 || type - 70 >= o.assets.game.types().length) throw new RangeError('건설 준비 소리 TYPE 오류'); const name = typeString(o, type, 'buildDoneSound'); return name && name.length ? o.soundNames.lookup(name) : 0; },
            play: (sound) => { o.services.sound.play(sound, 0, 0, 0, 0, 0); }, unpop: (sid, flags) => o.world.unpop(sid, flags), repop: (sid, flags) => o.world.repop(sid, flags),
            push: (x, y, type, owner) => { o.buildingEffects.displace(x, y, type, owner); },
        };
        o.building = new RawConstructionProcess(c.pool, c.types, s.buildingState, hooks, c.processes);
        o.buildingData = new RawConstructionData(c.pool, s.buildingState, { logic: o.building, host: c.processes, assertion: (expression, file, line) => o.world.storage.assertDisplay(expression, file, line) });
        this.#ready = true;
    }

    // 같은 현재 소유자→색 번호와 전용 표를 선택한다. primary가 없으면 COL 성공으로 대체하지 않는다.
    #ownerColors(type, owner) {
        const o = this.#owner, colors = o.core.bridgeState.ownerColors;
        if (owner >= colors.length) throw new RangeError('실제 캡처 현재 소유자 번호 오류');
        const color = colors[owner]; if (color < 0 || color > 8) throw new RangeError('건설 현재 소유자 색 번호 오류');
        if (o.services.ownerColorTables) return o.services.ownerColorTables.color(type, color);
        if (type >= o.buildingMapTokens.length) throw new RangeError('건설 캡처 TYPE 색표 범위');
        if (o.buildingMapTokens[type] === 1) return o.buildingOwnerColors[color];
        if (o.buildingMapTokens[type] !== 2 || ![111, 141, 137, 94, 97].includes(type)) throw new Error('현재 캡처 전용 소유자 색표 미복원');
        if (!o.buildingSpecialOwnerColors.has(type)) {
            let maps; // C++가 선택한 같은 primary를 계산 전체에서 보관한다. 항목마다 현재 포인터를 다시 읽지 않는다.
            if (o.services.colorSortInput) {
                const primary = o.services.colorSortInput.primary;
                maps = buildSpecialOwnerColorMapsFromLightness(Float64Array.from({ length: 256 }, (_, index) => primary.lightness(index)), type);
            } else maps = buildSpecialOwnerColorMaps(o.assets.game.palette.logical, type);
            o.buildingSpecialOwnerColors.set(type, maps);
        }
        return o.buildingSpecialOwnerColors.get(type)[color];
    }
    // 같은 캡처 버퍼/클립에 실제 SHP를 그린다. 음수 프레임을 자동 보정하지 않는다.
    #draw(image, type, frame, x, y) {
        if (frame < 0) throw new RangeError('현재 캡처 음수 프레임');
        drawShape(image.pixels, image.width, image.height, this.#owner.assets.game.shapes, type - 70, frame, x, y, { left: 0, top: 0, right: image.width - 1, bottom: image.height - 1 });
    }
    // 같은 source 자격별 원문+a4를 분배한다. 일반 유닛/건물의 빈ret8도 원문 분기 그대로다.
    #additional(sid, image, x, y) {
        const o = this.#owner, c = o.core, raw = c.pool.slot(sid);
        if (isPatchDais(raw)) { if (raw[40] & 16) throw new Error('추상 다이스 실제 캡처 색상 미연결'); drawDaisAdditional(raw, c.shapes[PATCH_DAIS_EXTRA_TYPE].frameCount, (type, frame) => this.#draw(image, type, frame, x, y)); return; }
        if (isPatchTemple(raw)) { if (raw[40] & 16) throw new Error('추상 신전 실제 캡처 색상 미연결'); const type = raw[10]; drawCampaignTempleFrames(o, sid, (frame) => this.#draw(image, type, frame, x, y)); return; }
        if (isPlainBuildingCapture(raw) || isPlainUnitCapture(raw)) return;
        const draw = (type, frame, px, py) => this.#draw(image, type, frame, px, py); // 각 원문 추가 그림이 선택한 같은 캡처 주소다.
        if (isGeyserCapture(raw)) { if (raw[40] & 16) throw new Error('추상 간헐천 실제 캡처 색상 미연결'); drawGeyserCapture(raw, draw, x, y); return; }
        if (isSunAviaryCapture(raw)) { if (raw[40] & 16) throw new Error('추상 훨리베이스 실제 캡처 색상 미연결'); drawSunAviaryCapture(raw, draw, x, y); return; }
        if (isThunderCannonCapture(raw)) { if (raw[40] & 16) throw new Error('추상 썬더 캐논 실제 캡처 색상 미연결'); drawThunderCannonCapture(raw, draw, x, y); return; }
        if (isBurningArcherCapture(raw)) {
            if (raw[40] & 16) throw new Error('추상 발사기 실제 캡처 색상 미연결'); const frame = dword(raw, 36), shape = c.shapes[raw[10]];
            if (frame >= shape.frames.length) throw new RangeError('발사기 캡처 현재 SHP 프레임'); const header = shape.frames[frame];
            drawBurningArcherCapture(raw, o.assets.game.types()[raw[10] - 70].definition.baseFrame, header.muzzleX, header.muzzleY, draw, x, y); return;
        }
        o.factoryDraw.additional(sid, x, y, (px, py, type, frame) => {
            const shape = c.shapes[type]; if (!shape.loaded) return;
            if (frame < 0 || frame >= shape.frameCount) throw new Error('Factory 추가 그림의 현재 프레임 원본 진단');
            if (o.buildingCaptureState.zoom !== 65536) throw new Error('Factory 추가 그림의 비단위 배율 미연결'); this.#draw(image, type, frame, px, py);
        });
    }
    // 해당 수신 위치에서 현재 RawCheckingFlag를 복사한 뒤 기존 실제 handler를 호출한다.
    receiveGeyserMined(packet) { this.#live(); const o = this.#owner; o.state.geyserMinedState.checking = captureCampaignFlag(o.state.gunState, 'checking'); o.geyserMined.handlePacket(packet); }
    // 부모가 World 프로세스를 먼저 비우고 같은 현재 callback의 대여만 무효화한다.
    dispose() { this.#disposed = true; }
    // 이 설치 구간만 완료됐음을 알린다. 전체 Load 성공과 구별한다.
    get ready() { return this.#ready; }
}
