# 효과음의 현재 보조 디렉터리

**2026-11-06 단위965 분류 정정:** 아래 당시 잔여42cd10 seek부모는 원격 CD 파일4BYTE확인/논리수신자11·응답80으로 확인했다. 일반 비멀티 FILE 복원 목록에서 제외하고 폐기 검증 정책에 남긴다. [965 기록](cpp-retired-cd-validation-20261106.md)을 따른다. 나머지785본문/직접74호출 목록은 당시 자료로 보존한다.


단위785는784(`83eb01e6`) 뒤에 적용한다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 같은 호스트에서 정적 원문·현재 소스를 대조했다. AGENTS.md·보호 원본·기존 fixture·추출 산출물은 바꾸지 않는다.

## 활성 공백과 연결

원문 일반 버튼249a0→4a9cb0→4a9550→4a9170은 기본4패턴 뒤 보조4패턴을 찾는다. 각 보조 패턴에서41a550의 현재 root를 읽는다. 시작441790은 이미 보조 root를 같은 VFS에 기록하지만, 기존 ClientAudio는 primary만 받은 디스크 resolver를 만들었으므로 보조/CD 폴더에만 있는 효과음을 찾을 수 없었다. 실제 PCM 재생으로 재현한 결과와 구별한다.

BaseFileSystem::SecondaryDirectory는 같은 현재 경로를 부수효과 없이 빌린다. 실제 Client는 기본 검색에서 이를 미리 읽지 않고 각 alternate 패턴 조회에서만 현재 경로를 복사해 기존 MakeDiskSoundResolver에 넘긴다. ClientAudio의 기존 seed/shared GameRandom 생성자는 기본 디스크 계약을 유지하고 실제 Client용 overload만 해당 resolver를 장치에 값으로 보관한다. audio의 명시 반납은 files보다 먼저이므로 callback 대상 수명이 유지된다.

기본/보조·언어/일반·감쇠 검색 순서와 WAVE 읽기·버퍼·음소거·재생·음향 난수·음악 선택은 유지한다. 이미 성공 적재한 SoundList 캐시를 root 변경 때문에 강제로 재적재하지 않는다. 일반 Client 시작의 primary는 기존 선택값을 유지하며 새 보조 조회만 늦게 읽는다. 실제 CD10.72 시작과10.78 경로는 별도 판본이다.

## 정적 소비자 분류와 후속

QuickHelp767·DataManager769·Config771·TYPE777·GIF781/782와 Help stream766·ObjectDump757/763·Editor669의 이미 연결한 reader/writer를 다시 복원하지 않는다. snapshot/CLI/CD/Editor 작업본 namespace도 유지한다. 관련 분류와 직접 호출 목록을 보고서에 함께 보관한다.

| 남은 부모 | 판단 |
| --- | --- |
| palette4a4850 | 직접776/1024BYTE 읽기·현재 팔레트 부분 효과·후속Screen까지 파일 수명을 별도로 대조한다. GIF 연결로 완료 처리하지 않는다. |
| Xlat4dea00→4de710 | 현재 snapshot과 원문 현재 FILE 입력 계약을 구별한다. 버퍼/파싱 시점 차이만으로 공개 오류를 합성하지 않는다. |
| font cache4a34a0→4a3240/4a3090 | 순차 File 읽기·생성 뒤 저장 부모가 남는다.774의 인코딩/글꼴 얼굴과 구별한다. |
| SHP49ebb0→4dd580 | 직접 OS mapping/view 경계다. generic whole read로 바꾸지 않는다. |
| Uber Script | 필수 읽기 뒤 성공 캐시이며 원문 caller·optional 계약을 확정한 뒤 판단한다. |
| rank44fa00→4b54e0 | 일반battle/!missionPost/playerKnown의재접속저장 전체 부모로MP후속에 둔다. 중간만 성공 처리하지 않는다. |

직접 호출 목록은7callee의 텍스트 호출74곳이다. write41a910의54곳/12부모, key41a620의2곳/2부모, seek41a700의2곳/2부모와font writer5·palette6·SHP2·Xlat3곳이다. 간접 호출 부재나 모든 consumer 완료를 증명하는 목록이 아니다. ColorMapCache·Config 저장·QuickHelp/BMP/ObjectDump의 기존 몸체를 새 공백으로 중복 계수하지 않는다.42cd10의 seek 상위 입력은 별도 확인 대상이다.

## 검증 범위

제품 변경은 준비된4파일에 집중하며, 기존 SoundDeviceTests를 기존 ScriptBindings 대상에 추가한CMake2행만 별도로 포함한다. 새 C++ 검사·실행 대상·fixture·Native·게임 실행은 추가하지 않는다. 기존 디스크/검색/무장치와 ClientAudio 회귀를 선택한다. 실제 PCM·CD 삽입·새MP·공중3종·공격/포획/희생/승리 재현은 하지 않는다. 현재 보조 root를 읽는 실제 Client callback 자체의 실행을 기존 helper 검사 통과로 대신하지 않는다.

## 최종 확인값

게임/ScriptBindings Release 빌드는 경고·오류0이다. 기존ClientAudio10개와SoundDevice의검색우선순위·실제디스크·무장치3개, 총13개실패0이다. 새검사/실행대상/fixture·Native·게임/PCM/실제CD·MP검사0이다. 기존생성자/난수·장치없는수명회귀와실제Client의각보조callback실행확인을구별한다.

EXE SHA256은 `2b96954c8f1b38c65ce52f07be5297bc8a1d419683a46d873dc0ac34357f5ebe`이며 기존보호2720개/fixture900은모두불변이다. [보고서](../../cpppj/secondary-sound-root-20261103.json)·[분류](../../cpppj/nonmp-file-consumers-20261103.json)·[직접74호출](../../cpppj/nonmp-file-direct-callers-20261103.json)과Git제외 `extracted/cpp-resume-785/`를 따른다. 분류JSON의confirmedGap은준비당시적용전공백이다. 다음786은팔레트현재File/부분쓰기·적용/반납이며전체목표는진행중이다.
