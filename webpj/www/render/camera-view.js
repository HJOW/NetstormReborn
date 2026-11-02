// 10.78 Renderer 앞 64바이트의 카메라 상태와 좌표 변환을 다룬다.
// 근거: cpppj RawCameraView.cpp(4973e0/497490/497220), RawCursorPosition.cpp(4971e0).
// 사용: const view = new CameraView(); view.initialize(); view.setPixels(17, 29); view.project(120, 130).
// 목표 이동·제한·보간·입력은 별도 정책이다. 생성 직후는 원본 BSS처럼 0이며 initialize는 명시 호출한다. (2026-11-02 WB03-Z)

// 원본 503b54/504194의 float32 역배율이다. 특히 Y를 double 1/11로 바꾸지 않는다.
const INVERSE_X = 0.0625;
const INVERSE_Y = Math.fround(1 / 11);
// 원본 signed64 _ftol 유효 범위의 상한(하한은 그 음수)이다.
const FTOL_LIMIT = 2 ** 63;

// signed64 절삭의 하위 signed32이다. indefinite(NaN/무한대/범위 밖)는 하위 DWORD가 0이다.
export function cameraFtolLow(value) {
    if (!Number.isFinite(value) || value < -FTOL_LIMIT || value >= FTOL_LIMIT) return 0;
    return Math.trunc(value) | 0;
}

// 정확한 16DWORD 상태이다. bytes는 원본 fixture·스냅숏용이며 복사 생성은 독립 저장소를 만든다.
export class CameraView {
    // 선택한 64바이트를 복사한다. 생략하면 BSS 0이다.
    constructor(bytes = new Uint8Array(64)) {
        if (!(bytes instanceof Uint8Array) || bytes.length !== 64) throw new RangeError('카메라 상태는 64바이트여야 합니다.');
        // 원본 순서의 저장 바이트와 little endian 읽기/쓰기 보기이다.
        this.bytes = new Uint8Array(bytes);
        this.data = new DataView(this.bytes.buffer);
    }

    // 같은 값의 독립 스냅숏이다. 이후 setter는 사본을 바꾸지 않는다.
    copy() { return new CameraView(this.bytes); }

    // 현재 signed 픽셀 원점이다. 월드 float와 서로 구분한다.
    get pixels() { return { x: this.data.getInt32(28, true), y: this.data.getInt32(32, true) }; }

    // 현재 float32 월드 원점이다. setWorld의 분수 부분을 보존한다.
    get world() { return { x: this.data.getFloat32(0, true), y: this.data.getFloat32(4, true) }; }

    // 모든 선행 store 뒤 현재 상태로 진단한다. callback이 상태를 바꾸면 Y 진단은 변경 뒤 값을 읽는다.
    _diagnose(assertion, firstLine) {
        // X·Y 순서로 원본 두 진단을 실행한다.
        for (let axis = 0; axis < 2; axis += 1) {
            if (this.data.getFloat32(axis * 4, true) >= 0 && this.data.getInt32(28 + axis * 4, true) < 0) {
                const message = axis === 0 ? 'scrollPosPixX >= 0' : 'scrollPosPixY >= 0';
                if (!assertion) throw new Error(`카메라 진단 미연결: ${message}`);
                assertion(message, firstLine + axis * 3);
            }
        }
    }

    // 4973e0: 입력 float32를 먼저 보존하고 정수 칸 크기와 double 곱→ftol로 픽셀을 얻는다.
    setWorld(x, y, assertion = null) {
        const d = this.data;
        d.setFloat32(0, x, true);
        d.setFloat32(4, y, true);
        d.setInt32(28, cameraFtolLow(d.getInt32(8, true) * d.getFloat32(0, true)), true);
        d.setInt32(32, cameraFtolLow(d.getInt32(12, true) * d.getFloat32(4, true)), true);
        d.setFloat32(56, d.getInt32(28, true), true);
        d.setFloat32(60, d.getInt32(32, true), true);
        this._diagnose(assertion, 0x344);
    }

    // 497490: signed32 입력을 Y→X 순서로 저장하고 원본 float32 역배율을 곱한다.
    setPixels(x, y, assertion = null) {
        const d = this.data;
        d.setInt32(32, y, true);
        d.setInt32(28, x, true);
        d.setFloat32(0, d.getInt32(28, true) * INVERSE_X, true);
        d.setFloat32(4, d.getInt32(32, true) * INVERSE_Y, true);
        d.setFloat32(56, d.getInt32(28, true), true);
        d.setFloat32(60, d.getInt32(32, true), true);
        this._diagnose(assertion, 0x354);
    }

    // 4993c0: 기본16×11·Q16 1배와 현재 원점0을 쓴다. 무관 DWORD를 지우지 않는다.
    initialize() {
        const d = this.data;
        d.setFloat32(48, 11, true);
        d.setFloat32(40, 11, true);
        d.setFloat32(44, 16, true);
        d.setFloat32(36, 16, true);
        d.setInt32(20, 11, true);
        d.setInt32(16, 16, true);
        d.setUint32(24, 65536, true);
        d.setFloat32(52, 1, true);
        d.setInt32(8, 16, true);
        d.setInt32(12, 11, true);
        this.setWorld(0, 0);
    }

    // 497220: float32 월드 좌표 ×16/11 +0.5의 ftol 뒤 현재 signed 픽셀 원점을 DWORD 감김으로 뺀다.
    // Math.round와 음수 좌표의 결과가 다르다. 그리기와 위치 음향이 같은 snapshot을 사용할 수 있다.
    project(x, y) {
        return {
            x: (cameraFtolLow(Math.fround(x) * 16 + 0.5) - this.data.getInt32(28, true)) | 0,
            y: (cameraFtolLow(Math.fround(y) * 11 + 0.5) - this.data.getInt32(32, true)) | 0,
        };
    }

    // 4971e0: 커서의 signed32 화면 위치에16/11을 더한 뒤 역배율·현재 월드 원점을 적용한다.
    // 그림 기준점의 단순 역함수가 아니며 배치 칸의 오른쪽 아래 경계를 돌려준다.
    screenToWorld(x, y) {
        return {
            x: Math.fround(((x + 16) | 0) * INVERSE_X + this.data.getFloat32(0, true)),
            y: Math.fround(((y + 11) | 0) * INVERSE_Y + this.data.getFloat32(4, true)),
        };
    }
}
