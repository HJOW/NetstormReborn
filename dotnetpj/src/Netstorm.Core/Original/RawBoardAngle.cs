// 방향 계산과 각도 상태가 같은 현재 Client 진단을 소비하는 원문 부품을 공유한다.
namespace Netstorm.Core.Original;

// 보드 각도 모듈을 담는 네임스페이스다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.

// 자체 검사 사본 대신 같은 소유자 뷰와 원문 식·파일·행의 장치를 전달하는 진단 묶음이다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.
public sealed class BoardAngleDiagnostics
{
    /// <summary>현재 전체 DWORD를 읽는 비소유 검사 뷰다. CD는 읽지 않는다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();

    /// <summary>선택한 진단이 복귀하면 원문은 반드시 현재 끝각 상수를 반환하는 진단 콜백이다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 진단 묶음의 얕은 사본을 만든다. 검사 뷰 참조를 공유한다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    /// <returns>복사된 진단 묶음이다.</returns>
    public BoardAngleDiagnostics Clone() => new()
    {
        Checking = Checking,
        Assertion = Assertion,
    };
}

// 원문 CRT 나머지 뒤 단정밀도 저장·현재 검사·진단 복귀와 판본별 반환을 유지하는 각도 처리기다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.
public static class RawBoardAngle
{
    /// <summary>끝각 상수 359.9999의 실제 단정밀도 값이다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    public const float EndAngle = 359.9999f;

    /// <summary>CD 끝각 판정의 실제 비트 상한이다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    private const int CdBitsLimit = unchecked((int)0x43b3ffffu);

    /// <summary>원문 진단 식이다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    private const string AngleExpression = "angle < 361.0f";

    /// <summary>원문 진단 파일이다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    private const string AngleFile = "\\Ns\\O\\Boardcoord.cpp";

    /// <summary>원문 진단 행이다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    private const uint AngleLine = 0xb3u;

    /// <summary>원문은 실수 인자를 CRT 나머지 연산으로 계산한 뒤 두 판본 모두 359.9999로 끝각을 처리하되 중간 실수 저장이 다르다. 2026-11-07 F2d 보드 각도 이식에서 추가됐다.</summary>
    /// <param name="angle">정규화할 각도다.</param>
    /// <param name="edition">원본 판본이다.</param>
    /// <param name="diagnostics">현재 검사·진단 묶음이다. null이면 독립 기본값을 쓴다.</param>
    /// <param name="modulus">원문의 별도 배정밀도 전역이다. 일반 소비자는 실제 360을 쓴다.</param>
    /// <returns>정규화된 각도다.</returns>
    public static double Normalize(float angle, OriginalEdition edition, BoardAngleDiagnostics? diagnostics = null, double modulus = 360.0)
    {
        // 실제 두 CRT의 0 입력 꼬리는 부호와 무관하게 +0을 반환한다
        if (angle == 0.0f)
        {
            return 0.0;
        }

        // C++ 나머지 연산과 같은 부호 절삭 나머지를 구한다
        double remainder = (double)angle % modulus;

        // CD는 이진 32비트 중간 저장을 거친다
        if (edition == OriginalEdition.Cd1072)
        {
            float stored = (float)remainder;
            if (remainder < 0.0)
            {
                stored = stored + 360.0f;
            }

            if (BitConverter.SingleToInt32Bits(stored) > CdBitsLimit)
            {
                stored = EndAngle;
            }

            return stored;
        }

        // 패치는 53비트 중간 저장을 유지한다
        double result = remainder < 0.0 ? remainder + 360.0 : remainder;
        if (result >= 360.0)
        {
            if (diagnostics is not null && diagnostics.Checking && result >= 361.0)
            {
                if (diagnostics.Assertion is null)
                {
                    throw new InvalidOperationException("보드 각도 원문 진단 장치 미연결");
                }

                diagnostics.Assertion(AngleExpression, AngleFile, AngleLine);
            }

            // 실제 주소의 실수 비트 43b3fffd를 반환한다
            return EndAngle;
        }

        return result;
    }
}
