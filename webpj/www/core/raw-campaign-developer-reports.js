// 31h 타입·그래프 진단을 현재 Session의 TYPE·그림·그래프 기록에 연결한다.
// 사용: executeDeveloperReports(text, core, graph, trace) → 처리한 명령이면 true, 다른 명령이면 false.
import { firstDeveloperInteger, parseDeveloperCommand } from './raw-campaign-developer-settings.js';

// 개발자 help가 수준29 추적 장치에 순서대로 출력하는 원문 32행이다.
const DEVELOPER_HELP_LINES = Object.freeze([
    "setOwner(so) sid newowner - sets the owner of sid via server.\n",
    "setHackMove(shm) 1|0 - sets the hack move flag on client.\n",
    "dump(d) sid - dumps lots of info about a given sid.\n",
    "setNoDamage(snd) 1|0 - sets the \"nodamage\" flag on client and server.\n",
    "scatter(sct) type - generates some random pieces of type near bc=<0,0>.\n",
    "dumpTypes(dts) 1|0 - dumps the type info. 1=detailed, 0=abreviated.\n",
    "dumpType(dt) type - dumps info about a specific type.\n",
    "shootMyFriends(smf) 1|0 - attackers will target pieces of same owner.\n",
    "create(c) type frame - create type of frame at bc=<20,20>.\n",
    "setLocalHitPoints(slhp) sid - set the hitpoints field for sid only on this client.\n",
    "setAbstract(sa) sid 1|0 - set the abstract flag on sid to 1 or 0 (Only on client).\n",
    "fillMyTank(fmt) amount - set all graphs' energy to amount (Only on this client and the server)\n",
    "applyDamage(ad) sid amount - calls take damage on sid. (Client and server)\n",
    "generatorTimer(gt) 1|0 - make stuff appear in combat gump fast\n",
    "combatGumpColsRows(cr) cols rows - make more stuff appear in combat gump\n",
    "heaptrace(ht) 1|0 - walk the heap and print stats each frame\n",
    "destroy(de) sid - kill a sid\n",
    "dumpArtData(dad) type - dumps art data (hot spot, etc)\n",
    "dumpPlayers(dp) - dump the active players.\n",
    "dumpgraph(dg) graphNumber - dump the graph\n",
    "roundtotile(rtt) 1|0 - allow cursor to have fractional pos\n",
    "setally(sally) player ally - set ally for player\n",
    "clrally(cally) player ally - clr ally of player\n",
    "dumpally(dally) player - dump ally status\n",
    "createShipShit(css) - creates random crap in your ship\n",
    "setbridgekillperiod(sbkp) - sets period for bridge kill scan\n",
    "setsubid(ssi) - sets the subid and handle associated with this fort\n",
    "quickVortex(qv) - makes vortices build instantly\n",
    "quickBuild(qb) - makes everything build instantly\n",
    "queryDirectDraw(qdd) - shows DirectDraw driver info\n",
    "addLabel(al) - adds a label to the build record stream\n",
    "getPriest(gp) - sticks a enemy priest next to your priest\n",
]);

// 원문 수준29 로그를 실제 선택된 추적 장치에 전달한다.
function log(trace, text) { if (typeof trace !== 'function') throw new Error('개발자 명령의 실제 추적 장치 미연결'); trace(29, text); }

// 현재 TYPE·그림과 그래프 레코드를 원문 순서로 진단 출력한다. (2026-11-18 WB04-E743)
export function executeDeveloperReports(text, core, graph, trace) {
    const { key, rest } = parseDeveloperCommand(text);
    if (key === 'help' || key === '?') {
        // 원문 각 행을 개별 호출하여 추적 장치의 오류와 출력 순서를 보존한다.
        for (const line of DEVELOPER_HELP_LINES) log(trace, line);
        return true;
    }
    // 호출 직전의 TYPE·그림을 읽고 원문의 512BYTE 출력 경계를 검사한다.
    const dumpType = (type) => {
        if (type >= core.types.length || type >= core.shapes.length) throw new RangeError('개발자 TYPE 덤프의 현재 물리 범위');
        const record = core.types[type], shape = core.shapes[type], name = record.name.split('\0', 1)[0];
        const message = `${String(type | 0).padStart(2, '0')}:${name}, flags=${record.flags1 | 0}, depth=${record.zOrder | 0} numFrames=${shape.frameCount | 0}\n`;
        if (message.length >= 512) throw new RangeError('개발자 TYPE 덤프 로그 물리 범위');
        log(trace, message);
    };
    if (key === 'dumptypes' || key === 'dts') {
        firstDeveloperInteger(rest);
        // 원문은 타입0을 제외하고 다음 행 직전에 현재 표 길이를 다시 읽는다.
        for (let type = 1; type < core.types.length; type += 1) dumpType(type);
        return true;
    }
    if (key === 'dumptype') { dumpType((firstDeveloperInteger(rest) ?? 0) >>> 0); return true; }
    if (key === 'dumpgraph' || key === 'dg') {
        const value = firstDeveloperInteger(rest);
        if (value === null) { log(trace, 'bad command\n'); return true; }
        const index = value >>> 0;
        if (index >= graph.records.length) throw new RangeError('개발자 graph 덤프의 실제255행 물리 범위');
        const record = graph.records[index];
        log(trace, `graph index: ${value} numSurface: ${(record.surfaces << 16) >> 16} inUse: ${(record.inUse << 16) >> 16}\n`);
        return true;
    }
    return false;
}
