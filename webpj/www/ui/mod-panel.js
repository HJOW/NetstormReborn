// 모드 설치/활성/오류를 관리하는 웹 전용 DOM 패널이다. 실제 저장·비실행 검증은 호스트가 주입한다.

// 원본 Tell과 충돌하지 않는 웹 전용 Options 대상이다.
export const MOD_PANEL_TARGET = 'Patch1101.Mods';
// 지원 언어의 관리 문구다. 패키지 이름/설명/오류는 HTML로 해석하지 않는다.
const TEXTS = Object.freeze({
    english: Object.freeze({ menu: 'Mods', title: 'Mods', note: 'Changes apply when you start the next mission.', import: 'Import mod…', file: 'Mod package', reload: 'Reload list', close: 'Close', empty: 'No mods installed.', enabled: 'Enabled', remove: 'Delete', replace: 'Replace this mod', cancel: 'Cancel replacement', duplicate: 'A mod with this id is installed. Replace it explicitly:', imported: 'Imported. The new mod is off.', replaced: 'Replaced. The new mod is off.', saved: 'Saved for the next mission.', removed: 'Deleted.', refreshed: 'Reloaded.', busy: 'Working…', error: 'Could not complete this action.', unavailable: 'Saved mods could not be read. Reload the list to try again.', large: 'Choose a UTF-8 .nsmod.json package no larger than 1 MiB.' }),
    korean: Object.freeze({ menu: '모드 관리', title: '모드 관리', note: '변경 사항은 다음 미션을 시작할 때 적용됩니다.', import: '모드 가져오기…', file: '모드 패키지', reload: '목록 다시 읽기', close: '닫기', empty: '설치된 모드가 없습니다.', enabled: '사용', remove: '삭제', replace: '이 모드로 교체', cancel: '교체 취소', duplicate: '같은 id의 모드가 있습니다. 교체 여부를 선택하십시오:', imported: '가져왔습니다. 새 모드는 꺼져 있습니다.', replaced: '교체했습니다. 새 모드는 꺼져 있습니다.', saved: '다음 미션에 적용하도록 저장했습니다.', removed: '삭제했습니다.', refreshed: '다시 읽었습니다.', busy: '처리 중…', error: '작업을 완료하지 못했습니다.', unavailable: '저장된 모드를 읽지 못했습니다. 목록을 다시 읽어 보십시오.', large: '1 MiB 이하의 UTF-8 .nsmod.json 패키지를 선택하십시오.' }),
    japanese: Object.freeze({ menu: 'モッド', title: 'モッド管理', note: '変更は次のミッション開始時に適用されます。', import: 'モッドを読み込む…', file: 'モッドパッケージ', reload: '一覧を再読込', close: '閉じる', empty: 'モッドはありません。', enabled: '使用', remove: '削除', replace: 'このモッドに置換', cancel: '置換を取消', duplicate: '同じ id のモッドがあります。置換するか選んでください:', imported: '読み込みました。新しいモッドはオフです。', replaced: '置換しました。新しいモッドはオフです。', saved: '次のミッションに適用する設定を保存しました。', removed: '削除しました。', refreshed: '再読み込みしました。', busy: '処理中…', error: '操作を完了できませんでした。', unavailable: '保存したモッドを読めません。一覧を再読み込みしてください。', large: '1 MiB 以下の UTF-8 .nsmod.json パッケージを選んでください。' }),
});
// 앱의 정규 언어 이름을 사용하며 알 수 없는 값은 영어로 표시한다.
export function modPanelTexts(language) { return Object.hasOwn(TEXTS, language) ? TEXTS[language] : TEXTS.english; }
// 여러 앱의 접근성 제목 id를 구별한다. 게임 RNG/시계는 소비하지 않는다.
let nextPanelId = 1;
// 새 HTML이나 URL을 평가하지 않는 DOM 작성기다.
function element(document, tag, text, styles = {}) { const node = document.createElement(tag); if (text !== undefined) node.textContent = text; Object.assign(node.style, styles); return node; }
// 버튼과 행은 작은 화면에서도 줄바꿈하며 기존 메뉴의 갈색/금색 색조를 사용한다.
const BUTTON = Object.freeze({ font: 'inherit', color: '#fff0ca', background: '#59462c', border: '2px outset #ae9360', padding: '7px 12px', cursor: 'pointer', overflowWrap: 'anywhere' });
const ROW = Object.freeze({ display: 'grid', gridTemplateColumns: 'minmax(0, 1fr) auto', gap: '8px 12px', borderTop: '1px solid #998052', padding: '12px 0', alignItems: 'start' });

// manager/store는 신뢰 호스트 장치다. untrusted guest의 공개 API에는 이 객체를 전달하지 않는다.
export class ModPanel {
    // 화면/저장/검증과 수명 상태다. 닫힌 후 비동기 완료로 화면을 되살리지 않는다.
    #document; #manager; #store; #validate; #maxBytes; #onClose; #root; #rows; #status; #input; #candidateBox; #candidateLabel; #importButton; #texts;
    #abort; #rowAbort = null; #closed = false; #busy = false; #candidate = null; #previousFocus;
    // 호스트는 validatePackage(input, signal)과 공통 maxPackageBytes를 주입한다.
    constructor({ parent, manager, language = 'english', validatePackage, maxPackageBytes, onClose }) {
        if (typeof validatePackage !== 'function' || !Number.isInteger(maxPackageBytes) || maxPackageBytes <= 0) throw new TypeError('모드 패널의 검증 장치/한도 누락');
        this.#document = parent.ownerDocument; this.#manager = manager; this.#store = manager.store; this.#validate = validatePackage; this.#maxBytes = maxPackageBytes; this.#onClose = onClose; this.#texts = modPanelTexts(language);
        const document = this.#document, view = document.defaultView, texts = this.#texts; this.#abort = new view.AbortController(); this.#previousFocus = document.activeElement;
        const overlay = element(document, 'div', undefined, { position: 'absolute', inset: '0', zIndex: '1000', display: 'grid', placeItems: 'center', background: '#0009', padding: '16px', boxSizing: 'border-box', color: '#fff0ca', font: `400 ${language === 'korean' ? 12 : 14}px/1.5 ${language === 'korean' ? 'Galmuri11' : language === 'japanese' ? '"Noto Sans JP"' : 'monospace'}`, overflow: 'hidden' });
        overlay.setAttribute('data-netstorm-mods', ''); this.#root = overlay;
        const panel = element(document, 'section', undefined, { width: 'min(680px, 100%)', maxHeight: '100%', overflow: 'auto', boxSizing: 'border-box', border: '3px ridge #c4a368', padding: '16px', background: '#352b20', boxShadow: '0 4px 24px #000b', overflowWrap: 'anywhere' });
        const title = element(document, 'h2', texts.title, { fontSize: language === 'korean' ? '24px' : '20px', fontWeight: '400', lineHeight: '1.5', margin: '0' }); title.id = 'netstorm-mods-' + nextPanelId++;
        panel.setAttribute('role', 'dialog'); panel.setAttribute('aria-modal', 'true'); panel.setAttribute('aria-labelledby', title.id);
        const heading = element(document, 'div', undefined, { display: 'flex', justifyContent: 'space-between', gap: '16px', alignItems: 'center' }); heading.append(title, this.#button(texts.close, () => this.close()));
        const toolbar = element(document, 'div', undefined, { display: 'flex', flexWrap: 'wrap', gap: '8px', margin: '12px 0' });
        this.#input = element(document, 'input'); this.#input.type = 'file'; this.#input.accept = '.nsmod.json,application/json'; this.#input.hidden = true; this.#input.setAttribute('aria-label', texts.file);
        this.#input.addEventListener('change', () => { const file = this.#input.files?.[0]; this.#input.value = ''; if (file) this.#run(() => this.#import(file)); }, { signal: this.#abort.signal });
        this.#importButton = this.#button(texts.import, () => this.#input.click(), 'write');
        toolbar.append(this.#importButton, this.#button(texts.reload, () => this.#run(async () => { await this.#manager.reload(); if (this.#manager.error) throw new Error(this.#manager.error); return texts.refreshed; }), 'reload'), this.#input);
        this.#rows = element(document, 'div');
        this.#candidateBox = element(document, 'div', undefined, { border: '1px solid #c4a368', padding: '12px', margin: '8px 0' }); this.#candidateBox.hidden = true;
        this.#candidateLabel = element(document, 'p', '', { margin: '0 0 8px', whiteSpace: 'pre-wrap' });
        const replacementButtons = element(document, 'div', undefined, { display: 'flex', flexWrap: 'wrap', gap: '8px' });
        replacementButtons.append(this.#button(texts.replace, () => this.#run(async () => { await this.#store.install(this.#candidate.text, { replace: true }); this.#candidate = null; return texts.replaced; }), 'write'), this.#button(texts.cancel, () => { this.#candidate = null; this.#render(); this.focus(); }, 'write'));
        this.#candidateBox.append(this.#candidateLabel, replacementButtons);
        this.#status = element(document, 'p', manager.error ?? '', { margin: '12px 0 0', whiteSpace: 'pre-wrap', color: '#ffe7b0' }); this.#status.setAttribute('role', 'status'); this.#status.setAttribute('aria-live', 'polite');
        panel.append(heading, element(document, 'p', texts.note, { margin: '8px 0' }), toolbar, this.#status, this.#candidateBox, this.#rows); overlay.append(panel); parent.append(overlay);
        // 캔버스의 키/문자 입력과 문서의 게임 붙여넣기에 전파하지 않는다.
        overlay.addEventListener('keydown', (event) => this.#key(event), { signal: this.#abort.signal }); overlay.addEventListener('keyup', (event) => event.stopPropagation(), { signal: this.#abort.signal }); overlay.addEventListener('paste', (event) => event.stopPropagation(), { signal: this.#abort.signal });
        this.#render(); this.focus();
    }
    // 닫기 버튼만 항상 활성이고 다른 명령은 busy/저장소 준비 상태를 따른다.
    #button(text, action, kind = null, signal = this.#abort.signal) {
        const button = element(this.#document, 'button', text, BUTTON); button.type = 'button'; if (kind) button.dataset.modAction = kind;
        button.addEventListener('click', action, { signal }); return button;
    }
    // 저장소는 공개 loaded getter를 제공해야 한다. 일반 quota 오류여도 삭제로 공간을 비울 수 있다.
    #controls() { for (const node of this.#root.querySelectorAll('[data-mod-action]')) node.disabled = this.#busy || (!this.#store.loaded && node.dataset.modAction !== 'reload'); }
    // 한 번에 한 명시 조작만 실행하고 실패 시 실제 저장 목록으로 checkbox를 복구한다.
    async #run(action) {
        if (this.#busy || this.#closed) return; this.#busy = true; this.#controls(); this.#status.textContent = this.#texts.busy;
        try { const message = await action(); if (!this.#closed && message) { this.#status.style.color = '#ffe7b0'; this.#status.textContent = message; } }
        catch (error) { if (!this.#closed) { this.#status.style.color = '#ffb4a5'; this.#status.textContent = `${this.#texts.error}\n${error?.code ? '[' + error.code + '] ' : ''}${String(error?.message ?? error).slice(0, 2048)}`; } }
        finally { if (!this.#closed) { this.#busy = false; this.#render(); this.focus(); } }
    }
    // 파일/AST 검증만 끝낸 뒤 중복은 명시 교체 후보로 남긴다. 엔트리는 설치 때 실행하지 않는다.
    async #import(file) {
        this.#candidate = null;
        if (file.size > this.#maxBytes) throw new Error(this.#texts.large);
        const bytes = new Uint8Array(await file.arrayBuffer()); if (this.#closed) return;
        const pkg = await this.#validate(bytes, this.#abort.signal); if (this.#closed) return;
        const text = JSON.stringify(pkg);
        if (this.#store.list().some((item) => item.manifest.id === pkg.manifest.id)) { this.#candidate = { text, manifest: pkg.manifest }; return this.#texts.duplicate + ' ' + pkg.manifest.id; }
        await this.#store.install(text); return this.#texts.imported;
    }
    // 모든 사용자 문자열은 노드 textContent로만 표시하고 예전 행의 수신기도 바로 반납한다.
    #render() {
        if (this.#closed) return;
        this.#rowAbort?.abort(); this.#rowAbort = new this.#document.defaultView.AbortController();
        const document = this.#document, texts = this.#texts, items = this.#store.list(), rows = [];
        for (const item of items) {
            const row = element(document, 'div', undefined, ROW), info = element(document, 'div'), label = element(document, 'label', undefined, { display: 'flex', gap: '8px', alignItems: 'start' }), enabled = element(document, 'input');
            enabled.type = 'checkbox'; enabled.checked = item.enabled; enabled.dataset.modAction = 'write'; enabled.setAttribute('aria-label', texts.enabled + ': ' + item.manifest.id);
            enabled.addEventListener('change', () => this.#run(async () => { await this.#store.setEnabled(item.manifest.id, enabled.checked); return texts.saved; }), { signal: this.#rowAbort.signal });
            label.append(enabled, element(document, 'span', item.manifest.name)); info.append(label, element(document, 'div', item.manifest.id + ' · ' + item.manifest.version, { color: '#c9b89b' }));
            if (item.manifest.description) info.append(element(document, 'p', item.manifest.description, { margin: '4px 0', whiteSpace: 'pre-wrap' }));
            if (item.lastError) info.append(element(document, 'p', item.lastError, { color: '#ffb4a5', whiteSpace: 'pre-wrap', margin: '4px 0' }));
            const remove = this.#button(texts.remove, () => this.#run(async () => { await this.#store.remove(item.manifest.id); return texts.removed; }), 'write', this.#rowAbort.signal); remove.setAttribute('aria-label', texts.remove + ': ' + item.manifest.id); row.append(info, remove); rows.push(row);
        }
        if (!rows.length) rows.push(element(document, 'p', this.#store.loaded ? texts.empty : texts.unavailable)); this.#rows.replaceChildren(...rows);
        this.#candidateBox.hidden = this.#candidate === null; if (this.#candidate) this.#candidateLabel.textContent = texts.duplicate + '\n' + this.#candidate.manifest.name + '\n' + this.#candidate.manifest.id + ' · ' + this.#candidate.manifest.version;
        this.#controls();
    }
    // Tab은 표시된 활성 컨트롤 안에서만 순환하고 ESC는 준비 대기 중에도 닫는다.
    #key(event) {
        event.stopPropagation();
        if (event.key === 'Escape') { event.preventDefault(); this.close(); return; }
        if (event.key !== 'Tab') return;
        const controls = [...this.#root.querySelectorAll('button,input')].filter((node) => !node.matches(':disabled') && !node.hidden && node.getClientRects().length);
        if (!controls.length) return;
        const index = controls.indexOf(this.#document.activeElement), next = event.shiftKey ? (index <= 0 ? controls.length - 1 : index - 1) : (index + 1) % controls.length;
        event.preventDefault(); controls[next].focus();
    }
    // 렌더링/비동기 조작 뒤 초점을 패널에 유지한다.
    focus() { if (!this.#closed) { const first = this.#root.querySelector('button:not(:disabled)'); first?.focus(); } }
    // 앱 종료/World 반납은 restoreFocus=false로 호출한다. 늦은 검증 결과는 더 이상 렌더링하지 않는다.
    close({ restoreFocus = true } = {}) {
        if (this.#closed) return; this.#closed = true; this.#abort.abort(); this.#rowAbort?.abort(); this.#candidate = null;
        const previous = this.#previousFocus, callback = this.#onClose; this.#onClose = null; this.#previousFocus = null; this.#root.remove();
        this.#manager = null; this.#store = null; this.#validate = null;
        callback?.(); if (restoreFocus && previous?.isConnected) previous.focus({ preventScroll: true });
    }
}
