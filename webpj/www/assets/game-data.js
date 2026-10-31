// 웹 판본의 게임 데이터 조회 모듈이다. 파일 조회(GameFiles)·설정 객체 층(Config)·언어 용어표·Tell 섹션 찾기를 한 객체로 묶는다.
// 원본의 설정 구성(Configinterface, cpppj o/ConfigInterface.cpp 와 같은 순서)을 따른다:
//  - 등록 순서: 언어 용어표(이름 없음) → 전역 설정(이름 없음) → `global.` → `fort.` 이며 조회는 마지막에 등록한 객체부터 하므로 전역 설정이 용어표보다 우선한다.
//  - 전역 설정 버퍼: `[ARGS]` + 버전 줄 + `[options.cfg]`(웹 사용자 설정) + `[setup.cfg]`(원본 setup.cfg) + `[END]`. 같은 키는 먼저 나온 줄이 이긴다.
//    웹 판본은 options.cfg 파일 대신 호스트가 넘긴 설정(언어·음량 등)을 같은 자리에 넣는다. 해상도·전체화면·설치 경로는 넣지 않는다.
//  - 경로 지정값(`missionSpec`·`guideSpec` 등)은 임시 `local.1~3` 객체에 인자를 넣고 치환한다.
//  - Tell 섹션은 tell 스크립트(`{missionSpec}` + local.1 = "tell" → `\D\tell.<언어>`)에서 `[이름]` 섹션을 찾는다(원본 004cf730 의 기본 스크립트 조회).
// 언어 파일이 없으면(예: 아직 없는 .korean) 영어 파일로 대체한다. 원본은 대체하지 않으나 웹 판본은 한국어 자료가 갖춰질 때까지 영어 화면을 보이기 위한 웹 적응이다.
// Tell 섹션은 파일뿐 아니라 섹션 단위로도 대체한다: 한국어 tell 파일은 웹에서 닿는 섹션만 번역한 부분 파일이라 없는 섹션은 영어 tell 파일에서 찾는다(2026-10-31 WB03-O).
// 설정 값 쓰기(원본 Config::Set, 004404e0)는 set() 이 맡는다: 같은 키의 줄을 모두 지우고 `키 = "값"` 줄을 전역 설정 버퍼 끝에 덧붙인다.
// 원본은 메인 루프가 변경 표시를 보고 options.cfg 를 저장하지만, 웹 판본은 쓸 때마다 호스트가 넘긴 onChange 훅을 불러 로컬 저장소에 저장하게 한다(platform/progress.js·settings.js).
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-10-28 WB02-D: 메뉴 대화상자 본문 조회를 위해 추가. 2026-10-30 WB03-H: 값 쓰기와 변경 통지 추가)
import { Config, ConfigRegistry, asciiLower, sectionRange } from './config.js';
import { GameFiles } from './game-files.js';
import { decodeOriginalText } from './text-encoding.js';

// 데이터 폴더 기준 TAFF 아카이브 경로이다. (2026-10-28 WB02-D)
export const ARCHIVE_PATH = 'netstorm.tarc';

// 데이터 폴더 기준 원본 기본 설정 파일 경로이다(인코딩된 파일). (2026-10-28 WB02-D)
export const SETUP_PATH = 'd/setup.cfg';

// 게임 버전이다(10.78 패치판). 전역 설정 앞머리의 MAJOR_VERSION·MINOR_VERSION·version 줄이 된다. (2026-10-28 WB02-D)
const MAJOR_VERSION = 10;
const MINOR_VERSION = 78;

// 영어(대체 언어)의 이름이다. 모르는 언어와 없는 언어 파일은 영어로 대체한다. (2026-10-28 WB02-D)
export const FALLBACK_LANGUAGE = 'english';

// 현재 Tell 이름을 담는 원본 지역 배열의 크기(종료 NUL 포함)이다. 이 크기 이상인 이름은 기록하지 않고 거부한다. (2026-10-30 WB03-J)
export const TELL_NAME_BYTES = 256;

// 게임 데이터 조회 객체이다.
// 사용: const data = GameData.build({ files: 적재기가 준 경로 → 바이트 Map, language: 'english', options: { soundVolume: 3 } });
//       data.tellSection('Credits') → 섹션 본문 문자열, data.substitute('{H2}'), data.pathSpec('missionSpec', 'tutorial1'). (2026-10-28 WB02-D)
export class GameData {
    // 파일 조회 계층과 언어를 받아 설정 객체 층을 구성한다. 직접 부르기보다 GameData.build 를 쓴다.
    // onChange(키, 값)는 set() 으로 값을 쓸 때마다 불리는 선택적 통지 훅이다(저장 연결용).
    constructor({ files, language = FALLBACK_LANGUAGE, options = {}, onChange = null }) {
        // 값 쓰기 통지 훅이다. 호스트가 값을 저장소에 옮기는 데 쓴다. 없으면 통지하지 않는다. (2026-10-30 WB03-H)
        this.onChange = onChange;
        // 파일 조회 계층이다.
        this.files = files;
        // 현재 언어 이름이다(파일 확장자로 쓰임). 'english' 이외는 대응 파일이 없으면 영어 파일로 대체한다.
        this.language = language;
        // 설정 객체 등록부다. `{@미션.키}` 조회는 미션 이름의 스크립트 파일(`missionSpec`)을 임시 객체에 읽어 넣는다(원본 00440760, 팁 문구 `{@tell.tip3}` 등). (2026-10-30 WB03-J: 적재 훅 연결)
        this.registry = new ConfigRegistry({ loadMission: (config, mission) => this._loadMission(config, mission) });
        // 마지막으로 조회한 Tell 섹션 이름이다(원본 Client 의 현재 Tell 이름 256바이트). 이름이 달라질 때 이전 이름이 `global.priorTell` 에 기록된다. (2026-10-30 WB03-J)
        this.currentTell = '';
        // 언어 용어표 객체(config.<언어>)다. 가장 먼저 등록되어 우선순위가 가장 낮다.
        this.terms = new Config(this.registry);
        // 전역 설정 객체(원본 configuration)다.
        this.configuration = new Config(this.registry);
        // `global.` 객체(Tell 의 priorTell 등 실행 중 값)다.
        this.global = new Config(this.registry, 'global');
        // `fort.` 객체다.
        this.fort = new Config(this.registry, 'fort');
        // 언어별로 읽어 둔 tell 스크립트 텍스트 캐시다.
        this._tellText = null;
        // 영어 대체 tell 스크립트 텍스트 캐시다(undefined = 아직 읽지 않음). (2026-10-31 WB03-O)
        this._fallbackTellText = undefined;
        this._loadConfiguration(options);
        this._loadLanguage();
    }

    // 데이터 폴더 파일 Map(경로 → 바이트)으로 GameData 를 만든다. netstorm.tarc 는 아카이브로 등록하고 모든 파일은 느슨한 파일로도 등록한다.
    // files 에 아카이브나 setup.cfg 가 없으면 어떤 파일이 빠졌는지 알리는 오류를 던진다.
    static build({ files, language = FALLBACK_LANGUAGE, options = {}, onChange = null }) {
        const archiveBytes = files.get(ARCHIVE_PATH);
        if (!archiveBytes) throw new Error(`필수 자산이 없습니다: ${ARCHIVE_PATH}`);
        if (!files.get(SETUP_PATH)) throw new Error(`필수 자산이 없습니다: ${SETUP_PATH}`);
        const loose = new Map(files);
        loose.delete(ARCHIVE_PATH);
        const gameFiles = new GameFiles({ loose });
        gameFiles.addArchive(archiveBytes);
        return new GameData({ files: gameFiles, language, options, onChange });
    }

    // 전역 설정 버퍼를 원본 시작 순서(00441790 → 00440e00)대로 채운다. options 는 웹 사용자 설정(키 → 값)이며 setup.cfg 보다 앞에 놓여 우선한다.
    // 언어는 options 에 없으면 currentLanguage 로 넣는다.
    _loadConfiguration(options) {
        const configuration = this.configuration;
        configuration.environmentEnabled = true;
        configuration.clear();
        configuration.append('[ARGS]');
        configuration.append(`MAJOR_VERSION=${MAJOR_VERSION}`);
        configuration.append(`MINOR_VERSION=${MINOR_VERSION}`);
        configuration.append(`version="v${MAJOR_VERSION}.${MINOR_VERSION}"`);
        configuration.append('[options.cfg]');
        const values = { currentLanguage: this.language, ...options };
        // 웹 사용자 설정을 `키 = "값"` 줄로 넣는다.
        for (const [key, value] of Object.entries(values)) configuration.append(`${key} = "${String(value)}"`);
        configuration.append('[setup.cfg]');
        configuration.loadBytes(this.files.read(SETUP_PATH), decodeOriginalText);
        configuration.append('[END]');
    }

    // 현재 언어의 용어표(languageSpec = "{DataDir}\config.{local.1}")를 읽는다. 파일이 없으면 영어 용어표로 대체하고, 그것도 없으면 빈 채로 둔다.
    _loadLanguage() {
        this.terms.clear();
        const bytes = this._readLanguageFile(this.pathSpec('languageSpec', this.language), this.language);
        if (bytes) this.terms.loadBytes(bytes, decodeOriginalText);
    }

    // `{@미션.키}` 조회가 임시 설정 객체(config)에 미션 스크립트를 읽어 넣는다(원본 ConfigInterface 의 미션 적재: missionSpec 경로 파일을 객체 버퍼에 덧붙임).
    // 파일이 없으면 아무것도 넣지 않는다. 언어 파일이므로 현재 언어의 파일이 없으면 영어로 대체한다. (2026-10-30 WB03-J)
    _loadMission(config, mission) {
        const bytes = this._readLanguageFile(this.pathSpec('missionSpec', mission), this.language);
        if (bytes) config.loadBytes(bytes, decodeOriginalText);
    }

    // 언어 파일을 읽는다. 없고 언어가 영어가 아니면 경로의 `.언어` 또는 `config.언어` 를 영어로 바꿔 한 번 더 시도한다. (웹 적응)
    _readLanguageFile(path, language) {
        const bytes = this.files.read(path);
        if (bytes || language === FALLBACK_LANGUAGE) return bytes;
        const english = path.replace(new RegExp(`${language}$`, 'i'), FALLBACK_LANGUAGE);
        return english === path ? null : this.files.read(english);
    }

    // 와일드카드 패턴(예: `\D\offical*.korean`)과 맞는 파일 이름 목록을 돌려준다. 현재 언어의 파일이 하나도 없고 언어가 영어가 아니면
    // 패턴 끝의 언어 이름을 영어로 바꿔 한 번 더 찾는다. 아직 없는 한국어 자료가 있어도 Campaign 같은 파일 목록이 빈 채로 나오지 않게 하는 웹 적응이다
    // (원본은 대체하지 않는다. 언어 파일 읽기 _readLanguageFile 과 같은 규칙). 사용: findFiles('\\D\\offical*.english'). (2026-10-30 WB03-H)
    findFiles(pattern) {
        const found = this.files.find(pattern);
        if (found.length > 0 || this.language === FALLBACK_LANGUAGE) return found;
        const english = pattern.replace(new RegExp(`${this.language}$`, 'i'), FALLBACK_LANGUAGE);
        return english === pattern ? found : this.files.find(english);
    }

    // 키의 값(치환까지 마친 문자열)을 조회한다. 없으면 null 이다.
    get(key) {
        return this.configuration.get(key);
    }

    // 원본 Config::Set(004404e0)을 전역 설정에 적용한다. 같은 키의 줄을 모두 지우고 `키 = "값"` 줄을 버퍼 끝에 덧붙이며(키에 '.' 이 있으면 접두어가 맞는 객체로 간다),
    // 이미 있던 키면 true 를 돌려준다. 값은 문자열로 바꿔 쓴다. 쓴 뒤 onChange(키, 문자열 값)를 부르며, silent 가 true 이면 부르지 않는다
    // (호스트가 저장소의 값을 이 층에 옮겨 적을 때 되돌아오는 통지를 막는다). 훅의 예외는 콘솔에만 남기고 쓰기 결과를 바꾸지 않는다.
    // 사용: data.set('DoneTheWarBegins', 1) → 이후 캠페인 목록의 `{DoneTheWarBegins}` 가 1 로 읽힌다. (2026-10-30 WB03-H)
    set(key, value, { silent = false } = {}) {
        const text = String(value);
        const existed = this.configuration.set(key, text);
        if (!silent) {
            try {
                this.onChange?.(key, text);
            } catch (error) {
                console.error('Netstorm: 게임 데이터 변경 통지에서 예외가 발생했습니다.', error);
            }
        }
        return existed;
    }

    // 정수 값을 십진 문자열로 써 넣는다(원본 SetInt, 서식 "%d"). 정수가 아니면 RangeError 이다. 나머지는 set 과 같다. (2026-10-30 WB03-H)
    setInt(key, value, options = {}) {
        if (!Number.isInteger(value)) throw new RangeError(`GameData.setInt: 정수가 아닙니다: ${value}`);
        return this.set(key, value, options);
    }

    // 문자열의 `{키}` 와 이스케이프를 푼다. 용어표(`{factory}` 등)와 전역 설정(`{H2}` 등)이 함께 쓰인다.
    substitute(text) {
        return this.configuration.substitute(text);
    }

    // 원본 004410f0(경로 지정값): 임시 `local` 객체에 인자(1~3번)를 넣고 키를 치환한 값을 돌려준다. 키가 없으면 빈 문자열이다.
    // 사용: pathSpec('missionSpec', 'tell') → `\D\tell.english`. (2026-10-28 WB02-D)
    pathSpec(key, argument1 = null, argument2 = null, argument3 = null) {
        const local = new Config(this.registry, 'local');
        try {
            // 주어진 인자만 `번호="값"` 줄로 넣는다.
            [argument1, argument2, argument3].forEach((argument, index) => {
                if (argument !== null) local.append(`${index + 1}="${argument}"`);
            });
            return this.configuration.get(key) ?? '';
        } finally {
            local.dispose();
        }
    }

    // 경로의 텍스트 파일을 문자열로 읽는다. 없으면 null 이다. 언어 파일은 영어로 대체한다.
    readText(path) {
        const bytes = this._readLanguageFile(path, this.language);
        return bytes ? decodeOriginalText(bytes) : null;
    }

    // tell 스크립트 전체 텍스트이다(`{missionSpec}` + local.1 = "tell"). 처음 한 번 읽어 캐시한다. 파일이 없으면 null 이다.
    get tellText() {
        if (this._tellText === null) this._tellText = this.readText(this.pathSpec('missionSpec', 'tell'));
        return this._tellText;
    }

    // 영어가 아닌 언어에서 현재 언어의 tell 스크립트에 섹션이 없을 때 대신 찾을 영어 tell 스크립트 텍스트이다. 영어이거나 영어 파일이 없으면 null 이다.
    // 한국어 tell 파일은 웹에서 닿는 섹션만 번역한 부분 파일이라, 번역이 없는 섹션은 영어로 대체하기 위한 웹 적응이다(원본은 섹션 단위로 대체하지 않는다). (2026-10-31 WB03-O)
    get fallbackTellText() {
        if (this._fallbackTellText === undefined) {
            this._fallbackTellText = null;
            if (this.language !== FALLBACK_LANGUAGE) {
                const bytes = this.files.read(this.pathSpec('missionSpec', 'tell').replace(new RegExp(`${this.language}$`, 'i'), FALLBACK_LANGUAGE));
                if (bytes) this._fallbackTellText = decodeOriginalText(bytes);
            }
        }
        return this._fallbackTellText;
    }

    // Tell 섹션 본문을 찾는다. 이름에 점이 있으면 먼저 등록된 객체(접두어)의 섹션을, 이어 기본 tell 스크립트의 섹션을 조회한다(원본 004cf730 FindTellSection).
    // 현재 미션 스크립트 조회는 미션 구현 때 연결한다. 없으면 null, 섹션이 있어도 본문이 비어 있으면 빈 문자열이다.
    // 사용: tellSection('UCampaign'). (2026-10-28 WB02-D)
    tellSection(name) {
        const dot = name.indexOf('.');
        if (dot >= 0) {
            const object = this.registry.findByName(name.slice(0, dot));
            const body = object?.section(name.slice(dot + 1)) ?? null;
            if (body !== null) return body;
        }
        // 현재 언어의 tell 스크립트를 먼저, 섹션이 없으면 영어 tell 스크립트를 찾는다. 본문이 비어 있는 섹션은 있는 것으로 본다.
        for (const text of [this.tellText, this.fallbackTellText]) {
            if (text === null) continue;
            const range = sectionRange(text, name);
            if (range) return text.slice(range.start, range.start + range.length);
        }
        return null;
    }

    // Tell 이름을 현재 이름으로 기록하고 섹션 본문을 찾는다(원본 004cf730 의 이름 기록 + 조회). 이름이 현재 이름과 대소문자를 무시하고 다르면
    // 먼저 이전 이름을 `global.priorTell`(본문의 `{global.priorTell}`)에 쓴다. 조회가 실패해도 이름 기록은 남는다. 이름이 256바이트 이상이면 RangeError 이다.
    // 호출자(Tell 창 공장)가 `param` 객체를 등록한 상태에서 부르며, 이름에는 이미 지역 인자를 뗀 순수 섹션 이름을 넘긴다. BLANK 는 이 함수 앞에서 걸러진다.
    // 사용: const body = data.findTellSection('Tips'); body === null 이면 섹션이 없다. (2026-10-30 WB03-J)
    findTellSection(name) {
        const nul = name.indexOf('\0');
        const local = nul >= 0 ? name.slice(0, nul) : name;
        if (local.length >= TELL_NAME_BYTES) throw new RangeError('Tell 이름이 원본 256바이트 지역 범위를 넘었습니다.');
        if (asciiLower(this.currentTell) !== asciiLower(local)) this.set('global.priorTell', this.currentTell, { silent: true });
        this.currentTell = local;
        return this.tellSection(local);
    }
}
