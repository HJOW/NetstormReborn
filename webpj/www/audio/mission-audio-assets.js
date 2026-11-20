// 매니페스트의 미션 효과음과 음악을 필요한 때에만 받아 현재 Web Audio 재생기에 등록한다.
// 사용: new MissionAudioAssets(loader, audio) → loadSound(name) 또는 loadMusic(name) → dispose().

// 원본 소리 이름은 한 파일 이름이다. 경로·NUL을 거부해 데이터 루트 밖 요청을 만들지 않는다.
function soundKey(name, extension) {
    if (typeof name !== 'string' || !name || /[\\/\0]/.test(name) || name === '.' || name === '..' || !name.toLowerCase().endsWith(extension)) throw new TypeError('미션 음향의 파일 이름 오류');
    return name.toLowerCase();
}

// 원본 sscanf처럼 경로의 첫 하이픈 뒤 부호 있는 십진수를 감쇠로 읽는다.
export function soundFileAttenuation(path) {
    const match = /^[^-]+-\s*([+-]?\d+)/.exec(String(path));
    if (!match) return 0;
    return Math.max(-2147483648, Math.min(2147483647, Number(match[1]))) | 0;
}

// 같은 현재 매니페스트와 AudioSystem을 빌리는 지연 적재 소유자다.
export class MissionAudioAssets {
    // 경로 표, 진행 중인 동일 요청, 반납 상태를 유지한다.
    #loader; #audio; #entries = new Map(); #soundEntries = []; #decoded = new Map(); #pending = new Map(); #language; #disposed = false;

    // 공개 루트의 음악·효과음 항목만 인덱싱한다. 대소문자 구별 없는 중복은 거부한다.
    constructor(loader, audio, { language = 'english' } = {}) {
        if (!loader?.manifest || !Array.isArray(loader.manifest.files) || typeof loader.loadFile !== 'function' || typeof audio?.decode !== 'function' || !(audio.sounds instanceof Map) || !(audio.music instanceof Map)) throw new TypeError('미션 음향 적재기의 실제 매니페스트·재생기 누락');
        if (typeof language !== 'string' || !/^[a-z0-9_-]+$/i.test(language)) throw new TypeError('미션 음향 언어 이름 오류');
        this.#loader = loader; this.#audio = audio; this.#language = language.toLowerCase();
        // 기본·언어별 효과음과 기본 음악의 경로를 보관하고 대소문자 중복을 거부한다.
        for (const entry of loader.manifest.files) {
            const match = /^(sound|music)\/(?:([a-z0-9_-]+)\/)?([^/\\]+)$/i.exec(entry.path);
            if (!match) continue;
            const kind = match[1].toLowerCase(), directory = match[2]?.toLowerCase() ?? '', extension = kind === 'sound' ? '.wav' : '.mus';
            if (kind === 'music' && directory) continue;
            const name = soundKey(match[3], extension), key = `${kind}:${directory}:${name}`;
            if (this.#entries.has(key)) throw new Error(`미션 음향 중복 경로: ${entry.path}`);
            this.#entries.set(key, entry);
            if (kind === 'sound') this.#soundEntries.push({ directory, name, entry });
        }
    }

    // 언어 디렉토리 뒤 기본 디렉토리에서 감쇠 접미사 패턴을 일반 접두 패턴보다 먼저 찾는다.
    resolveSound(name) {
        if (this.#disposed) throw new Error('반납한 미션 음향 적재기');
        const key = soundKey(name, '.wav'), dot = key.lastIndexOf('.'), stem = key.slice(0, dot), extension = key.slice(dot);
        // 같은 우선순위 안에서는 매니페스트의 원래 등록 순서를 유지한다.
        for (const directory of [this.#language, '']) {
            for (const prefix of [`${stem}-`, stem]) {
                const found = this.#soundEntries.find((item) => item.directory === directory && item.name.startsWith(prefix) && item.name.endsWith(extension));
                if (found) return { entry: found.entry, attenuation: soundFileAttenuation(found.entry.path) };
            }
        }
        return null;
    }

    // 현재 장치와 이름을 확인한 뒤 같은 파일의 다운로드·해독을 동시 호출 간 공유한다.
    async #load(kind, name) {
        if (this.#disposed || this.#audio.closed) throw new Error('반납한 미션 음향 적재기');
        const key = soundKey(name, kind === 'sound' ? '.wav' : '.mus'), cache = kind === 'sound' ? this.#audio.sounds : this.#audio.music;
        if (cache.has(key)) return cache.get(key);
        const entry = kind === 'sound' ? this.resolveSound(name)?.entry : this.#entries.get(`music::${key}`);
        if (!entry) throw new Error(`미션 음향 파일 누락: ${name}`);
        const index = entry.path.toLowerCase();
        if (this.#decoded.has(index)) { const buffer = this.#decoded.get(index); cache.set(key, buffer); return buffer; }
        let pending = this.#pending.get(index);
        if (!pending) {
            pending = (async () => {
                const bytes = await this.#loader.loadFile(entry);
                if (this.#disposed || this.#audio.closed) throw new Error('반납한 미션 음향 적재기');
                let buffer;
                try { buffer = await this.#audio.decode(bytes); }
                catch (error) { throw new Error(`미션 음향 해독 실패: ${entry.path}`, { cause: error }); }
                if (this.#disposed || this.#audio.closed) throw new Error('반납한 미션 음향 적재기');
                this.#decoded.set(index, buffer); return buffer;
            })();
            this.#pending.set(index, pending);
        }
        try { const buffer = await pending; if (this.#disposed || this.#audio.closed) throw new Error('반납한 미션 음향 적재기'); cache.set(key, buffer); return buffer; }
        finally { if (this.#pending.get(index) === pending) this.#pending.delete(index); }
    }

    // 효과음 하나를 읽고 해독한다. 이미 등록했으면 다운로드하지 않는다.
    loadSound(name) { return this.#load('sound', name); }
    // 현재 언어와 기본 효과음을 제한된 동시 요청으로 준비한다. 음악은 받지 않는다.
    async preloadSounds({ concurrency = 8, onProgress = null } = {}) {
        if (this.#disposed || this.#audio.closed) throw new Error('반납한 미션 음향 적재기');
        if (!Number.isInteger(concurrency) || concurrency < 1 || concurrency > 32 || (onProgress !== null && typeof onProgress !== 'function')) throw new TypeError('미션 효과음 준비의 동시 요청·진행 장치 오류');
        const names = [...new Set(this.#soundEntries.filter((item) => item.directory === '' || item.directory === this.#language).map((item) => item.name))];
        if (!names.length) throw new Error('미션 효과음 자산이 비어 있습니다');
        let next = 0, done = 0, failure = null; onProgress?.(done, names.length);
        // 작업자는 실패를 공유하여 이미 시작된 요청만 마치고 새 요청을 시작하지 않는다.
        const worker = async () => {
            while (!failure && next < names.length) {
                const name = names[next++];
                try { await this.loadSound(name); done += 1; onProgress?.(done, names.length); }
                catch (error) { failure ??= error; }
            }
        };
        await Promise.all(Array.from({ length: Math.min(concurrency, names.length) }, () => worker()));
        if (failure) throw failure;
        return done;
    }
    // 선택된 음악 한 곡만 읽고 해독한다. 재생과 곡 선택은 호출자가 맡는다.
    loadMusic(name) { return this.#load('music', name); }
    // 해독해 둔 음악 한 곡을 놓아 메모리를 돌려준다. 곡 하나가 해독 뒤 35MB 안팎이라 미션 장면 음악이 곡을 바꿀 때마다 쓰지 않는 곡을 놓는다.
    // 재생 중인 소스는 자체 참조를 가지므로 끊기지 않고, 받는 중인 요청이나 등록된 적 없는 곡은 건드리지 않으며 반납한 뒤에도 안전하다. (2026-11-20 WB04-E1119)
    releaseMusic(name) {
        const key = soundKey(name, '.mus'), entry = this.#entries.get(`music::${key}`);
        this.#audio.music.delete(key);
        if (entry) this.#decoded.delete(entry.path.toLowerCase());
    }
    // 반납 후 진행 중인 다운로드·해독 결과가 재생기에 등록되지 않도록 한다.
    dispose() { this.#disposed = true; this.#pending.clear(); this.#decoded.clear(); }
}
