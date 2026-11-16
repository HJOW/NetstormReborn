// Session InstallContents3135..3156의 실제 기술·허용표·Deck/생산/동기화를 연결한다(225553df, WB04-E598).
// 사용: connectContentLifetime 뒤 bind. Factory 연구/업그레이드와 전체 World/Session 공개는 후속 구간이다.
import { RawPlayerTechnology, bindPlayerTechnologyTypes } from './raw-player-technology.js';
import { RawTotalMade } from './raw-total-made.js';
import { RawProductionDeck } from './raw-production-deck.js';
import { RawCampaignProductionBindings } from './raw-campaign-production-bindings.js';
import { RawProductionSync } from './raw-production-sync.js';

// 현재 Player/내용물 원문의 비정렬 WORD/DWORD를 읽는다.
function read(raw, offset, width = 4) { const view = new DataView(raw.buffer, raw.byteOffset + offset, width); return width === 2 ? view.getUint16(0, true) : view.getUint32(0, true); }
// 선택된 원문 SessionMissing 위치에서만 실패한다. 성공 기본값을 만들지 않는다.
function missing(line) { return () => { throw new Error(`실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:${line}`); }; }

// 단일 Impl의 현재 생산 부모와 장기 Player Deck 메모리를 잇는 생성 구간이다.
export class RawCampaignSessionProduction {
    // 같은 owner와 한 번 설치/정상 복귀/반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문이 하위 생성자에 값으로 넘기는 기술/동기화 함수와 현재 생산 상태다.
    technologyHooks = null; syncHooks = null; productionState = null; syncState = null;

    // 생성은 실제 Deck 할당이나 기술/생산 동작을 실행하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 내용물 생성기 뒤 원문의 기술→집계/금지→Deck→생산→동기화 순서를 지킨다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.contentModules?.lifetimeReady || this.#started) throw new Error('Session 생산의 초기화 전/반복 결선');
        this.#started = true;
        // loadedTypes는 동일 GameAssets 로딩 시 만든 불변 이름표다. 수정 가능한 World TYPE 이름으로 바꾸지 않는다.
        this.technologyHooks = bindPlayerTechnologyTypes(o.assets.loadedTypes, { assertion: missing(3135), destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); } });
        o.technology = new RawPlayerTechnology(o.player, o.priests.carrier, o.contained, c.types, this.technologyHooks);
        o.totalMade = new RawTotalMade(c.types, c.bookkeeping, o.restrictions, missing(3140));
        o.productionDeck = new RawProductionDeck(o.player, (bytes) => {
            this.#live(); if (typeof o.services.production.allocationBytes !== 'function') throw new Error('실제 Session 생산 Deck 할당 입력 미연결'); o.services.production.allocationBytes(bytes);
        }, missing(3144), 0x30000000, o.playerRecords.productionDeckMemory());
        this.productionState = {
            get mission() { return o.objectState.mission; }, get fullDeck() { return o.fullDeck; },
            get freeTechnologyType() { return o.freeTechnologyType; }, get templeProduct() { return o.templeProduct; },
        };
        o.production = new RawCampaignProductionBindings(c, o.player, o.priests.carrier, o.contained, o.productionDeck, o.totalMade, o.contentFinder, this.productionState, { ...o.services.production.effects });
        this.syncHooks = {
            deck: () => { this.#live(); return o.productionDeck.bytes(o.productionDeck.get(o.playerState.templateOwner)); },
            technology: () => { this.#live(); return o.player.currentTechnology(); },
            temple: (owner) => { this.#live(); if (owner >= 9) throw new RangeError('생산 동기화 현재 Player 범위'); return read(o.players, owner * 0xb4 + 0x7c) !== 0; },
            cancelCursor: () => { this.#live(); if (!o.cursor) throw new Error('생산 동기화 실제 커서 미설치'); o.cursor.cancel(); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            findKind: (parent, mask, kind, index) => { this.#live(); const form = o.priests.carrier.findContained(parent, mask, kind, index); return form ? read(c.pool.slot(form), 18, 2) : 0; },
            create: (parent, kind, quantity, flags, attach, quality) => { this.#live(); return o.contained.create(parent, kind, quantity, flags, attach, quality); },
            assertion: (text) => { this.#live(); console.error(`생산 동기화 원본 진단: ${text}`); },
        };
        this.syncState = { localOwner: () => o.playerState.battleOwner, fullDeck: () => o.fullDeck };
        o.productionSync = new RawProductionSync(c.pool, c.types, o.contentFinder, this.syncState, this.syncHooks);
        this.#ready = true;
    }

    // 초기화 실패의 이미 설치한 Deck도 반납할 수 있으며 이후 자기 closure는 실행하지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session 생산'); }
    // World 프로세스를 정리한 뒤 같은 Player 포인터/실제 Deck 블록만 기존 소멸자에 반납한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; this.#owner.productionDeck?.dispose(); }
    // 기술·생산 접두의 정상 복귀다. Factory/전체 InstallContents 완료가 아니다.
    get ready() { return this.#ready; }
}
