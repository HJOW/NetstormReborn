// 매니페스트의 미션 효과음과 음악을 필요한 때에만 받아 현재 Web Audio 재생기에 등록한다.
// 사용: new MissionAudioAssets(loader, audio) → loadSound(name) 또는 loadMusic(name) → dispose().

// 원본 소리 이름은 한 파일 이름이다. 경로·NUL을 거부해 데이터 루트 밖 요청을 만들지 않는다.
function soundKey(name, extension) {
    if (typeof name !== 'string' || !name || /[\\/\0]/.test(name) || name === '.' || name === '..' || !name.toLowerCase().endsWith(extension)) throw new TypeError('미션 음향의 파일 이름 오류');
    return name.toLowerCase();
}

// 같은 현재 매니페스트와 AudioSystem을 빌리는 지연 적재 소유자다.
export class MissionAudioAssets {
    // 경로 표, 진행 중인 동일 요청, 반납 상태를 유지한다.
    #loader; #audio; #entries = new Map(); #pending = new Map(); #disposed = false;

    // 공개 루트의 음악·효과음 항목만 인덱싱한다. 대소문자 구별 없는 중복은 거부한다.
    constructor(loader, audio) {
        if (!loader?.manifest || !Array.isArray(loader.manifest.files) || typeof loader.loadFile !== 'function' || typeof audio?.decode !== 'function' || !(audio.sounds instanceof Map) || !(audio.music instanceof Map)) throw new TypeError('미션 음향 적재기의 실제 매니페스트·재생기 누락');
        this.#loader = loader; this.#audio = audio;
        // 같은 디렉토리와 확장자만 허용하고 다른 묶음의 같은 이름은 혼합하지 않는다.
        for (const entry of loader.manifest.files) {
            const match = /^(sound|music)\/([^/\\]+)$/i.exec(entry.path);
            if (!match) continue;
            const kind = match[1].toLowerCase(), extension = kind === 'sound' ? '.wav' : '.mus';
            const key = `${kind}:${soundKey(match[2], extension)}`;
            if (this.#entries.has(key)) throw new Error(`미션 음향 중복 경로: ${entry.path}`);
            this.#entries.set(key, entry);
        }
    }

    // 현재 장치와 이름을 확인한 뒤 같은 파일의 다운로드·해독을 동시 호출 간 공유한다.
    async #load(kind, name) {
        if (this.#disposed || this.#audio.closed) throw new Error('반납한 미션 음향 적재기');
        const key = soundKey(name, kind === 'sound' ? '.wav' : '.mus'), cache = kind === 'sound' ? this.#audio.sounds : this.#audio.music;
        if (cache.has(key)) return cache.get(key);
        const index = `${kind}:${key}`, entry = this.#entries.get(index);
        if (!entry) throw new Error(`미션 음향 파일 누락: ${name}`);
        let pending = this.#pending.get(index);
        if (!pending) {
            pending = (async () => {
                const bytes = await this.#loader.loadFile(entry);
                if (this.#disposed || this.#audio.closed) throw new Error('반납한 미션 음향 적재기');
                const buffer = await this.#audio.decode(bytes);
                if (this.#disposed || this.#audio.closed) throw new Error('반납한 미션 음향 적재기');
                cache.set(key, buffer); return buffer;
            })();
            this.#pending.set(index, pending);
        }
        try { return await pending; }
        finally { if (this.#pending.get(index) === pending) this.#pending.delete(index); }
    }

    // 효과음 하나를 읽고 해독한다. 이미 등록했으면 다운로드하지 않는다.
    loadSound(name) { return this.#load('sound', name); }
    // 선택된 음악 한 곡만 읽고 해독한다. 재생과 곡 선택은 호출자가 맡는다.
    loadMusic(name) { return this.#load('music', name); }
    // 반납 후 진행 중인 다운로드·해독 결과가 재생기에 등록되지 않도록 한다.
    dispose() { this.#disposed = true; this.#pending.clear(); }
}
