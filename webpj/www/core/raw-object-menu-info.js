// 원본 10.78 객체 메뉴의 제목/정보 4인자(004ad780)와 요약(004ada70)·현재 상태 경고·포획 문구(0042a2f0)를 복원한 모듈이다(cpppj `client/RawObjectMenuInfo.cpp/.h`, 10.78 전용).
// 사용:
//   const info = new RawObjectMenuInfo(pool, types, classes, state, hooks);   // pool = SidPool(10.78), types = 타입 레코드 188개(`flags2`·`theme`), classes = TYPE+aa 클래스 문자열 188개(각 23바이트 이하),
//   //   state = createObjectMenuInfoState({ localOwner, editor, missionPost }) (현재 540c70/5c85a4/594fa4 getter), hooks = 아래.
//   info.describe(type, count, owner, sid) → type/count/owner/sid 네 인자를 각각 보존해 제목·Level·소유 관계·편집 SubID/좌표·정렬·클래스 줄을 작성한다(-1 sid 는 편집 좌표 생략).
//   info.summary(object) → owner/TYPE 선행 snapshot → 가상 count → describe → 현재 extra 경고(Invisible/Paralyzed) → 포획 문구.   info.captureStatus(object) → 492090 이 참일 때만 Not Connected!/Capturable! 줄.
//   RawObjectMenuInfo.levelName(number) → 4a8980 의 0..9 로마숫자와 범위 밖 sentinel 바이트(번역 없음).   RawObjectMenuInfo.themeName(number) → 4a8940 이 참조하는 8문자열(번역은 호출 위치에서).
//   hooks = { count(sid) → 가상+80, description(type), ownerName(owner), translate(키), substitute(문자열) → 설정 치환(441740), print(패턴, 인자[]) → 번역 없는 4a89c0 고정 %s/%d, title(문자열), header(문자열),
//             alliance(owner, local) → 동맹 DWORD, groundImmobile(sid) → 492090, ownerStatus(owner) → Player+50 BYTE }.   bindObjectMenuInfoControls(hooks, controls) — 기존 제목/머리 작성기에 위임한다.
// 번역/설정 치환/현재 게임 조회는 호출 위치마다 현재 값을 읽고 도달한 미연결 효과는 성공으로 대체하지 않는다(Error '미연결'). 서식/이름/관계 문자열은 첫 NUL 과 원본 버퍼 한도(256/80/1024)를 지킨다.
// 원본 관측: `object-menu-info-x86.tsv` 1,434입력(Describe·Summary·CaptureStatus·LevelName·ThemeName 의 전체 반환·효과 순서·현재 필드)을 tests/raw-object-menu-info.test.mjs 가 대조한다.
// 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E546: 객체 메뉴 제목/정보 RawObjectMenuInfo 이식)
import { squidLevelName } from './raw-squid-name.js';

// 원본 전역은 실제 사용 위치에서 다시 읽는다(TYPE/소유자/count/SID 와 섞지 않는다). 읽기 함수는 현재 DWORD 를 돌려준다. (2026-11-15 WB04-E546)
export function createObjectMenuInfoState(overrides = {}) {
    return {
        altar: 164, nugget: 154, // 5412e8/5412c0: 제단의 count 감소와 너깃 소유자 0 예외이다.
        invisible: 117, paralyzed: 118, // 54122c/541230 의 현재 경고 그림 TYPE 이다.
        localOwner: null, editor: null, missionPost: null, // 540c70/5c85a4/594fa4 의 현재 DWORD getter 이다.
        ...overrides,
    };
}

// 제목/이름/현재 게임 조회 누락을 빈 문구나 가짜 성공으로 대체하지 않는다. (2026-11-15 WB04-E546)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('객체메뉴정보 실제하위미연결');
    return hook;
}

// 단정도 비트를 수로 바꾼다. (2026-11-15 WB04-E546)
function bitsToFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 좌표는 원본 little endian DWORD 로 조립한다. (2026-11-15 WB04-E546)
function read(raw, offset) {
    let value = 0;
    // 필요한 4바이트만 원본의 낮은 바이트부터 읽는다.
    for (let index = 0; index < 4; index += 1) value += raw[offset + index] * 2 ** (index * 8);
    return value >>> 0;
}

// 불확정 64비트의 하위 0 과 정상값의 32비트 감김을 보존한다(원본 _ftol 의 하위 signed DWORD). (2026-11-15 WB04-E546)
function truncate(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 원본 고정 버퍼는 NUL 을 포함하므로 같은 capacity 이상의 출력을 거부하고 첫 NUL 앞만 쓴다. (2026-11-15 WB04-E546)
function text(value, capacity) {
    const nul = value.indexOf('\0');
    if (nul >= 0) value = value.slice(0, nul);
    if (value.length >= capacity) throw new RangeError('객체메뉴정보 원본문자열버퍼초과');
    return value;
}

// 객체 메뉴의 제목/정보 4인자와 현재 상태 경고를 복원하는 처리기이다. 판본(10.78)·TYPE/클래스 표 크기(188)를 생성 때 확인하며 생성 중 메뉴/게임 효과는 실행하지 않는다. (2026-11-15 WB04-E546)
export class RawObjectMenuInfo {
    constructor(pool, types, classes, state, hooks) {
        if (pool.edition !== 'patch1078' || types.length !== 188 || classes.length !== 188) throw new TypeError('객체메뉴정보 10.78 풀/TYPE/클래스표 오류');
        this.pool = pool;
        this.types = types;
        this.classes = classes;
        this.state = state;
        this.hooks = hooks;
    }

    // 부모 원본에 없는 free/dead/void 필터를 추가하지 않는다(접근 시점마다 현재 슬롯을 읽는다).
    raw(object) {
        if (!object || object >= this.pool.capacity) throw new RangeError('객체메뉴정보 SID범위오류');
        return this.pool.slot(object);
    }

    // callback 뒤에도 같은 물리 TYPE 의 현재 필드를 읽는다.
    type(number) {
        if (number >= this.types.length) throw new RangeError('객체메뉴정보 TYPE범위오류');
        return this.types[number];
    }

    // 실제 TYPE 초기화의 class 길이 23 한도를 확인하고 C 문자열의 첫 NUL 까지만 노출한다.
    className(number) {
        this.type(number);
        const value = this.classes[number];
        const nul = value.indexOf('\0');
        const result = nul >= 0 ? value.slice(0, nul) : value;
        if (result.length > 23) throw new RangeError('객체메뉴정보 원본class23바이트초과');
        return result;
    }

    // 서식의 번역 여부와 호출 시점은 호출자가 선택하며 이 함수는 추가 번역하지 않는다(원본 256바이트 서식 결과).
    print(pattern, args) { return text(need(this.hooks.print)(pattern, args), 256); }

    // 4a8980: 0..9 로마숫자와 범위 밖 sentinel 바이트이다(번역하지 않는다).
    static levelName(number) { return squidLevelName(number); }

    // 4a8940 이 참조하는 8 문자열의 원문이다(Describe 는 첫 5개 또는 None 만 사용). 범위 밖 번호는 거부한다.
    static themeName(number) {
        const names = ['Wind', 'Rain', 'Thunder', 'Sun', 'Energy', 'Nature', 'Holy', 'Evil'];
        if (number < 0 || number >= names.length) throw new RangeError('객체메뉴정보 원소이름표범위오류');
        return names[number];
    }

    // 네 인자는 독립 값이다. 외부 효과 사이의 현재 class/flags/theme/모드는 해당 원본 위치에서 다시 읽는다. (004ad780)
    describe(type, count, owner, sid) {
        const h = this.hooks;
        const state = this.state;
        this.type(type);
        need(h.title)(text(need(h.description)(type), 256));
        count >>>= 0;
        owner >>>= 0;
        sid >>>= 0;
        if (type === state.altar) count = (count - 1) >>> 0;
        if (this.className(type) !== '' && (this.type(type).flags2 & 0x80074000)) {
            const pattern = need(h.translate)(' Level %s');
            const level = RawObjectMenuInfo.levelName((count | 0) < 0 ? 0 : count);
            need(h.title)(this.print(pattern, [level]));
        }
        if (type === state.nugget) owner = 0;
        const name = text((owner | 0) > 0 ? need(h.ownerName)(owner) : need(h.translate)('Nobody'), 80);
        // 현재 로컬 번호는 이름 조회 뒤 한 번 읽는다.
        const local = need(state.localOwner)() >>> 0;
        let relation;
        if (owner === local || !owner) relation = need(h.translate)('Owner');
        else relation = need(h.translate)(need(h.alliance)(owner, local) ? 'Ally' : 'Enemy');
        relation = text(relation, 256);
        if (need(state.editor)() && sid !== 0xffffffff) {
            if (sid > 65535) throw new RangeError('객체메뉴정보 편집SID표현범위초과');
            this.raw(sid);
            need(h.header)(this.print('\n%s: ~i%d~.', ['SubID', sid | 0]));
            const raw = this.raw(sid);
            const x = bitsToFloat(read(raw, 14));
            const y = bitsToFloat(read(raw, 18));
            const iy = truncate(y);
            const ix = truncate(x);
            need(h.header)(this.print('\nAt (~i%d~w,~i%d~w)~.', [ix, iy]));
        }
        need(h.header)(this.print('\n%s: ~i%s~.', [relation, name]));
        if (this.className(type) !== '') {
            const alignmentPattern = need(h.translate)('\nAlignment: ~i%s~.');
            const theme = this.type(type).theme | 0;
            const alignment = need(h.translate)(theme < 5 ? RawObjectMenuInfo.themeName(theme) : 'None');
            need(h.header)(this.print(alignmentPattern, [alignment]));
            const classPattern = need(h.translate)('\nClass: ~i%s~.');
            const translated = need(h.translate)(this.className(type));
            const substituted = text(need(h.substitute)(translated), 1024);
            need(h.header)(this.print(classPattern, [substituted]));
        }
        need(h.header)('\n');
    }

    // 4ada70 은 count 의 외부 효과 전에 owner/TYPE 를 보관한다. 그 후 경고 extra 는 현재 raw 를 다시 읽는다.
    summary(object) {
        const h = this.hooks;
        const state = this.state;
        const raw = this.raw(object);
        const owner = raw[34];
        const type = raw[10];
        const count = need(h.count)(object);
        this.describe(type, count, owner, object);
        if (this.raw(object)[40] & 0x10) {
            const pattern = need(h.translate)('~y~[I%d.*]Invisible!~.\n');
            need(h.header)(this.print(pattern, [state.invisible | 0]));
        }
        if (this.raw(object)[40] & 0x20) {
            const pattern = need(h.translate)('~y~[I%d.*]Paralyzed!~.\n');
            need(h.header)(this.print(pattern, [state.paralyzed | 0]));
        }
        this.captureStatus(object);
    }

    // 492090 이 거짓이면 현재 미션/Player 표를 읽지 않는다. 참이면 원본의 short-circuit 순서이다. (0042a2f0)
    captureStatus(object) {
        const h = this.hooks;
        this.raw(object);
        if (!need(h.groundImmobile)(object)) return;
        const disconnected = !need(this.state.missionPost)() && !(need(h.ownerStatus)(this.raw(object)[34]) & 1);
        need(h.header)(need(h.translate)(disconnected ? '~yNot Connected!~.\n' : '~yCapturable!~.\n'));
    }
}

// 같은 controlled 메뉴의 제목/머리 몸체를 호출하며 별도 문구·항목·레이아웃을 생성하지 않는다. (2026-11-15 WB04-E546)
export function bindObjectMenuInfoControls(hooks, controls) {
    hooks.title = (value) => controls.addTitle(value);
    hooks.header = (value) => controls.addHeader(value);
}
