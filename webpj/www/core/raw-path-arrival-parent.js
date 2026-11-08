// 공통 경로 도착(Pathprocess.cpp 0048b340)의 현재 진단 · 공간/종료 · 삭제 뒤 원문 결과 분기를 옮긴 모듈이다(cpppj `o/RawPathArrivalParent.cpp/.h`, 10.78).
//  - pathArrivalBlocked(pool, hash, types, spots, blockingGenus, point): 도착점이 지도 칸(+0.9999 편향 절삭)에서 spot bit3 를 가지면, 도착점 칸(편향 없는 절삭) 하나의 일반 finder 후보 중 현재 타입 genus 가
//    blockingGenus 와 겹치는 첫 후보에서 참을 돌려준다. 지도 밖 좌표는 원본 조회 대신 명시적으로 거부한다. blockingGenus 는 숫자 또는 현재 값을 돌려주는 함수이다.
//  - runPathBaseRobust(hooks): 0041be80. 항상 참을 돌려주되 SID 범위(form 0 이 아니고 용량 이상이면 `formSid == 0 || form.isValid()` 0x69 뒤 용량 재독, 부모 0 이 아니고 용량 이상이면 `attachToSid == 0 || s.isValid()` 0x6c)를 진단한다.
//  - runPathArrivalParent(checking, hooks): 저장 부모가 유효하고 살아 있지 않으면 진단 0x25c, 경로가 살아 있으면 Step(최종 좌표) → Unpop → Pop → (checking 이면 robust) → robust → 도착 막힘 판정 → Halt(참이면 막힘에서 정리하고 끝) →
//    finished=참 → (checking 이면 robust) → 저장 부모의 dead/free 면 진단 0x286 → 감시 토큰이 있으면 진단 0x289 → 감시 토큰 설정 후 부모 도착(arrival: 1000 종료/1001 이미 죽음/1002 대기) → 그 밖의 반환이면 진단 0x28d →
//    경로가 죽었으면(checking 이고 반환≠1001 이면 진단 0x290) 막힘 정리 → 토큰 해제·(checking 이면 robust)·살아 있으면 1002 는 finished=거짓, 그 밖은 kill → 막힘이면 정리한다. 각 효과 뒤 alive 를 다시 확인한다(삭제 뒤 객체를 읽지 않음).
// 10.72(CD) 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E145: 공통 경로 도착 부모 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 원문 진단 파일 이름과 줄·식, 도착 반환 코드(종료 1000·이미 죽음 1001·대기 1002), 지도 한 변, 도착 편향(0.9999f), spot 도착 비트, 타입 raw 위치이다. (2026-11-09 WB04-E145)
const PATH_SOURCE = '\\Ns\\O\\Pathprocess.cpp';
const BASE_SOURCE = '\\Ns\\O\\Baseprocess.cpp';
const KILL = 1000;
const ALREADY_DEAD = 1001;
const LINGER = 1002;
const MAP_SIDE = 256;
const BIAS = Math.fround(0.9999);
const ARRIVAL_SPOT = 8;
const TYPE = 10;

// 선택된 실제 경계가 없으면 거부한다(앞선 부분 효과는 유지한다). (2026-11-09 WB04-E145)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('공통 도착의 선택된 실제 경계 미연결');
    return hook;
}

// 기존 칸 계산: 편향을 더한 값을 0 방향 절삭하며, 비유한·지도 밖은 거부한다. (2026-11-09 WB04-E145)
function cell(value, biased) {
    const adjusted = Math.fround(value) + (biased ? BIAS : 0);
    if (!Number.isFinite(adjusted) || adjusted < 0 || adjusted >= MAP_SIDE) throw new RangeError('경로 지도 좌표');
    return Math.trunc(adjusted);
}

// 원문 spot8/일반 finder/현재 genus mask 의 도착 겹침 판정이다. (2026-11-09 WB04-E145)
export function pathArrivalBlocked(pool, hash, types, spots, blockingGenus, point) {
    const x = cell(point.x, true);
    const y = cell(point.y, true);
    if ((spots[y * MAP_SIDE + x] & ARRIVAL_SPOT) === 0) return false;
    const mask = typeof blockingGenus === 'function' ? blockingGenus() : blockingGenus;
    const left = cell(point.x, false);
    const top = cell(point.y, false);
    const finder = new RawSquidFinder(pool, hash, types);
    // 원문의 첫 현재 부류 일치에서 끝낸다.
    for (let candidate = finder.begin({ left, top, right: left, bottom: top }); candidate !== 0; candidate = finder.next()) {
        if ((types[pool.slot(candidate)[TYPE]].flags2 & mask) !== 0) return true;
    }
    return false;
}

// 0041be80: 물리 풀 base 는 고정이며 SID 0 은 원문처럼 허용한다. 반환은 항상 참이다. hooks = { form(), parent(), capacity(), assertion(식, 파일, 줄) }. (2026-11-09 WB04-E145)
export function runPathBaseRobust(hooks) {
    const form = need(hooks.form)();
    let capacity = need(hooks.capacity)();
    if (form && form >= capacity) {
        need(hooks.assertion)('formSid == 0 || form.isValid()', BASE_SOURCE, 0x69);
        capacity = need(hooks.capacity)();
    }
    const parent = need(hooks.parent)();
    if (parent && parent >= capacity) need(hooks.assertion)('attachToSid == 0 || s.isValid()', BASE_SOURCE, 0x6c);
    return true;
}

// 공통 도착 부모 몸체이다. checking 은 현재 DWORD 를 돌려주는 함수 또는 숫자이고, hooks 는 parent·validAlive(sid)·step()→{x,y}·unpop(sid)·pop(sid, point)·alive()·robust()·blocked(point)·halt()·
// finished(bool)·deadOrFree(sid)·watchPresent()·armWatch()·clearWatch()·arrival(sid)→반환 코드·kill()·clear(sid)·assertion(식, 파일, 줄)이다. (2026-11-09 WB04-E145)
export function runPathArrivalParent(checking, hooks) {
    const on = () => ((typeof checking === 'function' ? checking() : checking) >>> 0) !== 0;
    const assertArrival = (expression, line) => need(hooks.assertion)(expression, PATH_SOURCE, line);
    const alive = () => need(hooks.alive)();
    const parent = need(hooks.parent)();
    if (!need(hooks.validAlive)(parent)) assertArrival('walker.isValid() && !walker.isDead() && !walker.isFree()', 0x25c);
    if (!alive()) return;
    const point = need(hooks.step)();
    if (!alive()) return;
    need(hooks.unpop)(parent);
    if (!alive()) return;
    need(hooks.pop)(parent, point);
    if (!alive()) return;
    if (on()) need(hooks.robust)();
    if (!alive()) return;
    // 0041be80 의 반환은 항상 1 이므로 원문 false 분기는 미도달이다.
    need(hooks.robust)();
    if (!alive()) return;
    const blocked = need(hooks.blocked)(point);
    if (need(hooks.halt)()) {
        if (blocked) need(hooks.clear)(parent);
        return;
    }
    if (!alive()) {
        if (blocked) need(hooks.clear)(parent);
        return;
    }
    need(hooks.finished)(true);
    if (on()) need(hooks.robust)();
    if (!alive()) return;
    if (need(hooks.deadOrFree)(parent)) assertArrival('!walker.isDead() && !walker.isFree()', 0x286);
    if (!alive()) return;
    if (need(hooks.watchPresent)()) assertArrival('!deadFlag', 0x289);
    if (!alive()) return;
    need(hooks.armWatch)();
    const result = need(hooks.arrival)(parent);
    if (result !== KILL && result !== ALREADY_DEAD && result !== LINGER) assertArrival('returnCode == pathKILL || returnCode == pathALREADY_DEAD || returnCode == pathLINGER', 0x28d);
    if (!alive()) {
        if (on() && result !== ALREADY_DEAD) assertArrival('returnCode == pathALREADY_DEAD', 0x290);
        if (blocked) need(hooks.clear)(parent);
        return;
    }
    need(hooks.clearWatch)();
    if (on()) need(hooks.robust)();
    if (alive()) {
        if (result === LINGER) need(hooks.finished)(false);
        else need(hooks.kill)();
    }
    if (blocked) need(hooks.clear)(parent);
}
