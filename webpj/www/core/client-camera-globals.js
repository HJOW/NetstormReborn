// Client 수명의 전역 카메라 snapshot(0059a900)과 논리 Native Renderer 의 현재 view·목표·분모를 **서로 다른 주소**에서 보관하는 모듈이다
// (cpppj `client/ClientCameraGlobals.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 변경 없음). 카메라 64BYTE 상태는 기존 `CameraView`(WB03-Z, 16DWORD)를 쓴다.
// 칸: snapshot(전역 BSS 0, NativeInit/명시 setter/프레임 복사만 바꿈)·rendererView(논리 Renderer 앞 64BYTE, 호스트 화면 장치 교체와 무관)·target(원문 +48/+4c 한계, +50/+54 다음 픽셀 원점, +58 다음 Q16 배율)·
//   denominator(00540ce0/ce4, PE 초기 15/15, 인자 0 이면 기본 선택)·constructionHistory(최근 로컬 건설 위치 5칸+Tab 순번)·savedPositions(005caeb0 숫자 키 저장 10칸 80BYTE).
//   같은 형식이라는 이유로 목표/논리 Renderer/snapshot 을 alias 하지 않으며 명시 복사 위치만 현재 값을 바꾼다.
// 사용: const camera = new ClientCameraGlobals(); camera.initializeNativeRenderer(1024, 768, 256, redrawBox, revisionBox); camera.requestPixels(71, 83); camera.copyRendererSnapshot();
//   redraw/revision 인자는 `{ value }` 상자(소비 쪽 전역 005c85dc 등)이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E333: 4-D ClientCameraGlobals 이식)
import { CameraView, cameraFtolLow } from '../render/camera-view.js';
import { createConstructionHistory } from './construction-history-key.js';
import { SavedCameraPositions } from './saved-camera-key.js';

// 원문 칸 크기 필드 위치(+08/+0c 정수 칸 크기)와 Q16 배율 위치(+18)이다. (2026-11-12 WB04-E333)
const CELL_X = 8;
const CELL_Y = 12;
const ZOOM = 24;

// 중간 값이 float 로 줄지 않도록 double 곱셈의 입력 칸 크기를 읽는다. (2026-11-12 WB04-E333)
const cell = (view, offset) => view.data.getInt32(offset, true);

// unsigned 32비트 뺄셈 결과를 signed 로 되돌린다. (2026-11-12 WB04-E333)
const wrap = (value) => value | 0;

// 카메라 전역 소유자이다. 접근자는 호출마다 같은 객체를 돌려준다(주소 고정). (2026-11-12 WB04-E333)
export class ClientCameraGlobals {
    // 0059a900 의 현재 16DWORD 전역 snapshot(최초 BSS 0)이다.
    #snapshot = new CameraView();
    // 논리 Native Renderer 의 별도 앞 64BYTE(전역 snapshot 과 주소가 같지 않음)이다.
    #rendererView = new CameraView();
    // 원문 +48/+4c(한계), +50/+54(다음 픽셀 원점), +58(다음 배율)이며 현재 view 와 독립이다.
    #target = { limits: new Int32Array(2), pixels: new Int32Array(2), zoom: 0 };
    // 00540ce0/ce4: PE 초기값 15/15 의 별도 배경 분모 두 DWORD 이다(Q16 이나 카메라 원점과 합치지 않음).
    #denominator = { defaultValue: 15, current: 15 };
    // 최초 BSS 0 의 최근 건설 위치 이력과 숫자 키 저장 위치이다.
    #constructionHistory = createConstructionHistory();
    #savedPositions = new SavedCameraPositions();

    // 0059a900 의 현재 16DWORD 이다. 임의 픽셀에서 재계산하지 않는다. (2026-11-12 WB04-E333)
    snapshot() { return this.#snapshot; }

    // 논리 Native Renderer 의 별도 앞 64BYTE 이다. (2026-11-12 WB04-E333)
    rendererView() { return this.#rendererView; }

    // 원문 목표·한계(limits[2]·pixels[2]·zoom)이다. 목표 쓰기만으로 현재 view 를 바꾸지 않는다. (2026-11-12 WB04-E333)
    target() { return this.#target; }

    // 두 별도 분모(defaultValue·current)의 같은 장기 owner 이다. (2026-11-12 WB04-E333)
    denominator() { return this.#denominator; }

    // 최근 40BYTE(5점)와 별도 Tab 순번의 유일한 Client owner 이다. (2026-11-12 WB04-E333)
    constructionHistory() { return this.#constructionHistory; }

    // 005caeb0 의 80BYTE 숫자 키 저장 위치이다. (2026-11-12 WB04-E333)
    savedPositions() { return this.#savedPositions; }

    // 실제 4993c0 의 명시 초기화: 논리 Renderer 의 기본 view → 칸 크기×세계 크기−화면 크기의 ftol 하위값(두 한계, 배율은 +58)과 목표 원점 0 → snapshot 으로 16DWORD 복사 → worldRedraw 1 → mapRevision 증가이다.
    // 화면 크기가 0 이하면 거부한다. 세계 크기는 저장된 단정도 값을 두 축에 재사용한다. (2026-11-12 WB04-E333)
    initializeNativeRenderer(width, height, worldCells, worldRedraw, mapRevision) {
        if (width <= 0 || height <= 0) throw new RangeError('Native Renderer 초기 화면 크기 오류');
        this.#rendererView.initialize();
        const cells = Math.fround(worldCells);
        // 실제 PE 는 +48 → +58 → +4c 순서로 쓴다(497f80 의 두 한계 store 와 구별).
        this.#target.limits[0] = cameraFtolLow(cell(this.#rendererView, CELL_X) * cells - width);
        this.#target.zoom = this.#rendererView.data.getUint32(ZOOM, true);
        this.#target.limits[1] = cameraFtolLow(cell(this.#rendererView, CELL_Y) * cells - height);
        this.#target.pixels[0] = 0;
        this.#target.pixels[1] = 0;
        this.#snapshot.bytes.set(this.#rendererView.bytes);
        worldRedraw.value = 1;
        mapRevision.value = (mapRevision.value + 1) >>> 0;
    }

    // 497f80: 칸 크기(double 곱셈의 중간 정밀도 유지) × 입력 단정도 세계 크기 − 현재 화면 정수의 ftol 하위값을 두 한계에 쓴다. view/snapshot 을 초기화하지 않는다. (2026-11-12 WB04-E333)
    updateTargetLimits(width, height, worldWidth, worldHeight) {
        this.#target.limits[0] = cameraFtolLow(cell(this.#rendererView, CELL_X) * Math.fround(worldWidth) - width);
        this.#target.limits[1] = cameraFtolLow(cell(this.#rendererView, CELL_Y) * Math.fround(worldHeight) - height);
    }

    // 목표 픽셀 원점만 쓴다(497e00/497f20 의 좌표/전역 카운터 처리는 상위 호출자 몫). 현재 Renderer/snapshot 은 프레임의 적용 위치까지 보존한다. (2026-11-12 WB04-E333)
    requestPixels(x, y) {
        this.#target.pixels[0] = x;
        this.#target.pixels[1] = y;
    }

    // 10.78 의 497f20: 음수 16/11 곱 → ftol 하위 DWORD → NEG/SUB 로 목표 X/Y 를 쓰고 두 store 뒤에만 005c85dc(revision)를 증가한다. 화면 가장자리 제한은 뒤 Present 의 별도 경로이다. (2026-11-12 WB04-E333)
    requestCenter(x, y, width, height, revision) {
        const fx = Math.fround(x);
        const fy = Math.fround(y);
        this.#target.pixels[0] = wrap(0 - (cameraFtolLow(-16 * fx) >>> 0) - (Math.trunc(width / 2) >>> 0));
        this.#target.pixels[1] = wrap(0 - (cameraFtolLow(-11 * fy) >>> 0) - (Math.trunc(height / 2) >>> 0));
        revision.value = (revision.value + 1) >>> 0;
    }

    // 433aa0: 0059a8b0(worldRedraw) 증가 → 497f20 → 실제 497960 경계(commit)를 순서대로 실행한다. 화면 flip/목표 제한은 하지 않으며 commit 이 없으면 앞선 쓰기를 보존한 채 거부한다. (2026-11-12 WB04-E333)
    centerImmediately(x, y, width, height, worldRedraw, revision, commit) {
        worldRedraw.value = (worldRedraw.value + 1) >>> 0;
        this.requestCenter(x, y, width, height, revision);
        if (typeof commit !== 'function') throw new Error('즉시 카메라의 실제497960 장치 미연결');
        commit();
    }

    // 497960 의 두 497490 호출 조각이다: 논리 Renderer 와 전역 snapshot 에 같은 픽셀 원점을 쓴다. scroll/dirty/커서 통지 전체 부모 완료로 세지 않는다. (2026-11-12 WB04-E333)
    applyPixelsToRendererAndSnapshot(x, y) {
        this.#rendererView.setPixels(x, y);
        this.#snapshot.setPixels(x, y);
    }

    // 436450 프레임 말미의 16DWORD 복사이다(목표/viewport 는 복사하지 않음). (2026-11-12 WB04-E333)
    copyRendererSnapshot() {
        this.#snapshot.bytes.set(this.#rendererView.bytes);
    }

    // 4978d0: 0 이면 기본 분모를 고르고, 현재값과 다를 때만 현재 분모를 먼저 저장한 뒤 worldRedraw 를 1 로 쓴다. (2026-11-12 WB04-E333)
    selectDenominator(value, worldRedraw) {
        const chosen = value === 0 ? this.#denominator.defaultValue : value;
        if (chosen !== this.#denominator.current) {
            this.#denominator.current = chosen;
            worldRedraw.value = 1;
        }
    }
}
