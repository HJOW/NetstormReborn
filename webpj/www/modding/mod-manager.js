// 미션마다 새 격리 인스턴스를 만들고 같은 프레임 사본/원래 입력 경계로만 모드를 구동한다.
import { copyModJson, MOD_LIMITS } from './mod-api.js';
import { ModWorker } from './mod-worker.js';

// 게임 객체를 붙잡지 않는 기본 진단 장치다. guest 로그도 이미 공통 JSON 경계를 통과했다.
function defaultReport(kind, detail) { if (kind === 'log') console[detail.level](`[mod:${detail.modId}] ${detail.text}`); else console.warn('[mod]', kind, detail); }

// host는 ModGameHost, source는 ModTimeSource다. 시험은 같은 계약의 유계 장치를 주입할 수 있다.
export class ModManager {
    // 앱 참조는 host 하나에만 있고 dispose에서 즉시 끊는다. scope에는 값 사본/Worker만 둔다.
    #host; #source; #store; #create; #report; #forcedOff; #scope = null; #scopes = new Set();
    // 비동기 사건은 직렬화하며 모든 작업 동안 한 시계 장벽을 유지한다.
    #tail = Promise.resolve(); #jobs = 0; #held = false; #closing = false; #loaded = false; #error = null;
    // 공개 API가 아닌 신뢰 호스트 조립점이다. fixed/oracle 모드는 저장소 읽기/실행을 모두 건너뛴다.
    constructor({ host, source, store, createWorker = (options) => ModWorker.open(options), report = defaultReport, forcedOff = false }) { this.#host = host; this.#source = source; this.#store = store; this.#create = createWorker; this.#report = report; this.#forcedOff = forcedOff; }
    // 초기 적재 실패를 기존 저장 자료를 덮어쓰는 빈 성공으로 바꾸지 않는다.
    async load() {
        if (this.#closing || this.#loaded || this.#forcedOff) return;
        try { await this.#store.load(); if (!this.#closing) this.#loaded = true; }
        catch (error) { if (!this.#closing) { this.#error = String(error?.message ?? error).slice(0, 2048); this.#report('storageError', { message: this.#error }); } }
    }
    // UI는 자료 오류와 현재 실행 여부를 읽되 Worker/Session을 받지 않는다.
    get error() { return this.#error; }
    get pending() { return !this.#closing && this.#jobs > 0; }
    // 검증/설치 UI가 공유할 정상 저장소다. 모드 전역에는 제공하지 않는다.
    get store() { return this.#store; }
    // 명시 저장소 재읽기를 UI에서 요청한다. 이미 시작한 scope의 패키지 사본은 바꾸지 않는다.
    async reload() { this.#loaded = false; this.#error = null; await this.load(); }
    // 중첩 작업이 모두 끝나야 원래 Client 시각을 다시 흐르게 한다.
    #release() { if (this.#held) { this.#held = false; this.#source.leave(); } }
    // 처리되지 않은 Promise 거절을 만들지 않고 호스트 장치 오류도 진단으로 남긴다.
    #schedule(action) {
        this.#jobs += 1;
        if (!this.#held && !this.#closing) { this.#held = true; this.#source.enter(); }
        this.#tail = this.#tail.then(action).catch((error) => { if (!this.#closing) this.#report('hostError', { message: String(error?.message ?? error).slice(0, 2048) }); }).finally(() => { this.#jobs -= 1; if (!this.#jobs) this.#release(); });
        return this.#tail;
    }
    // 시험/앱 정리에서 현재 직렬 작업까지 기다린다. 게임 루프는 pending만 읽는다.
    whenIdle() { return this.#tail; }
    // 해당 모드를 즉시 반납한 다음 저장 활성도 끈다. 실패해도 다른 모드의 사건을 막지 않는다.
    async #fault(item, error, phase) {
        item.disabled = true; item.worker?.dispose(); item.worker = null;
        if (this.#closing) return;
        const message = `${phase}: ${error?.code ?? 'scriptError'}: ${String(error?.message ?? error)}`.slice(0, 2048);
        this.#report('runtimeError', { modId: item.id, message });
        try { await this.#store.setEnabled(item.id, false, { error: message, expectedPackageText: JSON.stringify(item.pkg) }); }
        catch (failure) { if (!this.#closing) { this.#error = String(failure?.message ?? failure).slice(0, 2048); this.#report('storageError', { modId: item.id, message: this.#error }); } }
    }
    // 로그는 콜백 성공 뒤에만, 입력은 다음 원래 입력 직전까지 사본으로 보관한다.
    #output(scope, item, output, ending = false) {
        if (((this.#closing || scope.ending) && !ending) || item.disabled) return;
        const copied = copyModJson(output);
        // 로그는 공개 API가 정규화한 결과지만 브리지 교체 시에도 비정상 레벨을 출력하지 않는다.
        for (const log of copied.logs) if (['info', 'warn', 'error'].includes(log.level)) this.#report('log', { modId: item.id, ...log });
        if (!ending && copied.commands.length) scope.staged.push({ item, output: copied });
    }
    // 모드별 sequence를 정확히 한 번 올리고 같은 경계의 사본을 전달한다.
    async #dispatch(scope, item, name, snapshot, extra = {}) {
        if (item.disabled || !item.worker) return;
        const event = { name, sequence: ++item.sequence, sessionId: scope.id, ...(name === 'commandResult' ? {} : { gameSeconds: snapshot.state.gameSeconds }), ...(name === 'sessionStart' || name === 'sessionEnd' ? { mission: snapshot.state.mission } : {}), ...extra };
        try { const output = await item.worker.dispatch(event, snapshot); this.#output(scope, item, output, name === 'sessionEnd'); }
        catch (error) { await this.#fault(item, error, name); }
    }
    // 준비된 실제 Session을 프레임 종료 때 읽는다. 고정 오라클/미설치 경로에는 비동기 대기를 만들지 않는다.
    afterFrame() {
        if (this.#closing || this.#forcedOff || !this.#loaded || this.pending) return;
        const snapshot = this.#host.snapshot(), id = snapshot.state.sessionId;
        if (this.#scope?.id !== id) {
            this.end('replace');
            if (!id) return;
            const enabled = this.#store.list().filter((item) => item.enabled).sort((a, b) => a.manifest.id < b.manifest.id ? -1 : a.manifest.id > b.manifest.id ? 1 : 0);
            const items = enabled.map(({ manifest }) => ({ id: manifest.id, pkg: this.#store.getPackage(manifest.id), worker: null, disabled: false, sequence: 0, results: [] }));
            const scope = { id, snapshot, seed: this.#host.sessionSeed(), items, staged: [], nextTick: snapshot.state.gameSeconds + MOD_LIMITS.tickGameSeconds, ending: false };
            this.#scope = scope; this.#scopes.add(scope);
            if (!items.length) return;
            this.#schedule(async () => {
                // ModStore.list는 id ASCII 순서다. 등록/시작 효과도 이 순서를 유지한다.
                for (const item of items) {
                    if (this.#closing || scope.ending) break;
                    try {
                        item.opening = new AbortController();
                        item.worker = await this.#create({ signal: item.opening.signal }); item.opening = null;
                        if (this.#closing || scope.ending) { item.worker.dispose(); item.worker = null; break; }
                        this.#output(scope, item, await item.worker.register(item.pkg, scope.seed));
                    } catch (error) { item.opening = null; if (!this.#closing && !scope.ending) await this.#fault(item, error, 'register'); }
                }
                // 모든 등록이 끝나야 같은 경계의 sessionStart를 보낸다.
                for (const item of items) { if (this.#closing || scope.ending) break; await this.#dispatch(scope, item, 'sessionStart', snapshot); }
            });
            return;
        }
        const scope = this.#scope;
        if (!scope) return; scope.snapshot = snapshot;
        if (!scope.items.length || scope.items.every((item) => item.disabled)) return;
        const tick = snapshot.state.gameSeconds >= scope.nextTick, results = scope.items.some((item) => !item.disabled && item.results.length);
        if (!tick && !results) return;
        if (tick) scope.nextTick = snapshot.state.gameSeconds + MOD_LIMITS.tickGameSeconds;
        this.#schedule(async () => {
            // 결과 사건은 모드당 경계당 최대1회/16항목이다. 사건 안에서 만든 입력은 다음 경계까지 기다린다.
            for (const item of scope.items) {
                if (this.#closing || scope.ending) return;
                if (item.disabled || !item.results.length) continue;
                const pending = item.results; item.results = [];
                await this.#dispatch(scope, item, 'commandResult', snapshot, { results: pending });
            }
            // 큰 시간 점프도 tick 한 번으로 합치고 모드 id 순서로 처리한다.
            if (tick) for (const item of scope.items) { if (this.#closing || scope.ending) return; await this.#dispatch(scope, item, 'tick', snapshot); }
        });
    }
    // Guide 뒤 원래 입력 단계 앞에서 호출한다. 기존 사람 입력 뒤에 FIFO로 예약한다.
    beforeInput() {
        if (this.#closing || this.pending || !this.#scope) return;
        const scope = this.#scope, staged = scope.staged; scope.staged = [];
        // staged는 경계→사건 phase→id 순서다. 각 모드의 지역 sequence를 서로 섞어 정렬하지 않는다.
        for (const { item, output } of staged) {
            if (item.disabled || scope.ending) continue;
            try { item.results.push(...this.#host.commit(output, { eventSequence: output.eventSequence, sessionId: scope.id })); }
            catch (error) { this.#schedule(() => this.#fault(item, error, 'input')); }
        }
    }
    // 원래 World를 반납하기 전에 호출한다. 비동기 종료에는 마지막 값 사본만 남긴다.
    end(reason = 'leave') {
        const scope = this.#scope; if (!scope) return;
        this.#scope = null; scope.ending = true; scope.staged = [];
        // 아직 guest를 시작하지 않은 부팅은 이탈 자체를 모드 오류로 저장하지 않고 취소한다.
        for (const item of scope.items) item.opening?.abort();
        if (!scope.items.length) { this.#scopes.delete(scope); return; }
        this.#schedule(async () => {
            try {
                // sessionStart를 받지 못한 VM에는 sessionEnd를 보내지 않고 바로 반납한다.
                for (const item of scope.items) if (item.sequence > 0 && !item.disabled) await this.#dispatch(scope, item, 'sessionEnd', scope.snapshot, { reason });
            } finally { for (const item of scope.items) { item.worker?.dispose(); item.worker = null; } this.#scopes.delete(scope); }
        });
    }
    // ResetAll/앱 종료 뒤 오류 저장을 금지하고 앱/시계 장벽을 즉시 놓는다. 마지막 사건은 사본만으로 끝낸다.
    dispose() {
        if (this.#closing) return this.#tail;
        this.end('destroy'); this.#closing = true; this.#host.dispose(); this.#store.dispose(); this.#release();
        return this.#tail.finally(() => { for (const scope of this.#scopes) for (const item of scope.items) item.worker?.dispose(); this.#scopes.clear(); });
    }
}
