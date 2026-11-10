# 표시·전송·건설의 직접 소유 경계 감사 — 작업1254

**정적 감사만 수행했다. 확정 제품은1253/11.01이며 새 빌드·검사·GUI 실행은0이다. 전체 M6/목표는 미완료다.** 선택적 pin이 있는 모든 위치를 무조건 실패로 바꾸지 않고 실제 활성 caller의 생성/공개/반납 수명을 대조했다.

| 위치 | 분류/판정 | 실제 근거 |
| --- | --- | --- |
| RawSurfaceStorage::NoteChanged | 직접 소유의 작은 변경 관측 / 현행 유지 | World 생성자에서 최초 사용 가능하다. 실제 callback은 같은 Impl의 changed=true만 수행하며 외부 함수 호출이나 자기 대여 교체가 없다. |
| CurrentDisplayView/Suppressed/Invalidate | 생성/로드 및 공개후 표시 / 현행 유지 | World Impl162~164에서 displayRuntime 결합 직후 같은 생성자의 Display()가 CurrentDisplayView를 소비한다. 공개 전 실제 C++ 호출자가 owner를 보유한다. 공개 후 실제 pin은 종료/교체 시 throw하거나 실제 RuntimeCall을 반환한다. 함수 사본은 getter/쓰기 전에 선택한다. |
| MetaRefresh | 선택한 파생 표시/Client fallback / 현행 유지 | 현재 대여에 실제 metaRefresh가 있을 때만 선택하며 없으면 같은 Client장치를 쓴다. 선택 Runtime은 공개후pin/직접 호출자 수명을 따른다. |
| Transmission connected/broadcast/send | Native장치 우선/독립 typed fallback / 현행 유지·실행 제외 | Native의 실제 transmissionDevices가 우선이다. 잔여 대여는 독립 typed/직접 생성자 부모 수명을 따르며 상시 pin 필수로 바꾸면 기존 독립 계약을 깨뜨린다. MP/패킷 실행은 하지 않는다. |
| Transmission processPacket | 현재 World Host 또는 장기 Host / 현행 유지·실행 제외 | 현재 대여가 있으면 같은 실제 Host의 기존 writer를 쓴다. 대여가 없으면 장기 Processes Host의 실제 PID/현재 buffer를 쓰며 가짜 반환을 넣지 않는다. |
| TransmitPop | 현재 전송 본문 또는 장기 직접 writer / 현행 유지·실행 제외 | 현재 World lambda는 장기 RawSquidTransmission::Pop을 호출하며 Storage::TransmitPop으로 재귀하지 않는다. 공개 후 같은 실제 pin을 사용하고 공개 전 실제 직접 caller가 보유한다. |
| CurrentConstructionCall | 공개 전 직접 생성/로드/최종 정리 / 현행 유지 | 기존 주석과 실제 World/Session caller의 소유를 따른다. 공개된 경로는 ClientGameplayGlobals::BindRuntimeOwner→BindRuntimePin→PinCurrentRuntime으로 실제 전체 GameWorld 수명을 유지한다. actual pin은 무효 owner를 성공NULL로 반환하지 않는다. |

실제 pin 구현은 현재 Session/owner 정체성과 weak 수명을 검사하고 무효면 throw한다. 유효하면 RuntimeCall이 전체 owner와 자식 reset 관문을 보유한다. 바인딩 없는 NoWorld fallback은 같은 장기 본문을 사용한다. 이 감사는 임의로 주입한 callback의 모든 행동이나 전체 process/등록표의 마감 판정은 아니다.1250~1253에서 교정한 선택된 큰 후단의 필수 pin/owner는 유지한다.

마지막 디컴파일은 Client 양판본2026-11-03/R2026-11-07, 현재와 같은 HJOW-Athlon이다. 보호 원본/CD/fixture·AGENTS75e4eed9e2,808파일과 감사 소스5파일 불변이다. MP 실행은0이다.

다음은 원문의 기본 활성 지연큐 M6-l이다. 해당 큐와 M6-m이 연결된 뒤 전수 송수신/기록/등록/해제 감사를 다시 닫아야 하므로 M6-n을 전체 완료로 바꾸지 않는다. 큰 NoWorld 하위·최초 후처리·Path/AI/배치/Fort Finish도 남는다. 글꼴 외부 배포 규칙은 M8/K1/K4/K5에서 확인한다.

[감사 보고서](../../cpppj/direct-runtime-owner-audit-20261110.json), 원시 `extracted/cpp-resume-1254/`.
