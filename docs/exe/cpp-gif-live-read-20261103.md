# 781 — GIF 현재 FILE·키와 Decode 수명

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 산출물은 수정하지 않았다. 783 커밋 `35de22d2` 위에 고정 준비본781을 최소 병합했다. 기존 일반41baa0 reader와 구별해 원문 `004dae60`의 정확한 길이·입력0·Decode 뒤 File 반납을 복원했다.

key/keyLength0 → 필수 Open(name,0,1) → isOpen 진단 → 현재 길이만큼 malloc/0fill → 같은 File의 Read(-1) → 원문407171의 첫 descriptor 치수 → width/height 기록 → bitmap 할당 → Decode → 입력free → File/키 소멸 순서다. 최소4·추가NUL·새 Probe·짧은 fread 재시도를 넣지 않는다. 평문 부분 읽기는 입력0 tail을 남기고 암호 부분 읽기는 기존4096BYTE 작업 공간의 미작성 값도 XOR/복사한다. host 작업 공간0 seed를 원문 미정 스택과 같다고 주장하지 않는다.

Kencloud와 Background의 실제 두 소비자만 현재 VFS root·동일 고리·TAFF metadata와 backing FILE에 연결했다. Kencloud의 Decode 앞 블록 할당·같은 Memory·512×512 판정/반납·기존 난수/캐시는 유지한다. Background는 이전 bitmap 반납/0 뒤 파일을 읽고 치수를 Decode 전에 쓰며 새 bitmap 공개/geometry는 정상 복귀 뒤다. 실패한 선행 효과를 되돌리거나 빈 정상 이미지로 바꾸지 않는다. 기본 snapshot Read·CD/host 값형 GIF·기존 decoder는 유지한다.

준비본의 검사 헤더 누락을 `o/BaseFile.h` 직접 include로 교정했고, 기존부터 필수였던 ConfigInterface reader 인수를 `ConfigInterface::FileReader{}`로 명시했다. 새 reader나 가짜 Config 성공값은 설치하지 않았다. 이 두 교정은 실행 전 반영했고 실제 컴파일 실패는 없었다.

원문56입력/두 정밀도112회가 준비본 최초·재관측과 이번 재관측에서 일치했다. 실제 LZW와 행 출력을 실행한 입력은8개, 한 정밀도의 행 출력은12행이다. 나머지 Decode와 Open/CRT/malloc/free는 명시 장치다. validDims+invalidLZW의 예외는 기존 host decoder 경계이며 원문 LZW의 throw를 증명한 것이 아니다. 원문 SEH funclet은 File만 정리하고 host RAII는 입력/임시 bitmap·미완성 Open의 OS 자원을 추가 회수한다.

새4개와 기존 Kencloud8/Background8, 최종 **20개/실패0**이다. 전체 보호 자산의 독립 Native 픽셀, descriptor/interlace, 현재 root·고리·불완전 decode·공통 창/입력/클립과 누른 stone texture까지 기존 기대가 유지됐다. 게임/파일 집중 대상만 Release 빌드했으며 전체 C++/71대상/34경로/ZIP·새 게임/GUI/PCM·공중3종·포획/희생·특정 적 공격·승리/자연 완주·MP 검사는 실행하지 않았다.

직접 caller5곳 중 Background/Kencloud 두 곳이 이번 범위이며 title0/6/7의 세 소비자는 다음782다. 모든 GIF/제품 이미지 수명 완료로 합산하지 않는다. [보고서](../../cpppj/gif-live-read-20261103.json)와 `extracted/cpp-resume-781/`에 결과와 해시를 남긴다.

Release 경고/오류0이다. EXE SHA-256은 `72fae232a2f00ae0b3c397f4b9eb87fda6f5e7083b006da0f76c1d42be6bfc40`, 기존 보호2718개 불변·현재2719개/fixture899이며 다음 기준은 `extracted/cpp-resume-781/protected-after.json`이다.
