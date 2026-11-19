// 현재 실제 Factory의 원본 메뉴 열기와 객체 Form 부착을 연결한다.
import { RawObjectMenuOpen, openPointerObjectMenu } from '../core/raw-object-menu-open.js';
import { objectMenuSlots } from '../render/object-menu-slots.js';

// Factory의 가상 +ac만 처리하며 다른 객체는 호출자가 선택한다.
export class ClientFactoryMenuOpen {
    // 현재 World와 실제 메뉴 열기 및 음향이다.
    #world; #opener; #sound;
    constructor(world, writer, summary, factory, sound) {
        if (!world?.session || !writer || !summary?.summary || !factory?.open || typeof sound !== 'function' ||
            ['beginFancy', 'attachObject'].some((name) => typeof writer[name] !== 'function')) throw new TypeError('Factory 메뉴 열기의 실제 World·Client 장치 누락');
        this.#world = world; this.#sound = sound; const core = world.session().core();
        this.#opener = new RawObjectMenuOpen(core.pool, core.types, { beginFancy: () => writer.beginFancy(), summary: (sid) => summary.summary(sid),
            populate: (sid) => factory.open(sid), attach: (sid, x, y) => writer.attachObject(sid, x, y) });
    }
    // 현재 슬롯의 실제 가상 표가 Factory 작성·열기인지 검사한다.
    supports(sid) {
        const pool = this.#world.session().core().pool; if (!sid || sid >= pool.capacity) return false;
        const raw = pool.slot(sid), table = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true); let slots;
        try { slots = objectMenuSlots(table); } catch (error) { if (error instanceof RangeError) return false; throw error; }
        return slots.populate === 0x00454c00 && slots.open === 0x004754b0;
    }
    // 실제 Factory가 수락한 경우에만 원본 열기 효과음을 재생한다.
    open(sid, event) {
        if (!this.supports(sid)) throw new Error('Factory 외 객체 메뉴는 아직 미연결');
        return openPointerObjectMenu(() => this.#opener.openFactory(sid, event), this.#sound);
    }
}
