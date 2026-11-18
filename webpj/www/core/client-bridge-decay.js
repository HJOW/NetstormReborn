// Client 공통 프레임에서 같은 장기 다리 붕괴 스캔과 현재 Session 칸 처리기를 사용한다.
// 사용: runSurfaceBridgeDecay(storage, globals, time, paused, cell), runClientBridgeDecay(client), reportClientBridgeDecay(client).
import { RawBridgeDecayScan } from './raw-bridge-decay.js';

// C++ 기본 스트림의 유효 숫자 6자리 표기를 현재 double에 적용한다.
function streamNumber(value) {
    if (Number.isNaN(value)) return 'nan'; if (!Number.isFinite(value)) return value < 0 ? '-inf' : 'inf';
    if (Object.is(value, -0)) return '-0'; if (value === 0) return '0';
    const scientific = value.toExponential(5), parts = scientific.split('e'), exponent = Number(parts[1]);
    if (exponent < -4 || exponent >= 6) {
        const mantissa = parts[0].replace(/(\.\d*?)0+$/, '$1').replace(/\.$/, '');
        return `${mantissa}e${exponent < 0 ? '-' : '+'}${String(Math.abs(exponent)).padStart(2, '0')}`;
    }
    return Number(scientific).toFixed(Math.max(0, 5 - exponent)).replace(/(\.\d*?)0+$/, '$1').replace(/\.$/, '');
}

// 같은 권한·스캔 저장소를 사용하며 정지·Editor·권한 관문은 기존 본체에 맡긴다.
export function runSurfaceBridgeDecay(storage, globals, time, paused, cell) {
    if (storage.authority !== globals.authority()) throw new Error('공통 다리 스캔의 현재 권한 소유자 불일치');
    if (storage.pool.edition !== 'patch1078') return;
    const authority = globals.authority();
    const mode = { server: authority.server, authority: authority.boss, editor: globals.world.editor !== 0 };
    new RawBridgeDecayScan(storage.pool, storage.types, storage.bridgeDecayScan, () => storage.bridgeDecayPeriod, mode, cell).frame(time, paused);
}

// Client 공통 프레임에서 표면이 있으면 스캔하고 선택된 칸만 현재 Session에 전달한다.
export function runClientBridgeDecay(client) {
    const storage = client.surfaceStorageIfInitialized(); if (!storage) return;
    const globals = client.gameplayGlobals();
    runSurfaceBridgeDecay(storage, globals, client.time(), client.paused(), (sid) => globals.requireSession().world().decayBridgeCell(client.time(), sid));
}

// 상태를 쓰지 않고 현재 스캔과 실제 후보 수를 진단 문자열로 반환한다.
export function reportClientBridgeDecay(client) {
    const storage = client.surfaceStorageIfInitialized(); if (!storage) return '';
    const globals = client.gameplayGlobals(), scan = storage.bridgeDecayScan; let eligible = 0;
    if (storage.pool.edition === 'patch1078') {
        // 현재 전체 물리 슬롯에서 원본과 같은 활성 다리 후보만 센다.
        for (let sid = 0; sid < storage.pool.capacity; sid += 1) {
            const raw = storage.pool.slot(sid); if (!(raw[11] & 7) && raw[10] < storage.types.length && (storage.types[raw[10]].flags2 & 4) && !(raw[40] & 1)) eligible += 1;
        }
    }
    return `bridge_scan_global\t${scan.first}\t${scan.last}\t${scan.cursor}\t${streamNumber(scan.next)}\t${streamNumber(storage.bridgeDecayPeriod)}\t${globals.world.editor}\t${Number(globals.authority().boss)}\t${Number(globals.authority().server)}\t${Number(globals.borrowed())}\t${eligible}\n`;
}
