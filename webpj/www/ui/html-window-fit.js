// 원문 HTML 전체Fit의 현재 노드/측정·메뉴 pop·중앙 부착을 옮긴다(cpppj client/HtmlWindowFit.cpp/.h, 기준선1f00a25a).
// 실제 가상측정/트리를 빌리며 배치 결과를 별도 표시모형에 저장하지 않는다. 사용: new HtmlWindowFit(hooks).attach(token,x,y,parent,mode). (2026-11-14 WB04-E457)

// 선택한 하위만 요구하며 앞선 배치 효과를 되돌리지 않는다.
function need(hook) { if (typeof hook !== 'function') throw new Error('HTML 전체 Fit 실제 장치 미연결'); return hook; }
// 원문 signedDWORD 덧셈/뺄셈의 낮은32비트다.
const add = (a, b) => (a + b) | 0;
const sub = (a, b) => (a - b) | 0;
// 원문_ftol의 signed64 절삭 뒤 낮은DWORD다. 비정상/범위밖 결과의 낮은DWORD는0이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 새 FST32 저장은 현재 숫자를 단정도로 반올림하고 이전 직접복사 비트 캐시를 지운다.
function setFloat(node, field, value) { node[field] = Math.fround(value); if (node.floatBits) delete node.floatBits[field]; }
// 원문의 실제20행 밖 접근을 명시 범위오류로 드러낸다.
function slot(index) { if ((index >>> 0) >= 20) throw new RangeError('HTML Fit 실제20행 밖 접근'); return index >>> 0; }

// 같은 실제HTML 부모/본문/버튼/현재메뉴를 사용해 전체 가상Attach를 수행한다.
export class HtmlWindowFit {
    // 하위함수는 C++처럼 값 캡처하고 실제 owner/노드는 각 사용 위치에서 다시 빌린다.
    #hooks;
    // node/base만 선행 필수다. 나머지 측정/표시 장치는 선택 시 요구한다.
    constructor(hooks) { if (typeof hooks?.node !== 'function' || typeof hooks?.base !== 'function') throw new TypeError('HTML Fit 같은 실제 노드 필요'); this.#hooks = { ...hooks }; }
    // 기본/파생이 같은 현재 부모주소여야 한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node || node !== this.#hooks.base(token)) throw new Error('HTML Fit 실제 부모 없음'); return node; }
    // 같은 현재본문/버튼/메뉴 기본주소를 읽는다.
    #base(token) { const node = token ? this.#hooks.base(token) : null; if (!node) throw new Error('HTML Fit 실제 자식 없음'); return node; }
    // 매조회 현재Client의 signedDWORD를 읽으며 기본 화면값을 임의로 채우지 않는다.
    #read(address) { return need(this.#hooks.read)(address) | 0; }
    // FST32 저장점·현재 재조회·메뉴pop·자식부착 뒤 부모중앙부착과깊이보정을 원문순서로 실행한다.
    attach(token, x, y, parent, mode) {
        this.#node(token); const h = this.#hooks; let maximum = 0;
        // 각 실제 버튼측정 뒤 현재행/현재count를 다시 읽는다. unordered도 원문 대입 경로다.
        for (let index = 0; (index | 0) < (this.#node(token).controlCount | 0); index += 1) {
            const current = slot(index); need(h.fitButton)(this.#node(token).controls[current].child);
            const width = this.#base(this.#node(token).controls[current].child).baseWidth; if (!(maximum > width)) maximum = width;
        }
        const count = this.#node(token).controlCount | 0; let span = Math.fround(count * maximum);
        if (count > 1) span = Math.fround(((Math.imul(count, 15) - 15) | 0) + span);
        need(h.push)(this.#node(token).body); const requested = sub(this.#node(token).requestedWidth | 0, 40); let target = span;
        if (!(span > requested)) target = Math.fround(requested);
        need(h.fitContent)(this.#node(token).body, 0, 0); let measured = this.#base(this.#node(token).body);
        if (!(measured.baseWidth <= target)) {
            let previousHeight = 0; let previousWidth = 0;
            // 폭20씩 재측정하며 현재화면 한도/요청높이/같은높이50폭/종횡비 순서로 판정한다.
            for (;;) {
                setFloat(measured, 'baseWidth', target); setFloat(measured, 'width', target * measured.scale); need(h.fitHeight)(this.#node(token).body);
                const available = this.#read(0x5ca9b4);
                if (sub(available, 40) < target + 40) { const body = this.#base(this.#node(token).body); const width = sub(available, 80); setFloat(body, 'baseWidth', width); setFloat(body, 'width', width * body.scale); need(h.fitHeight)(this.#node(token).body); break; }
                if (this.#node(token).requestedHeight) { const body = this.#base(this.#node(token).body); const height = sub(this.#node(token).requestedHeight | 0, 65); if (height > body.baseHeight) { setFloat(body, 'baseHeight', height); setFloat(body, 'height', height * body.scale); } }
                measured = this.#base(this.#node(token).body);
                if (measured.baseHeight !== previousHeight) { previousHeight = ftol(measured.baseHeight); previousWidth = ftol(target); }
                if (target - previousWidth > 50) {
                    const first = this.#base(this.#node(token).body); setFloat(first, 'baseWidth', previousWidth); setFloat(first, 'width', previousWidth * first.scale);
                    const second = this.#base(this.#node(token).body); setFloat(second, 'baseHeight', previousHeight); setFloat(second, 'height', previousHeight * second.scale); break;
                }
                target = Math.fround(target + 20); if (!(measured.baseWidth * 2 < measured.baseHeight * 4)) break;
            }
            if (this.#read(0x531860) < target + 40 || this.#read(0x531864) < this.#base(this.#node(token).body).baseHeight + 65) {
                const limit = sub(this.#read(0x531860), 40); if (target > limit) target = Math.fround(limit);
                const body = this.#base(this.#node(token).body); setFloat(body, 'baseWidth', target); setFloat(body, 'width', target * body.scale);
                const second = this.#base(this.#node(token).body); const height = target * 0.5; setFloat(second, 'baseHeight', height); setFloat(second, 'height', height * second.scale);
                if (this.#node(token).requestedHeight) { const current = this.#base(this.#node(token).body); const requestedHeight = sub(this.#node(token).requestedHeight | 0, 65); if (requestedHeight > current.baseHeight) { setFloat(current, 'baseHeight', requestedHeight); setFloat(current, 'height', requestedHeight * current.scale); } }
                need(h.slider)(this.#node(token).body, 1); need(h.scrollable)(this.#node(token).body, 1); need(h.requestScroll)(this.#node(token).body, 0);
            }
        } else need(h.fitHeight)(this.#node(token).body);
        const root = this.#node(token); let body = this.#base(root.body); const fullWidth = target + 40;
        setFloat(root, 'baseWidth', fullWidth); setFloat(root, 'width', fullWidth * root.scale); const fullHeight = body.baseHeight + 65; setFloat(root, 'baseHeight', fullHeight); setFloat(root, 'height', fullHeight * root.scale);
        const halfWidth = ftol(fullWidth * 0.5); let buttonY = ftol(body.baseHeight + 30);
        if (this.#node(token).menuColumns) {
            const firstToken = need(h.currentMenu)(); const first = this.#base(firstToken); const menuHalf = ftol(first.baseWidth * 0.5); let menuHeight = ftol(first.baseHeight); let menuX = sub(halfWidth, menuHalf); if (menuX < 20) menuX = 20;
            if ((this.#node(token).menuColumns | 0) > 1) {
                menuX = sub(menuX, Math.trunc(menuHalf / 2)); need(h.attachMenu)(menuX, buttonY, token); const oldHeight = Math.fround(menuHeight); const current = need(h.currentMenu)();
                if (oldHeight > this.#base(current).baseHeight) menuHeight = ftol(oldHeight); else menuHeight = ftol(this.#base(need(h.currentMenu)()).baseHeight);
                menuX = add(menuX, add(menuHalf, menuHalf));
            }
            need(h.attachMenu)(menuX, buttonY, token); const current = this.#node(token); const height = menuHeight + current.baseHeight + 10;
            setFloat(current, 'baseHeight', height); setFloat(current, 'height', height * current.scale);
            const width = (Math.imul(Math.imul(this.#node(token).menuColumns, menuHalf), 2) + 40) | 0;
            if (width > this.#node(token).baseWidth) { const node = this.#node(token); setFloat(node, 'baseWidth', width); setFloat(node, 'width', width * node.scale); }
            buttonY = ftol(buttonY + first.baseHeight + 10); this.#node(token).menuColumns = 0;
        }
        let buttonX = ftol((this.#node(token).baseWidth - span) * 0.5); const yPosition = Math.fround(buttonY);
        // Push/Attach 뒤 현재행/개수를 다시 읽으며 모든버튼의 폭을 현재최대로 맞춘다.
        for (let index = 0; (index | 0) < (this.#node(token).controlCount | 0); index += 1) {
            const current = slot(index); need(h.push)(this.#node(token).controls[current].child); const button = this.#base(this.#node(token).controls[current].child);
            setFloat(button, 'baseWidth', maximum); setFloat(button, 'width', maximum * button.scale); const xPosition = Math.fround(buttonX);
            need(h.attach)(this.#node(token).controls[current].child, xPosition, yPosition, token, 0); buttonX = ftol(xPosition + maximum + 15);
        }
        body = this.#base(this.#node(token).body); const bodyX = Math.trunc(ftol(this.#node(token).baseWidth - body.baseWidth) / 2); need(h.attach)(this.#node(token).body, Math.fround(bodyX), 20, token, 0);
        if (need(h.reference)(this.#node(token).auxiliarySerial)) { x = Math.fround((this.#read(0x5c78f0) - this.#node(token).baseWidth) * 0.5); y = Math.fround((this.#read(0x5c78f4) - this.#node(token).baseHeight) * 0.5); }
        else {
            const left = this.#read(0x5ca9cc); const right = this.#read(0x5ca9d4); const bottom = this.#read(0x5ca9d8); const top = this.#read(0x5ca9d0);
            x = Math.fround((sub(right, left) - this.#node(token).baseWidth) * 0.5 + left); y = Math.fround((sub(bottom, top) - this.#node(token).baseHeight) * 0.5 + this.#read(0x5ca9d0));
        }
        need(h.attachBase)(token, x, y, parent >>> 0, mode >>> 0); need(h.addDepth)(token, (0xffffffb0 - this.#node(token).depth) >>> 0);
    }
}
