// 모드마다 불투명 iframe/Worker를 소유하고 실제 호스트 시계로 실행을 중단하는 신뢰 브리지다.
import { ModError, MOD_API_VERSION, MOD_ERROR_CODES, MOD_LIMITS, copyModJson } from './mod-api.js';
import { loadModWorkerSource } from './mod-worker-source.js';

// unsafe-eval은 Worker의 신뢰 SES 컴파일러에만 필요하다. guest의 eval/Function과 생성자 경로는 SES가 닫는다.
const POLICY = "default-src 'none'; script-src 'unsafe-inline' 'unsafe-eval' blob:; worker-src blob:; connect-src 'none'; frame-src 'none'; object-src 'none'; base-uri 'none'; form-action 'none'";
// 신뢰 SDK 시작과 종료 중계에만 별도 여유를 주며 모드 평가의 한도는 계약의 1000ms다.
const BOOT_TIMEOUT = 10000, CLOSE_TIMEOUT = 100;

// srcdoc 안에서만 실행한다. 부모를 확인한 첫 메시지로 만든 전용 포트 외의 입력은 무시한다.
function frameBootstrap() {
    let started = false;
    addEventListener('message', (event) => {
        if (started || event.source !== parent || !event.ports[0]) return;
        started = true;
        const port = event.ports[0];
        try {
            const url = URL.createObjectURL(new Blob([event.data.source], { type: 'text/javascript' })), worker = new Worker(url);
            URL.revokeObjectURL(url);
            worker.onmessage = (reply) => port.postMessage(reply.data);
            worker.onerror = (error) => { error.preventDefault(); port.postMessage({ type: 'boot-error', message: String(error.message).slice(0, 2048) }); };
            // 종료 통지를 보낸 뒤에도 부모가 포트를 닫을 때까지 살아 있어 중단 수신을 확인할 수 있다.
            port.onmessage = (request) => {
                if (request.data?.op === 'terminate') { worker.terminate(); port.postMessage({ type: 'terminated' }); port.close(); }
                else worker.postMessage(request.data);
            };
            port.start();
        } catch (error) { port.postMessage({ type: 'boot-error', message: String(error?.message ?? error).slice(0, 2048) }); }
    });
}

// 부모는 SDK 결과도 복사/한도 검사한다. 모드 권한으로 브리지나 DOM 참조를 반환하지 않는다.
export class ModWorker {
    // DOM/메시지 자원과 단일 대기 요청은 브리지 수명 동안만 유지한다.
    #view; #frame; #port; #pending = null; #nextId = 1; #ready; #boot = null; #closed = false; #closeTimer = null;
    // 앱 종료 중 SDK 부팅/설치 검증을 즉시 취소하는 호스트 전용 신호다.
    #signal; #abort;
    // 신뢰 소스를 먼저 읽고 브라우저 장치가 준비된 인스턴스만 반환한다. source는 신뢰 시험 호스트의 주입점이다.
    static async open({ view = globalThis, source, signal } = {}) {
        if (signal?.aborted) throw new ModError('disposed', '모드 실행기 적재가 취소되었습니다.');
        const input = source ?? await loadModWorkerSource({ view, signal });
        if (signal?.aborted) throw new ModError('disposed', '모드 실행기 적재가 취소되었습니다.');
        const worker = new ModWorker(view, input, signal);
        await worker.#ready; return worker;
    }
    // sandbox에는 allow-same-origin을 주지 않는다. Worker Blob도 iframe 안에서 만들어 CSP를 상속한다.
    constructor(view, source, signal) {
        this.#view = view;
        this.#ready = new Promise((resolve, reject) => { this.#boot = { resolve, reject, timer: view.setTimeout(() => this.#fail(new ModError('scriptError', '모드 실행기 시작 시간 초과')), BOOT_TIMEOUT) }; });
        try {
            const frame = view.document.createElement('iframe'), channel = new view.MessageChannel();
            this.#frame = frame; this.#port = channel.port1;
            frame.hidden = true; frame.tabIndex = -1; frame.setAttribute('aria-hidden', 'true'); frame.setAttribute('sandbox', 'allow-scripts');
            frame.srcdoc = `<meta http-equiv="Content-Security-Policy" content="${POLICY}"><script>(${frameBootstrap.toString()})();<` + '/script>';
            this.#port.onmessage = ({ data }) => this.#receive(data); this.#port.start();
            frame.onload = () => { if (!this.#closed) frame.contentWindow.postMessage({ source }, '*', [channel.port2]); };
            view.document.body.append(frame);
            this.#signal = signal; this.#abort = () => this.dispose();
            signal?.addEventListener('abort', this.#abort, { once: true });
            if (signal?.aborted) this.dispose();
        } catch (error) { this.#fail(new ModError('scriptError', String(error?.message ?? error).slice(0, 2048))); }
    }
    // 실제 시간 감시 또는 규약 오류는 해당 Worker만 닫고 대기 Promise를 거절한다.
    #fail(error) { this.dispose(error); }
    // 준비/결과/반납 메시지의 버전과 요청 번호를 확인한다. 닫힌 세션의 늦은 응답은 버린다.
    #receive(data) {
        if (data?.type === 'terminated' && this.#closed) { this.#cleanup(); return; }
        if (this.#closed) return;
        if (this.#boot && data?.type === 'ready' && data.protocol === 1 && data.apiVersion === MOD_API_VERSION) {
            const boot = this.#boot; this.#boot = null; this.#view.clearTimeout(boot.timer); boot.resolve(); return;
        }
        const pending = this.#pending;
        if (!this.#boot && pending && data?.type === 'result' && data.protocol === 1 && data.id === pending.id && data.op === pending.op && (Object.hasOwn(data, 'result') !== Object.hasOwn(data, 'error'))) {
            if (Object.hasOwn(data, 'error')) { this.#fail(new ModError(MOD_ERROR_CODES.includes(data.error?.code) ? data.error.code : 'scriptError', typeof data.error?.message === 'string' ? data.error.message.slice(0, 2048) : '모드 실행 오류')); return; }
            try {
                const result = pending.op === 'validate' ? this.#packageResult(data.result) : copyModJson(data.result);
                this.#pending = null; this.#view.clearTimeout(pending.timer); pending.resolve(result);
            } catch (error) { this.#fail(error); }
            return;
        }
        this.#fail(new ModError('scriptError', '모드 실행기 응답/순서 오류'));
    }
    // validate 응답은 guest 실행 전의 신뢰 분석기 결과다. 큰 구문 트리 분석을 메인 페이지에서 반복하지 않는다.
    #packageResult(text) {
        if (typeof text !== 'string' || text.length > MOD_LIMITS.maxPackageBytes || new TextEncoder().encode(text).length > MOD_LIMITS.maxPackageBytes) throw new ModError('invalidPackage', '모드 검증 결과 한도 초과');
        const pkg = JSON.parse(text); Object.freeze(pkg.manifest); Object.freeze(pkg.modules); return Object.freeze(pkg);
    }
    // 한 번에 한 요청만 보내며 타이머는 게임의 정지/고정 시계와 독립이다.
    async #request(op, payload) {
        await this.#ready;
        if (this.#closed) throw new ModError('disposed', '모드 Worker가 반납되었습니다.');
        if (this.#pending) throw new ModError('scriptError', '모드 Worker 동시 요청 금지');
        return new Promise((resolve, reject) => {
            const id = this.#nextId++, timer = this.#view.setTimeout(() => this.#fail(new ModError('timeLimit', '모드 실행 시간 한도 초과(1000ms)')), MOD_LIMITS.timeLimitMilliseconds);
            this.#pending = { id, op, resolve, reject, timer };
            try { this.#port.postMessage({ protocol: 1, id, op, ...payload }); }
            catch (error) { this.#fail(new ModError('scriptError', '모드 요청을 직렬화할 수 없습니다.')); }
        });
    }
    // 설치 검증은 코드를 평가하지 않는다. 호출자는 완료 후 검증용 Worker를 반납한다.
    validate(input) {
        if (!((typeof input === 'string' && input.length <= MOD_LIMITS.maxPackageBytes && new TextEncoder().encode(input).length <= MOD_LIMITS.maxPackageBytes) || (input instanceof Uint8Array && input.byteLength <= MOD_LIMITS.maxPackageBytes))) return Promise.reject(new ModError('invalidPackage', '모드 패키지 입력/크기 오류'));
        return this.#request('validate', { package: input });
    }
    // 등록과 사건 전달은 별도 Worker의 SES Compartment에서 실행한다.
    register(pkg, sessionSeed) { return this.#request('register', { package: typeof pkg === 'string' ? pkg : JSON.stringify(pkg), sessionSeed }); }
    // 요청 값도 공통 JSON 한도를 먼저 검사해 호스트 객체를 보내지 않는다.
    dispatch(event, snapshot) { return this.#request('dispatch', { event: copyModJson(event), snapshot: copyModJson(snapshot) }); }
    // 즉시 비활성화한 뒤 종료 수신 또는 짧은 최종 기한에 포트/iframe을 제거한다.
    dispose(error = new ModError('disposed', '모드 Worker가 반납되었습니다.')) {
        if (this.#closed) return;
        this.#closed = true;
        if (this.#boot) { this.#view.clearTimeout(this.#boot.timer); this.#boot.reject(error); this.#boot = null; }
        if (this.#pending) { this.#view.clearTimeout(this.#pending.timer); this.#pending.reject(error); this.#pending = null; }
        this.#closeTimer = this.#view.setTimeout(() => this.#cleanup(), CLOSE_TIMEOUT);
        try { this.#port?.postMessage({ op: 'terminate' }); } catch (failure) { this.#cleanup(); }
    }
    // 반납 확인 이후에도 남는 DOM 콜백/메시지 closure를 끊는다.
    #cleanup() {
        this.#signal?.removeEventListener('abort', this.#abort); this.#signal = null; this.#abort = null;
        if (this.#closeTimer !== null) this.#view.clearTimeout(this.#closeTimer);
        this.#closeTimer = null;
        if (this.#port) { this.#port.onmessage = null; this.#port.close(); this.#port = null; }
        if (this.#frame) { this.#frame.onload = null; this.#frame.remove(); this.#frame = null; }
    }
}

// 저장소에 주입하는 검증기다. 구문 오류/큰 패키지/무한 분석도 UI 스레드와 분리한다.
export async function validateModPackageIsolated(input, options) {
    const worker = await ModWorker.open(options);
    try { return await worker.validate(input); } finally { worker.dispose(); }
}
