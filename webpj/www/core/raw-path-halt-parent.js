// 공통 경로 Halt 부모(Pathprocess.cpp 00489ff0)의 현재 진단 · 저장 부모 · 자기 삭제 뒤 복귀 · 실제 후속 경계를 옮긴 모듈이다(cpppj `o/RawPathHaltParent.cpp/.h`, 10.78).
//  - runPathHaltParent(checking, hooks): 경로 가상 Halt(+38)를 부르기 전에 최초 부모를 저장하고(parent) 감시 토큰을 건다(armWatch). Halt 결과(10/11/12)를 얻은 뒤 경로가 이미 죽었으면(alive 거짓)
//    checking(현재 DWORD, 호출자가 값 대신 읽는 함수나 숫자)이 비영이고 결과가 12(이미 죽음)가 아닐 때 원문 진단 `result == haltALREADY_DEAD`(\Ns\O\Pathprocess.cpp 0x2d5)를 낸 뒤 참을 반환한다
//    (삭제 뒤에는 객체를 읽지 않고 진단 장치·수명 관찰만 쓴다). 살아 있으면 감시 토큰을 풀고, 결과 10 이고 저장 부모의 현재 flags 비트(0x20)가 강제 정지면 결과를 11 로 바꾼다.
//    결과 11 이면 현재 boss 또는 로컬 범위(mayKill)일 때 경로를 kill 하고 참을 반환한다. 아니면 현재 권한(boss)이고 저장 부모의 현재 TYPE genus 가 운반 계열(0x30000)이고 목표(TargetB 또는 TargetA)가 있으면
//    감시 토큰을 걸고 재지정(retarget, 자기 삭제할 수 있음)한 뒤 경로가 죽었으면 참, 아니면 토큰을 풀고 계속한다. 마지막 반환은 `결과 !== 10`(10 이면 계속 이동, 그 밖에는 정지)이다.
//  - 선택된 위치의 경계가 없으면 이미 일어난 효과를 되돌리지 않고 거부한다(armWatch/clearWatch 는 독립 수명 관찰만 쓰는 호출자가 생략할 수 있다).
// 10.72(CD) 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E144: 공통 경로 Halt 부모 이식으로 추가)

// 원문 진단의 식과 파일 이름, 줄 번호, 가상 Halt 결과 코드(진행 계속 10, 강제 정지 11, 이미 죽음 12), 운반 계열 genus 마스크, 강제 정지 flags 비트이다. (2026-11-09 WB04-E144)
const ALREADY_DEAD_EXPRESSION = 'result == haltALREADY_DEAD';
const PATH_FILE = '\\Ns\\O\\Pathprocess.cpp';
const ALREADY_DEAD_LINE = 0x2d5;
const CONTINUE = 10;
const FORCED_STOP = 11;
const ALREADY_DEAD = 12;

// 선택된 실제 경계가 없으면 거부한다(이미 발생한 효과를 되돌리지 않는다). (2026-11-09 WB04-E144)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('공통 Halt의 선택된 실제 경계 미연결');
    return hook;
}

// 공통 Halt 의 부모 몸체를 실행한다. checking 은 호출 도중에도 현재 DWORD 를 읽을 수 있도록 숫자 또는 값을 돌려주는 함수이다. 반환값은 "정지(true) 또는 계속(false)"이다. (2026-11-09 WB04-E144)
// hooks: parent()→저장 부모 SID·halt(parent)→10/11/12·alive()→경로 수명·forcedStop(parent)·mayKill()·kill()·boss()·carrier(parent)·hasTargets()·retarget(parent)·
// assertion(식, 파일, 줄)·armWatch()·clearWatch(). 사용: runPathHaltParent(() => checkingDword, hooks). (2026-11-09 WB04-E144)
export function runPathHaltParent(checking, hooks) {
    const readChecking = () => (typeof checking === 'function' ? checking() : checking) >>> 0;
    const parent = need(hooks.parent)();
    if (hooks.armWatch) hooks.armWatch();
    let result = need(hooks.halt)(parent);
    if (!need(hooks.alive)()) {
        if (readChecking() !== 0 && result !== ALREADY_DEAD) need(hooks.assertion)(ALREADY_DEAD_EXPRESSION, PATH_FILE, ALREADY_DEAD_LINE);
        return true;
    }
    if (hooks.clearWatch) hooks.clearWatch();
    if (result === CONTINUE && need(hooks.forcedStop)(parent)) result = FORCED_STOP;
    if (result === FORCED_STOP) {
        if (need(hooks.mayKill)()) need(hooks.kill)();
        return true;
    }
    if (need(hooks.boss)() && need(hooks.carrier)(parent) && need(hooks.hasTargets)()) {
        if (hooks.armWatch) hooks.armWatch();
        need(hooks.retarget)(parent);
        if (!need(hooks.alive)()) return true;
        if (hooks.clearWatch) hooks.clearWatch();
    }
    return result !== CONTINUE;
}
