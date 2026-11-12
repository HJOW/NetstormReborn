// 미션 스크립트(`mission` 설정 객체)와 요새 파일 선택을 옮긴 모듈이다(cpppj `client/Mission.cpp/.h`, 10.78 원본 FUN_00482fb0 ↔ CD 00481020).
//  - MissionScript(configuration, name, extraSettings = '', source = null): 순서는 ① 등록부에 `mission` 이름의 설정 객체 만들기 → ② 추가 설정(명시 source 가 있으면 `appendArguments(extra, source)`, 없으면
//    `configuration.appendConfigArguments`) → ③ `missionSpec` 경로의 스크립트 읽기(명시 source 면 그 읽기만, 아니면 `configuration.loadFile`) → ④ 읽혔으면 `fileName` 쓰기. 명시 source 는 추가 설정 파일·언어 스크립트·Fort 에
//    같은 수명으로 쓰이며 전역 reader 를 바꾸거나 우회하지 않는다(부재도 전역 자산으로 대체하지 않는다).
//  - get(key) = 객체 접두어 `mission.` 를 붙인 조회(스크립트를 못 읽었으면 null). **기준선 뒤 +12줄 반영:** 표시 제목 `title` 만 현재 언어의 스크립트가 다른 경로이면 그 파일(익명 임시 Config)의 `title` 을 먼저 읽는다(AI/SP 같은
//    실행 키는 처음 읽은 스크립트를 유지하고, 표시 언어가 바뀌어도 현재 월드의 실행 대본을 교체하지 않는다). section(name)도 같은 방식으로 현재 언어 파일의 섹션을 먼저 찾고 없으면 처음 읽은 스크립트의 섹션을 쓴다.
//  - fortName(): `mission.loadFort`(없거나 빈 값이면 미션 이름), fortPath(): `fortSpec` 경로. readFortImage(선택한 이름)은 현재 `fortSpec` 으로 해석한 경로를 이 미션의 source 에서 읽는다(부재·예외는 전역 자산으로 우회하지 않고
//    `Game file not found: <경로>` 또는 읽기 예외 그대로). 빈 파일은 정상 값이다. fortReader()/fortFileSelector(fileName)/fortFileReader(fileName)은 source 의 소유 복사를 닫아 두어 스크립트와 설정이 사라진 뒤에도 같은
//    파일을 읽는다. fortFileSelector 는 I/O 없이 그 순간의 `fortSpec` 경로(와 호출자가 준 이름 변환)를 고정한 `{ fileName, read }` 만 돌려준다.
//  - missionType(), sectionNames(prefix)(00440640), set(key, value)(객체 접두어 없이 현재 mission 객체에 기록), fileSource()(명시 source 만, 기본 자산 reader 는 null), dispose()(등록부에서 내려옴, 여러 번 불러도 안전).
//  - createGuideMissionScript(configuration, name, modeBox, source = null): 새 미션을 만든 뒤(생성 예외는 모드·`global.inMission` 을 건드리지 않음) 네이티브 모드 상자를 true 로, `global.inMission` 을 "1" 로 쓴다.
//    releaseGuideMissionScript(missionBox, modeBox, configuration): 현재 미션이 있을 때만 스크립트를 먼저 내리고(등록부 반납) 모드 상자를 false, `global.inMission` 을 "0" 으로 쓴다(없으면 효과 없음).
//  configuration 은 `pathSpec(key, 인자)`·`registry`·`reader()`·`loadFile(config, path)`·`appendConfigArguments(config, text)`·`set(key, value)` 를 가진 설정 인터페이스(GameData)이다. source 와 reader 는
//  경로 → `Uint8Array`(빈 파일 포함) 또는 `null`/`undefined`(부재)를 돌려주고 I/O 실패는 던진다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E342: 4-D B 구획 Mission 이식)
import { Config } from '../assets/config.js';
import { decodeOriginalText } from '../assets/text-encoding.js';
import { createMissionFortFileSelection, createMissionFortSourceImage } from './mission-fort-source.js';

// 미션 설정 객체 이름과 조회 키 접두어이다(객체 이름 `mission` 에 구분자 `.` 를 붙인 것). (2026-11-12 WB04-E342)
const MISSION_NAME = 'mission';
const MISSION_PREFIX = `${MISSION_NAME}.`;

// 부재(null/undefined)와 빈 파일(길이 0 Uint8Array)을 구별하는 존재 판정이다. (2026-11-12 WB04-E342)
function present(bytes) {
    return bytes !== null && bytes !== undefined;
}

// 이미 선택한 같은 경로의 명시 부재/예외를 다른 자산 reader 로 우회하지 않는다. 기존 BaseFileSystem::Read 의 부재 진단을 유지한다. (2026-11-12 WB04-E342)
function readFortPath(source, path) {
    const bytes = typeof source === 'function' ? source(path) : null;
    if (!present(bytes)) throw new Error(`Game file not found: ${path}`);
    return bytes;
}

// 최초 설치와 기존 로그인 reader 도 같은 현재 `fortSpec` 을 한 번 선택한다. (2026-11-12 WB04-E342)
function readFortSource(configuration, source, selectedFort) {
    return readFortPath(source, configuration.pathSpec('fortSpec', selectedFort));
}

// 미션 스크립트 한 개이다. 등록부에 `mission` 설정 객체를 올리므로 쓰고 나면 dispose() 로 내려야 한다. (2026-11-12 WB04-E342)
export class MissionScript {
    // 현재 언어·실행 설정을 조회하는 설정 인터페이스이다.
    #configuration;
    // 미션 이름(스크립트 파일 이름)이다.
    #name;
    // 처음 읽은 `missionSpec` 경로이다.
    #scriptPath;
    // 호출자가 직접 고른 작업 namespace 인지이다(기본 자산 reader 를 복사해 작업본으로 오인하지 않는다).
    #explicitSource;
    // 선택한 작업 namespace/자산 reader 의 소유 복사이다.
    #source;
    // 안내·명령·섹션을 조회할 `mission` 설정 객체이다.
    #script;

    // 객체 생성 → 추가 설정 → `missionSpec` 파일 순서이다. 생성 도중 예외가 나면 등록부에 올린 객체를 내린 뒤 다시 던진다. (2026-11-12 WB04-E342)
    constructor(configuration, name, extraSettings = '', source = null) {
        this.#configuration = configuration;
        this.#name = String(name);
        this.#explicitSource = typeof source === 'function';
        this.#source = this.#explicitSource ? source : configuration.reader();
        this.#script = new Config(configuration.registry, MISSION_NAME);
        try {
            if (this.#explicitSource) this.#script.appendArguments(extraSettings, this.#source);
            else configuration.appendConfigArguments(this.#script, extraSettings);
            this.#scriptPath = configuration.pathSpec('missionSpec', this.#name);
            if (this.#explicitSource) {
                const bytes = this.#source(this.#scriptPath);
                if (present(bytes)) this.#script.loadBytes(bytes, decodeOriginalText);
            } else configuration.loadFile(this.#script, this.#scriptPath);
            if (this.#script.isValid()) this.#script.set('fileName', this.#name);
        } catch (error) {
            this.#script.dispose();
            throw error;
        }
    }

    // 미션 이름(스크립트 파일 이름)이다. (2026-11-12 WB04-E342)
    get name() { return this.#name; }

    // 처음 읽은 스크립트 경로(`missionSpec`)이다. (2026-11-12 WB04-E342)
    get scriptPath() { return this.#scriptPath; }

    // 스크립트를 실제로 읽었는지(버퍼가 만들어졌는지)이다. (2026-11-12 WB04-E342)
    get loaded() { return this.#script.isValid(); }

    // 언어 파일 경로가 바뀐 경우만 같은 source 로 익명 임시 Config 를 읽어 돌려준다(없으면 null). 실행 AI/SP/상태와 원문 키는 건드리지 않으며 호출자가 dispose() 한다. (2026-11-12 WB04-E342)
    #localizedDisplayScript() {
        const path = this.#configuration.pathSpec('missionSpec', this.#name);
        // 같은 파일은 기존 캐시/원문 조회만 쓴다.
        if (path === '' || path === this.#scriptPath) return null;
        const display = new Config(this.#configuration.registry);
        try {
            if (this.#explicitSource) {
                const bytes = this.#source(path);
                if (present(bytes)) display.loadBytes(bytes, decodeOriginalText);
            } else this.#configuration.loadFile(display, path);
        } catch (error) {
            display.dispose();
            throw error;
        }
        if (display.isValid()) return display;
        display.dispose();
        return null;
    }

    // `mission.키` 조회이다(스크립트를 읽지 못했으면 null). 표시 제목만 현재 언어 파일을 먼저 본다. (2026-11-12 WB04-E342)
    get(key) {
        if (key === 'title') {
            const display = this.#localizedDisplayScript();
            if (display) {
                try {
                    const title = display.get('title');
                    if (title !== null) return title;
                } finally {
                    display.dispose();
                }
            }
        }
        return this.#script.get(`${MISSION_PREFIX}${key}`);
    }

    // 읽을 요새 이름이다: `mission.loadFort` 가 있고 비어 있지 않으면 그 값, 아니면 미션 이름이다. (2026-11-12 WB04-E342)
    fortName() {
        const fort = this.get('loadFort') ?? '';
        return fort === '' ? this.#name : fort;
    }

    // `fortSpec` 에 요새 이름을 넣어 만든 경로이다. (2026-11-12 WB04-E342)
    fortPath() {
        return this.#configuration.pathSpec('fortSpec', this.fortName());
    }

    // Init 요청이 고른 이름의 `fortSpec` 을 현재 Config 로 해석해 같은 미션 source 에서 읽는다. loadFort 를 다시 선택하지 않는다. (2026-11-12 WB04-E342)
    readFortImage(selectedFort) {
        return readFortSource(this.#configuration, this.#source, selectedFort);
    }

    // source 의 소유 복사를 닫아 Session 로그인 재로드에 넘기는 읽기 함수이다. 스크립트 반납 뒤에도 현재 Client 설정으로 같은 `fortSpec` 을 해석한다. (2026-11-12 WB04-E342)
    fortReader() {
        const configuration = this.#configuration;
        const source = this.#source;
        return (selectedFort) => readFortSource(configuration, source, selectedFort);
    }

    // 현재 `fortSpec` 과 실제 이름(fileName 변환)만 고르는 선택기이다. 반환한 read 는 그 경로/source 를 소유하며 스크립트 반납과 이후 설정 변경에도 같은 파일을 읽는다. 선택은 파일 I/O 를 하지 않는다. (2026-11-12 WB04-E342)
    fortFileSelector(fileName = null) {
        const configuration = this.#configuration;
        const source = this.#source;
        return (selectedFort) => {
            const path = configuration.pathSpec('fortSpec', selectedFort);
            return createMissionFortFileSelection(typeof fileName === 'function' ? fileName(path) : path, () => readFortPath(source, path));
        };
    }

    // 같은 선택기를 즉시 읽는 기존 호출용 경계이다. 읽기 중 설정이 바뀌어도 이미 선택한 이름과 바이트를 함께 돌려준다. (2026-11-12 WB04-E342)
    fortFileReader(fileName = null) {
        const select = this.fortFileSelector(fileName);
        return (selectedFort) => {
            const selection = select(selectedFort);
            const bytes = selection.read();
            return createMissionFortSourceImage(bytes, selection.fileName);
        };
    }

    // 명시 source 만 소유 복사해 Guide 교체에 넘긴다. 기본 자산 reader 는 null 로 표현해 다음 미션도 기본 경계를 유지한다. (2026-11-12 WB04-E342)
    fileSource() {
        return this.#explicitSource ? this.#source : null;
    }

    // `mission.missionType`(없으면 빈 문자열)이다. (2026-11-12 WB04-E342)
    missionType() {
        return this.get('missionType') ?? '';
    }

    // 미션 파일의 원본 브리핑 본문을 설정 치환 전 상태로 읽는다(객체 접두어를 붙이지 않음). 현재 언어 파일의 섹션을 먼저 찾고 없으면 처음 읽은 스크립트의 섹션이다. 없으면 null. (2026-11-12 WB04-E342)
    section(name) {
        const display = this.#localizedDisplayScript();
        if (display) {
            try {
                const body = display.section(name);
                if (body !== null) return body;
            } finally {
                display.dispose();
            }
        }
        return this.#script.section(name);
    }

    // 현재 미션 Config 의 `[접두어…` 섹션 이름 목록(`이름;` 연결)이다. Start 의 tellAtList 작성과 같은 객체를 쓴다. (2026-11-12 WB04-E342)
    sectionNames(prefix) {
        return this.#script.sectionNames(prefix);
    }

    // 객체 접두어를 더하지 않고 현재 미션 Config 에 직접 기록한다(Start 의 실제 설정 쓰기). 이전에 있던 키면 true 이다. (2026-11-12 WB04-E342)
    set(key, value) {
        return this.#script.set(key, value);
    }

    // 등록부에서 내려온다. 여러 번 불러도 안전하며 이후 이 미션으로 조회하지 않는다. (2026-11-12 WB04-E342)
    dispose() {
        this.#script.dispose();
    }
}

// 원문 00482bf0 의 네이티브 모드/Config 쓰기를 현재 Guide 가 소유한 실제 미션 생성에 연결한다. 생성이 예외를 던지면 모드와 `global.inMission` 은 그대로이다. (2026-11-12 WB04-E342)
export function createGuideMissionScript(configuration, name, modeBox, source = null) {
    const mission = new MissionScript(configuration, name, '', source);
    modeBox.value = true;
    configuration.set('global.inMission', '1');
    return mission;
}

// 원문 00483210/00483260 은 보유 Config 해제 뒤 네이티브 0, 그 뒤 `global.inMission` 0 을 공개한다. 현재 미션이 없으면 효과가 없다. (2026-11-12 WB04-E342)
export function releaseGuideMissionScript(missionBox, modeBox, configuration) {
    if (!missionBox.value) return;
    missionBox.value.dispose();
    missionBox.value = null;
    modeBox.value = false;
    configuration.set('global.inMission', '0');
}
