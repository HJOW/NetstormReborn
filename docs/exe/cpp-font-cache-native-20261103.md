# 실제 FontStore의 현재 파일·생성 캐시 저장

단위789는788(`b8deab82`)의 순차 File 부모를 실제 FontStore에 연결한다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재 호스트와 같다. 원본·추출물·AGENTS.md·기존 fixture는 수정하지 않는다.

## 실제 연결

FontStore는 같은 이름 고리의(-2,d,-3,literal)로 원문 CHFNT 경로를 작성한다. 부재/서명·버전 실패는 기존 GDI 생성으로 내려가고, 같은 생성 글리프를 설치한 뒤004a3090의 순차 writer로 저장한다. 이미 보관한 VFX 블록을 재인코딩하거나 새 그림으로 대체하지 않는다. BitmapFont.CacheBytes와 ShapeDatabase.Bytes는 현재 표/블록을 읽기만 한다. 기존 엄격한 값형 BitmapFont 검사·슬롯/스타일/원문18조합·Rebuild/현재얼굴 쓰기 순서는 유지한다.

현재 reader는 등록 TAFF metadata와 backing FILE을 사용한다. 등록 snapshot Read/TryRead는 기존 계약을 유지한다. 순차 읽기/쓰기와 실패 접두는 [788](cpp-font-cache-file-20261103.md)의100입력 고정 관측을 재사용하며 이번에 새 CPU 원문 관측을 합산하지 않는다.

## ACP·UTF-8와 파일 보호

raw ACP 글꼴 이름은 같은 원문 고리와 TAFF 키에 남긴다. BaseFileSystem의 새 OpenWithDiskName/ProbeWithDiskName은 해당 디스크 query만 변환하고 Unicode 설치root를 다시 ACP로 해독하지 않는다. raw고리는 변환 전에 먼저 작성한다. 기본 Open/Probe는 변환 장치를 주지 않으므로 기존 소비자 계약을 유지한다. Probe의 TAFF 우선과 Open의 디스크 우선도 구별한다. 잘린 ACP의 변환 실패는 디스크 실패이며 raw TAFF fallback을 유지한다.

writer의 OS 경계는 canonical 경로로 개발 저장소 originals/originalCD 내부를 차단한다. 기존 원본을 쓰기 테스트 대상으로 삼지 않는다. 경로 판정 실패도 쓰기를 막는다. 보호원본·읽기전용 TAFF fallback·쓰기불가 설치에서 캐시 저장은false이며 이미 생성/설치한 메모리 글꼴은 유지한다. 이 선택은 원본 파일 보호와 Windows 설치 권한 호환을 위한 현대 경계다. 실제 일반 사용자 권한 검증과 모든 자동 파일의 Saved Games 이관은5차에 남는다.

## 검증 범위와 한계

새4개는 등록 뒤 backing 변경/기존 snapshot 보존, 원문 마지막서명BYTE/후행 BYTE 처리와 기존 strict값형 검사 구별, 실제 GDI 생성→전체 표/글리프 저장→새 FontStore 재사용, 현재ACP raw아카이브/Unicode디스크의 충돌, 쓰기불가/보호 경로를 검사한다. GDI는 메모리 DC이며 게임/별도창·PCM을 실행하지 않는다. 기존 ACP/재생성/주소재사용 그리기·원문 순차 부모·기본 File열기/Probe 회귀를 함께 확인한다.

첫 빌드에서 새 존재조회root와 기존 내부root 이름이 겹쳐 C4456 경고1개가 발생했다. 게임은 빌드됐지만 새 검사 파일의 경로 함수 네임스페이스 누락4곳에서 C2653/C3861 오류8개가 나 검사 빌드는 실패했다. 지역 이름과 검사 파일 별칭을 보완했으며 원본 관측값은 바꾸지 않았다. 최종 빌드와 집중 검사 결과는 아래에 기록했다. 기존 기본 File열기 검사에는 원본1,364입력, Probe에는640입력 고정 관측이 포함된다. 좁은 집중 검사를 전체71대상·전체C++·전체34/ZIP 통과로 합산하지 않는다.

호스트는 글리프당16MiB·전체64MiB 물리 상한을 두고 미정 할당/암호 작업을 명시0으로 시드한다. 실제 짧은 파일의0xffffffff 크기표를 할당 전에 거부하는 검사도 포함한다. 미완성Open의 OS 자원은 RAII로 추가 회수한다. 실제 FontStore의 실패한 임시 raw저장소는 현대 소유권으로 회수하며 원문의 실패 후 정적 포인터/힙 배치 자체를 보장하지 않는다. 원문이 허용하는 미정 mallocNULL/손상 크기/SEH 전체 ABI와 현재 BitmapFont의 검증 경계는 구별한다. 실제 fwrite 반환은 원문 부모처럼 검사하지 않으므로 반환true를 파일시스템 내구성 보장으로 확대하지 않는다.

새 게임/GUI/PCM/물리·공중3종 행동·포획/희생·특정 적 공격·승리/자연완주·멀티플레이 검사는0이다. 다음은 SHP 직접 map/view와 Script 활성 부모, checking 잔여50·나머지 비멀티/호환성이다. 전체 목표는 진행 중이다.

## 최종 확인값

최종 Release 네 대상 빌드는 경고/오류0이며 새4개와 기존22개, 총26개가 실패0이다. 새 Native 관측은 없고788의100입력·기본 File열기1364입력·Probe640입력과 ACP/실제 FontStore 회귀를 재사용했다. GDI 생성 본문과 기존 두 파일 부모·BitmapFont/ShapeDatabase 소유 데이터는 변경하지 않았다.

EXE SHA256은 `120d361925072e3a2f4a00bf7bfa140da8e29c1228bdf2b466d0406b12399364`이다. 기존 보호2723개 모두 불변·새fixture0/fixture903이며 다음 기준은 `extracted/cpp-resume-789/protected-after.json`이다. [보고서](../../cpppj/font-cache-native-20261103.json)와 원시 폴더를 따른다. 전체C++/71대상/34경로/ZIP·게임/MP/지정 플레이/물리/PCM은 추가하지 않았다.
