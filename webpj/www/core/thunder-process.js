// 번개 팔레트 프로세스(10.78 00470d40 등록·00470e30 초기화·00471650 프레임·00471820 중단)를 실제 ProcessForm/부모 종속 삭제 수명에 연결한 모듈이다(cpppj `client/ThunderProcess.cpp/.h`, 10.78 전용).
//  - THUNDER_PROCESS_TYPE(55): 0054114c 의 10.78 초기 TYPE 이다. 원문 생성자는 flags 0x10 으로 기술 부모에 부착한다 — `host.attach(process, THUNDER_PROCESS_TYPE, parentSid, 0x10)`.
//  - ThunderProcess: E376 의 `PaletteFlash` 계산(색 두 번 읽기·단계별 복구·전체화면에서만 팔레트 적용/복구)을 SquidProcess 한 개로 감싼다.
//      생성 인자 fullScreen 은 지속(전체화면 0.5초·창 모드 1/60초)을, brightness 는 요청 밝기(0 이면 기본 64)를 정한다. 팔레트는 부착 전에는 읽지 않는다(onAttached 가 처음 읽음).
//      runFrame(): 효과가 계속되면 아무것도 하지 않고, 끝났으면 Form 삭제 경로(host.kill(this, 0), 원문 0041bf10)로 자기를 지운다. kill 뒤에는 this 에 다시 접근하지 않는다.
//      destroy()(C++ ~ThunderProcess): 화면이 살아 있는 동안(initialized 이고 전체화면) 저장 팔레트를 복구하고 등록 목록 정리 통지 `destroyed(pid)` 를 부른다.
//      자연 종료는 PaletteFlash 가 이미 복구하고 initialized 를 껐으므로 추가 팔레트 변경이 없고, 외부 kill·부모 삭제로 끊기면 한 번만 복구한다. destroyed 는 Form 을 중복 삭제하지 않는다.
// 사용: const process = new ThunderProcess(fullScreen, { wallSeconds, fullScreen, readPalette, applyPalette, restorePalette }, (pid) => { … }, brightness); host.attach(process, THUNDER_PROCESS_TYPE, parent, 0x10).
// 장치(훅)는 PaletteFlash 와 같다(E376). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E381: 5-A 시각 효과 소단위로 추가, 기준선 뒤 cpppj 변경 없음)
import { SquidProcess } from './squid-process.js';
import { PaletteFlash, PaletteFlashState, thunderDuration, thunderOffset } from './palette-flash.js';

// 번개 프로세스의 10.78 TYPE 번호이다(0054114c). (2026-11-13 WB04-E381)
export const THUNDER_PROCESS_TYPE = 55;

// 번개 팔레트 효과를 Form/부모 종속 수명에 묶은 프로세스이다. 부착 전에는 form/parent/pid 가 0 이다. (2026-11-13 WB04-E381)
export class ThunderProcess extends SquidProcess {
    // 두 부분 팔레트·64단계·실시간 기한의 실제 효과 상태와, 그 상태에 색 계산·복구를 실행하는 기존 팔레트 효과이다.
    #state = new PaletteFlashState();
    #flash;
    // 소멸 때 현재 Kernel 번호를 알려 주는 정리 통지이다(null 이면 통지하지 않는다).
    #destroyed;

    // 생성 모드와 요청 밝기를 보관한다. 장치 묶음이 불완전하면 PaletteFlash 가 거부한다(생성 전에 팔레트를 읽지 않는다). 사용: new ThunderProcess(true, hooks, (pid) => {}, -17).
    constructor(fullScreen, hooks, destroyed = null, brightness = 0) {
        super();
        this.#state.duration = thunderDuration(fullScreen);
        this.#state.offset = thunderOffset(brightness);
        this.#flash = new PaletteFlash(this.#state, hooks);
        this.#destroyed = destroyed;
    }

    // 실제 Kernel/Form/부모 연결 뒤 원본 가상+14 에서 팔레트를 초기화한다. 공통 부착이 먼저 쓴 Form/부모/PID 를 그대로 둔다.
    onAttached() {
        this.#flash.init();
    }

    // 실시간 진행이 끝나면 원문 0041bf10 처럼 Form 삭제 경로로 자기를 지운다(삭제된 뒤에는 이 객체를 다시 읽지 않는다).
    runFrame() {
        if (!this.#flash.frame()) this.host.kill(this, 0);
    }

    // Kernel 이 지울 때 부르는 소멸 처리이다: 중단이면 현재 화면 팔레트를 복구하고 등록 목록에서 같은 PID 를 지운다.
    destroy() {
        this.#flash.cancel();
        if (this.#destroyed) this.#destroyed(this.pid);
    }
}
