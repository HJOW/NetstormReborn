# dotnetpj 0.556 — Storage 물리와 World 대여 수명

2026-11-15, HJOW-Athlon. cpppj 제품1313의 RawSurfaceStorage 일반/준비 대여와 RawSurfaceWorld 생성 실패·일반 종료 경계를 옮겼다. M1은0/34이며 새 경로는 아직 플레이할 수 없다. 공유 장부 본체/전역 주소와 실행기의 장기 보유는 다음 작업이다.

## 구현

일반 fresh는 원래 초기화 값을 유지하면서 Graph 기록/stack·집계3표·섬3204바이트·청크·실패점의 물리 소유자를 교체하지 않는다. 배열을 빌린 실행기가 다음 World에서 옛 사본을 보지 않게 한다. 일반 전체0 초기화와 Native 부분 reset은 별개이며, 준비 대여에는 이러한 초기화를 삽입하지 않는다. 기존 첫 대여의 Pool/Hash/spot 보존과 다음 일반 대여의 reset 정책도 유지한다.

`RawSurfaceWorld(..., prepared: true)`는 실제 PATCH Storage와 자산/타입 크기를 대여 전에 검사하고 같은 현재 TYPE/frames 및 Graph 물리를 사용한다. 대여 뒤 Graph 생성이 실패하면 이미 수행한 값을 롤백하지 않고 lease만 반납한다. 성공한 대여 세대는 남긴다. 일반 Dispose는 현재 동기 호출이 끝난 뒤 자기 ProcessHost 대여와 Factory/PostPop의 짧은 장치를 반납하고 마지막에 물리 대여를 해제한다. State 반납으로 이전 Regular가 남은 경우는 기존 저장소 계약을 유지한다. 실제 Runtime 전체 pin 대여와 Native 장기 실행기 소유를 완성했다는 뜻은 아니다.

공통 PostPop의 ReleaseRuntimeBindings는 현재 Session용 AI/선택/owner/pin·Teleport·등록 파생만 놓고 기본 장기 장치·TYPE·목록/깊이는 유지한다. 현재 개발 Host의 별도 풀은 아직 전환하지 않았다.

## 검증

신규5검사는 일반 fresh의 전체 물리 주소/값, 준비 대여와 중복 거부의 불변, 두 준비 World의 같은 Graph 배열과 반납,187타입 Graph 생성 실패 뒤 바이트/세대/재대여, 자산/프레임 크기 불일치의 대여 전 거부를 확인한다. 원본 기대값을 새로 만들거나 변경하지 않았다. 최초 관련321/321, 대여 파일 분리 뒤327/327, 최종 입력 경계까지 **327/327** 통과다. TRX는 `extracted/dotnet-storage-lifetime-0556-initial/`·`-final/`·`-final2/`다. Host Release 경고0·오류0이며 전체 회귀는0.546 기록을 유지한다.

L2는 동일 SHA cpp34 기준선을 재사용하고 dn일반31개를 새로 실행했다. 모두 종료0·수집 오류0·원본1,394파일 전후 동일이다. 0.555 수정본 대비 상태 변화가 없고 파일 전체 동일16·frame행만 차이15경로다. 전체 일치0/34·데모dn3 미연결이다. 자료 `extracted/dotnet-l2-0556/`에 보관했다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP917행(L1 431·port486), cpppj 대응415단위·fixture312개다. 수명 부분 대응4행을 더하고 기존 행/순서·해시·floor/reflected를 유지했다. 현재 큰 cpp 파일 전체를 닫거나 누락 계열을 줄이지 않아 R4 누락138단위/13,913줄·R6 누락382단위/45,090줄은 유지한다.

다음은 Storage의 공유 PostPop/삭제/로딩 상태와 현재 전역 주소, 장기 공장·공간·Graph/이웃 실행기 보유·실제 World 대여다. 미이식 파생 장치·현재 HostSession 전환·상위 피해/해방/삭제/배치를 잇고 R4-2 이후를 진행한다.
