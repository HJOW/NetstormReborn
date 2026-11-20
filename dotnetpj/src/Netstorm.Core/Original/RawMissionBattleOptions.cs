// 현재 전투 옵션과 실제 TYPE/사용표의 파생값을 원문 순서로 공유한다.
namespace Netstorm.Core.Original;

/// <summary>원본 미션 전투 옵션의 초기화다. 근거: cpppj RawMissionBattleOptions.</summary>
public sealed partial class RawMissionBattleOptions
{
    /// <summary>원본22개 기본 선택이며 예약40바이트를 덮지 않는다.</summary>
    private static ReadOnlySpan<byte> InitialOptions => [0, 2, 2, 0, 3, 0, 2, 1, 1, 0, 0, 1, 0, 1, 1, 2, 2, 1, 2, 2, 0, 0];

    /// <summary>PATCH는 예약40바이트를 보존하고 CD는 정확한8바이트를 원문 기본값으로 초기화한다.</summary>
    public static void Reset(OriginalEdition edition, Span<byte> options)
    {
        if (edition == OriginalEdition.Cd1072)
        {
            if (options.Length != 8) throw new ArgumentException("미션 옵션 초기화의 CD8바이트 계약 오류"); options.Clear(); ReadOnlySpan<byte> initial = [1, 2, 0, 1, 1, 0, 1]; initial.CopyTo(options); return;
        }
        if (edition != OriginalEdition.Patch1078 || options.Length != 87) throw new ArgumentException("미션 옵션 초기화의 PATCH87BYTE 계약 오류");
        options[..23].Clear(); options[63..].Clear();
        // 실제 색상0..7을71..78바이트에 설치한다.
        for (int i = 0; i < 8; i++) options[71 + i] = (byte)i;
        InitialOptions.CopyTo(options);
    }
}
