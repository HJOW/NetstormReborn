// 현재 실제 사제의 원본 선택·객체 메뉴 열기와 Form 부착을 연결한다.
import { RawObjectMenuOpen, openPointerObjectMenu } from '../core/raw-object-menu-open.js';
import { objectMenuSlots } from '../render/object-menu-slots.js';

// 사제 메뉴 경로만 연결하며 다른 가상 표는 호출자가 별도로 처리한다.
export class ClientPriestMenuOpen {
    // 현재 World와 실제 원본 메뉴 열기 몸체다.
    #world; #opener; #sound;
    constructor(world, writer, summary, priest, devices) {
        if (!world?.session || !world?.openSelectionMenu || !writer || !summary?.summary || !priest?.open || !devices ||
            ['beginFancy', 'currentWidth', 'attachObject'].some((name) => typeof writer[name] !== 'function') ||
            ['altar', 'delegate', 'project', 'content', 'range', 'screenWidth', 'sound'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('사제 메뉴 열기의 실제 World·Client 장치 누락');
        this.#world = world; this.#sound = devices.sound; const core = world.session().core();
        this.#opener = new RawObjectMenuOpen(core.pool, core.types, {
            altar: devices.altar, delegate: devices.delegate, beginFancy: () => writer.beginFancy(), summary: (sid) => summary.summary(sid),
            populate: (sid) => priest.open(sid), project: devices.project, content: devices.content, range: devices.range,
            menuWidth: () => writer.currentWidth(), screenWidth: devices.screenWidth, attach: (sid, x, y) => writer.attachObject(sid, x, y),
        });
    }
    // 현재 슬롯의 실제 +a8/+ac 주소가 사제 메뉴인지 검사한다.
    supports(sid) {
        const pool = this.#world.session().core().pool; if (!sid || sid >= pool.capacity) return false;
        const raw = pool.slot(sid), table = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true);
        let slots;
        try { slots = objectMenuSlots(table); } catch (error) { if (error instanceof RangeError) return false; throw error; }
        return slots.populate === 0x00494870 && slots.open === 0x00492080;
    }
    // 실제 선택 관문을 지난 뒤 메뉴를 열고 성공할 때만 원본 효과음을 재생한다.
    open(sid, event) {
        if (!this.supports(sid)) throw new Error('사제 외 객체 메뉴는 아직 미연결');
        return openPointerObjectMenu(() => this.#world.openSelectionMenu(sid, event, false, (object, input) => this.#opener.open(object, input)), this.#sound);
    }
}
