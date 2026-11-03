# 실제 SHP의 직접 매핑과 셰이프 저장소 수명

단위791은790(`d8a64a9b`)의 직접 매핑 부모를 실제10.78 자산 로딩에 연결한다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재 호스트와 같다. 원본·추출물·AGENTS.md·기존 fixture는 변경하지 않는다.

## 이름 선택과 직접 매핑

실제 Patch1078 시작은 같은 이름 고리에(-2,빈문자열,-2,d,-3,_shapes.shp)를 작성한다. 기존0041ad10의 선택 부모 ResolveExternalFilePath를 현재 VFS/동일 FileSpec 장치와 연결한다. 상대 이름은 기본disk→기본TAFF→보조disk→보조TAFF다. disk는 현재 rb 열기/닫기로만 확인하고 TAFF는 metadata만 조회한다. FileSpec은 선택 뒤 직접map을 호출하기 전에 반납한다.

TAFF가 일치하면 원문처럼 원래 이름을 반환한다. 이 이름의 직접map이 실패해도 payload를 추출하거나 VFS whole read/보조 성공으로 바꾸지 않는다. 실제 파일은790의 동일 부모를 통해 CreateFileW/읽기전용 섹션/전체 view로 매핑한다. 원문 security12는 논리 필드이며64비트 W API에는 실제 SECURITY_ATTRIBUTES 크기를 사용한다. 같은 드라이브 상대 경로/Unicode root는 기존 chdir 대신 명시 게임root로 해석한다.

GameAssets의 새 선택 callback은 실제 Patch1078에서만 사용한다. CLI/독립 GameAssets/CD의 기본 입구는 기존 VFS 값형 읽기를 유지한다. 이 연결을0049ebb0의 모든 타입 초기화/진단 부모 완료나 CD 새 Native 관측으로 확대하지 않는다.

## 불변 backing과 현대 자원 반납

ShapeDatabase의 기존 vector 입구는 자신의 불변 vector 소유권을 유지하고, 새 view 입구는 같은 외부 소유자와 읽기전용 span을 받는다. 복사는 불변 backing을 공유하며 프레임 표는 값으로 복사한다. 이동은 원본 span을 비운다. 기존 파서/범위/메타데이터/RLE 본문은 공유하며 map에서 vector로 payload를 복사하지 않는다. 실제 GameAssets callback의 반환 전 주소와 최종 셰이프 주소도 비교한다.

실제 파일 핸들/섹션/view는 마지막 ShapeDatabase 소유까지 유지한다. 마지막 반납은 view→mapping→file 순서다. 원문의004dd580이 누락한 반납과 예외/파서 실패 후 OS 자원 회수는 현대 RAII 호환 경계로 명시한다. 파서의 실제 span 길이를 위해 원문 세 매핑 호출이 끝난 뒤 GetFileSizeEx를 추가한다. 이는 원문 호출/전체SEH와 동일한 OS 실패 관측이라는 주장이 아니다.

789에서 줄끝이 바뀌었던 VFX 두 파일은 변경 없는 줄을788의 원래 바이트로 복구했다. 이 복구와 셰이프 소유/파서 분리의 논리 변경을 구별한다. 새 원문 기대값이나 Native fixture는 만들지 않는다. 직접 부모의72입력 관측은790을 재사용한다.

## 검사 범위

새4개는 독립 Unicode scratch파일의 공유view/복사·이동과 마지막 반납, 열기/빈 파일 매핑/파서 실패 후 자원 회수, 기본/보조/TAFF 순서와 직접map 실패, 보호원본의 실제5,618,120BYTE 및 프레임/그림과 기본 GameAssets 대조다. 파일 공유의 쓰기 핸들·삭제 검사는 검사에서 만든 scratch파일에만 수행한다. 원본은 읽기 매핑/바이트 대조만 하며 쓰기·삭제·변경 대상으로 삼지 않는다.

기존 직접 부모·값형 파서/손상 검증·두 판본 Native 픽셀·글꼴 생성 캐시와 주소 재사용의 그림 캐시 반납 회귀를 집중 확인한다. 빌드/검사 결과와 최종 EXE 해시는 [보고서](../../cpppj/native-shape-file-20261103.json)에 기록한다. 원시 자료는 `extracted/cpp-resume-791/`에 있다. 새 게임/GUI/물리/PCM·공중3종 행동·포획/희생·특정 적 공격·승리/자연완주·멀티플레이 검사는0이다.

첫 제품 빌드는 성공했으나 기존 AssetTests를 새 집중 대상에 재사용하면서 NETSTORM_GRAPHICS_FIXTURE 정의가 첫 검사 빌드에 없어 C2065 오류1개로 실패했다. 정의와 주소 재사용 회귀를 포함한 구성으로 재빌드했으며 원본 fixture/기대 픽셀은 변경하지 않는다.

두 번째 빌드 뒤 새4개 중3개는 통과했으나 원본 GameAssets 비교는 검사 루트가 한 단계 상위를 가리켜 실패했다. 원본 경로 입력만 바로잡고 제품/fixture는 유지해 최종 집중 검사를 다시 실행했다. 초기 로그도 그대로 보존한다.

전체71대상/전체C++/전체34/ZIP은 반복하지 않는다. 다음은 Script 활성 파일 부모와 checking잔여50·나머지 비멀티/호환성이다. 전체 목표는 진행 중이다.

## 최종 확인값

최종 제품/집중 대상 Release 빌드는 경고/오류0이다. 새4개와 기존13개, 총17개가 실패0이다. 원본 파서 본문은 줄끝 외 동일하며790 순수 부모는 변경하지 않았다. 보호원본5,618,120BYTE/프레임/픽셀은읽기만비교했고쓰기공유/반납/삭제는scratch파일에서만확인했다.

EXE SHA256은 `f8f2a0d6c74b0b8ec4774f810abfc4dd84c6d933ffa2a374dcf167564cde1d56`이다. 기존보호2724개모두불변·새fixture0/fixture904이며다음기준은 `extracted/cpp-resume-791/protected-after.json`이다. [보고서](../../cpppj/native-shape-file-20261103.json)와원시폴더에최초구성/검사입력실패와최종통과를구별했다. 새게임/MP/전체71대상/전체C++/34경로/ZIP/지정플레이/물리/PCM은추가하지않았다.
