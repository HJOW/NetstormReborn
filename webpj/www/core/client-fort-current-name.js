// 현재 Client의 Fort 이름·Template·설정 물리를 원본 이름 선택 본체에 연결한다.
// 사용: new ClientFortCurrentName(names, templates, data, gameplay, storage, effects).set('BridgeTheGap').
// 파일 삭제·Editor 및 멀티플레이 전용 효과는 웹에서 제공하지 않는다.
import { asciiLower } from '../assets/config.js';
import { decodeWindows1252, encodeOriginalText } from '../assets/text-encoding.js';
import { setCurrentFortName } from './raw-fort-current-name.js';

// NUL까지의 실제 바이트를 원본 좁은 문자열로 읽는다.
function text(bytes) {
    const end = bytes.indexOf(0); if (end < 0) throw new RangeError('Client Fort 이름의 NUL 범위');
    return decodeWindows1252(bytes.subarray(0, end));
}
// 설정이 만든 경로를 원본 좁은 바이트와 NUL로 만든다.
function terminated(value) { const input = encodeOriginalText(value), result = new Uint8Array(input.length + 1); result.set(input); return result; }

// Client 수명의 같은 이름·Template·설정·게임 물리를 빌린다.
export class ClientFortCurrentName {
    // 같은 저장소와 선택된 원본 외곽 효과다.
    #names; #templates; #data; #gameplay; #storage; #effects;
    // 현재 Client의 실제 소유자와 제목 장치가 있어야 구성한다.
    constructor(names, templates, data, gameplay, storage, effects) {
        if (!names || !templates || !data || !gameplay || !storage || storage.authority !== gameplay.authority() || typeof effects?.refreshTitle !== 'function') throw new TypeError('Client Fort 현재 이름의 실제 소유자/제목 누락');
        this.#names = names; this.#templates = templates; this.#data = data; this.#gameplay = gameplay; this.#storage = storage; this.#effects = effects;
    }
    // 같은 9개 헤더의 선택된 20BYTE를 고정 뷰로 빌린다.
    #header(slot) {
        const headers = this.#templates.headers();
        if (!Number.isInteger(slot) || slot < 0 || slot >= 9 || headers.length < (slot + 1) * 20) throw new RangeError('Client Fort Template 헤더 범위');
        return headers.subarray(slot * 20, slot * 20 + 20);
    }
    // 원본 좁은 이름을 NUL 포함 입력으로 바꾸고 현재 이름 부모를 실행한다.
    set(name) { this.setBytes(terminated(name)); }
    // 호출자가 소유한 원본 BYTE와 선택 scratch를 사용해 같은 이름·Template을 갱신한다.
    setBytes(name, scratch = new Uint8Array(256)) {
        const names = this.#names, templates = this.#templates, data = this.#data, game = this.#gameplay, storage = this.#storage, effects = this.#effects;
        const globals = { owner: () => game.player.templateOwner, fort: () => Number(storage.ownerMode.fort), editor: () => game.world.editor, mission: () => Number(storage.links.mission),
            get dirty() { return names.catalogDirty().value; }, set dirty(value) { names.catalogDirty().value = value; } };
        const hooks = { ...effects, header: (slot) => this.#header(slot), currentName: () => names.currentBytes(),
            managerName: (token) => templates.managerBytes(token).subarray(12, 268), selectFileSpec: () => names.selectFileSpec(),
            pathSpec: (value) => terminated(data.pathSpec(names.selectedFileSpec(), text(value))),
            samePath: (first, second) => asciiLower(text(first)) === asciiLower(text(second)),
            deleteRequest: () => { throw new Error('웹은 Fort 파일 삭제를 지원하지 않습니다'); },
            config: (key, value) => data.set(key, text(value)) };
        setCurrentFortName(name, scratch, globals, hooks);
    }
}
