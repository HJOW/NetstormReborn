// Session 생성자1427..1462의 소유자색·파편 캡처·입자·EvilRay 장치를 연결한다(225553df, WB04-E584).
// 사용: owner.connectParticles(). 전체 Load/표시 프레임·건물 캡처 설치는 후속 생성 구간이다.
import { buildStandardOwnerColorMaps } from '../assets/owner-colors.js';
import { RawCampaignParticles } from '../render/raw-campaign-particles.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../render/raw-shape-to-buffer.js';
import { drawShape } from '../render/vfx-draw.js';
import { RawBridgeDraw, createBridgeDrawState } from './raw-bridge-draw.js';
import { drawDevastationAdditional } from './raw-bomb-blast-bindings.js';
import { isPatchTemple } from './raw-temple-draw.js';
import { isPatchDais } from './raw-dais-draw.js';
import { isBurningArcherCapture } from './raw-burning-lifetime-draw.js';
import { isPlainUnitCapture, vtableOf } from './raw-plain-unit-capture.js';
import { isPlainWorkshopCapture } from './raw-plain-workshop-capture.js';
import { isThunderCannonCapture } from './raw-thunder-cannon-capture.js';
import { isSunAviaryCapture } from './raw-sun-aviary-capture.js';
import { isGeyserCapture } from './raw-geyser-capture.js';
import { suppressSpellHelpFrame } from './spell-frame-visibility.js';
import { buildSquidFrameAssertionDetail } from './squid-display.js';

// RawCampaignDevastation.inc1..9의 네 자격 함수가 허용하는 같은16VT다. 실제 주문/효과는 실행하지 않는다.
const CONNECTED_BOMBS = new Set([0x503188, 0x5026a0, 0x502778, 0x502840, 0x502908, 0x502b70, 0x5029d0, 0x502aa8, 0x5030c0, 0x502448, 0x502370, 0x502510, 0x502ff8, 0x5025d8, 0x502e68, 0x502f30]);
// 같은16VT 자격을 파편과 현재 주문 수신 앞단이 공유한다. 원문 DWORD 주소는 비교 값일 뿐 실행하지 않는다.
export function connectedCampaignBombTable(table) { return CONNECTED_BOMBS.has(table >>> 0); }
// 현재 raw가 같은16VT 중 하나인지 확인한다. 숫자 주소를 함수로 실행하지 않는다.
function connectedBomb(raw) { return connectedCampaignBombTable(vtableOf(raw)); }
// 건설/파편이 공유하는 Session391의 정확한 건물 캡처 자격이다. 일반 표시 자격과 다른 TYPE 집합을 유지한다.
export function isPlainBuildingCapture(raw) {
    const type = raw[10]; const table = vtableOf(raw);
    return isPlainWorkshopCapture(raw) || (type >= 75 && type <= 77 && table === 0x501ea0) || ((type === 78 || type === 147) && table === 0x50e940) || (type === 79 && table === 0x50ea08) || (type === 139 && table === 0x509aa0) || (type === 142 && table === 0x50ead0) || (type === 149 && table === 0x50e668) || (type === 164 && table === 0x5076b8) || (type === 95 && table === 0x506e50) || (type === 157 && table === 0x506f08);
}
// 원문2015의 Flyer 자격이다. 생성/생산/행동 관측은 이 함수가 수행하지 않는다.
function flyer(raw) { const type = raw[10]; const table = vtableOf(raw); return (type === 144 && table === 0x50a388) || (type === 73 && table === 0x50a1e8) || (type === 138 && table === 0x50a050); }

// Impl703..710의 초기 멤버다. 지역 토큰/색표와 현재 캡처 옵션은 생성자 결선 전에 같은 owner가 소유한다.
export function createCampaignParticleTables() { return { shrapnelMapTokens: new Uint32Array(188), shrapnelOwnerColors: new Map(), bridgeDrawState: createBridgeDrawState(), shrapnelCaptureState: createShapeToBufferState() }; }

// 같은 World·팔레트·장기 카메라·공유 난수를 빌리는 생성 구간이다. 실패 접두는 다시 실행하지 않는다.
export class RawCampaignSessionParticles {
    // 실제 기반 owner와 한 번 실행/정상 복귀/반납 상태다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문 지역 장치 묶음이다. 실제 입자 시스템이 같은 callback을 값 인수로 보관한다.
    shrapnel = null; evilDevices = null; beamSound = null;
    // 기반 owner만 빌린다. 실제 모듈 생성은 bind에서 원문 순서를 따른다.
    constructor(owner) { this.#owner = owner; }
    // Gun 몸체 뒤 소유자색·실제 캡처·입자·EvilRay 장치를 한 번 연결한다.
    bind() {
        const o = this.#owner; const c = o.core; const state = o.shrapnelCaptureState;
        if (this.#disposed || !o.gunModules?.ready || this.#started) throw new Error('Session 입자의 초기화 전/반복 결선');
        this.#started = true;
        // 원문43be30의 세 표만 일반 소유자 색을 설치한다. TYPE별 토큰은 같은188칸이다.
        for (const type of [c.bridgeState.bridgeType, 73, 144]) {
            // std::array::at의 범위 오류를 TypedArray의 조용한 쓰기 생략으로 바꾸지 않는다.
            if ((type >>> 0) >= o.shrapnelMapTokens.length) throw new RangeError('파편 소유자 색표 TYPE 물리 범위');
            o.shrapnelMapTokens[type] = 1; if (!o.shrapnelOwnerColors.has(type)) o.shrapnelOwnerColors.set(type, buildStandardOwnerColorMaps(type, c.bridgeState.bridgeType));
        }
        state.ownerTypeMaps = o.services.ownerColorTables ? o.services.ownerColorTables.tokens : o.shrapnelMapTokens;
        state.addTypeHotspot = true;
        // 현재 장기 소유자→색 번호를 매번 다시 읽고 같은 실제 색표를 빌린다.
        state.ownerColors = (type, owner) => {
            if (o.services.ownerColorTables) {
                if (owner >= c.bridgeState.ownerColors.length) throw new RangeError('공통 파편 캡처 owner 범위');
                const color = c.bridgeState.ownerColors[owner]; if (color < 0 || color > 8) throw new RangeError('공통 파편 캡처 색 번호');
                return o.services.ownerColorTables.color(type, color);
            }
            if (!o.shrapnelOwnerColors.has(type) || owner >= c.bridgeState.ownerColors.length) throw new RangeError('파편 캡처 현재 타입/owner 색표 오류');
            const color = c.bridgeState.ownerColors[owner]; if (color < 0 || color > 8) throw new RangeError('파편 캡처 현재 소유자 색 번호 오류');
            return o.shrapnelOwnerColors.get(type)[color];
        };
        o.bridgeDraw = new RawBridgeDraw(c.pool, o.bridgeDrawState);
        // Devastation은 빈 기본 그림(-1)이며 실제 추가 그림만 같은 버퍼에 그린다.
        state.mainFrame = (sid) => connectedBomb(c.pool.slot(sid)) ? -1 : o.bridgeDraw.mainFrame(sid);
        state.additional = (sid, image, x, y) => {
            if (connectedBomb(c.pool.slot(sid))) {
                drawDevastationAdditional(c.pool, sid, x, y, (type, frame, px, py) => {
                    if (type < 70 || frame < 0) throw new RangeError('Devastation 캡처 TYPE/프레임 범위');
                    if (suppressSpellHelpFrame(c.types[type], o.assets.game.types()[type - 70].definition, frame)) return;
                    drawShape(image.pixels, image.width, image.height, o.assets.game.shapes, type - 70, frame, px, py, { left: 0, top: 0, right: image.width - 1, bottom: image.height - 1 });
                }); return;
            }
            o.bridgeDraw.additional(sid, x, y);
        };
        o.shrapnelCapture = new RawShapeToBuffer(c.pool, c.types, c.shapes, o.assets.game.shapes, state);
        this.shrapnel = { crt: o.particleCrt, lifetime: {
            // 실제 World/Host와 같은 의존 목록을 쓰며 파편 전용 대체 풀은 만들지 않는다.
            create: (type, flags) => o.world.create(type >>> 0, flags >>> 0), pop: (sid, x, y, flags) => { o.world.pop(sid, Math.fround(x), Math.fround(y), flags >>> 0); }, depend: (form, dummy) => { c.dependencies.create(form, dummy); },
            capture: (sid) => this.capture(sid),
        }, destroyUnpaired: (sid) => { o.world.destroy(sid, 0); } };
        // 실제 SoundList의 파일명을 같은 현재 위치 재생 장치로 보낸다.
        this.beamSound = (x, y, name) => { if (typeof o.services.sound?.playOnceAt !== 'function') throw new Error('캠페인 빔 실제 파일명 음향 미연결'); o.services.sound.playOnceAt(Math.fround(x), Math.fround(y), o.soundNames.lookup(name), 0, 0); };
        o.particles = new RawCampaignParticles(c, o.random, o.assets.game.palette, { sound: this.beamSound, shrapnel: this.shrapnel, adjacentByKind: o.services.particlePaletteInput?.adjacentByKind });
        o.particles.bindCameraView(o.gameplayGlobals.camera().snapshot());
        const aux = o.gameplayGlobals.gameAux(); o.particles.bindTailProcessTypes(aux); o.state.buildArrivalState.sharedStreakProcessType = () => aux.streakProcessType;
        this.evilDevices = {
            // TYPE+70/+74의 현재 발사/충돌 소리를 읽는다. stepSound는 쓰지 않는다.
            soundId: (sid, end) => { const type = c.pool.slot(sid)[10]; if (type >= o.missileTypes.length) throw new RangeError('EvilRay 현재 음향 TYPE 물리 범위'); return end ? o.missileTypes[type].impactSound : o.missileTypes[type].firingSound; },
            // 장치는 호출 때 선택하므로 자기 callback에서 교체해도 이번 선택은 유지한다.
            sound: (x, y, sound) => { const selected = o.services.sound?.playOnceAt; if (typeof selected !== 'function') throw new Error('EvilRay 실제 위치 재생 장치 미연결'); selected(Math.fround(x), Math.fround(y), sound >>> 0, 0, 0); },
            frameError: (number, frame, abstract) => { o.world.storage.recordAssertionDetail(buildSquidFrameAssertionDetail(c.types, c.shapes, number, frame, abstract)); },
            assertion: (expression, _file, _line) => { const selected = o.services.ai?.assertion; if (typeof selected !== 'function') throw new Error('EvilRay 실제 Client 진단 장치 미연결'); selected(expression); },
        };
        o.particles.connectEvilRayDevices(this.evilDevices); this.#ready = true;
    }
    // 기존의 정확한 건물/유닛 전용 캡처를 현재 선택하며, 나머지는 원문의 Flyer/Bridge/16VT만 허용한다.
    capture(sid) {
        const o = this.#owner; const raw = o.core.pool.slot(sid);
        if (isPatchTemple(raw) || isPatchDais(raw) || isBurningArcherCapture(raw) || isPlainBuildingCapture(raw) || isPlainUnitCapture(raw) || isThunderCannonCapture(raw) || isSunAviaryCapture(raw) || isGeyserCapture(raw) || o.factoryDraw?.handles(sid)) {
            if (!o.buildingCapture) throw new Error('실제 건물/유닛 캡처 설치 전 호출'); return o.buildingCapture.capture(sid);
        }
        if (!flyer(raw) && !(raw[10] === o.core.bridgeState.bridgeType && vtableOf(raw) === 0x5034c8) && !connectedBomb(raw)) throw new Error(`다른 실제 파편 source의 전용 색표/추가 그림 미복원 TYPE${raw[10]}`);
        return o.shrapnelCapture.capture(sid);
    }
    // 부모가 World 프로세스를 먼저 정리한 뒤 입자 시스템을 원문 역순으로 한 번 반납한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; this.#owner.particles?.dispose(); }
    // 이 생성 구간만 정상 복귀했는지다. 전체 Session/Load 완료와 구별한다.
    get ready() { return this.#ready; }
}
