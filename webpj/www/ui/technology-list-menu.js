// 기준선 1f00a25a TechnologyListMenu.cpp/.h의 원소별/전체 기술 메뉴를 옮긴다(004cb320/004cb3f0/004cb4b0/004cb690).
// 사용: new TechnologyListMenu(hooks).countKnown(output, { value: stored }, theme, level) 또는 appendTheme/appendAll(target).
// 실제 Session TYPE/소유/레벨과 현재 메뉴를 훅으로 빌린다. 빈 출력은 NULL 목록이며 40칸을 넘는 후보도 세고 원문처럼 순환 방문한다.
// 기준선 뒤 변경은 없다. 랭크 분기는 정적 복원만 하며 온라인 상태를 새로 만들지 않는다. (2026-11-14 WB04-E450)

// 선택한 경계에서만 실제 장치 유무를 검사한다. 누락을 빈 성공으로 바꾸지 않는다.
function need(hook) { if (typeof hook !== 'function') throw new Error('기술 목록의 실제 장치 미연결'); return hook; }

// 실제 기술 필터와 현재 메뉴 작성기를 공유하는 원문 부모다.
export class TechnologyListMenu {
    // 매 호출 현재 Session/Config/UI를 읽는 경계다.
    #hooks;
    // 구성 시 게임 상태를 조회하거나 메뉴를 만들지 않는다.
    constructor(hooks) { this.#hooks = { ...hooks }; }

    // theme 5는 레벨도 무시한다. 저장 용량과 전체 일치 수를 구별하고 호출자가 준 stored를 초기화하지 않는다.
    #count(output, stored, theme, level, known) {
        const h = this.#hooks; theme |= 0; level |= 0;
        if ((level < 0 || level > 2) && level !== -1) need(h.assertion)(known ? 0xb94 : 0xbad);
        const parent = need(h.currentTechnology)(); let count = 0;
        // 소유 조회가 바꾼 현재 TYPE 수를 다음 조건에서 다시 읽는다.
        for (let index = 0; index < (need(h.typeCount)() | 0); index += 1) {
            const type = need(h.type)(index);
            if (index === (need(h.priestType)() >>> 0) || type.group === 10 || (theme !== 5 && (type.theme !== theme || type.level !== level))) continue;
            if (known && !need(h.owns)(parent, index)) continue;
            if (output?.length && (stored.value | 0) < output.length) {
                if ((stored.value | 0) < 0) throw new RangeError('기술 목록의 음수 저장 인덱스');
                output[stored.value | 0] = index; stored.value = ((stored.value | 0) + 1) | 0;
            }
            count = (count + 1) | 0;
        }
        return count;
    }

    // 소유 조회가 반환한 종류 WORD가 비영인 후보만 센다.
    countKnown(output, stored, theme, level) { return this.#count(output, stored, theme, level, true); }
    // 소유 장치를 요구하지 않으나 선행 currentTechnology 조회는 보존한다.
    countCandidates(output, stored, theme, level) { return this.#count(output, stored, theme, level, false); }

    // 대상 DWORD를 그대로 가상 레벨에 넘기고 두 40칸 수집 뒤 레벨 순으로 행을 작성한다.
    appendTheme(target, theme) {
        const h = this.#hooks; target >>>= 0; theme |= 0;
        const level = (need(h.level)(target) - 1) | 0; const known = new Uint32Array(40); const candidates = new Uint32Array(40);
        const knownStored = { value: 0 }; const candidateStored = { value: 0 };
        this.countKnown(known, knownStored, theme, level);
        const count = this.countCandidates(candidates, candidateStored, theme, level); const parent = need(h.currentTechnology)();
        // 음수 level은 앞선 조회만 수행한다. 최대 signed 값에서 호스트 증가를 멈춘다.
        for (let current = 0; current <= level; current += 1) {
            // 저장 용량이 아닌 전체 후보 수만큼 돌고 저장 개수로 나머지를 구한다.
            for (let index = 0; index < count; index += 1) {
                const number = candidateStored.value ? candidates[index % candidateStored.value] : 0; const type = need(h.type)(number);
                if (type.level !== current || number === (need(h.templeProduct)() >>> 0)) continue;
                const owned = need(h.owns)(parent, number); const pattern = need(h.translate)('%s (Level %d)');
                const displayLevel = (type.level + 1) | 0; const description = need(h.description)(number);
                const label = need(h.format)(pattern, description, displayLevel); const command = need(h.learnCommand)();
                need(h.item)(label, command, number, owned === 0);
            }
            if (current === 0x7fffffff) break;
        }
    }

    // 전체 메뉴만 대상 WORD로 접고 네 원소 제목/행 뒤 업그레이드 또는 랭크, SP 보상 행을 붙인다.
    appendAll(target) {
        const h = this.#hooks; target &= 0xffff;
        const rawLevel = need(h.level)(target) >>> 0; const level = (rawLevel - 1) | 0; need(h.reference)(target);
        // 표시하지 않는 NULL 수집의 소유/진단 효과도 각 원소에서 먼저 수행한다.
        for (let theme = 0; theme < 4; theme += 1) {
            const stored = { value: 0 }; this.countCandidates(null, stored, theme, level); this.countKnown(null, stored, theme, level);
            const name = need(h.themeName)(theme); const label = need(h.formatString)('~y%s', name); const command = need(h.doNothingCommand)();
            need(h.item)(label, command, 0, false); this.appendTheme(target, theme); need(h.separator)();
        }
        const surface = need(h.surface)(target) >>> 0; const parent = need(h.currentTechnology)(); let known = 0; let total = 0;
        // 전체 보유 집계에는 원소/레벨 필터를 적용하지 않는다.
        for (let index = 0; index < (need(h.typeCount)() | 0); index += 1) {
            const type = need(h.type)(index);
            if (index !== (need(h.priestType)() >>> 0) && type.group !== 10 && need(h.owns)(parent, index)) known = (known + 1) | 0;
        }
        need(h.currentTechnology)();
        // 두 번째 집계도 현재 수와 현재 TYPE를 다시 읽는다.
        for (let index = 0; index < (need(h.typeCount)() | 0); index += 1) {
            const type = need(h.type)(index); if (index !== (need(h.priestType)() >>> 0) && type.group !== 10) total = (total + 1) | 0;
        }
        let label; let command; let parameter = target; let enabled = false;
        if (total <= known) {
            need(h.rank)(); const firstRank = need(h.rank)() >>> 0; let maximum = need(h.maximumRank)() | 0;
            let next = maximum < ((firstRank + 1) | 0) ? need(h.maximumRank)() | 0 : (need(h.rank)() + 1) | 0;
            if (next < 0) next = 0;
            else {
                const secondRank = need(h.rank)() >>> 0; maximum = need(h.maximumRank)() | 0;
                next = maximum < ((secondRank + 1) | 0) ? need(h.maximumRank)() | 0 : (need(h.rank)() + 1) | 0;
            }
            const pattern = need(h.translate)('ADVANCE TO RANK %d!!'); label = need(h.formatNumber)(pattern, (next + 1) | 0);
            command = need(h.learnCommand)(); parameter = need(h.noIslandType)(); enabled = true;
        } else if (level === 2) {
            const pattern = need(h.translate)('Altar at Max %sLevel'); label = need(h.formatString)(pattern, ''); command = need(h.doNothingCommand)(); parameter = 0;
        } else {
            if (level < 2 && surface && surface < (need(h.poolCapacity)() >>> 0)) {
                const home = need(h.homeIsland)(); const island = need(h.island)(surface); enabled = island === need(home)();
            }
            const pattern = need(h.translate)('Upgrade Altar to Level %d'); label = need(h.formatNumber)(pattern, (rawLevel + 1) | 0); command = need(h.upgradeCommand)();
        }
        need(h.item)(label, command, parameter, enabled);
        const money = need(h.translate)('Get 5,000 ~[IfortGump.3]'); const learn = need(h.learnCommand)(); const nugget = need(h.nuggetType)();
        need(h.item)(money, learn, nugget, true);
    }
}
