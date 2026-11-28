// 긴 설정 JSON을 편집/복사하는 메인 메뉴 전용 DOM 창이다. 입력 내용은 HTML로 해석하지 않는다.
import { SETTINGS_TRANSFER_MAX_LENGTH } from '../platform/settings-transfer.js';

// 원본 Tell 이름과 겹치지 않는 두 Options 명령 대상이다.
export const SETTINGS_LOAD_TARGET = 'Patch1101.LoadSettings';
export const SETTINGS_COPY_TARGET = 'Patch1101.CopySettings';
// 각 언어의 제목·동작·오류 안내다.
const TEXTS = Object.freeze({
    english: { load: 'Load Settings', copy: 'Copy Settings', note: 'Settings and campaign progress (JSON)', apply: 'Apply', cancel: 'Cancel', close: 'Close', copied: 'Copied to clipboard.', failed: 'Could not load settings. Check the JSON format and values.', copyFailed: 'Select the text and copy it with Ctrl+C.', storage: 'Could not save settings. Your current game is unchanged.' },
    korean: { load: '설정 불러오기', copy: '설정 복사', note: '설정 및 캠페인 진행상황 (JSON)', apply: '적용', cancel: '취소', close: '닫기', copied: '클립보드에 복사했습니다.', failed: '설정을 불러오지 못했습니다. JSON 형식과 값을 확인해 주세요.', copyFailed: '텍스트를 선택한 뒤 Ctrl+C로 복사해 주세요.', storage: '설정을 저장하지 못했습니다. 현재 게임은 유지됩니다.' },
    japanese: { load: '設定を読み込む', copy: '設定をコピー', note: '設定とキャンペーン進行状況 (JSON)', apply: '適用', cancel: 'キャンセル', close: '閉じる', copied: 'クリップボードにコピーしました。', failed: '設定を読み込めません。JSONの形式と値を確認してください。', copyFailed: 'テキストを選択してCtrl+Cでコピーしてください。', storage: '設定を保存できません。現在のゲームは維持されます。' },
});

// Optionsの行と実際の画面が同じ表示文言を使う。
export function settingsTransferTexts(language) { return TEXTS[language] ?? TEXTS.english; }

// 종료는 한 번만 실행하고 사건 처리기와 DOM을 함께 반납한다.
export class SettingsTransferPanel {
    // 현재 DOM·입력란·상태와 종료 콜백을 보유한다.
    #element; #input; #status; #text; #onClose; #abort = new AbortController();
    // parent는 실제 게임 루트이며 onApply는 검증/저장/메인 메뉴 재시작을 수행한다.
    constructor({ parent, language, mode, value = '', onApply, onClose }) {
        this.#text = settingsTransferTexts(language); this.#onClose = onClose;
        const document = parent.ownerDocument, text = this.#text; // 현재 게임 문서와 같은 언어 문구다.
        const overlay = document.createElement('div'), dialog = document.createElement('section'); // 게임 안에서만 표시하는 창 배경과 본문이다.
        overlay.style.cssText = 'position:absolute;inset:0;z-index:40;display:flex;align-items:center;justify-content:center;background:rgba(0,0,0,.7);padding:12px;box-sizing:border-box';
        dialog.style.cssText = 'width:min(720px,100%);max-height:90%;display:flex;flex-direction:column;gap:10px;padding:18px;box-sizing:border-box;background:#23292d;color:#f3ead4;border:2px solid #a29569;font:16px Galmuri11,monospace';
        dialog.setAttribute('role', 'dialog'); dialog.setAttribute('aria-modal', 'true'); dialog.setAttribute('aria-label', mode === 'load' ? text.load : text.copy);
        const title = document.createElement('h2'), note = document.createElement('label'), input = document.createElement('textarea'), status = document.createElement('p'), actions = document.createElement('div'); // 고정 제목·JSON 입력·결과·동작 단추 영역이다.
        title.textContent = mode === 'load' ? text.load : text.copy; title.style.cssText = 'margin:0;font:inherit;font-size:20px'; note.textContent = text.note;
        input.setAttribute('aria-label', text.note); input.value = value; input.maxLength = SETTINGS_TRANSFER_MAX_LENGTH; input.readOnly = mode === 'copy'; input.spellcheck = false;
        input.style.cssText = 'width:100%;min-height:180px;height:40vh;resize:vertical;box-sizing:border-box;background:#11181b;color:#f3ead4;font:14px monospace';
        status.setAttribute('role', 'status'); status.style.cssText = 'margin:0;min-height:1.5em'; actions.style.cssText = 'display:flex;gap:12px;justify-content:flex-end';
        this.#element = overlay; this.#input = input; this.#status = status;
        const button = (label, action) => { const node = document.createElement('button'); node.type = 'button'; node.textContent = label; node.style.cssText = 'font:inherit;padding:8px 18px;cursor:pointer'; node.addEventListener('click', action, { signal: this.#abort.signal }); actions.appendChild(node); return node; }; // 게임 입력으로 새지 않는 실제 동작 단추를 만든다.
        if (mode === 'load') button(text.apply, () => { try { if (!input.value.trim()) this.close(); else onApply(input.value); } catch (error) { status.textContent = error.code === 'storage' ? text.storage : text.failed; } });
        else button(text.copy, () => { this.copy(); });
        button(mode === 'load' ? text.cancel : text.close, () => this.close());
        dialog.append(title, note, input, status, actions); overlay.appendChild(dialog); parent.appendChild(overlay);
        for (const name of ['pointerdown', 'pointerup', 'mousedown', 'mouseup', 'click', 'wheel', 'keyup', 'beforeinput', 'paste']) overlay.addEventListener(name, (event) => event.stopPropagation(), { signal: this.#abort.signal }); // DOM 창의 편집/마우스가 게임 조작으로 전달되지 않게 한다.
        overlay.addEventListener('keydown', (event) => {
            event.stopPropagation();
            if (event.key === 'Escape') { event.preventDefault(); this.close(); }
            else if (event.key === 'Tab') { const items = [...dialog.querySelectorAll('textarea,button')], first = items[0], last = items.at(-1); if (event.shiftKey && document.activeElement === first) { event.preventDefault(); last.focus(); } else if (!event.shiftKey && document.activeElement === last) { event.preventDefault(); first.focus(); } } // Tab 초점을 창 안에서 순환시킨다.
        }, { signal: this.#abort.signal });
        input.focus(); if (mode === 'copy') { input.select(); this.copy(); }
    }
    // 사용자 동작의 현재 클립보드 API를 먼저 사용하고, 차단된 환경에서는 선택된 JSON을 남긴다.
    async copy() {
        const document = this.#element.ownerDocument, clipboard = document.defaultView?.navigator?.clipboard; // 같은 문서의 사용자 클립보드 장치다.
        try {
            if (clipboard?.writeText) await clipboard.writeText(this.#input.value);
            else { this.#input.select(); if (!document.execCommand('copy')) throw new Error('클립보드 복사 불가'); }
            this.#status.textContent = this.#text.copied;
        } catch { this.#input.select(); this.#status.textContent = this.#text.copyFailed; }
    }
    // 이미 열린 창을 다시 요청하면 기존 JSON 입력란으로 초점을 돌린다.
    focus() { this.#input.focus(); }
    // 창 DOM과 연결된 모든 사건을 한 번만 반납한다.
    close() { if (!this.#element.isConnected) return; this.#abort.abort(); this.#element.remove(); this.#onClose?.(); }
}
