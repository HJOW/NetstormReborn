// 원본 Quick Help와 팝업이 공유하는 마지막 명령과 80바이트 인자를 보관한다.
// 사용: popup.mark(command, Uint8Array|null); popup.argumentBytes()로 현재 첫 NUL 앞을 읽는다.

// 새 Client에서 한 번 만들며 State 정리는 명령만 직접 변경한다.
export class QuickHelpPopupState {
    // 마지막 명령 DWORD와 첫 NUL 뒤도 보존하는 고정 인자 물리다.
    command = 0; argument = new Uint8Array(80);

    // null은 전체를 지우고 바이트 인자는 첫 NUL까지 복사한다. 길이 오류는 변경 전에 거부한다.
    mark(command, text = null) {
        if (text === null) { this.command = command >>> 0; this.argument.fill(0); return; }
        if (!(text instanceof Uint8Array)) throw new TypeError('Quick Help 팝업 인자는 바이트 배열이어야 한다');
        const end = text.indexOf(0), length = end < 0 ? text.length : end;
        if (length >= this.argument.length) throw new RangeError('Quick Help 팝업 인자80바이트 초과');
        this.command = command >>> 0; this.argument.set(text.subarray(0, length)); this.argument[length] = 0;
    }

    // 현재 물리의 첫 NUL 앞 뷰를 빌리고 종료 바이트가 없으면 거부한다.
    argumentBytes() {
        const end = this.argument.indexOf(0); if (end < 0) throw new RangeError('Quick Help 팝업 인자의 NUL 누락');
        return this.argument.subarray(0, end);
    }
}
