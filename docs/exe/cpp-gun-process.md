# 실제 GunProcess 상태·전송·Kernel 연결

마지막 디컴파일 수행 PC는 **VM-W11-CODEX, 2026-10-13**이다. AGENTS.md와 두 LEFT_JOBS를 읽고 같은 PC의 정밀 프로젝트를 재사용했다. 원본/복사본 게임·GUI·OS 진입·보호 파일 변경·커밋·푸시는 없다.

## 복원 몸체

`RawGunProcess.h/cpp`는 패치 `0046a1a0` / CD `00434090`의 실제 `new(0x38)` 필드 초기화와 내부 Baseprocess `+0x28`의 상태 기계를 구현한다. `+8` dirty byte, `+0x10` double 다음 시각, `+0x18` WORD 표적, `+0x1c` DWORD 단계, `+0x20` group, `+0x2c/+0x30` Form/Parent DWORD만 원본 폭으로 쓰며 padding은 보존한다.

바깥 가상 표 `0050b668` / CD `00501118`는 수신 observer와 발사 함수의 두 항목이다. 실제 Kernel `RunFrame +0x18`은 안쪽 가상 표 `0050b648` / CD `005010f8`에서 `0046b830` / CD `004342f0`을 가리킨다. 바깥 표를 Kernel 가상 표로 읽지 않는다.

프레임은 부모 group/abstract/astral 검사, buried/fixed/disabled 반환, boss의 무효·free·dead·동맹 표적 해제, 시간 도달, 단계0~10의 한 단계 진행 순서를 유지한다. 1단계 검색의 mode 대상은 **현재 부모 Gun SID**다. idle 검색 실패는 실제 float `0.4f`와 `[0,0.2f]` 난수, 현재 double 게임 시각을 합쳐 기록한다. 시간 비교는 디컴파일 C가 아니라 실제 x87 분기를 따른다. 패치 `fnstsw / test ah,1`은 `!(time <= now)`이면 반환하며 NaN도 반환한다. CD `test ah,0x41`은 `now < time`일 때 반환하므로 unordered는 진행한다. 최초 C++ 집중의 두 판본6개 CHECK 실패를 이 차이로 교정했고 초기 로그를 보존했다.

표적 해제/교체·현재 애니메이션 반복 조회·Kill `0x40`·사거리 갱신·현재 Form 재전송을 명시 경계로 실행한다. CD의 stop helper까지 따라가면 표적 교체의 애니메이션 조회도 총 세 번이다. 실제 Kernel에 해당 animation이 없으면 kill 경계는 원본 `FromForm`의 null 반환을 그대로 유지해야 한다. 표적 삭제 notify는 clear 재전송 뒤 부모가 dead가 아니면 **다시 한 번** 재전송한다.

발사 `0046bd20` / CD `00434790`은 차단 검사, 현재 부모 descriptor의 8DWORD, 현재 표적 유효성, **발사 가능 판정** 경계 반환, 가능 상태2/불가 idle, 현재 부모 타입 `+0x4c` float 간격과 현재 double 시각의 cooldown을 복원한다. 이 몸체의 `shoot` API 이름으로 실제 투사체 생성까지 완료했다고 해석하지 않는다. 표적 검색의 각 후보도 같은 `0046af80→0046ac00` 판정을 사용한다. 패치 디컴파일 C의 시각 float 변환은 실제 `fld dword / fadd qword`에 없으므로 적용하지 않는다. 수신 observer는 parent 변경·dirty8·상태3/6가 모두 맞을 때만 현재 animation을 두 번 조회해 중지한다.

## 실제 ProcessHost 연결

`CreateGunProcess(host,logic,parent,34)`는 생성자 필드 준비 뒤 **flags0**으로 같은 풀의 실제 Host/Kernel에 부착한다. 로컬 flag로 서버 전송을 우회하지 않는다. 초기 생성자 독립 관찰은 `Construct(fields,parent)`를 사용해 raw padding까지 비교한다.

원본 서버 ProcessForm 부착 `004b1440` / CD `004af380`은 내부 writer `+8` → 양수 길이의 공유 버퍼 송신 → flags `0x40` 추가 → 부모 체인/Pop 순서다. Host의 `ProcessFormTransmission.writeData`에서 실제 `GunProcess::WritePacket(buffer,parentChanged)`를 호출하고, `transmit(length)`에서 같은 버퍼를 송신해야 한다. OnAttached보다 writer가 먼저 실행되므로 WritePacket은 실제 base Form/Parent를 raw 필드에 먼저 동기화한다.

`WriteData/WriteProcess`는 protocol1/message `0x3a`, 길이 WORD, Form DWORD, dirty byte, 선택 부모 DWORD 및 시간8/표적2/단계4/group4바이트를 원본 순서로 기록한 뒤 dirty를0으로 소비한다. force는 dirty `0xff`이며 부모 첨부 여부는 인자에 따른다. 이미 dirty1이어도 부모 인자가0이면 부모 payload를 새로 쓰지 않는다. 부족한 호스트 버퍼는 쓰기 전에 거부한다.

표적 변경의 `repopForm` 경계는 `host.FromForm(fields.Form())`의 실제 GunProcess를 찾아 `host.RefreshForm(process)`에 연결한다. 실제 Refresh는 현재 raw 부모 → Unpop0 → Attach0이며 OnAttached를 다시 부르지 않는다. Kernel의 Form/PID/소유권과 raw Gun 상태의 수명은 같은 객체가 가진다.

## 독립 원본 관찰과 검사

본체 manifest `gunprocess-functions.json` 패치23/CD25는 중단 전 성공 산출물을 재사용했다. 새 `gunprocess-helper-functions.json` 패치20/CD6도 순차 readOnly export Exit0이다. `extracted/gunprocess*/`의 각 함수 범위와 PE를 제한 Unicorn x86에 매핑하며 스택/raw/packet 밖 쓰기와 허용 함수 밖 실행을 거부한다.

`tools/decomp_gunprocess_oracle.py`의 최종 독립 관찰은 **2,996개 = 각 판본1,498개**, 정상 반환2,996개·원본 assert0·SHA/입력 격자 감사 통과다. 생성자·모든 프레임 단계·boss/server/fort/friendly·차단·무효/동맹 표적·시간/NaN·해제/교체·Notify·Fire·Attached·observer·모든 dirty 하위5비트/force/include/부모 여부·padding/stale packet을 비교한다. raw 타입 입력이 head 준비 중 덮이던 초기 oracle 오류를 실제 group getter로 발견하고 head WORD만 기록하도록 교정했다. 초기2,348개와 smoke/재실행을 최종 관찰에 중복 합산하지 않는다. 초기 실패 로그는 보존한다.

`GunProcessTests.cpp`의 native 판본별2검사, 미연결 효과/짧은 buffer 검사, 외부 호출 없는 Clear의42개 기존 native 입력을 빈 경계로 비교하는 검사, 실제 Host flags0/OnAttached 이전 writer/Kernel Frame/Refresh 단계 보존/Kill 수명 검사 **총5개가 FP 교정 후 모두 통과**했다. root 최종 공유 Release는 경고/오류0, 전체 CTest는 **928개·실패0·88.83초**다. 근거 파일은 `cpppj/gunprocess.json`, fixture는 `cpppj/tests/fixtures/gunprocess-x86.tsv`, 최초/교정 집중 로그는 `extracted/campaign-process-focused-GunProcess*.log`다. 자연 분기42개는 기존2,996 관찰의 재사용이며 추가 native로 합산하지 않는다.

## 삭제 몸체의 정적 감사와 수명 검사 한계

root 추가 감사 요청으로 별도 `gunlifetimeaudit-functions.json`의 두 판본5/5개를 readOnly로 추출했다. `tools/analyze_gun_lifetime.py`는 실제 PE 가상 표와 빈 통지 `ret4`, 기반 소멸자 poison bytes 및 원문 SHA를 확인하며 **추가 native 관찰0개**다. 근거는 `cpppj/recovery-gun-lifetime-audit.json`과 `extracted/gunlifetimeaudit*/`다.

패치 inner `+0xc/+0x10`은 모두 `0040b380`의 빈 몸체이며 CD는 각각 `00434a60` / `00413a10`의 빈 몸체다. 따라서 Gun 전용 OnFormDestroy/OnParentDying에서 표적 감시·애니메이션을 지우는 별도 본문은 없다. 패치 inner 소멸자 `0046a080`은 실제 `sub ecx,0x28`으로 outer 시작점을 얻어 `00443c00→00443c20→0041bd00`을 실행하며, 기반 vtable을 쓰고 Form/Parent DWORD를 **-12345**로 오염시킨 뒤 선택적으로 outer 메모리를 해제한다. CD `00436dc0→00436e1f→0048f570`도 같은 기반 poison/선택 free이며 Gun 감시·animation 삭제 함수를 호출하지 않는다. outer 첫 항목은 observer이므로 소멸자로 읽지 않는다.

C++ Gun 어댑터의 실제 Host Kill 검사는 Form 체인 해제·Kernel 소유권·삭제 깊이 종료를 검증한다. **native deleting destructor의 poison 쓰기/CRT free·표적 감시 종속의 공통 destroy 통지·현재 animation 프로세스 전체 수명까지 대조한 검사가 아니다.** 이 범위를2,996 native/C++ 완료 몸체에 새로 포함하지 않으며, 외부 감시/animation 및 부모 삭제 경계의 전체 수명은 후속 통합 대조가 필요하다.

## 계속 복원할 실제 경계

이 모듈은 실제 호출할 필수 효과 누락 시 예외를 유지하며 외부 호출 없는 Clear 자연 분기는 그대로 허용한다. 검색·animation·발사 가능 판정에 noop를 넣거나 타입 지원 목록만 늘리지 않는다. 현재 body/독립 관찰의 명시 외부 경계는 실제 타깃 검색, center, animation 생성, 감시 종속 생성/해제, range/mode, 실제 발사 가능 판정 및 전송 큐다. 실제 투사체 생성은 이 body의 관찰 범위 밖이다.

- 타깃 검색 `0046b6f0`은 Archer `0046b400` / Cannon `0046b0f0`으로 분기한다. Cannon은 네 방향의 전방 finder와 중심 거리, Archer는 원형 finder/동맹/우선 점수를 사용한다. 후보 선택·좌표/각도·통행 판정 전체를 같은 raw pool/hash/spot에 복원해야 한다.
- shot `0046b070` / CD `00434c30`은 썬 캐논만 현재 목표 중심→실제 방향→`00466df0`/CD `004a1050` 각도 허용을 검사한 뒤 일반 shot 판정 `0046af80`/CD `00434b20`으로 간다. 패치 각도0~23은 `00540550` 표의 `0x8000`, 특수24~27은 방향4/0/6/2 일치다. 이 표를 지원 타입 목록으로 대신하지 않는다.
- `0046ac00`은 group0의 `0046aaf0`/group1의 `00469320`으로 발사 가능 판정을 분배하며 투사체를 만들지 않는다. 실제 투사체/발사 효과는 원소별 발사 animation 생성자와 Frames에서 이어 찾아야 한다. stage2/5/8에 필요한 생성자와 표적 도착 통지를 실제 Kernel에 묶어야 프레임이 다음 단계로 진행한다. 완성 전 actual mission 공격을 성공으로 표시할 수 없다.
- 타입 `+0x74`는 **impactSound**다. 이미 복원된 Vortex 후처리와 동일하게 실제 Sound 경계로 연결해야 하며 원소 번호로 해석하지 않는다. Gun group은 `+0x9c`, theme은 `+0x98`, 발사 간격은 `+0x4c`이다. 현재 `RiftTypeRecord`에 발사 간격/소리 token이 없으므로 실제 타입 자산과 Sound 경계를 상위 구성에서 공급해야 한다.

캠페인1-1 실제 플레이/전투/건설/승패 동일성 완료를 이 단위만으로 주장하지 않는다.
