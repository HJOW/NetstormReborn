# AMD FidelityFX Super Resolution 1.0 (FSR 1) 서드파티 자료

NetStorm 11.01의 7차 요구(옵션에서 선택하는 FSR 1.0 업스케일링)에 쓰는 AMD 공식 헤더 두 개와 라이선스다. 코드를 고치지 않고 **내려받은 그대로** 둔다.

| 파일 | 내용 | SHA-256 |
| --- | --- | --- |
| `ffx_a.h` | FidelityFX 공통 CPU/GPU 이식 헤더(HLSL/GLSL/C 겸용) | `f60e2722fcd13989523b9164d776ab382b3692791767f3bf8bb19967f763f3fb` |
| `ffx_fsr1.h` | FSR 1.0 EASU(공간 업스케일)·RCAS(선명도) 알고리즘, v1.20210629 | `93c3922362ea7fc99cbcc698ca30c98de4f8c246d1fbb0b09e015ddef38ce3a5` |
| `LICENSE.txt` | MIT 라이선스(Copyright (c) 2021 Advanced Micro Devices, Inc.) | `db089274ce766da70f5b7d791029c3486f9f9e27c8c79c652689603d3192e802` |

- 출처: <https://github.com/GPUOpen-Effects/FidelityFX-FSR> 의 `ffx-fsr/ffx_a.h`, `ffx-fsr/ffx_fsr1.h`, `license.txt`(master, 커밋 `a21ffb8f6c13233ba336352bdff293894c706575`, 2021-08-20).
- 사용 방식: 두 헤더를 **두 곳**에서 쓴다. (1) C++ 쪽 `client/FsrConstants.cpp`가 `A_CPU`로 포함해 EASU/RCAS 상수(`FsrEasuCon`/`FsrRcasCon`)를 계산한다. (2) GPU 쪽 `client/GpuPresenter.cpp`가 같은 헤더 파일 바이트를 실행 파일에 넣어 두었다가 `D3DCompile`의 `#include` 처리기로 HLSL(`A_GPU`/`A_HLSL`)에 공급한다. 알고리즘 자체는 AMD 원본이며 NetStorm은 입력 읽기 콜백과 출력 위치만 연결한다.
- 라이선스: MIT. 소스/바이너리 배포에 `LICENSE.txt`를 함께 둔다(배포 스크립트가 `third-party\fsr1\`로 복사한다).
