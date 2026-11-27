# 아비어리·공용 Form 경계의 현행 판정 — WB04-E1444

2026-11-27 HJOW-Athlon. 인계3.6의 “아비어리·공용 Form writer 경계에서 확인하지 못한 칸(확신 낮음, E594·E1210)”을 현재 구현과 대응표에 대조했다. 아래 연결은 이미 구현돼 있으며 이번에 제품 코드를 바꾸지 않았다.

| 경계 | 현재 웹 선택·소비 |
| --- | --- |
| 공용 ProcessForm | `RawSurfaceWorld`의 PopRuntime과 `RawSurfaceStorage`의 대여 없는 fallback 모두 같은 장기 Host의 `writeFormPacket`을 부른다. 실제 현재 PID·runtime pin과 원래 부모 진단 뒤 일반 Regular0/공유 Regular28BYTE/파생 전체 writer를 선택한다. 공통 GameAux2048BYTE를 사용한다. |
| 파생 송신 | Session의 Dais→Aviary/Timer/Flyer→Construction→전투→자산 합성이 실제 Host에 게시된다. Sun/Rain/Wind의 현재 Process 클래스로 writer를 고르고 원래 프로토콜38h/73h/74h 데이터를 만든다. 일반 웹은 typed 경로다. Native 장치 선택은 별도 플랫폼 분기이며 그 부재를 전체 아비어리 미구현으로 보지 않는다. |
| 아비어리 본체 | 생성 순서의 Sun/Wind/Rain 바인딩이 같은 실제 Pool·Process Host·소유권·World 생성/Pop과 비행 본체에 연결된다. 미연결 선택을 빈 성공으로 넘기지 않는다. |
| 수신 | 실제 Session dispatch가38h/73h/74h를 등록한다. 새 Form은 실제 필드/Process를 같은 Host에 부착하고 기존 Form은 현재 PID의 필드를 갱신한다. Sun/Rain은 `readAviaryProcessData`, Wind는 원래 전용 reader를 쓴다. |
| E1210 | 대응표의 해당 단위는 아비어리 writer가 아니라 NativeFortSaveVirtual 네 가상 조회기 판정이다. ObjectRobust·ContentRobust·HasLevel·Level은 Fort 저장/보상/의식의 현재 웹 부모에 이미 반영돼 있다. 이 옛 참조를 writer 잔여 근거로 계속 두지 않는다. |

기존 `process-form-writer`, `surface-transmission`, `raw-campaign-session-transmission`, `aviary-process-data` 네 검사 파일을 명시해46/46 통과했다. 현재 Host/PID·대여 반납·같은 헤더/버퍼·일반 Regular0·공유 부동소수 비트·현재 writer 선택/수신 필드 폭과 실패 접두를 확인한다. 새 테스트/fixture/원본 실행과 공중3종 전용 실제 플레이는 없다. 정적 게시 경로와 계약 검증 결과이며 모든 아비어리의 원본 자연 플레이 동일성을 새로 증명한 것은 아니다. E1210은 기존 대응표/호출부 감사이며 이번46개에 별도 재실행 수를 더하지 않는다.

이로써 모호했던3.6의 Form/아비어리 경계 판정은 닫는다. GameAux에 이름이 없는24개 Native 헤더·역할·진단 값의 플랫폼/수명 분류는 다른 행에 그대로 남긴다. Native 전체 저장소 정체성이나129개 전역 전체 완료를 선언하지 않는다. E1450 전체 회귀 중 작성 소스/설정/대응표를 바꾸지 않고 문서만 마감했다. E1445와 함께 종료 뒤v26에2개 이월하고v22=6/8이다.

[46개 원문](webpj-e1444-evidence/focus.log) · [현재 소스 위치/해시](webpj-e1444-evidence/source-audit.json) · [공용 부모](webpj-process-form-writer-20261116.md) · [아비어리 decoder](webpj-aviary-process-data-20261115.md).
