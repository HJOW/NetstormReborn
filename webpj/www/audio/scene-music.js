// 원본 10.78 장면별 음악 선택·기한·날씨 효과를 현재 Client 경계에 연결한다.

// 원본 원소 곡·팔레트·날씨 효과음 표다. 곡 표는 앱의 장면 음악 구동기가 다음 곡 미리 받기에 쓰도록 내보낸다. (2026-11-20 WB04-E1119)
export const TRACKS = Object.freeze(['wind22.mus', 'rain22.mus', 'thu22.mus', 'sun22.mus']);
const PALETTES = Object.freeze(['windy.col', 'rainy.col', 'thundery.col', 'sunny.col']);
const WEATHER = Object.freeze(['', '', 'thunderCrack.wav', '']);
const MENU = 'ser22.mus', SACRIFICE = 'sacrifice.mus', ANTICIPATION = 'anticipation.mus', FANFARE = 'fanfare.mus', DEFEAT = 'defeat.mus';

// C locale 대소문자 비교를 사용하며 비ASCII 문자는 그대로 둔다.
function sameName(first, second) {
    const fold = (value) => value.replace(/[A-Z]/g, (letter) => String.fromCharCode(letter.charCodeAt(0) + 32));
    return fold(first) === fold(second);
}

// 원본은 범위 밖 표를 읽으므로 웹에서는 실제 범위 오류로 거부한다.
function at(table, index) { if (!Number.isInteger(index) || index < 0 || index >= table.length) throw new RangeError('장면 음악 색인 범위 오류'); return table[index]; }

// Client 수명으로 보유하며 필요한 현재 전역은 호출할 때 다시 읽는다.
export class SceneMusicState {
    // 원본 현재 곡 순환 색인과 결과·곡 기한이다.
    index = 3; fanfareEnd = 0; defeatEnd = 0; songEnd = 0;
    // 현재 전투 결과와 Player 상태이며 currentBattle이 있으면 현재 owner를 우선 조회한다.
    resultState = 0; battle = 0; playersReady = 0; localPlayer = 0; currentBattle = null;
    // 현재 날씨 색과 네 원소 색 및 팔레트 갱신 표시다.
    tint = 167; tints = new Uint32Array(4); paletteDirty = 0;
}

// 기존 현재 상태·게임 난수와 재생·시간·날씨 장치를 빌린다.
export class SceneMusic {
    // 같은 현재 전역, 공유 게임 난수, 실제 부수 효과다.
    #state; #random; #hooks;
    // 모든 외부 경계가 있어야 원본 분기를 수행할 수 있다.
    constructor(state, random, hooks) {
        if (!state || typeof random?.next !== 'function' || !hooks || ['wallSeconds', 'currentName', 'select', 'duration', 'sacrificing', 'waitingRoom', 'refresh', 'ascendancyPalette', 'loadPalette', 'thunderFlash', 'weatherSound'].some((name) => typeof hooks[name] !== 'function')) throw new TypeError('장면 음악 실제 경계 누락');
        this.#state = state; this.#random = random; this.#hooks = hooks;
    }
    // 매 선택 지점에서 현재 전투 owner를 다시 읽는다.
    currentBattle() { return this.#state.currentBattle ? this.#state.currentBattle() : this.#state.battle; }
    // 결과 곡은 기한 전 또는 NaN 비교 불가 동안 잠긴다.
    resultMusicLocked() {
        const h = this.#hooks, s = this.#state, current = h.currentName();
        if (sameName(current, FANFARE) && !(h.wallSeconds() >= s.fanfareEnd)) return true;
        return sameName(h.currentName(), DEFEAT) && !(h.wallSeconds() >= s.defeatEnd);
    }
    // 현재 곡·결과 잠금·승리 관문을 지난 곡만 선택하고 실제 길이에 따라 기한을 설정한다.
    request(name) {
        const h = this.#hooks, s = this.#state;
        if (sameName(h.currentName(), name)) return;
        if (this.resultMusicLocked() && !sameName(name, ANTICIPATION)) return;
        if (sameName(name, FANFARE) && s.resultState !== 1) return;
        h.select(name);
        if (sameName(name, FANFARE)) s.fanfareEnd = h.duration() + h.wallSeconds();
        else if (sameName(name, DEFEAT)) s.defeatEnd = h.duration() + h.wallSeconds();
        if (30 < h.duration()) { s.songEnd = h.duration() + h.wallSeconds(); return; }
        s.songEnd = h.wallSeconds() + 180;
    }
    // 원소 색인을 먼저 올리고 현재 희생 여부에 따라 다음 곡을 고른다.
    next() {
        const s = this.#state; s.index = (((s.index + 1) | 0) % 4) | 0;
        if (s.playersReady && this.#hooks.sacrificing(s.localPlayer)) { this.request(SACRIFICE); return; }
        this.request(at(TRACKS, s.index)); this.applyWeather();
    }
    // 실제 벽시각이 기한에 도달했을 때만 현재 전투·대기 상태를 읽는다.
    frame() {
        const h = this.#hooks; if (!(h.wallSeconds() >= this.#state.songEnd)) return;
        if (this.currentBattle()) { this.next(); return; }
        if (h.waitingRoom()) { this.request(ANTICIPATION); return; }
        this.request(MENU);
    }
    // 전투 여부에 따라 메뉴 또는 공유 게임 난수의 다음 원소 곡을 시작한다.
    start() {
        const s = this.#state;
        if (!this.currentBattle()) { s.index = 3; this.request(MENU); }
        else { s.index = Math.trunc(this.#random.next(4000) / 1000); this.next(); }
        this.frame(); s.tint = at(s.tints, s.index);
    }
    // 실제 화면 갱신 뒤 선택 팔레트와 천둥 효과음을 원본 순서로 적용한다.
    applyWeather() {
        const h = this.#hooks, s = this.#state; h.refresh();
        if (h.ascendancyPalette()) { s.tint = at(s.tints, s.index); h.loadPalette(at(PALETTES, s.index)); s.paletteDirty = 1; }
        const sound = at(WEATHER, s.index); if (!sound) return;
        if (s.index === 2) h.thunderFlash(); h.weatherSound(sound);
    }
}
