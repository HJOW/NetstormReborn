# 764 — 글꼴·음질 명령과 현재 표시/장치 수명

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon/2026-10-23 유지. 단위764 완료·전체 목표 미완료다.

## 현재 입력과 소유자

ChangeFontFace(542764/0200001b)와 ChangeSoundQuality(542a68/020000dd)를 같은 이름표·HTML 허용표·일반 FIFO와 UberGump에 연결했다. Guide/Gump/뗌 뒤, HelpOpen/Console/공통 정지 앞에서 마스크한 두 명령만 조기 분배한다. 기존762의 동기 highPriorityValue 읽기와 raw 메인 루프는 유지했다. 새 기본 UI 항목은 만들지 않았다.

글꼴은 signed y→현재 OS 목록→같은 FontStore의 전체 캐시 반납/18개 유효 조합 재생성→현재 H/W→전체 dirty→현재 이름 재조회→Config Set 순서다. 정확 type4/ANSI charset0만 원래 순서로 두 번 열거한다. NULL은Arial, 유효 빈 이름은 빈 이름이며 slot1은Courier New다. 현재 이름은 요청 이름의 최대31BYTE/첫NUL이며 GDI가 대체한 물리 이름과 구별한다. 음수 인덱스의 원문 assert 뒤 범위 밖 주소는 host 예외로 명시 거부한다. 열거 사이 개수 변화도 가짜 행으로 채우지 않는다.

새 FontStore::Rebuild는 옛 글리프 셰이프를 파기한다. Renderer는 ShapeDatabase 주소별 압축 해제 이미지를 보관하므로, 재생성 전에 ClearDecodedImages로 그 캐시만 반납한다. 일반 SetScene/InvalidateAll/매 프레임은 캐시를 유지한다. 기존 참조를 빌리는 메뉴는 같은 FontStore를 계속 사용하고 다음 Compose가 새 글꼴을 수집한다. 새 캐시 수명은 원문에 없는 host 캐시의 안전성 보완이다.

음질은 기존 Shutdown→실제 signed%d의 soundQuality 설정/동기 옵션 해석→priestmove.wav와다섯0이다. 원시 signed 옵션값은 보존하고 장치의 품질 범위/하향은 기존 SoundDevice가 정한다. 옵션 재해석은 전역→기존 장치 열기/닫기→효과음/음악 음량→현재 곡 순서다. 열기 실패가 끈 enabled를 후속 옵션 대입으로 되살리지 않는다. 옵션 재열기는 WinMain 초기 천둥 preload를 반복하지 않으며 초기 Initialize의 순서와 음악 스레드 합류 정책은 유지했다.

## 검증과 한계

원문 글꼴410입력/820관측, 음질32+옵션192입력/448관측, 이름/FIFO21입력을 동결했다. root 추가820/448/21회가 기존 여섯 fixture와 바이트 일치했다. 실제 OS/GDI/Config/장치 등 하위 경계는 관측에 명시하며 CPU 결과를 물리 출력으로 세지 않는다. 최초 CRT 스택/__aulldvrm/미정리 인자 교정과 음향 이름 추정 교정은 준비 원시/README에 보존했다.

최종 Release 경고/오류0. 집중 **27개(새9개)/실패0**. 이름/FIFO·315열거/54정상lookup·15부모/부분실패·음질32/옵션192·실제 무장치 음소거/기한·18CHFNT 재읽기와 기존 음향/동기priority 회귀를 확인했다. 두 픽셀 회귀는 주소 재사용을 결정적으로 강제하며 폭3이 같은 옛 왼쪽 A와 새 오른쪽 A를 실제 Renderer로 구별한다. 새 실행대상0/기존66개·전체71대상이다.

기존 보호 2677개 모두 불변·새fixture6 포함2683개/fixture863. 다음 기준은 extracted/cpp-resume-764/protected-after.json이며 제품 SHA-256은 `0f599ea981231546428b5b4aeb7273d8c7fccf1ef35192023eb74cafccd7ea5d`다. [보고서](../../cpppj/presentation-font-sound-20261103.json)와 같은 extracted의 최초/최종 빌드·집중 로그·원문 반복 원시를 따른다.

ACP 이름/UTF8 경로/비ASCII GDI 선택·실제 OS 글꼴 픽셀·음향 장치 지원/PCM·실제 재열기 재생위치/기한 전체·다른 자산의 주소 캐시 수명은 완료가 아니다. 첫 옵션해석/바람음 선택과 CD10.72는 별도다. 새게임/GUI/CLI/MP·물리/PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP 검사는0이다. 다음은765와 각 reader/일반 명령 복원이다.
