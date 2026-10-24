# 실제 Console 편집 UI와 공유 버퍼 기반 — 2026-10-24

현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호 원문 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀 소스 SHA256은 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`다. 새 디컴파일·원본 게임 직접 실행·GUI 실행은0이다.

## 구현

[RawTextEntryControls](../../cpppj/src/client/RawTextEntryControls.cpp)는 원문 TextEntry/Console 본체를 같은 공통 Gump 등록표·자식 우선 수명·실제 배경/Slider·원본 글꼴/픽셀/클립에 연결한다. Console마다 실제 편집 버퍼와 두 Slider/각 세 자식/돌 버튼 배경을 만들어 **13개 실제 노드**를 사용한다. 여러 Console을 동시에 소유하며 하나씩 반납할 수 있다. 미부착/기본 생성 전 실패도 같은 Close에서 정리한다. RawMenu의 Node/Background/Attach/Handle/Idle/Draw/scalar와 RawButton 화면 교체 정리도 같은 실제 저장소로 분배한다.

[TextEntryStorage](../../cpppj/src/client/TextEntryStorage.cpp)는 미초기화 new[]·재할당·free를 실제 물리로 수행한다. 내부 오프셋 주소도 같은 할당을 조회하며 문자 본문과 줄offset 표를 별도로 소유한다. 호스트 포인터를32비트로 자르지 않는다. 원문 exact-capacity NUL의 할당 밖 쓰기는 호스트 범위 오류로 명시 거부한다. OOM 전체 unwind 복원은 아니다. 원문52730에서 반납하지 않는 제외 문자열은 SetExcluded 교체/해제 또는 저장소 수명 끝에서 반납한다.

StyleTextBufferStore의 ConstructBorrowed는 외부 상태 자체에 기존 실제 StyleTextBuffer를 설치한다. 같은 상태의 중복 생성을 거부하며 일반 버퍼는 여전히 자기 상태를 소유한다. PE004fc6e0의 `push 0x80; mov ecx,0x548758; call 0x4bbe40`과004ff0d0의 같은 전역 소멸을 정적으로 확인했다. **실제 Client 전역 로그/factory 접합은 다음 단계**이며 지역 로그의 같은 주소/실제 글꼴·조각·callback·소멸/재생성을 별도로 검증한다.

준비되어 있던 ChatWindow/HudChatGeometry/TextEntryGump/TextEntryBuffer 소스와 네 Native 검사는 미등록이었다. 제품/검사 빌드에 등록했다. clipboard/recipient/completion/submit은 선택 시 실제 문맥을 요구하며 미연결을 성공으로 숨기지 않는다. 통합 검사의 명시 장치는 원시 콜백 계약을 확인하는 장치이며 실제 Win32 clipboard/Chatmanager 구현을 뜻하지 않는다.

## 검증

- 실제 집중 검사는 **46검사/실패0**이다. 신규 Native4+실제 통합5와 기존 메뉴/수명/NSInfo/생산/HUD/Uber를 대조했다. 실제 편집 물리4301BYTE 재할당·260개LF/줄offset 확장·내부 주소, Console13노드/두 Slider/선택·복사·제외문자 붙여넣기·Tab 인자/제출, 실제 글리프·선택/커서·벽시계 점멸·클립/padding, 두 Console 동시 소유·개별/화면 교체 반납, 외부 로그의 같은 상태/실제 조각/소멸을 검사했다. 최초 집중1은 Native4 뒤 실제 통합5 중 붙여넣기 기대1check가 실패했다. 원문451b80/준비 Native 검증을 다시 확인해 SetExcluded가 LF를 제외 목록에서 제거하는 원문 계약임을 확인하고 잘못된 검사 기대만 고쳤다. 제품/fixture는 이 수정으로 바꾸지 않았다.
- 읽기 전용 Unicorn 재생은 **180+2292+570+3960=7002행** 모두 보존된 입력/결과/명령 수와 같다. 현재 호스트/PE/정밀 소스 SHA를 엄격히 확인하고 예전 VM 고정 main/fixture 쓰기 경로는 호출하지 않았다. 원본 게임 프로세스를 실행한 것이 아니다. 각 상위 Native 관측의 장치 경계와 실제 자산 기반 조립 검사를 구별한다.
- 전체 CTest는 **2719검사/실패0·종료0·310.68초(wrapper)**다. 최종 집중/전체 제품과 검사 바이너리는 같다. 실제 Console BMP를 직접 확인했다. GUI/전체 게임 픽셀 동일성·실제 화면 ChatWindow 연결의 완료 증거로 확대하지 않는다.
- 기존 originals1394개는 같은 보호 기준과 일치하고 originals/CD/Native 추적 변경0이다. 작업 중 확인한 AGENTS 네임스페이스 주석 규칙 두 행의 외부 변경은 그대로 보존하고 이 단위 커밋에서 제외한다. 빌드/최초 실패·수정 경과는 아래 보고서의 원시 로그와 terminal 기록을 따른다.

최종 제품 SHA256: `226ee33c0aab924a5b69aa1675aed1819bc8c649a22cb288394cdfcd84ef87cb`

최종 검사 SHA256: `33c8f433ac434abf68f513716d17f41baf98e68e4342b3c8159971f70c7ab617`

[제품 보고서](../../cpppj/text-entry-client-runtime-20261024.json)에 변경 소스13/참조13/보존 Native5/닫힌 원시61파일의 SHA를 기록한다. 원시는 `extracted/cpp-chat-window-400-*`다.

## 다음 접합

ChatWindow의 실제 네 자식·Client 수명의128줄 로그/append callback·clear/quit 등록·helper 메뉴/명령/수신자/Tab 목록을 연결한다. 원문 ChatWindow 소멸은 현재 전역을 조건 없이0으로 쓰므로 임의 조건부 보존으로 바꾸지 않는다. 실제 창에는 아직 이 Console/ChatWindow를 설치하지 않았다. HTML 전체 공장과 새 Uber/State/Core/Pool 순서, Production/Minimap의 기존/새 제공자 동시 소유가 이어진다. 모든 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오 전체·완전 동일성 및393 Wide SaveOptions 원인은 남으며 /goal은 활성 상태다.
