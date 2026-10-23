# 실제 Client의 Gump 프레임 순서 연결

2026-10-23 단위371. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 VM-W11-Codex / 2026-10-23이다. 보호10.78과 기존 정밀 원문을 읽었으며 전체 디컴파일 산출물·원본/CD 파일을 변경하지 않았다. 선행 [프레임 감사](cpp-gump-frame-order-20261023.md)의 제안 patch를 검토하여 실제 Client에 적용했다.

## 실행 순서

`UberGump::Input`은 같은 Client 수명의 `GumpPointerBoundaryState`로 현재 Alt 폴링과 언어3의 Ctrl 보정, 창 밖 버튼 기록, 재진입 좌우 release를 처리한다. [독립 원문 대조](cpp-gump-pointer-boundary-20261023.md)는 3,878입력·두 정밀도7,756회·549,668명령이며 차이는0이다. 가운데 버튼 보정을 추가하지 않는다. 재진입 사건은 `PushCommand`로 변환하지 않고 기존40칸 큐의 raw `Push`로 예약한다.

이후 현재 Tutorial의 Guide 폴링 통지를 실행하되 반환값을 검사하지 않는다. 원본4d65a7과 일반 사건의 소비 관문4d6e6b는 다르다. 카메라/커서의 기존 Tick 어댑터를 실행한 다음 `NotifyPointer`에서 조건부 Overlay→현재 raw code/x/y Broadcast를 실행하고, 마지막에 일반 입력 큐를 소비한다. Overlay는 PE005427e4의 별도 사건0200003b를 실제 가상 처리기에 전달하며 반환0과 미호출을 구별한다.

창 밖에서는 Overlay/Broadcast를 생략한다. 캡처를 임의 취소하지 않고 이후 정상 입력이나 재진입 합성 release가 같은 실제 Route에서 해제하게 한다. 모드·focus·활성 커서와 현재 modal의 원본 관문을 유지하며, 아직 raw 부모가 아닌 논리 HTML 차단은 호스트 경계로 남긴다.

`Client::Run`의 정상 갱신은 설정 저장→SceneFrame→미니맵→Gump Cleanup→Gump Idle→Kernel 순서다. Cleanup이 예약한 OnExit는 다음 Input에서 처리한다. Idle에서 Kill한 대상은 다음 Cleanup까지 저장소/등록을 유지한다. GameClock Pause를 이 갱신의 차단 조건으로 사용하지 않는다.

원본 MovieUpdatePause0054dc58은 별도 깊이다. 현재 BSS0 값을 명시하고 양수일 때 Idle만 수행하는 원본 가지를 분리했지만, Moviegump의 생성·종료가 이 깊이를 소유하는 연결은 미복원이다. 이 가지가 실제 동영상으로 검증됐다고 해석하지 않는다.

## 검증 기록

- Release 빌드1은 종료0·경고/오류0이다. 새 검사 helper가 이미 붙은 Fancy 제목을 Push 없이 재부착하여 삭제 자식 순환 예외를 일으켰다. 실제 부모를 유지하도록 검사 장치를 수정했다. ShowSp의 Attach clamp에 어긋난 기대도 실제 상승 속도/노드 입력으로 교정했다. 빌드2 뒤 제목 입력의 미완료 마지막 줄 때문에 남은11check를 실제 LF 완료줄/현재 부모 Bounds로 교정했다. 제품이나 fixture를 바꾸어 실패를 숨기지 않았다.
- 최종 Release 빌드3은 종료0·경고/오류0이다. 제품 SHA-256은 `6d27b145fb1ffbfe2284bf2c17765897ff1536ec150177ba9ea6afd0f52f555b`이며 빌드1 이후 제품 변경은 없다.
- 실제1024 메뉴 core, 창 밖 버튼 기록/재진입 해제, Pause 중 메뉴 재열기/영구 메시지, 1280 미니맵 드래그/뗌/미션 이탈은 통과했다. 각각 전체 originals 해시와 허용 설정 복원을 확인했다.
- 1280×720 메뉴 확장/현재 행·음량2/4·해상도No·메뉴 열린 QWASZX·바깥 월드선택, 생산의6슬롯/회전/배치/재공급, 해상도OnExit의No계속실행·Yes정상조기종료와복원전800×600저장을 모두 통과했다. 전체 originals 해시·허용 설정 복원을 확인했다.
- 최종 관련8필터·56검사는 실패check0·종료0이다. 전체 CTest1은2416검사·실패check0·종료0/215.06초(전체215.70초)이며 LastTest 내부2416/0도 확인했다. 최종 검증 뒤 제품·검사 C++ 변경은 없다.

## 남은 경계

RawButtons 공통 표와 Warning/논리 HTML 부모는 아직 단일 전역 표/삭제 큐로 통합되지 않았다. 기존 Poll/Warning Idle 결합 메서드는 독립 검사 호환용으로 남고 실제 Client는 분리 API만 사용한다. 기존 카메라 이동·HTML 메뉴의 근사와 전체 HUD dock/viewport·Editor·모든 캠페인 사용자 완주는 이 단위의 완료 범위가 아니다.

검사 CLI의 `pointer` 단계는 폴링 code/x/y만 바꾸며 큐에 사건을 넣지 않는다. 재진입 GUI는 이 상태에서 실제 제품이 release를 예약하고 미니맵 capture를 해제하는지 확인한다. OS 자체의 마우스 메시지 생성을 검증하는 도구는 아니다.
