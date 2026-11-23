// 미션 화면의 장면 음악 감독(원본 469f60 SceneFrame·469fc0 StartScene)을 앱의 Web Audio 재생기와 미션 음악 적재기에 연결하는 구동기이다.
// 미션이 시작되면 원소 곡(wind22·rain22·thu22·sun22)을 무작위 곡부터 순환하며 재생하고, 곡이 끝날 때쯤 다음 곡으로 넘기며, 미션을 떠나면 메인 메뉴 곡(ser22)으로 돌아간다.
// 사용:
//   const driver = new SceneMusicDriver({ audio, prepare: (name) => loader.loadMusic(name), release: (name) => loader.releaseMusic(name),
//       wallSeconds: () => clock.wallSeconds(), random: { next: (n) => 0..n-1 정수 }, weatherSound: (name) => audio.playSound(name), onError: (error, name) => {} });
//   driver.begin();   // 미션 State7(원본 StartScene(true))에서 한 번 부른다. 곡 하나를 골라 받는 대로 재생한다.
//   driver.frame();   // 매 프레임(원본 SceneFrame). 곡 기한이 지나면 다음 원소 곡으로 넘긴다.
//   driver.end();     // 미션을 떠날 때(원본 StartScene(false)). 메뉴 곡을 다시 요청하고 받아 둔 미션 곡을 놓는다.
//   driver.request('sacrifice.mus'); // 미션 중 특정 곡 요청(원본 ClientAudio::RequestMusic, 희생 의식). (2026-11-20 WB04-E1120)
// 원본과 다른 점(웹 적응):
//  - 곡 길이는 파일을 열어 재는 대신 `MUSIC_DATA_BYTES` 표(각 곡 WAV data 청크 크기)로 같은 식에서 구한다. 시험이 실제 파일 머리와 대조한다.
//  - 곡 선택 난수는 월드 난수열을 건드리지 않도록 호출자가 준 별도 난수를 쓴다(원본은 게임 난수를 한 번 쓴다).
//  - 다음 원소 곡은 현재 곡이 시작될 때 미리 받아 곡이 바뀔 때 끊김이 없게 하고, 쓰지 않는 곡은 놓아 해독 버퍼가 두 곡(현재·다음)과 메뉴 곡을 넘지 않게 한다.
//  - 희생 의식 곡(sacrifice.mus)·대기실 곡·원소 팔레트·번개 섬광은 대응하는 월드·화면 부품이 없어 아직 연결하지 않는다(희생 판정은 항상 거짓).
// (2026-11-20 WB04-E1119: 미션 중 메인 메뉴 곡이 계속 재생되던 것을 원본의 원소 곡 순환으로 교체)
import { SceneMusic, SceneMusicState, TRACKS } from './scene-music.js';

// 메인 메뉴(전투 밖) 곡 이름이다. 미션을 떠나면 이 곡으로 돌아가며 해독 버퍼를 놓지 않는다. (2026-11-20 WB04-E1119)
export const MENU_TRACK = 'ser22.mus';

// 각 곡 WAV(22,050Hz·16비트·2채널)의 data 청크 크기(바이트)이다. 값은 공개 자산(www/data/music)의 파일 머리에서 읽은 것이다. (2026-11-20 WB04-E1119)
export const MUSIC_DATA_BYTES = Object.freeze({
    'anticipation.mus': 9395732, 'defeat.mus': 7155260, 'fanfare.mus': 1901424, 'rain22.mus': 18881856, 'sacrifice.mus': 12340620,
    'ser22.mus': 17988096, 'sun22.mus': 20535316, 'thu22.mus': 19841472, 'wind22.mus': 18236232,
});

// 곡 이름(대소문자 무시)의 길이(초)를 돌려준다. 원본 SoundMusic 과 같은 식 `길이/(비트×0.125)/빈도/채널` 을 쓰며 표에 없는 곡은 0 이다. (2026-11-20 WB04-E1119)
export function musicSeconds(name) {
    const bytes = MUSIC_DATA_BYTES[String(name).toLowerCase()];
    return bytes === undefined ? 0 : bytes / (16 * 0.125) / 22050 / 2;
}

// 장면 음악 구동기이다. 감독(SceneMusic)의 곡 선택·기한 규칙을 그대로 쓰고 장치 경계만 앱에 연결한다. (2026-11-20 WB04-E1119)
export class SceneMusicDriver {
    // 재생기·적재기·시계·난수와 감독 상태이다. selected 는 원본 선택기의 현재 곡 이름(받는 중이어도 선택한 곡)이다.
    #audio; #prepare; #release; #weatherSound; #onError; #state; #scene; #selected = MENU_TRACK; #active = false;

    // 장치를 받아 감독을 만든다. 필수 장치가 없으면 원본처럼 조용히 건너뛰지 않고 오류를 던진다. (2026-11-20 WB04-E1119)
    // 인자: audio(AudioSystem), prepare(이름)→Promise, release(이름), wallSeconds()→초, random.next(n)→정수, weatherSound(이름), onError(오류, 곡 이름).
    constructor({ audio, prepare, release, wallSeconds, random, weatherSound = () => {}, onError = () => {} } = {}) {
        if (!audio || !(audio.music instanceof Map) || typeof audio.playMusic !== 'function' || typeof prepare !== 'function' || typeof release !== 'function'
            || typeof wallSeconds !== 'function' || typeof random?.next !== 'function' || typeof weatherSound !== 'function' || typeof onError !== 'function') throw new TypeError('장면 음악 구동기의 재생기·적재기·시계·난수 장치 누락');
        this.#audio = audio; this.#prepare = prepare; this.#release = release; this.#weatherSound = weatherSound; this.#onError = onError;
        this.#state = new SceneMusicState();
        this.#scene = new SceneMusic(this.#state, random, {
            wallSeconds, currentName: () => this.#selected, select: (name) => this.#select(name), duration: () => this.#musicAudible() ? musicSeconds(this.#selected) : 0,
            // 희생 의식 곡·대기실 곡·팔레트·화면 갱신·번개 섬광은 대응 부품이 없어 비어 있다(위 머리 설명).
            sacrificing: () => false, waitingRoom: () => false, refresh: () => {}, ascendancyPalette: () => false, loadPalette: () => {}, thunderFlash: () => {},
            weatherSound: (name) => this.#weatherSound(name),
        });
    }

    // 미션 장면 음악이 돌고 있는지(begin 뒤 end 전)이다. 앱은 이 동안 메뉴 곡을 다시 틀지 않는다. (2026-11-20 WB04-E1119)
    get active() { return this.#active; }
    // 선택한 현재 곡 이름이다. 시험과 진단에 쓴다. (2026-11-20 WB04-E1119)
    get selected() { return this.#selected; }

    // 미션 시작에서 불러 전투 장면을 시작한다. 무작위 원소 곡을 고르고 이미 시작했으면 아무 일도 없다. (2026-11-20 WB04-E1119)
    begin() {
        if (this.#active) return;
        this.#active = true; this.#state.battle = 1; this.#scene.start();
    }

    // 매 프레임 불러 곡 기한이 지났으면 다음 원소 곡으로 넘긴다. 미션 장면이 아니면 아무 일도 없다. (2026-11-20 WB04-E1119)
    frame() {
        if (this.#active) this.#scene.frame();
    }

    // 미션 중 특정 곡을 요청한다(희생 의식의 sacrifice.mus 등). 같은 곡이거나 결과 곡 잠금·승리 관문에 걸리면 감독이 무시한다. 미션 장면이 아니면 아무 일도 없다. (2026-11-20 WB04-E1120)
    request(name) {
        if (this.#active) this.#scene.request(String(name));
    }

    // 미션을 떠날 때 불러 전투 밖 장면으로 돌려 메뉴 곡을 요청하고 받아 둔 미션 곡을 놓는다. 시작하지 않았으면 아무 일도 없다. (2026-11-20 WB04-E1119)
    end() {
        if (!this.#active) return;
        this.#active = false; this.#state.battle = 0; this.#scene.start(); this.#trim();
    }

    // 감독이 곡을 고르면 이미 해독한 곡은 바로 틀고 아니면 받은 뒤에 틀며, 받는 사이 다른 곡이 선택됐으면 틀지 않는다. (2026-11-20 WB04-E1119)
    #select(name) {
        this.#selected = name;
        if (this.#audio.music.has(name.toLowerCase())) { this.#start(name); return; }
        this.#load(name).then(() => this.#start(name)).catch((error) => this.#onError(error, name));
    }

    // 곡을 받아 해독한다. 실패하면 알리고 다시 던져 호출자가 재생을 건너뛰게 한다. (2026-11-20 WB04-E1119)
    #load(name) {
        // 받기 장치는 곧바로 부르되(요청이 한 틱 늦지 않게) 동기 예외도 거부로 바꾼다.
        return new Promise((resolve) => resolve(this.#prepare(name))).then(() => {
            // 받는 동안 미션을 떠났거나 곡이 바뀌었으면 쓸 곳이 없으므로 곧바로 놓는다.
            this.#trim();
        });
    }

    // 음악이 실제로 열릴 수 있는 상태(소리·음악 옵션이 켜짐)인지이다. 원본 MusicChannelDuration 은 주 채널이 파일을 연 적이 없거나 닫았으면(소리·음악을 끈 상태) 0 을 돌려주고, 감독은 길이 0 을 180초 뒤 재확인으로 처리한다.
    // 그래서 음악을 끈 동안의 곡 기한(날씨·천둥 주기 포함)은 곡 길이 표가 아니라 180초이다. 시험용 대역 재생기처럼 속성이 없으면 켜짐으로 본다. (2026-11-23 WB04-E1258)
    #musicAudible() { return this.#audio.musicEnabled !== false && this.#audio.soundEnabled !== false; }

    // 선택이 여전히 이 곡이면 재생을 요청하고 다음 원소 곡을 미리 받는다. 재생기가 닫혔으면 아무 일도 없다. (2026-11-20 WB04-E1119)
    #start(name) {
        if (this.#selected !== name || this.#audio.closed) return;
        this.#audio.playMusic(name);
        this.#trim();
        if (!this.#active) return;
        const next = this.#nextTrack();
        if (next && !this.#audio.music.has(next)) this.#prefetch(next);
    }

    // 지금 곡 다음에 올 원소 곡 이름이다. 현재 곡이 원소 곡이 아니면 없다. (2026-11-20 WB04-E1119)
    #nextTrack() {
        if (!TRACKS.includes(this.#selected)) return null;
        return TRACKS[(this.#state.index + 1) % TRACKS.length];
    }

    // 다음 곡을 재생 없이 받아 둔다. 실패는 알리기만 하고 곡이 바뀔 때 다시 받는다. (2026-11-20 WB04-E1119)
    #prefetch(name) {
        this.#load(name).catch((error) => this.#onError(error, name));
    }

    // 지금 곡·다음 곡·메뉴 곡 말고는 해독 버퍼를 놓는다. 미션이 아닐 때는 메뉴 곡만 남긴다. (2026-11-20 WB04-E1119)
    #trim() {
        const keep = new Set([MENU_TRACK, this.#selected.toLowerCase()]);
        const next = this.#active ? this.#nextTrack() : null;
        if (next) keep.add(next);
        // 순회 중 지우므로 이름을 먼저 복사한다.
        for (const key of [...this.#audio.music.keys()]) if (!keep.has(key)) this.#release(key);
    }
}
