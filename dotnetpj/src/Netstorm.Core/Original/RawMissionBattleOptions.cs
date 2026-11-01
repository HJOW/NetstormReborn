namespace Netstorm.Core.Original;

/// <summary>원본 미션 전투 옵션의 초기화다. 근거: cpppj RawMissionBattleOptions.</summary>
public static class RawMissionBattleOptions
{
    private static ReadOnlySpan<byte> InitialOptions => [0, 2, 2, 0, 3, 0, 2, 1, 1, 0, 0, 1, 0, 1, 1, 2, 2, 1, 2, 2, 0, 0];

    /// <summary>예약된 40바이트를 보존하며 10.78의 현재 87바이트 옵션을 초기화한다.</summary>
    public static void Reset(OriginalEdition edition, Span<byte> options)
    {
        if (edition != OriginalEdition.Patch1078 || options.Length != 87) throw new ArgumentException("미션 옵션 초기화의 PATCH87BYTE 계약 오류");
        options[..23].Clear(); options[63..].Clear();
        for (int i = 0; i < 8; i++) options[71 + i] = (byte)i;
        InitialOptions.CopyTo(options);
    }
}
