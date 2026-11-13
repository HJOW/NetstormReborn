// 캠페인(세션) 하나의 입자 표시 수명을 모으는 본체이다(cpppj `client/RawCampaignParticles.cpp/.h`, 10.78 전용). 이미 이식한 `core/` 의 여덟 표시 시스템(불꽃 꼬리·건설 마나 줄기·번개 빔·연기·악마 광선·파편·분해·연소·건설)을
// 같은 월드의 raw 풀/Host/Kernel 과 하나의 공통 표시 목록(`RenderProcessSystem`)에 묶고, 팔레트 색 띠·dirty 통지·VFX 선/타원/픽셀 출력·위치 음향을 현재 클라이언트 경계로 연결한다.
// 게임 계산은 바꾸지 않으며(원본 몸체는 각 시스템이 소유) 이 모듈은 장치 경계(색 표 메모리, 8비트 표면, dirty 큐, 음향, 카메라, 현재 TYPE 전역)만 공급한다.
//  - new RawCampaignParticles(core, random, palette, options): core 는 `RawSurfaceWorld.core()`(pool·types·shapes·processes·neighbors 사용), random 은 게임 난수(`GameRandom`), palette 는 `GamePalette`(`logical` 256 DWORD).
//    options = { sound(x, y, 파일명)?, shrapnel?: { lifetime: { create, pop, depend, capture }, destroyUnpaired(dummy), crt }, adjacent? }. shrapnel 이 있으면 파편·분해가 생기고 crt 는 필수이다.
//    adjacent 는 색 표 바로 뒤 인접 DWORD 의 명시 입력(기본 관찰값 1, `particle-palette-memory.js`)이다. core/random/palette 는 이 객체보다 오래 살아야 한다.
//  - add(11, 부모)·addStreak(부모)·addEvilRay(부모)·addBeam(부모, 표적, 파일명)·addScreenBolt(부모, 시작, 끝, 수명)·addShrapnel(원천, 종류)·addDissolve(원천, 첫째, 둘째, xOriented)·addConstruction(부모, instant)
//    ·addBurning(피해자, 모드)·addCooling(부모, 포구)·addSmoke(요청): 각 원본 생성자/래퍼를 같은 Host 에 부착한다. 확보 실패는 웹에 없으므로(reserve 는 항상 성공) null 이 되지 않는다.
//  - connectEvilRayDevices(장치)·connectConstruction(캡처, 최대 HP)·connectBurning(캡처, 정지 음향, 피해자 상태): 실제 장치를 첫 사용 전에 연결한다(중복/누락 거부). bindTailProcessTypes(aux)·bindWind(getter)·
//    bindWeakenedMana(getter)·bindHitPointTypes(aux)·bindCameraView(CameraView): 같은 클라이언트의 장기 전역을 빌린다(다른 소유자 재연결은 거부).
//  - prepareFrame(now, frameNumber, weakenedMana)·prepareBridgeClock(now, frameNumber): 전체 Kernel 실행 직전 현재 게임 시각을 공급한다. setView(view, display): 표시 영역/dirty 소비자(Renderer)를 연결한다.
//  - sprites(clip): 공통 표시 목록을 수집해 정렬 키와 직접 그리기 절차(`{ clip, order: { x, y, depth }, procedure: { bounds, draw }, database: null }`)로 돌려준다. collectDrawing/drawEntry 는 같은 수집/출력의 낮은 단계 진입이다.
//  - dispose(): 남은 효과를 선언 역순(악마 광선 → 연기 → 연소 → 분해 → 파편 → 건설 → 줄기 → 빔 → 불꽃)으로 정리한다(C++ 소멸자).
// 웹 범위: 실행 환경에 `bad_alloc` 이 없으므로 확보 실패(null 반환) 경로는 없다. 10.78 전용(10.72 분기 없음). 앱 수명에는 연결하지 않았다(PORT_MAP session 0). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E390: 5-A RawCampaignParticles 이식으로 추가)
import { DirtyRegions } from './dirty-regions.js';
import { drawVfxEllipse } from './vfx-ellipse.js';
import { drawVfxLinePixels } from './vfx-line.js';
import { projectCameraCoordinates } from './camera-view.js';
import { ParticlePaletteMemory, OBSERVED_ADJACENT_DWORD } from './particle-palette-memory.js';
import { ConstructionVisualSystem } from '../core/construction-visual-system.js';
import { BurningVisualSystem } from '../core/burning-visual-system.js';
import { DissolveVisualSystem } from '../core/dissolve-visual-system.js';
import { EvilRayVisualSystem } from '../core/evil-ray-visual-system.js';
import { FieryTailVisualSystem } from '../core/fiery-tail-visual-system.js';
import { FieryTailVariant } from '../core/raw-fiery-tail-lifetime.js';
import { LightningVisualSystem } from '../core/lightning-visual-system.js';
import { ShrapnelVisualSystem } from '../core/shrapnel-visual-system.js';
import { SmokeVisualSystem } from '../core/smoke-visual-system.js';
import { RenderProcessSystem, renderProcessRemovalBounds } from '../core/render-process-system.js';
import { RENDER_ENTRY_BYTES } from '../core/raw-render-collection.js';
import { createConstructionVisualState } from '../core/raw-construction-visual.js';
import { createConstructionFrameState } from '../core/raw-construction-frame.js';
import { createBurningFrameState } from '../core/raw-burning-frame.js';
import { createLightning, createLightningAtScreen } from '../core/raw-lightning-lifetime.js';
import { buildEvilRayPalette } from '../core/raw-evil-ray-lifetime.js';
import { buildSmokePalette } from '../core/raw-smoke-effect.js';
import { buildFieryTailPalette, buildLightningPalette, buildParticleGradient, mergeParticleGradients } from '../core/particle-gradient.js';
import { readRiftShapeFrame } from '../core/raw-rift-shape-access.js';
import { SquidForms } from '../core/squid-forms.js';

// 원본 생성자가 첫 인자를 새 객체의 부모가 아니라 덮어쓸 물리 SID 로 쓰는 네 내장 Form 생성자 주소 → Form 타입이다. (2026-11-13 WB04-E390)
const FORM_CONSTRUCTORS = Object.freeze([[0x004ae760, 1], [0x004ae3d0, 2], [0x004ae9f0, 3], [0x004aea40, 6]]);
// 확보 없이 진단만 하고 NULL 을 반환하는 두 생성자(Kaboom/Particle) 주소와 진단 파일/줄이다. (2026-11-13 WB04-E390)
const KABOOM_CONSTRUCTOR = 0x00470b00;
const PARTICLE_CONSTRUCTOR = 0x00487ef0;
const KABOOM_LINE = 0xbc;
const PARTICLE_LINE = 0x1b0;
// 현재 TYPE 의 줄기(12)·불꽃(11)·악마 광선(10) 생성자 주소이다. (2026-11-13 WB04-E390)
const STREAK_CONSTRUCTOR = 0x00458760;
const FIERY_CONSTRUCTOR = 0x00458700;
const EVIL_RAY_CONSTRUCTOR = 0x004536a0;
// 공유 색 객체 호스트 토큰이다(색 번호가 아님): 불꽃 1, 빔 2, 연기 3, 악마 광선 4. 건설은 3, 연소는 4 를 각자의 시스템 안에서 따로 쓴다. (2026-11-13 WB04-E390)
const FIERY_PALETTE_TOKEN = 1;
const LIGHTNING_PALETTE_TOKEN = 2;
const SMOKE_PALETTE_TOKEN = 3;
const EVIL_PALETTE_TOKEN = 4;
const CONSTRUCTION_PALETTE_TOKEN = 3;
const BURNING_PALETTE_TOKEN = 4;
// 시스템별 실제 확보량(바이트)이다: 입자 배열과 색 객체. 다른 값은 원본 요청과 어긋나므로 거부한다. (2026-11-13 WB04-E390)
const FIERY_BYTES = Object.freeze([2880, 8]);
const LIGHTNING_BYTES = Object.freeze([2880, 8, 64]);
const SMOKE_BYTES = Object.freeze([4800, 8]);
const EVIL_BYTES = Object.freeze([1200, 8]);
const BURNING_BYTES = Object.freeze([3600, 8]);
const SHRAPNEL_OBJECT_BYTES = 104;
const DISSOLVE_OBJECT_BYTES = 120;
// 기본 표시 영역이다(독립 호출의 fallback 이며 bound 카메라의 소유자가 아님). (2026-11-13 WB04-E390)
const DEFAULT_WIDTH = 1024;
const DEFAULT_HEIGHT = 768;

// 팔레트 256색의 RGBX DWORD(R | G<<8 | B<<16) 배열을 번호 순서대로 만든다. 예약 바이트는 색 계산에 쓰지 않는다. 모든 색 띠 생성이 이 입력을 쓰며 시험이 직접 확인하도록 내보낸다. (2026-11-13 WB04-E390)
export function particlePaletteInput(palette) {
    const logical = new Array(256);
    // 번호 순서를 그대로 옮긴다.
    for (let index = 0; index < 256; index += 1) logical[index] = palette.logical[index] >>> 0;
    return logical;
}

// 슬롯의 비정렬 little endian DWORD 하나를 읽는다. (2026-11-13 WB04-E390)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 슬롯의 비정렬 float32 를 읽는다. (2026-11-13 WB04-E390)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, readDword(raw, offset), true);
    return view.getFloat32(0, true);
}

// 입자 시스템 묶음 본체이다. 사용: const particles = new RawCampaignParticles(core, random, palette, { sound, shrapnel }); particles.setView(view, display); particles.add(11, 부모). (2026-11-13 WB04-E390)
export class RawCampaignParticles {
    // 실제 월드 묶음, 게임 난수, 현재 팔레트, 위치 음향이다.
    #core;
    #random;
    #palette;
    #sound;
    // 공통 표시 배율 상태와 모든 시스템이 공유하는 프레임 상태(게임 시각·배율·카메라)이다.
    #renderState = createConstructionVisualState();
    #frame = { now: 0, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
    // 불꽃(1)·빔(2)·연기(3)·악마 광선(4)의 공유 색 객체 토큰 상태와 색 표 메모리/읽기 뷰이다.
    #lifetime = { palette: 0, sharedPalette: null, sharedProcessType: null };
    #colors = { count: 0, readable: [] };
    #memory;
    #lightningLifetime = { palette: 0 };
    #lightningMemory;
    #streakLifetime = { palette: 0, sharedPalette: null, sharedProcessType: null };
    #streakColors = { count: 0, readable: [] };
    #smokeLifetime = { palette: 0 };
    #smokeMemory;
    #smokeColors = { count: 0, readable: [] };
    #evilDevices = {};
    #evilLifetime = { palette: 0, processType: 10, source: 0, sharedProcessType: null, sharedSource: null };
    #evilMemory;
    #evilColors = { count: 0, readable: [] };
    #evilClip = { left: 0, top: 0, right: 0, bottom: 0 };
    // 건설 표시 전역과 색 띠 입력이다.
    #constructionFrame = createConstructionFrameState();
    #constructionColors = { token: 0, count: 0, entries: [] };
    #constructionMemory;
    #constructionGradients = [];
    // 연소 표시 전역(공유 색 토큰, 연소점 선택 옵션, 프레임 입력, 색 표)과 현재 프레임 번호이다.
    #burningVisual = { processType: 14, palette: 0 };
    #burningSources = { weakenedMana: false, manaType: 158, daisType: 165, sharedWeakenedMana: null, sharedManaType: null, sharedDaisType: null, victim: { checking: () => 0 } };
    #burningFrame = createBurningFrameState();
    #burningColors = { count: 0, readable: [] };
    #burningMemory;
    #burningGradients = [];
    #burningFrameNumber = 0;
    // 표시 영역(독립 fallback)·실제 카메라 snapshot·직전 통지 이력·dirty 소비자·장치 연결 전 대기 표이다.
    #view = { cameraX: 0, cameraY: 0, left: 0, top: 0, right: DEFAULT_WIDTH, bottom: DEFAULT_HEIGHT };
    #cameraView = null;
    #notifiedPixels = [0, 0];
    #display = null;
    #pending = new DirtyRegions(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    // 현재 Draw 동안만 존재하는 출력 대상 { pixels, pitch, height, clip } 이다.
    #target = null;
    // 장기 전역 소유자(재연결 검사용)이다.
    #tailOwner = null;
    #hitPointOwner = null;
    // 공통 표시 목록과 아홉 표시 시스템이다. 선택 연결 시스템은 null 로 시작한다.
    #rendering;
    #fiery;
    #lightning;
    #streak;
    #construction = null;
    #shrapnel = null;
    #dissolve = null;
    #burning = null;
    #smoke;
    #evil;

    // 공통 목록과 기본 시스템(불꽃·빔·줄기·연기·악마 광선)을 만들고, 파편 장치가 있으면 파편·분해도 만든다. (2026-11-13 WB04-E390)
    constructor(core, random, palette, options = {}) {
        const { sound = null, shrapnel = null, adjacent = OBSERVED_ADJACENT_DWORD } = options;
        if (sound !== null && typeof sound !== 'function') throw new TypeError('입자 위치 음향은 함수여야 합니다.');
        this.#core = core;
        this.#random = random;
        this.#palette = palette;
        this.#sound = sound;
        // 색 표 메모리마다 같은 인접 DWORD 입력을 쓴다.
        this.#memory = new ParticlePaletteMemory(adjacent);
        this.#lightningMemory = new ParticlePaletteMemory(adjacent);
        this.#smokeMemory = new ParticlePaletteMemory(adjacent);
        this.#evilMemory = new ParticlePaletteMemory(adjacent);
        this.#constructionMemory = new ParticlePaletteMemory(adjacent);
        this.#burningMemory = new ParticlePaletteMemory(adjacent);
        this.#rendering = new RenderProcessSystem(core.pool, core.processes, this.#renderState, this.#registryHooks());
        this.#fiery = new FieryTailVisualSystem(this.#rendering, random, this.#lifetime, this.#frame, this.#colors, this.#fieryHooks());
        this.#lightning = new LightningVisualSystem(this.#rendering, random, this.#lightningLifetime, this.#frame, this.#lightningHooks());
        // 원본 458680 과 빔 생성자는 같은 0055a758 공유 색 객체를 쓴다: 줄기의 토큰은 빔 수명 상태의 palette 를 그대로 가리킨다.
        const beamLifetime = this.#lightningLifetime;
        this.#streakLifetime.sharedPalette = { get value() { return beamLifetime.palette; }, set value(token) { beamLifetime.palette = token; } };
        const streakHooks = this.#fieryHooks();
        const beamHooks = this.#lightningHooks();
        // 프레임/출력은 불꽃과 같고 첫 색 생성만 빔과 같다.
        streakHooks.lifetime.reserve = beamHooks.lifetime.reserve;
        streakHooks.lifetime.createPalette = beamHooks.lifetime.createPalette;
        streakHooks.lifetime.buildPalette = beamHooks.lifetime.buildPalette;
        this.#streak = new FieryTailVisualSystem(this.#rendering, random, this.#streakLifetime, this.#frame, this.#streakColors, streakHooks, FieryTailVariant.Streak);
        this.#smoke = new SmokeVisualSystem(this.#rendering, random, this.#smokeLifetime, this.#frame, this.#smokeColors, this.#smokeHooks());
        this.#evil = new EvilRayVisualSystem(this.#rendering, this.#evilLifetime, this.#frame, this.#evilColors, this.#evilRayHooks());
        if (shrapnel) {
            if (!shrapnel.crt) throw new TypeError('파편 실제 공유 CRT 상태 미연결');
            this.#dissolve = new DissolveVisualSystem(this.#rendering, random, this.#frame, this.#dissolveHooks(shrapnel));
            this.#shrapnel = new ShrapnelVisualSystem(this.#rendering, random, shrapnel.crt, this.#frame, this.#shrapnelHooks(shrapnel));
        }
    }

    // 공통 표시 목록이 모든 효과의 포인터 배열을 직접 소유하므로 별도 확보/반납을 하지 않는 등록 경계이다. (2026-11-13 WB04-E390)
    #registryHooks() {
        return { reserve: () => true, releaseArray: () => {} };
    }

    // 현재 소리/카메라 입력이다: 표시 영역은 독립 fallback, 카메라 원점은 bound 카메라가 있으면 그 정수 픽셀이다. (2026-11-13 WB04-E390)
    #currentView() {
        const view = { ...this.#view };
        if (this.#cameraView) {
            const pixels = this.#cameraView.pixels;
            view.cameraX = pixels.x;
            view.cameraY = pixels.y;
        }
        return view;
    }

    // 현재 카메라(없으면 fallback 원점)를 모든 하위 프레임 입력에 공급한다. 카메라가 bound 이면 64바이트 상태의 float 픽셀(56/60)을 그대로 쓴다. (2026-11-13 WB04-E390)
    #refreshCamera() {
        const view = this.#currentView();
        const x = this.#cameraView ? this.#cameraView.data.getFloat32(56, true) : Math.fround(view.cameraX);
        const y = this.#cameraView ? this.#cameraView.data.getFloat32(60, true) : Math.fround(view.cameraY);
        this.#frame.cameraX = x;
        this.#frame.cameraY = y;
        this.#constructionFrame.cameraX = x;
        this.#burningFrame.cameraX = x;
        this.#constructionFrame.cameraY = y;
        this.#burningFrame.cameraY = y;
    }

    // 월드 float 좌표를 현재 카메라의 화면 정수 좌표 [x, y] 로 바꾼다(SoundToScreen 과 497220 이 같은 몸체). (2026-11-13 WB04-E390)
    #toScreen(x, y) {
        const view = this.#currentView();
        return projectCameraCoordinates(view.cameraX, view.cameraY, x, y);
    }

    // 현재 TYPE 로 읽어 빌려 줄 색 표를 만든다: 다음 Draw 에 모든 시스템이 현재 메모리의 읽기 뷰를 다시 받는다. (2026-11-13 WB04-E390)
    #refreshReadable() {
        // 색 표를 이미 확보한 메모리만 읽기 뷰를 갱신한다.
        if (this.#memory.count) this.#colors.readable = this.#memory.snapshot();
        if (this.#lightningMemory.count) this.#streakColors.readable = this.#lightningMemory.snapshot();
        if (this.#burningMemory.count) this.#burningColors.readable = this.#burningMemory.snapshot();
        if (this.#smokeMemory.count) this.#smokeColors.readable = this.#smokeMemory.snapshot();
        if (this.#evilMemory.count) this.#evilColors.readable = this.#evilMemory.snapshot();
    }

    // 현재 Draw 대상 표면이 있어야 하는 출력 경계에서 쓰는 검사이다. (2026-11-13 WB04-E390)
    #requireTarget(message) {
        if (!this.#target) throw new Error(message);
        return this.#target;
    }

    // 불꽃 꼬리(11)의 색 생성·dirty·선 출력·끝점 경계이다. 입자 배열/색 메모리는 이 객체가 실제 확보하고 요청 크기만 확인한다. (2026-11-13 WB04-E390)
    #fieryHooks() {
        return {
            lifetime: {
                reserve: (bytes) => {
                    if (!FIERY_BYTES.includes(bytes)) throw new Error('Fiery Tail 실제 확보량 오류');
                    return true;
                },
                // 단일 공유 색 객체의 호스트 토큰이며 색 번호가 아니다.
                createPalette: () => FIERY_PALETTE_TOKEN,
                buildPalette: (token) => {
                    if (token !== FIERY_PALETTE_TOKEN) throw new Error('Fiery Tail 공유 색 객체 토큰 오류');
                    this.#memory.assign(buildFieryTailPalette(particlePaletteInput(this.#palette)));
                    this.#colors.count = this.#memory.count;
                    this.#colors.readable = this.#memory.snapshot();
                },
            },
            frame: {
                invalidate: (rect) => this.#invalidate(rect),
                line: (line) => {
                    const target = this.#requireTarget('Fiery Tail 실제 그리기 표면 미연결');
                    drawVfxLinePixels(target.pixels, target.pitch, target.height, target.clip, { x: line.x1, y: line.y1 }, { x: line.x2, y: line.y2 }, line.color);
                },
            },
            // 현재 부모 raw 좌표를 읽으며 최초 발사 위치를 캐시하지 않는다.
            endpoint: (sid) => {
                const raw = this.#core.pool.slot(sid);
                return this.#toScreen(readFloat(raw, 14), readFloat(raw, 18));
            },
            // 실제 입자 저장소/객체는 원본 수명 몸체와 Kernel 이 반납한다.
            release: () => {},
        };
    }

    // 하나의 입자 효과를 부모에 붙이기 전에 현재 카메라를 공급한다. 불꽃(TYPE 11) 외의 번호는 미복원이다. (2026-11-13 WB04-E390)
    add(type, parent) {
        this.#refreshCamera();
        if (type !== 11) throw new Error(`캠페인 비행 표시 효과 미복원 TYPE${type}`);
        this.#fiery.add(parent);
    }

    // 원본 12(건설 마나 줄기)를 공통 목록/Kernel 에 붙이고, 빔이 먼저 만든 색 메모리를 현재 상태로 읽는다. (2026-11-13 WB04-E390)
    addStreak(parent) {
        this.#refreshCamera();
        const process = this.#streak.add(parent);
        this.#streakColors.count = this.#lightningMemory.count;
        this.#streakColors.readable = this.#lightningMemory.snapshot();
        return process;
    }

    // 현재 TYPE 의 6개 입자 생성 함수 주소를 구별해 호출한다: 내장 Form 네 생성자는 VT 만 바꾸고 Kaboom/Particle 은 진단 뒤 NULL 반환, 줄기/불꽃/악마 광선은 시스템에 붙인다. (2026-11-13 WB04-E390)
    createTailByConstructor(address, parent, assertion = null) {
        // 내장 Form 네 생성자는 새 객체를 만들지 않고 덮어쓸 물리 SID(첫 인자)의 VT 만 쓴다.
        for (const [constructor, type] of FORM_CONSTRUCTORS) {
            if (address === constructor) {
                new SquidForms(this.#core.pool).constructVtableAt(type, parent);
                return;
            }
        }
        if (address === KABOOM_CONSTRUCTOR || address === PARTICLE_CONSTRUCTOR) {
            if (typeof assertion !== 'function') throw new Error('현재 TYPE 생성 함수의 원문 진단 장치 미연결');
            // 진단이 자기 연결을 반납해도 이번 호출의 캡처는 끝까지 보존한다.
            const selected = assertion;
            selected('0', address === KABOOM_CONSTRUCTOR ? '.\\Kaboom.cpp' : '.\\Particle.cpp', address === KABOOM_CONSTRUCTOR ? KABOOM_LINE : PARTICLE_LINE);
            return;
        }
        if (address === STREAK_CONSTRUCTOR) {
            this.addStreak(parent);
            return;
        }
        if (address === FIERY_CONSTRUCTOR) {
            this.add(11, parent);
            return;
        }
        if (address === EVIL_RAY_CONSTRUCTOR) {
            this.addEvilRay(parent);
            return;
        }
        throw new Error('현재 TYPE가 선택한 다른 입자 생성 함수 미복원');
    }

    // 현재 TYPE 의 줄기/불꽃/악마 광선 생성자가 읽는 장기 전역(GameAuxState)을 한 번 빌린다. 다른 소유자로의 재연결은 거부한다. (2026-11-13 WB04-E390)
    bindTailProcessTypes(aux) {
        if (!aux || typeof aux.fieryTailProcessType !== 'number') throw new TypeError('입자 현재 TYPE 소유자는 GameAuxState 여야 합니다.');
        if (this.#tailOwner && this.#tailOwner !== aux) throw new Error('입자 현재 TYPE 소유자 재연결 오류');
        this.#tailOwner = aux;
        this.#lifetime.sharedProcessType = () => aux.fieryTailProcessType;
        this.#streakLifetime.sharedProcessType = () => aux.streakProcessType;
        this.#evilLifetime.sharedProcessType = () => aux.evilRayProcessType;
        this.#evilLifetime.sharedSource = () => aux.particleSource;
    }

    // 악마 광선의 반납 관찰/배율 검사 뒤 부모에 붙인다(부모 0 은 시스템이 보관). (2026-11-13 WB04-E390)
    addEvilRay(parent) {
        this.#refreshCamera();
        return this.#evil.add(parent);
    }

    // 악마 광선이 도달할 때 현재 클라이언트의 TYPE 음향 번호/재생/SHP 상세 오류/진단 장치를 읽게 한다. 도달하지 않는 장치는 강제하지 않는다. (2026-11-13 WB04-E390)
    connectEvilRayDevices(devices) {
        this.#evilDevices = devices ?? {};
    }

    // 악마 광선의 색 생성·SHP 폭 조회·음향·dirty·타원 출력 경계이다. 원문 수치 몸체는 유지하고 같은 실제 메모리/메타데이터/출력에 연결한다. (2026-11-13 WB04-E390)
    #evilRayHooks() {
        // 선택 장치를 호출 순간에 읽는다(자기 연결을 반납해도 이번 호출은 보존).
        const device = (name, message) => {
            const selected = this.#evilDevices[name];
            if (typeof selected !== 'function') throw new Error(message);
            return selected;
        };
        const assertion = (expression, file, line) => device('assertion', 'EvilRay 실제 진단 장치 미연결')(expression, file, line);
        return {
            lifetime: {
                reserve: (bytes) => {
                    if (!EVIL_BYTES.includes(bytes)) throw new Error('EvilRay 실제 확보량 오류');
                    return true;
                },
                createPalette: () => EVIL_PALETTE_TOKEN,
                buildPalette: (token) => {
                    if (token !== EVIL_PALETTE_TOKEN) throw new Error('EvilRay 공유 색 토큰 오류');
                    this.#evilMemory.assign(buildEvilRayPalette(particlePaletteInput(this.#palette)));
                    this.#evilColors.count = this.#evilMemory.count;
                    this.#evilColors.readable = this.#evilMemory.snapshot();
                },
                assertion,
            },
            frame: {
                assertion,
                // 현재 TYPE/저장 프레임/abstract 의 원문 인자로 SHP 프레임 폭(signed 16)을 읽는다.
                shapeWidth: (sid) => {
                    const raw = this.#core.pool.slot(sid);
                    const type = raw[10];
                    const frame = readDword(raw, 36) | 0;
                    if (type >= this.#core.types.length || type >= this.#core.shapes.length) throw new RangeError('EvilRay 현재 SHP TYPE 물리 범위');
                    const shape = this.#core.shapes[type];
                    const record = this.#core.types[type];
                    const access = {
                        typeNumber: (selected) => selected.token,
                        assertion,
                        // 진단 앞의 실제 Client 상세 owner 를 선택한다.
                        frameError: (number, savedFrame, abstract) => device('frameError', 'EvilRay 실제 SHP 상세 장치 미연결')(number, savedFrame, abstract),
                        shape: (selected, currentFrame) => {
                            const found = this.#core.shapes[selected];
                            if (!found.loaded || currentFrame < 0 || currentFrame >= found.frames.length) throw new RangeError('EvilRay 진단 복귀 뒤 SHP 물리 범위');
                            return [found.frames[currentFrame].width, 0];
                        },
                    };
                    const width = readRiftShapeFrame({ token: type, flags1: record.flags1, frameCount: shape.frameCount, loaded: shape.loaded, header: record.maxHitPoints }, frame, raw[40] & 1, access)[0];
                    return (Math.trunc(width) << 16) >> 16;
                },
                soundId: (sid, end) => device('soundId', 'EvilRay 실제 TYPE 음향 장치 미연결')(sid, end),
                sound: (x, y, id) => device('sound', 'EvilRay 실제 위치 음향 장치 미연결')(x, y, id),
                invalidate: (rect) => this.#invalidate(rect),
                // 원문이 강제하는 0,0,639,479(닫힌 구간) pane 이며 다른 Sprite 의 cut 으로 이 clip 을 줄이지 않는다.
                clip: (rect) => {
                    this.#requireTarget('EvilRay 실제 타원 표면 미연결');
                    this.#evilClip = { left: rect[0], top: rect[1], right: rect[2] + 1, bottom: rect[3] + 1 };
                },
                circle: (circle) => {
                    const target = this.#requireTarget('EvilRay 실제 타원 표면 미연결');
                    drawVfxEllipse(target.pixels, target.pitch, target.height, this.#evilClip, { x: circle.x, y: circle.y }, circle.radius, circle.radius - 1, circle.color);
                },
            },
            // vector/공통 등록/Kernel 이 실제 입자/객체를 반납한다.
            release: () => {},
        };
    }

    // 냉각 연기(21)의 12색 48바이트 색 표와 선 출력/dirty 경계이다. (2026-11-13 WB04-E390)
    #smokeHooks() {
        return {
            lifetime: {
                reserve: (bytes) => {
                    if (!SMOKE_BYTES.includes(bytes)) throw new Error('냉각 연기 실제 확보량 오류');
                    return true;
                },
                // 다른 표시와 구별하는 공유 색 객체 토큰이며 색 번호가 아니다.
                createPalette: () => SMOKE_PALETTE_TOKEN,
                buildPalette: (token) => {
                    if (token !== SMOKE_PALETTE_TOKEN) throw new Error('냉각 연기 공유 색 객체 오류');
                    this.#smokeMemory.assign(buildSmokePalette());
                    this.#smokeColors.count = this.#smokeMemory.count;
                    this.#smokeColors.readable = this.#smokeMemory.snapshot();
                },
            },
            frame: {
                invalidate: (rect) => this.#invalidate(rect),
                line: (line) => {
                    const target = this.#requireTarget('냉각 연기 실제 그리기 표면 미연결');
                    drawVfxLinePixels(target.pixels, target.pitch, target.height, target.clip, { x: line.x1, y: line.y1 }, { x: line.x2, y: line.y2 }, line.color);
                },
            },
            release: () => {},
        };
    }

    // 004a8770 의 포인트 추출 뒤 실제 21·200입자·10회/간격 3 연기를 만든다(부모 0 거부). (2026-11-13 WB04-E390)
    addCooling(parent, hotspot) {
        this.#refreshCamera();
        return this.#smoke.addCooling(parent, hotspot);
    }

    // 일반 대본 004a8660 의 8회/5간격 연기이며 냉각 래퍼의 10회/3간격 후처리를 쓰지 않는다. (2026-11-13 WB04-E390)
    addSmoke(request) {
        this.#refreshCamera();
        return this.#smoke.add(request);
    }

    // 공유 빔(19)의 색 생성·중심·dirty·선 출력·파일명 음향 경계이다. 계산은 원본 몸체이고 현재 중심/메모리/출력 장치만 연결한다. (2026-11-13 WB04-E390)
    #lightningHooks() {
        return {
            lifetime: {
                reserve: (bytes) => {
                    if (!LIGHTNING_BYTES.includes(bytes)) throw new Error('빔 실제 확보량 오류');
                    return true;
                },
                // 빔 전용 공유 색 객체 토큰이며 색 번호가 아니다.
                createPalette: () => LIGHTNING_PALETTE_TOKEN,
                buildPalette: (token) => {
                    if (token !== LIGHTNING_PALETTE_TOKEN) throw new Error('빔 공유 색 객체 토큰 오류');
                    this.#lightningMemory.assign(buildLightningPalette(particlePaletteInput(this.#palette)));
                },
            },
            center: (sid) => this.#core.neighbors.center(sid),
            frame: {
                invalidate: (rect) => this.#invalidate(rect),
                line: (line) => {
                    const target = this.#requireTarget('빔 실제 그리기 표면 미연결');
                    drawVfxLinePixels(target.pixels, target.pitch, target.height, target.clip, { x: line.x1, y: line.y1 }, { x: line.x2, y: line.y2 }, line.color);
                },
                sound: (x, y, name) => {
                    if (!this.#sound) throw new Error('빔 실제 파일명 위치 음향 미연결');
                    this.#sound(x, y, name);
                },
            },
            // 경로/입자 저장소와 객체는 실제 수명 몸체/Kernel 이 반납한다.
            release: () => {},
        };
    }

    // 실제 wrapper(00472700/00472690)의 표적 생존 검사 뒤 공유 빔을 같은 Host/표시 목록에 만든다. 죽은 표적의 raw 주소 의미와 생성 성공을 구별해 돌려준다. (2026-11-13 WB04-E390)
    addBeam(parent, target, sound = null) {
        this.#refreshCamera();
        return createLightning(this.#core.pool, parent, target, sound, (source, victim, name) => {
            if (name && !this.#sound) throw new Error('빔 실제 파일명 음향 소비자 미연결');
            return this.#lightning.add(source, victim, name);
        });
    }

    // 실제 472770 의 화면 좌표 → 현재 카메라 → 직접 좌표 빔 생성이다. 대체 표적 SID 를 만들지 않는다. (2026-11-13 WB04-E390)
    addScreenBolt(parent, from, to, life) {
        this.#refreshCamera();
        const view = this.#currentView();
        return createLightningAtScreen(parent, from, to, life, view.cameraX, view.cameraY, (source, first, last, duration, name) => {
            if (!this.#sound) throw new Error('화면빔 실제파일명 음향미연결');
            return this.#lightning.addAtScreen(source, first, last, duration, name);
        });
    }

    // 파편(16)의 실제 104바이트 객체/저장소 확보 경계와 dirty/픽셀 출력 경계이다. 캡처한 색 번호를 현재 8비트 표면에 그대로 쓴다. (2026-11-13 WB04-E390)
    #shrapnelHooks(input) {
        return {
            lifetime: {
                ...input.lifetime,
                reserve: (bytes) => {
                    if (bytes !== SHRAPNEL_OBJECT_BYTES) throw new Error('파편 실제 객체 확보량 오류');
                    return true;
                },
            },
            destroyUnpaired: input.destroyUnpaired,
            frame: {
                // 실제 vector 가 요청 저장소를 확보한다.
                reserve: (bytes) => {
                    if (!bytes) throw new Error('파편 실제 저장소 확보량0 오류');
                    return true;
                },
                // 실제 이전 입자/작업 복사본은 프레임 몸체의 소유 저장소가 반납한다.
                release: () => {},
                invalidate: (rect) => this.#invalidate(rect),
                pixel: (pixel) => {
                    const target = this.#requireTarget('파편 실제 픽셀 표면 미연결');
                    const cut = target.clip;
                    if (pixel.x < cut.left || pixel.x >= cut.right || pixel.y < cut.top || pixel.y >= cut.bottom) return;
                    if (pixel.x < 0 || pixel.x >= target.pitch || pixel.y < 0 || pixel.y >= target.height) throw new RangeError('파편 실제 clip/표면 경계 불일치');
                    target.pixels[pixel.y * target.pitch + pixel.x] = pixel.color & 0xff;
                },
            },
            // 실제 bitmap/입자/객체는 원본 수명 몸체와 Kernel 이 반납한다.
            release: () => {},
        };
    }

    // 파편 세계/캡처 장치가 연결된 세션에서만 세 종류 파편을 만든다. 확보 실패는 웹에 없으므로 null 이 되지 않는다. (2026-11-13 WB04-E390)
    addShrapnel(source, kind) {
        if (!this.#shrapnel) throw new Error('캠페인 실제 파편 세계/캡처 미연결');
        return this.#shrapnel.add(source, kind);
    }

    // 분해(18)의 120바이트 객체 확보와 파편과 공유하는 프레임 출력 경계이다. 원본 new 0 도 성공 포인터라 프레임 저장소 확보는 항상 성공한다. (2026-11-13 WB04-E390)
    #dissolveHooks(input) {
        const shared = this.#shrapnelHooks(input);
        return {
            lifetime: {
                create: input.lifetime.create,
                pop: input.lifetime.pop,
                depend: input.lifetime.depend,
                capture: input.lifetime.capture,
                reserve: (bytes) => {
                    if (bytes !== DISSOLVE_OBJECT_BYTES) throw new Error('분해 실제 객체 확보량 오류');
                    return true;
                },
            },
            destroyUnpaired: input.destroyUnpaired,
            frame: { ...shared.frame, reserve: () => true },
            release: shared.release,
        };
    }

    // 캡처가 소유한 픽셀을 무중력 이동/수명/현재 표면에 연결한 애시드 분해를 만든다. 미연결 캡처를 빈 효과로 대체하지 않는다. (2026-11-13 WB04-E390)
    addDissolve(source, first, second, xOriented) {
        if (!this.#dissolve) throw new Error('캠페인 실제 분해 세계/캡처 미연결');
        return this.#dissolve.add(source, first, second, xOriented);
    }

    // 건설 표시(17)에 실제 캡처/최대 HP/색 띠/출력 경계를 한 번 연결한다. 첫 건설 전에 호출해야 하며 중복/누락은 거부한다. (2026-11-13 WB04-E390)
    connectConstruction(capture, maximum) {
        if (this.#construction || typeof capture !== 'function' || typeof maximum !== 'function') throw new TypeError('실제 건설 표시 연결 오류');
        const visual = {
            // 실제 vector 와 색 할당이 실패하면 예외로 보고한다.
            reserve: () => true,
            // 건설 전용 호스트 토큰이며 팔레트 색 번호와 구별한다.
            createPalette: () => CONSTRUCTION_PALETTE_TOKEN,
            gradient: (count, from, to, width) => {
                this.#constructionGradients.push(buildParticleGradient(particlePaletteInput(this.#palette), from, to, count, width));
            },
            buildPalette: (token) => {
                if (token !== CONSTRUCTION_PALETTE_TOKEN || this.#constructionGradients.length !== 2) throw new Error('건설 실제 공유 색 띠 연결 오류');
                this.#constructionMemory.assign(mergeParticleGradients(this.#constructionGradients));
                // 시스템이 같은 객체를 계속 읽으므로 새 객체로 바꾸지 않고 필드를 갱신한다.
                Object.assign(this.#constructionColors, { token, count: this.#constructionMemory.count, entries: this.#constructionMemory.snapshot() });
                this.#constructionGradients = [];
            },
            capture,
            // 이전 입자 vector 는 원본 몸체가 직접 반납한다.
            releaseParticles: () => {},
        };
        const frame = {
            maxHitPoints: maximum,
            invalidate: (rect) => this.#invalidate({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }),
            draw: (draw) => {
                const target = this.#requireTarget('건설 실제 그리기 표면 미연결');
                // 원본 입력 사각형의 각 행을 현재 dirty clip 안에서만 복사한다.
                for (let row = draw.clip.top; row < draw.clip.bottom; row += 1) {
                    const y = draw.y + row;
                    if (y < target.clip.top || y >= target.clip.bottom || y < 0 || y >= target.height) continue;
                    // 255 투명 픽셀은 기존 화면을 보존한다.
                    for (let column = draw.clip.left; column < draw.clip.right; column += 1) {
                        const x = draw.x + column;
                        if (x < target.clip.left || x >= target.clip.right || x < 0 || x >= target.pitch) continue;
                        const color = draw.pixels[row * draw.pitch + column];
                        if (color !== 255) target.pixels[y * target.pitch + x] = color;
                    }
                }
            },
        };
        this.#construction = new ConstructionVisualSystem(this.#rendering, this.#constructionFrame, this.#constructionColors, this.#random, visual, frame, () => {});
    }

    // 공통 등록 배열에 실제 17 프로세스를 추가한다. 미연결을 빈 효과로 바꾸지 않는다. (2026-11-13 WB04-E390)
    addConstruction(parent, instant) {
        this.#refreshCamera();
        if (!this.#construction) throw new Error('실제 건설 표시 미연결');
        return this.#construction.add(parent, instant);
    }

    // 연소(14)에 실제 캡처/정지 음향/피해자 진단 상태/색 띠/출력 경계를 한 번 연결한다(중복/필수 누락 거부). 첫 색 표도 원본 gradient/병합 몸체로 만든다. (2026-11-13 WB04-E390)
    connectBurning(capture, stopSound, victim = { checking: () => 0 }) {
        if (this.#burning || typeof capture !== 'function' || typeof stopSound !== 'function') throw new TypeError('실제 Burning 연결 중복/필수 경계 누락');
        const hooks = {
            visual: {
                reserve: (bytes) => {
                    if (!BURNING_BYTES.includes(bytes)) throw new Error('Burning 실제 확보량 오류');
                    return true;
                },
                // 색 번호와 다른 공유 객체 토큰이다.
                createPalette: () => BURNING_PALETTE_TOKEN,
                gradient: (from, to, count, width) => {
                    this.#burningGradients.push(buildParticleGradient(particlePaletteInput(this.#palette), from, to, count, width));
                },
                buildPalette: (token) => {
                    if (token !== BURNING_PALETTE_TOKEN || this.#burningGradients.length !== 1) throw new Error('Burning 실제 첫 색표 연결 오류');
                    this.#burningMemory.assign(mergeParticleGradients(this.#burningGradients));
                    Object.assign(this.#burningColors, { count: this.#burningMemory.count, readable: this.#burningMemory.snapshot() });
                    this.#burningGradients = [];
                },
            },
            frame: { capture, stopSound },
            invalidate: (rect) => this.#invalidate({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }),
            clip: () => {
                const clip = this.#requireTarget('Burning 실제 VFX clip 미연결').clip;
                return { left: clip.left, top: clip.top, right: clip.right, bottom: clip.bottom };
            },
            pixel: (x, y, color) => {
                const target = this.#target;
                if (!target || x < 0 || x >= target.pitch || y < 0 || y >= target.height) throw new RangeError('Burning 실제 표면/좌표 오류');
                target.pixels[y * target.pitch + x] = color;
            },
            // 실제 버퍼와 객체는 원본 몸체/Kernel 이 반납한다.
            release: () => {},
        };
        // 두 상태의 피해자 진단 장치도 같은 Client owner 를 가리키며 자체 진단 값을 만들지 않는다.
        this.#burningFrame.victim = victim;
        this.#burningSources.victim = victim;
        this.#burningFrame.pixelScaleX = this.#renderState.scaleX;
        this.#burningFrame.pixelScaleY = this.#renderState.scaleY;
        this.#refreshCamera();
        this.#burningFrame.viewport = { left: this.#view.left, top: this.#view.top, right: this.#view.right, bottom: this.#view.bottom };
        this.#burning = new BurningVisualSystem(this.#rendering, this.#random, this.#core.types, this.#burningVisual, this.#burningSources, this.#burningFrame, this.#burningColors, () => this.#burningFrameNumber, hooks);
    }

    // 실제 모드 0/1 wrapper 의 같은 생성자이다. 미연결/계산 오류를 숨기지 않는다. (2026-11-13 WB04-E390)
    addBurning(victim, mode) {
        this.#refreshCamera();
        if (!this.#burning) throw new Error('실제 Burning14 캡처/수명 미연결');
        return this.#burning.add(victim, mode);
    }

    // 전체 Kernel 실행 직전 현재 게임 시각/프레임 번호/약화 마나 옵션을 공급한다. 별도 고정 틱이나 벽시각으로 대체하지 않는다. (2026-11-13 WB04-E390)
    prepareFrame(now, frameNumber = 0, weakenedMana = false) {
        this.#refreshCamera();
        this.prepareBridgeClock(now, frameNumber);
        this.#burningSources.weakenedMana = weakenedMana;
        if (this.#constructionMemory.count) this.#constructionColors.entries = this.#constructionMemory.snapshot();
    }

    // 원문 55b4d0/55b4a8 은 스캔보다 먼저 고정된다: 시간 필드만 쓰며 카메라/색표를 갱신하지 않는다(이른 원문 스캔의 삭제/파편도 현재 시각을 읽음). (2026-11-13 WB04-E390)
    prepareBridgeClock(now, frameNumber) {
        if (!Number.isFinite(now)) throw new RangeError('캠페인 입자 게임 시각 오류');
        this.#frame.now = now;
        this.#constructionFrame.now = now;
        this.#burningFrame.now = now;
        this.#burningFrameNumber = frameNumber >>> 0;
    }

    // 현재 실제 카메라 snapshot(CameraView)을 빌린다. 이 snapshot 은 모든 입자보다 오래 살아야 하며 이후 표시 영역의 카메라 원점은 이것이 소유한다. (2026-11-13 WB04-E390)
    bindCameraView(view) {
        if (typeof view?.data?.getFloat32 !== 'function' || !view.pixels) throw new TypeError('입자 카메라는 CameraView 여야 합니다.');
        this.#cameraView = view;
        this.#view.cameraX = 0;
        this.#view.cameraY = 0;
        this.#refreshCamera();
    }

    // 같은 Client 의 현재 바람 X getter 를 연소 프레임에 빌린다. 다른 getter 로의 재연결은 거부한다. (2026-11-13 WB04-E390)
    bindWind(current) {
        if (typeof current !== 'function') throw new TypeError('연소 바람은 getter 함수여야 합니다.');
        if (this.#burningFrame.sharedWind && this.#burningFrame.sharedWind !== current) throw new Error('연소 바람의 다른 owner 중복 연결');
        this.#burningFrame.sharedWind = current;
    }

    // 같은 Client 의 약화 마나 전역 getter 를 연소점 선택에 빌린다. 다른 getter 로의 재연결은 거부한다. (2026-11-13 WB04-E390)
    bindWeakenedMana(current) {
        if (typeof current !== 'function') throw new TypeError('연소 약화 마나는 getter 함수여야 합니다.');
        if (this.#burningSources.sharedWeakenedMana && this.#burningSources.sharedWeakenedMana !== current) throw new Error('연소 HP의 약화 전역 중복 다른 주소 연결');
        this.#burningSources.sharedWeakenedMana = current;
    }

    // 연소점이 같은 Client 의 현재 사제/다이스 TYPE 전역(GameAuxState)을 빌린다. 다른 소유자로의 재연결은 거부한다. (2026-11-13 WB04-E390)
    bindHitPointTypes(aux) {
        if (!aux || typeof aux.priestType !== 'number' || typeof aux.daisType !== 'number') throw new TypeError('연소 TYPE 소유자는 GameAuxState 여야 합니다.');
        if (this.#hitPointOwner && this.#hitPointOwner !== aux) throw new Error('연소점 HP의 현재 TYPE owner 재연결 오류');
        this.#hitPointOwner = aux;
        this.#burningSources.sharedManaType = () => aux.priestType;
        this.#burningSources.sharedDaisType = () => aux.daisType;
    }

    // 표시 영역과 dirty 소비자(Renderer)를 연결한다. 카메라/영역이 바뀌면 영역 전체를 다시 그리게 하고, 같은 view 호출도 대기 중인 dirty 를 장치에 전달한다. (2026-11-13 WB04-E390)
    setView(view, display) {
        const pixels = this.#cameraView ? [this.#cameraView.pixels.x, this.#cameraView.pixels.y] : [view.cameraX, view.cameraY];
        // 현재 소유자와 직전 통지 이력을 구별해 변경 여부를 판정한다.
        const changed = pixels[0] !== this.#notifiedPixels[0] || pixels[1] !== this.#notifiedPixels[1]
            || view.left !== this.#view.left || view.top !== this.#view.top || view.right !== this.#view.right || view.bottom !== this.#view.bottom;
        this.#notifiedPixels = pixels;
        this.#view = { cameraX: view.cameraX, cameraY: view.cameraY, left: view.left, top: view.top, right: view.right, bottom: view.bottom };
        if (this.#cameraView) {
            this.#view.cameraX = 0;
            this.#view.cameraY = 0;
        }
        this.#refreshCamera();
        this.#display = display ?? null;
        this.#burningFrame.viewport = { left: view.left, top: view.top, right: view.right, bottom: view.bottom };
        if (changed) {
            this.#pending = new DirtyRegions(view.right, view.bottom);
            this.#pending.invalidateAll();
        }
        this.#flushDirty();
    }

    // 원본 현재/이전 영역 요청을 실제 renderer 또는 제한된 대기 표로 보낸다. (2026-11-13 WB04-E390)
    #invalidate(rect) {
        if (this.#display?.invalidate) {
            this.#display.invalidate({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }, 0);
            return;
        }
        this.#pending.add({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom });
    }

    // 실제 장치가 없을 때는 큐를 지우지 않는다. 장치 연결 후에는 각 원본 요청을 한 번씩(순서 보존) 전달한다. (2026-11-13 WB04-E390)
    #flushDirty() {
        if (!this.#display?.invalidate) return;
        if (this.#pending.fullRedraw) {
            this.#display.invalidate({ left: this.#view.left, top: this.#view.top, right: this.#view.right, bottom: this.#view.bottom }, 0);
        } else {
            // 장치가 연결된 뒤에도 원본 변경 표의 요청 순서를 보존한다.
            for (const entry of this.#pending.entries) this.#display.invalidate({ left: entry.rect.left, top: entry.rect.top, right: entry.rect.right, bottom: entry.rect.bottom }, entry.flags);
        }
        this.#pending.clear();
    }

    // 공통 표시 목록을 한 번 수집해 다른 SHP 와 같은 전체 정렬에 넣을 직접 그리기 항목으로 만든다. 실제 출력은 dirty 표면이 생긴 뒤 수행한다. (2026-11-13 WB04-E390)
    sprites(clip) {
        this.#refreshCamera();
        const slots = this.#rendering.slots;
        const output = new Uint8Array(slots.length * RENDER_ENTRY_BYTES);
        const view = this.#currentView();
        const world = {
            left: Math.fround((clip.left + view.cameraX) / 16),
            top: Math.fround((clip.top + view.cameraY) / 11),
            right: Math.fround((clip.right + view.cameraX) / 16),
            bottom: Math.fround((clip.bottom + view.cameraY) / 11),
        };
        const count = this.#rendering.collect(world, output, slots.length);
        const data = new DataView(output.buffer);
        const result = [];
        // 수집된 항목마다 현재 등록 필드를 다시 찾아 직접 그리기 절차를 만든다.
        for (let index = 0; index < count; index += 1) {
            const base = index * RENDER_ENTRY_BYTES;
            const token = data.getUint32(base + 8, true);
            const fields = this.#rendering.lookup(token);
            if (!fields) continue;
            const bounds = renderProcessRemovalBounds(fields, this.#frame.cameraX, this.#frame.cameraY);
            // 공통 kind3 의 실제 부모 x/y+0.001·signed 깊이를 보존하고 여기서 종류별 정렬을 하지 않는다.
            result.push({
                database: null,
                clip: { left: clip.left, top: clip.top, right: clip.right, bottom: clip.bottom },
                order: { x: data.getFloat32(base, true), y: data.getFloat32(base + 4, true), depth: data.getInt16(base + 12, true) },
                procedure: {
                    bounds: { left: bounds[0], top: bounds[1], right: bounds[2], bottom: bounds[3] },
                    draw: (pixels, pitch, height, cut) => this.#draw(token, pixels, pitch, height, cut),
                },
            });
        }
        return result;
    }

    // 하나의 실제 dirty 표면 동안만 VFX 출력 대상이 유효하다. 예외 뒤에도 대상을 해제하고, 색 표 읽기 뷰는 매 Draw 마다 현재 메모리로 다시 받는다. (2026-11-13 WB04-E390)
    #draw(token, pixels, pitch, height, clip) {
        this.#refreshCamera();
        if (!this.#rendering.lookup(token)) return;
        if (this.#target) throw new Error('입자 표면 중첩 그리기 오류');
        this.#target = { pixels, pitch, height, clip };
        try {
            this.#refreshReadable();
            this.#rendering.drawToken(token);
        } finally {
            this.#target = null;
        }
    }

    // 실제 등록/프로세스 수명을 확인하는 읽기 전용 조회용 공통 표시 목록이다. (2026-11-13 WB04-E390)
    get rendering() {
        return this.#rendering;
    }

    // 같은 16/11 배율의 현재 목록을 종류별로 다시 나누지 않고 원문 잔여 공간에 수집한다. (2026-11-13 WB04-E390)
    collectDrawing(view, output, limit) {
        this.#refreshCamera();
        return this.#rendering.collect(view, output, limit);
    }

    // 같은 정렬 토큰의 현재 생존 프로세스를 실제 표면에 출력한다(기존 VFX 잠금/예외 정리를 공유). (2026-11-13 WB04-E390)
    drawEntry(token, pixels, pitch, height, clip) {
        this.#draw(token, pixels, pitch, height, clip);
    }

    // 남은 효과를 선언 역순으로 Host 삭제(flags 0x10)해 정리한다. 효과가 참조하는 장치/팔레트/상태보다 먼저 반납한다. (2026-11-13 WB04-E390)
    dispose() {
        this.#evil.dispose();
        this.#smoke.dispose();
        if (this.#burning) this.#burning.dispose();
        if (this.#dissolve) this.#dissolve.dispose();
        if (this.#shrapnel) this.#shrapnel.dispose();
        if (this.#construction) this.#construction.dispose();
        this.#streak.dispose();
        this.#lightning.dispose();
        this.#fiery.dispose();
    }
}
