// Canvas 영상과 게임 전용 출력 버스를 함께 녹화한다. 사용자 마이크·화면 공유 장치는 열지 않는다.
// 생성: new GameRecorder({ canvas, audio: () => 현재 AudioSystem, view: window }). start/stop 뒤 dispose로 트랙·주소를 반납한다.
import { downloadBytes } from './external-opener.js';
const BYTE_LIMIT = 256 * 1024 * 1024;
// Chromium·Firefox의 게임 영상에 사용할 WebM 형식 후보다.
const MIME_TYPES = ['video/webm;codecs=vp8,opus', 'video/webm;codecs=vp9,opus', 'video/webm'];
// 정수 옵션 범위를 검사하고 생략한 값에는 기본값을 쓴다.
function integer(value, fallback, low, high, name) {
    const selected = value ?? fallback;
    if (!Number.isInteger(selected) || selected < low || selected > high) throw new RangeError(`${name} 범위는 ${low}~${high} 정수입니다.`);
    return selected;
}
// 녹화 수명과 결과 Blob 주소를 소유한다. Game/Kernel의 시계·객체를 바꾸지 않는다.
export class GameRecorder {
    // 현재 Canvas·소리 조회기와 해당 문서의 브라우저 API다.
    #canvas; #audio; #view;
    // 한 번에 한 녹화만 소유하며 종료 중 호출은 같은 Promise를 돌려준다.
    #recorder = null; #result = null; #pending = null; #resolve = null; #reject = null;
    // 실제 파일 내려받기는 공통 플랫폼 경계에 전달하며 원본 Blob은 다음 녹화/반납까지만 유지한다.
    #blob = null;
    // 실제 장치·출력 분기와 완료 전 인코딩 조각이다.
    #destination = null; #bus = null; #stream = null; #chunks = []; #bytes = 0;
    // 벽시각 녹화 한도·시작 시각·실패 원인이다. 게임 정지 중에도 녹화 시간은 흐른다.
    #timer = null; #began = 0; #error = null; #disposed = false;
    // 내려받기 전 결과 주소의 생존을 관리한다.
    #urls = new Set();
    // 실제 문서의 Canvas와 현재 소리 인스턴스만 빌린다.
    constructor({ canvas, audio, view = globalThis }) {
        if (!canvas || typeof audio !== 'function') throw new TypeError('게임 녹화의 Canvas·소리 조회기 필요');
        this.#canvas = canvas; this.#audio = audio; this.#view = view;
    }
    // 호스트 상태 조회에 쓰는 작은 값 사본이다.
    status() { return { recording: this.#recorder?.state === 'recording', stopping: Boolean(this.#recorder && this.#recorder.state !== 'recording'), bytes: this.#bytes, lastResult: this.#result ? { ...this.#result } : null }; }
    // 현재 화면·게임 소리만 연결한 뒤 인코더를 시작한다. 실패하면 이미 만든 장치도 반납한다.
    start({ framesPerSecond, maximumSeconds } = {}) {
        if (this.#disposed) throw new Error('이미 반납된 게임 녹화기입니다.');
        if (this.#recorder) throw new Error('게임 녹화가 이미 진행 중입니다.');
        const fps = integer(framesPerSecond, 30, 5, 60, '녹화 FPS'), seconds = integer(maximumSeconds, 300, 1, 1800, '녹화 시간');
        const audio = this.#audio(), Recorder = this.#view.MediaRecorder;
        if (!Recorder || typeof this.#canvas.captureStream !== 'function') throw new Error('이 브라우저는 Canvas WebM 녹화를 지원하지 않습니다.');
        if (!audio?.context || audio.context.state !== 'running' || !audio.output) throw new Error('시작 화면을 클릭하고 게임 소리 장치를 준비한 뒤 녹화하십시오.');
        const mimeType = MIME_TYPES.find(type => Recorder.isTypeSupported(type));
        if (!mimeType) throw new Error('게임 영상·소리를 함께 담는 WebM 형식을 지원하지 않습니다.');
        // 다음 녹화를 시작하면 이전 결과 주소를 반납해 큰 Blob을 계속 쌓지 않는다.
        for (const url of this.#urls) this.#view.URL.revokeObjectURL(url);
        this.#urls.clear();
        this.#chunks = []; this.#bytes = 0; this.#result = null; this.#blob = null; this.#error = null; this.#began = Date.now();
        try {
            // 모든 게임 소리의 단일 unity 버스를 추가 분기해 기존 스피커 출력을 유지한다.
            this.#bus = audio.output; this.#destination = audio.context.createMediaStreamDestination(); this.#bus.connect(this.#destination);
            this.#stream = this.#canvas.captureStream(fps);
            // 영상 스트림을 즉시 소유한 뒤 오디오를 붙여 부분 실패에도 모든 트랙을 반납한다.
            for (const track of this.#destination.stream.getAudioTracks()) this.#stream.addTrack(track);
            this.#recorder = new Recorder(this.#stream, { mimeType, videoBitsPerSecond: 4000000, audioBitsPerSecond: 128000 });
            this.#pending = new Promise((resolve, reject) => { this.#resolve = resolve; this.#reject = reject; });
            // 자동 종료의 실패도 미처리 Promise 경고로 게임 루프에 전파하지 않는다.
            this.#pending.catch(() => {});
            this.#recorder.ondataavailable = event => {
                if (event.data?.size) { this.#chunks.push(event.data); this.#bytes += event.data.size; }
                if (this.#bytes >= BYTE_LIMIT && this.#recorder?.state === 'recording') this.stop().catch(() => {});
            };
            this.#recorder.onerror = event => { this.#error = event.error ?? new Error('게임 녹화 인코더 오류'); if (this.#recorder?.state === 'recording') this.stop().catch(() => {}); };
            this.#recorder.onstop = () => this.#finish(mimeType);
            this.#recorder.start(1000);
            this.#timer = this.#view.setTimeout(() => { this.stop().catch(() => {}); }, seconds * 1000);
            return { recording: true, mimeType, framesPerSecond: fps, maximumSeconds: seconds, includesGameAudio: true };
        } catch (error) { this.#cleanup(); this.#recorder = null; this.#reject?.(error); this.#resolve = null; this.#reject = null; this.#pending = null; throw error; }
    }
    // 활성 인코더의 마지막 조각까지 받은 뒤 결과를 반환한다. 중복 종료는 같은 결과를 유지한다.
    stop() {
        if (!this.#recorder) return this.#result ? Promise.resolve(this.#result) : Promise.reject(new Error('진행 중인 게임 녹화가 없습니다.'));
        if (this.#recorder.state === 'recording' || this.#recorder.state === 'paused') this.#recorder.stop();
        return this.#pending;
    }
    // 완료한 게임 영상만 기존 파일 내려받기 장치에 전달한다. 별도의 주소 이동 경계를 만들지 않는다.
    download() {
        if (this.#disposed || !this.#blob || !this.#result) throw new Error('내려받을 완료 녹화가 없습니다.');
        return downloadBytes(this.#canvas.ownerDocument, this.#result.fileName, this.#blob, this.#result.mimeType);
    }
    // 인코더 종료 후에만 Blob을 만들어 파일명·다운로드 주소·크기와 함께 보존한다.
    #finish(mimeType) {
        const resolve = this.#resolve, reject = this.#reject;
        try {
            if (this.#error) throw this.#error;
            const blob = new this.#view.Blob(this.#chunks, { type: mimeType });
            if (!blob.size) throw new Error('게임 녹화 결과가 비어 있습니다.');
            this.#blob = blob;
            const url = this.#disposed ? null : this.#view.URL.createObjectURL(blob);
            if (url) this.#urls.add(url);
            this.#result = { mimeType: blob.type, bytes: blob.size, fileName: `Netstorm-${this.#began}.webm`, url, durationMilliseconds: Date.now() - this.#began };
            resolve(this.#result);
        } catch (error) { reject(error); }
        finally { this.#cleanup(); this.#recorder = null; this.#chunks = []; this.#resolve = null; this.#reject = null; this.#pending = null; }
    }
    // 녹화 분기만 끊으며 게임의 스피커 출력 연결은 유지한다.
    #cleanup() {
        if (this.#timer !== null) this.#view.clearTimeout(this.#timer); this.#timer = null;
        // 문맥 종료로 이미 끊긴 분기는 무시하되 트랙·타이머 정리를 끝까지 수행한다.
        try { if (this.#bus && this.#destination) this.#bus.disconnect(this.#destination); } catch {}
        // Canvas 영상과 녹화용 오디오 트랙만 종료한다.
        for (const track of this.#stream?.getTracks() ?? []) track.stop();
        for (const track of this.#destination?.stream.getTracks() ?? []) track.stop();
        this.#stream = null; this.#destination = null; this.#bus = null;
    }
    // 한 번의 반납으로 자동 종료·실패·완료 주소까지 정리한다.
    async dispose() {
        if (this.#disposed) return; this.#disposed = true;
        if (this.#recorder) { try { await this.stop(); } catch {} }
        // 결과 주소를 모두 반납해 녹화를 반복해도 큰 Blob을 계속 붙잡지 않는다.
        for (const url of this.#urls) this.#view.URL.revokeObjectURL(url);
        this.#urls.clear(); this.#result = null; this.#blob = null;
    }
}
