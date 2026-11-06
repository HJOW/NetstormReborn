# raw 외부 Kernel World의 소유권과 현재 회수 대여 — 단위978

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 우선순위4의 legacy 외부 raw Kernel 경로를 실제 Client의 장기 Kernel 대여와 구별해 정리한다. 원본·디컴파일 산출물은 읽기만 했으며 게임·통신·신규 멀티플레이 검사는 실행하지 않는다.

## 확인한 두 실제 공백

`RawSurfaceWorld`는 장기 Storage가 pin한 Kernel과 다른 raw `Kernel*`를 받으면 같은 World 수명의 `legacyHost`를 만든다. 기존에는 이 경로가 Storage의 현재 가상 회수 대여를 설치하지 않았다. 반면 긴 Pool의 할당 회수는 항상 `RawSurfaceStorage::Processes().ReclaimForAllocation`을 호출했다. 따라서 현재 World가 살아 있어도 할당 소진에서 실제 후보의 가상 삭제에 도달하면 미연결 오류가 났다. 단위976의 PATCH004af1d0→현재 VT+10/flags0 계약은 실제 Client shared 경로에만 연결돼 있었던 것이다.

최종 소유권 반납에도 두 차이가 있었다. 직접 `RawSurfaceWorld` 소멸은 shared 대여만 반납하므로 raw 외부 Kernel에 자기 Host를 참조하는 프로세스를 남겼다. Session의 명시 종료는 반대로 `DrainProcesses`로 외부 Kernel 전체를 반복 Clear했다. 그 결과 다른 World Host나 Client가 소유한 객체까지 반납할 수 있었다. 이 두 동작은 게임의 Native 단일 Clear와 별개인 C++ 호스트 수명 정책의 결함이다.

## 현재 대여와 소유 필터

`RawProcessHostStorage`는 raw 외부 Kernel을 소유하거나 다른 Kernel로 바꾸지 않고 현재 World의 가상 회수 callable에만 대여를 발급한다. World가 강한 참조를 소유하고 Storage는 weak 참조를 보관한다. 현재 shared 대여와 legacy 대여의 중복을 거부하며, 선택된 회수 callable은 정상·예외 복귀까지 동기 호출의 지역 참조로 유지한다. World 종료 뒤에는 같은 Pool의 회수 선택을 명시적으로 거부하고 다음 World는 새 callable을 대여한다. raw 외부 Kernel 자체는 기존 계약대로 모든 World보다 오래 살아야 한다.

명시 종료와 직접 World 소멸은 `Kernel::DrainOwnedForShutdown`의 같은 실제 `legacyHost->Owns` 필터를 사용한다. 다른 Host/Client 프로세스를 보존하고 자기 슬롯0·덮어쓴 retired 소유권·소멸 중 자기 Host로 재등록된 객체를 모듈과 Pool이 살아 있는 동안 반납한다. shared 대여의 단일 Native Clear·Return 증빙과 이전 Regular 잔류 정책은 변경하지 않는다. 공개 `ClearProcesses`/`DrainProcesses`의 명시 전체 Kernel 동작도 이 최종 소유권 필터와 구별한다.

## 집중 검사와 한계

새 `LegacyWorldLifetime_` 검사4개는 다음을 조립한다.

- 외부 Client 프로세스와 두 독립 World Host를 같은 실제 Kernel에 등록한 뒤 명시 종료·직접 소멸의 소유 필터와 Host/Pool 생존을 확인한다.
- Kernel capacity1의 기존 요약/상세/단언 정상 복귀로 실제 슬롯0과 retired 소유권을 만든 뒤 자기 두 객체만 반납하는지 확인한다.
- 실제 ContentFormHost/ContentFormLifetime/공통 가상 삭제를 설치하고 현재 World에서 할당 회수·종료 대여 거부·같은 물리 재대여를 확인한다.
- 선택된 callable이 자기 대여를 반납해도 동기 호출 중 다른 대여가 겹치지 않고, 복귀 뒤에는 새 대여가 가능한지 확인한다.

새 검사는 GUI나 원본 게임을 실행하지 않는다. 특정 적 공격·포획·희생·승리, 멀티플레이·패킷·통신과 공중3종 전용 검사는 하지 않는다. 원문 allocator/OS 메모리 부족·원본32비트 heap/SEH 동작을 재현한 검증이 아니며, 최종 호스트 소유권 반납을 게임의 Native Clear 실행으로 합산하지 않는다. 기존 공유 Host 집중11개도 통과했으며 이를 실제 Client의 모든 자연 플레이 회귀로 확대하지 않는다.

## 최종 결과

최종Release 제품/집중 빌드 종료0·경고0·오류0이다. 첫 빌드는 새 검사의 보호된Pid 직접조회 C2248로 실패했으며 검사 파생 클래스에서 자기 PID만 공개하는 getter로 교정했다. 제품을 바꾸거나 기존 기대값을 약화하지 않았고 최초/최종 로그를 보존했다. 고유26검사(새4/기존22)가 첫 실행부터 실패0이다. 실제C++소유권/대여경계관측이며새Native/fixture/GUI/MP실행은없다. 기존보호2800개/fixture980개는불변이다. 제품978/11.01은11,392,000바이트·SHA256 `7323cd376f04143fdcbade4128045392fd41778ac6ee79c5879445bdb40d5fa8`이다. [단위보고서](../../cpppj/legacy-world-lifetime-20261106.json)를따른다. 다음979와4번마감판정을계속한다.
