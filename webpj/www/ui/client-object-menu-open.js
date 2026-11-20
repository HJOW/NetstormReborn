// 사제·Factory가 아닌 공통 객체(+ac 004adb30)의 원본 메뉴 열기와 Form 부착을 연결한다(cpppj `CampaignObjectMenuClient::Open`의 공통 분기).
// 건물·방어물의 Damageable 작성(populate 0044cbd0: 내 객체의 가상 개수 → 회수 뒤 공통 항목)과 일반 객체 작성(4b1270)만 다루며, 다른 가상 표는 호출자가 별도로 처리하거나 거부한다.
// 사용: const opener = new ClientObjectMenuOpen(world, writer, summary, objects, devices); if (opener.supports(sid)) opener.open(sid, event).
//   devices = { altar, delegate, project, content, range, screenWidth, sound }(사제 메뉴 열기와 같은 경계). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1086: Tutorial2 H1 발사기 회수 메뉴를 위해 추가)
import { RawObjectMenuOpen, openPointerObjectMenu } from '../core/raw-object-menu-open.js';
import { objectMenuSlots } from '../render/object-menu-slots.js';

// 원본 공통 열기와 두 작성 몸체의 가상 주소다.
const COMMON_OPEN = 0x004adb30, DAMAGEABLE = 0x0044cbd0, PLAIN = 0x004b1270;

// 현재 World의 공통 객체 메뉴를 같은 Client 메뉴 작성기에 연다.
export class ClientObjectMenuOpen {
    // 현재 World, 원본 열기 몸체, 효과음, 공통 객체 작성기다.
    #world; #opener; #sound; #objects;
    constructor(world, writer, summary, objects, devices) {
        if (!world?.session || !writer || !summary?.summary || typeof objects?.open !== 'function' || typeof objects?.openDamageable !== 'function' || !devices ||
            ['beginFancy', 'currentWidth', 'attachObject'].some((name) => typeof writer[name] !== 'function') ||
            ['altar', 'delegate', 'project', 'content', 'range', 'screenWidth', 'sound'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('공통 객체 메뉴 열기의 실제 World·Client 장치 누락');
        this.#world = world; this.#sound = devices.sound; this.#objects = objects; const core = world.session().core();
        this.#opener = new RawObjectMenuOpen(core.pool, core.types, {
            altar: devices.altar, delegate: devices.delegate, beginFancy: () => writer.beginFancy(), summary: (sid) => summary.summary(sid),
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
    // 같은 슬롯의 작성 주소에 따라 Damageable(회수 포함) 또는 일반 객체 메뉴를 쓴다.
    #populate(sid) {
        const slots = this.#slots(sid);
        if (slots?.populate === DAMAGEABLE) return this.#objects.openDamageable(sid);
        if (slots?.populate === PLAIN) return this.#objects.open(sid);
        throw new Error('공통 객체 메뉴의 작성 주소 미연결');
    }
    // 공통 열기(+ac 004adb30)이고 연결한 두 작성 몸체 중 하나이면 true 다.
    supports(sid) { const slots = this.#slots(sid); return Boolean(slots) && slots.open === COMMON_OPEN && (slots.populate === DAMAGEABLE || slots.populate === PLAIN); }
    // 건설 중 거부·제단 위임 뒤 메뉴를 열고 열릴 때만 원본 효과음을 재생한다.
    open(sid, event) {
        if (!this.supports(sid)) throw new Error('공통 객체 외 메뉴는 아직 미연결');
        return openPointerObjectMenu(() => this.#opener.open(sid, event), this.#sound);
    }
}
