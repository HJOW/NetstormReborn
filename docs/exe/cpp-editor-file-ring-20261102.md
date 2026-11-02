# cpppj 단위738 — Edit Template 파일 조회의 공유 경로 고리

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이며 새 디컴파일은 없다. 실행 제품은 **738 / 표시11.01**이다.

Template의 Edit 작업본 읽기는 임시 BaseFileSystem마다 별도 경로 고리를 만들었고, Exists/ReadOnly는 직접 CRT만 호출했다. 원문의41ab10과41af80→41ad10은 절대 파일도 같은 장치 getter를 한 칸 사용하며, 기존 실제 VFS 읽기도 같은 고리를 사용한다. 기존32행 장치/352행 작성기/1024행 읽기 정책과 원문 정적 호출 순서를 근거로, EditorFiles에 공유 소유자를 연결했다.

실제 UberGump의 장기 작업본·목록 장치는 Client의 FileNameOwner를 전달받는다. EditorFortFileDevices도 source의 같은 소유자를 작업본에 연결하므로 Template의 Exists/ReadOnly/Read가 같은 주소와 번호를 계속 쓴다. 읽기는 이전의 빈 TAFF/같은 작업 root 정책을 유지하며 원본 자산 아카이브를 등록하거나 삭제된 작업본을 원본으로 대체하지 않는다. cpppj 전용 장부의 SiblingNamespace는 독립 소유자를 사용해 장부 I/O를 원문 고리 소모에 합산하지 않는다. 경로 보호·Unicode/W CRT와 기존 쓰기 정책은 유지한다. 이번에는 일반 쓰기/Template writer/권한 변경/삭제의 다른 경로 소비자를 새로 연결하지 않았다.

수정 전737제품에서 실제 장치의 Exists/ReadOnly/Read는 모두 성공했지만 같은 source의 번호가 전진하지 않아 **1검사/3단언 실패**였다. 연결 후 새4개·기존47개 **집중51개/실패0**이다. 실제 Unicode 작업본의 성공한 세 조회 각1칸, 누락 Exists1/권한진단1/같은 작업 VFS 읽기 실패2칸, 10회 뒤 빌린 뷰 덮어쓰기, 장부 소유자 분리, 보호 경로의 I/O 전 거부를 확인했다. 실제 CampaignTemplateStorage의 서명/35섹션 Load는 readonly/read 두 칸 뒤 제목 경계에 도달했다.

기존 원문 재생은 Device32·작성기352·일반 임시 문자열204·저장 파일 CRT263·읽기정책1024·TAFF등록27의 **1902행**이다. 새 Native/fixture0이며 전체 Template/FileSpec ABI·모든 파일 실패/전체 픽셀 근거로 확대하지 않는다. 기존 Edit 파일/권한/저장 tail·Unicode/목록/원본 미복원 회귀도 통과했다. Release 경고/오류0·설정CLI1024/종료0이며 새 GUI는 없다.

EXE SHA-256은 `56d190464315a86f43bfc2418c769386f19321550b330c097971dd7a13ce0340`이다. 보호2652개(AGENTS1·원본1394·CD425·fixture832)는 모두 불변이고 새fixture0이다. [통합 보고서](../../cpppj/editor-file-ring-20261102.json), 원시는 `extracted/cpp-editor-file-ring-738/`다.

사용자 새 지시의 **다음 우선 작업**은 게임 전용/테스트 전용/전체 빌드 선택과 루트 README.md·README.en.md 컴파일 안내 갱신이다. 733의5차/플랫폼 요구와 미구현 상태, 전체1~8차 목표는 유지한다. 새 GUI/전체34/전체 C++/ZIP·포획/희생·특정 적 공격/승리·공중3종 전용·MP/통신 검사는 추가하지 않았다.
