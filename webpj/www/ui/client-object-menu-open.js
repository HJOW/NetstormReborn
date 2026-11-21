// 사제·Factory가 아닌 공통 객체(+ac 004adb30)의 원본 메뉴 열기와 Form 부착을 연결한다(cpppj `CampaignObjectMenuClient::Open`의 공통 분기).
// 건물·방어물의 Damageable 작성(populate 0044cbd0: 내 객체의 가상 개수 → 회수 뒤 공통 항목)과 일반 객체 작성(4b1270)만 다루며, 다른 가상 표는 호출자가 별도로 처리하거나 거부한다.
// 사용: const opener = new ClientObjectMenuOpen(world, writer, summary, objects, devices); if (opener.supports(sid)) opener.open(sid, event).
//   devices = { altar, delegate, project, content, range, screenWidth, sound }(사제 메뉴 열기와 같은 경계). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1086: Tutorial2 H1 발사기 회수 메뉴를 위해 추가)
import { RawObjectMenuOpen, openPointerObjectMenu } from '../core/raw-object-menu-open.js';
import { objectMenuSlots } from '../render/object-menu-slots.js';

// 원본 공통 열기와 두 작성 몸체의 가상 주소다.
const COMMON_OPEN = 0x004adb30, DAMAGEABLE = 0x0044cbd0, PLAIN = 0x004b1270;
// 파생 객체 메뉴 본체(+a8)의 일곱 주소이다: Meltdown·상태 변경·Dig Up/Turn To Spell·도움말·Storm Power·회수 개수·신전 AI 메뉴(+a8 4dcca0). 열기(+ac)는 공통이다. (2026-11-20 WB04-E1127)
const VARIANTS = new Set([0x0041c210, 0x00422000, 0x00462390, 0x00462550, 0x00485600, 0x0048eca0, 0x004dcca0]);

// 현재 World의 공통 객체 메뉴를 같은 Client 메뉴 작성기에 연다.
export class ClientObjectMenuOpen {
    // 현재 World, 원본 열기 몸체, 효과음, 공통 객체 작성기다.
    #world; #opener; #sound; #objects;
    constructor(world, writer, summary, objects, devices) {
        if (!world?.session || !writer || !summary?.summary || typeof objects?.open !== 'function' || typeof objects?.openDamageable !== 'function' || !devices ||
            ['beginFancy', 'currentWidth', 'attachObject'].some((name) => typeof writer[name] !== 'function') ||
            ['altar', 'project', 'content', 'range', 'screenWidth', 'sound'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('공통 객체 메뉴 열기의 실제 World·Client 장치 누락');
        this.#world = world; this.#sound = devices.sound; this.#objects = objects; const core = world.session().core();
        this.#opener = new RawObjectMenuOpen(core.pool, core.types, {
            // 제단 받침(dais)의 열기는 같은 좌표 제단의 가상 열기(+ac)로 위임한다. 제단의 열기도 공통 열기(004adb30)이므로 같은 몸체를 다시 부른다. (2026-11-21 WB04-E1137)
            altar: devices.altar, delegate: (altar, event) => this.#delegate(altar, event), beginFancy: () => writer.beginFancy(), summary: (sid) => summary.summary(sid),
            populate: (sid) => this.#populate(sid), project: devices.project, content: devices.content, range: devices.range,
            menuWidth: () => writer.currentWidth(), screenWidth: devices.screenWidth, attach: (sid, x, y) => writer.attachObject(sid, x, y),
        });
    }
    // 현재 슬롯의 가상 표에서 열기·작성 주소를 읽는다. 알 수 없는 표면 null 이다.
    #slots(sid) {
        const pool = this.#world.session().core().pool; if (!sid || sid >= pool.capacity) return null;
        const raw = pool.slot(sid), table = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true);
        try { return objectMenuSlots(table); } catch (error) { if (error instanceof RangeError) return null; throw error; }
    }
    // 위임 대상 제단의 열기가 공통 열기일 때만 같은 몸체로 연다. 다른 열기 주소면 임의 메뉴로 바꾸지 않고 거부한다.
    #delegate(altar, event) {
        if (this.#slots(altar)?.open !== COMMON_OPEN) throw new Error('제단 위임의 열기 주소 미연결');
        return this.#opener.open(altar, event);
    }
    // 같은 슬롯의 작성 주소에 따라 Damageable(회수 포함) 또는 일반 객체 메뉴를 쓴다.
    #populate(sid) {
        const slots = this.#slots(sid);
        if (slots?.populate === DAMAGEABLE) return this.#objects.openDamageable(sid);
        if (slots?.populate === PLAIN) return this.#objects.open(sid);
        if (VARIANTS.has(slots?.populate)) return this.#objects.openVariant(sid, slots.populate);
        throw new Error('공통 객체 메뉴의 작성 주소 미연결');
    }
    // 공통 열기(+ac 004adb30)이고 연결한 두 작성 몸체 중 하나이면 true 다.
    supports(sid) { const slots = this.#slots(sid); return Boolean(slots) && slots.open === COMMON_OPEN && (slots.populate === DAMAGEABLE || slots.populate === PLAIN || VARIANTS.has(slots.populate)); }
    // 건설 중 거부·제단 위임 뒤 메뉴를 열고 열릴 때만 원본 효과음을 재생한다.
    open(sid, event) {
        if (!this.supports(sid)) throw new Error('공통 객체 외 메뉴는 아직 미연결');
        return openPointerObjectMenu(() => this.#opener.open(sid, event), this.#sound);
    }
}
