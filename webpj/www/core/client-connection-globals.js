// Client 수명의 Native 연결표(22 연결·22 송신버퍼·32 pending 표와 개수)와 실제 로컬 양끝의 수명 장치를 보관하는 모듈이다(cpppj `client/ClientConnectionGlobals.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 ec0b6e97 까지 반영).
// `RawConnectionRegistry`(E308)를 그대로 확장해 Client 와 단독 미션의 두 연결 끝이 같은 표·토큰 발급기·동기화 토큰을 공유하게 한다: 최초 BSS 0 만 구성하며 Session 획득/반납이 표나 개수를 다시 초기화하지 않는다.
// 사용: const connections = new ClientConnectionGlobals(); // 독립 표(최초 BSS 0)
//   const connections = new ClientConnectionGlobals(local); // local.clockSynchronization().connection(005c8bf4)을 동기화 토큰으로 빌린다(현재 값은 쓰지 않음, 복제하지 않음).
//   connections.cleanup() 이 같은 표 묶음의 보기를 돌려주고, connections.shutdownDevices() 는 앱 최종 종료 때 잔류 노드를 명시 정리한다.
// local 계약: `clockSynchronization()` 이 시간 동기화 세 DWORD 접근자(`connection`·`replyMode`·`gameTicks`, `resetClockSynchronization` 의 입력과 같은 형태)를 돌려주는 객체이다. 실제 `ClientLocalStateGlobals` 는 4-D 에서 옮기므로
//   그 전까지는 이 최소 계약만 시험 장치로 만족한다. local 이 없으면 C++ `invalid_argument` 와 같은 오류로 거부한다.
// 기준선 뒤 cpppj 변경 판정(LEFT_JOBS 3.6절): ① `devicePin`(실제 Source 의 shared_ptr 수명 보유, a76499e3·50a5a184)은 GC 가 대신하므로 옮기지 않는다. ② `ShutdownDevices` 의 표 정리는 `shutdownDevices()` 로 옮긴다
//   (Source 순환을 끊는 `PacketSources::Shutdown` 래퍼는 shared_ptr 순환 해소용이라 해당 없음). ③ `PacketSources()` 의 Resolve 경로와 `PacketDelays()`(ec0b6e97, 실소켓 지연 FIFO·`lag` 선택·`ResetDefault`)는
//   `ClientMain`·`ClientCampaignTransport`·Meta 가 소비하므로 그 단위(4-C/4-D)를 옮길 때 단독 도달 여부와 함께 판정한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E325: 4-C ClientConnectionGlobals 이식)
import { RawConnectionRegistry } from './raw-connection-registry.js';

// 연결표 소유자 클래스이다. 등록소의 모든 기능(토큰 발급·등록·정리 등)을 그대로 쓰고 Client 수명의 동기화 토큰 대여와 앱 종료 입구만 더한다. (2026-11-12 WB04-E325)
export class ClientConnectionGlobals extends RawConnectionRegistry {
    // 사용: new ClientConnectionGlobals() 는 새 Client 의 원문 고정 배열 수명이고, new ClientConnectionGlobals(local) 은 기존 005c8bf4 의 같은 칸과 수명을 빌린다.
    // 인자를 명시했는데 비었거나(null/undefined) 시간 동기화 계약이 없으면 거부한다(C++ `invalid_argument`: "연결표의 실제 Client 동기화 owner 누락"). (2026-11-12 WB04-E325)
    constructor(...args) {
        super();
        // 인자 없는 생성은 독립 표이다.
        if (args.length === 0) return;
        const local = args[0];
        if (!local) throw new Error('연결표의 실제 Client 동기화 owner 누락');
        // local 이 가진 시간 동기화 세 DWORD 접근자에서 연결 토큰 칸만 빌린다.
        const synchronization = local.clockSynchronization?.();
        if (!synchronization || typeof synchronization.connection !== 'number') throw new TypeError('연결표의 동기화 owner 는 clockSynchronization().connection 이 필요합니다.');
        // 등록소는 { value } 상자를 요구하므로 같은 칸을 읽고 쓰는 접근자 상자로 감싼다(사본 없음, 쓰기는 DWORD 로 감김).
        this.bindSynchronization({
            get value() { return synchronization.connection; },
            set value(next) { synchronization.connection = next >>> 0; },
        });
    }

    // 앱 최종 종료 전용: UI/시계/장기 데이터를 반납하기 전에 전역 Native 연결 표에 남은 노드와 초기 패킷 버퍼를 명시 정리한다. 진행 중 부모 안에서는 거부된다(등록소 규칙). (2026-11-12 WB04-E325: 기준선 뒤 50a5a184 반영)
    shutdownDevices() {
        this.shutdownTrackedDevices();
    }
}
