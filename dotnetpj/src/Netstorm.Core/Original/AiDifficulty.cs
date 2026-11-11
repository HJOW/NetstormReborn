// 11차 난이도 조절: 적 인공지능 설정(600바이트)에 쉬움/어려움 배율을 적용하는 순수 계산이다.
// 원본 10.78에는 난이도 개념이 없으므로 보통(기본)은 설정을 한 바이트도 바꾸지 않고, 쉬움/어려움만 클론이 추가한 배율을 곱한다.
// 2026-11-11 0.347에서 cpppj o/AiDifficulty.h/.cpp를 이식했다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

/// <summary>
/// 난이도 단계다. 값은 설정 파일(options.cfg)의 difficulty 값과 같고, 값이 없거나 범위 밖이면 보통이다(11차).
/// </summary>
/// <remarks>2026-11-11 0.347에서 cpppj <c>o/AiDifficulty.h</c>의 열거를 이식했다. 게임 규칙 계산만 두며 UI·설정 파일·세션에 의존하지 않는다.</remarks>
public enum AiDifficultyLevel : uint
{
    /// <summary>쉬움: 인공지능의 다리 건설·유닛 생성 주기가 4배로 늘고 채취 유닛 수가 절반(최소 1)으로 준다.</summary>
    Easy = 1,
    /// <summary>보통(기본값): 원본과 동일하다.</summary>
    Normal = 2,
    /// <summary>어려움: 인공지능의 다리 건설·유닛 생성 주기가 2배 빨라진다(주기가 절반).</summary>
    Hard = 3,
}

/// <summary>
/// 600바이트 인공지능 설정의 복사본에 난이도·강제 적 정리를 적용하는 순수 계산이다
/// (cpppj <c>o/AiDifficulty.h/.cpp</c>).
/// </summary>
/// <remarks>2026-11-11 0.347에서 이식했다. 입력은 바꾸지 않고 항상 복사본(또는 빈 배열)으로 돌려준다.</remarks>
public static class AiDifficulty
{
    /// <summary>설정 파일에 저장하는 난이도 키 이름이다(옵션 메뉴와 스커미시 화면이 공유한다).</summary>
    public const string DifficultyConfigKey = "difficulty";
    /// <summary>쉬움에서 주기에 곱하는 배율(느려짐)이다.</summary>
    public const float EasyCycleFactor = 4.0f;
    /// <summary>어려움에서 주기에 곱하는 배율(빨라짐)이다.</summary>
    public const float HardCycleFactor = 0.5f;
    /// <summary>인공지능 설정의 float 주기 필드 오프셋이다. ai%sBridgeDrawRate/StuffRefreshRate/TimeBetweenMoves에 해당한다.</summary>
    private static readonly int[] CycleOffsets = [0x220, 0x224, 0x228];
    /// <summary>인공지능 설정의 채취 유닛 수(ai%sCollectors, signed long) 오프셋이다.</summary>
    private const int CollectorsOffset = 0x234;
    /// <summary>인공지능 설정의 강제 적 소유자(ai%sEnemy, signed long) 오프셋이다.</summary>
    private const int ForcedEnemyOffset = 0x240;

    /// <summary>저장된 정수(0/누락 포함)를 난이도로 바꾼다. 1은 쉬움, 3은 어려움이고 그 밖의 모든 값은 보통이다.</summary>
    /// <param name="value">설정 파일의 difficulty 값이다.</param>
    /// <returns>대응하는 난이도다.</returns>
    public static AiDifficultyLevel FromValue(uint value)
    {
        if (value == (uint)AiDifficultyLevel.Easy)
        {
            return AiDifficultyLevel.Easy;
        }
        if (value == (uint)AiDifficultyLevel.Hard)
        {
            return AiDifficultyLevel.Hard;
        }
        return AiDifficultyLevel.Normal;
    }

    /// <summary>난이도를 설정 파일에 저장하는 정수로 바꾼다(FromValue의 역함수).</summary>
    /// <param name="level">난이도다.</param>
    /// <returns>저장용 정수다.</returns>
    public static uint ToValue(AiDifficultyLevel level) => (uint)level;

    /// <summary>설정이 600바이트인지 확인한다.</summary>
    /// <param name="settings">검사할 설정이다.</param>
    /// <exception cref="ArgumentException">설정이 600바이트가 아니다.</exception>
    private static void CheckSize(ReadOnlySpan<byte> settings)
    {
        if (settings.Length != RawMissionAiSettings.Size)
        {
            throw new ArgumentException("인공지능 설정은 600바이트여야 한다", nameof(settings));
        }
    }

    /// <summary>
    /// 10차 NB10(싱글 미션의 잘못된 player side로 종료): 미션이 정한 강제 적(ai%sEnemy, 설정 +0x240)이
    /// 소유자 범위 밖(1..8이 아님)이거나 자기 자신이면 0(적 자동 선택)으로 바꾼 복사본을 돌려준다.
    /// 바꿀 것이 없으면 빈 배열을 돌려주며 입력은 바꾸지 않는다.
    /// </summary>
    /// <param name="settings">600바이트 인공지능 설정이다.</param>
    /// <param name="owner">인공지능 소유자 번호다.</param>
    /// <returns>정리한 복사본, 바꿀 것이 없으면 빈 배열이다.</returns>
    /// <exception cref="ArgumentException">설정이 600바이트가 아니다.</exception>
    public static byte[] SanitizeAiForcedEnemy(ReadOnlySpan<byte> settings, uint owner)
    {
        CheckSize(settings);
        uint forced = BinaryPrimitives.ReadUInt32LittleEndian(settings.Slice(ForcedEnemyOffset, 4));
        // 정상이거나 자동 선택이면 바꾸지 않는다
        if (forced == 0 || (forced <= 8 && forced != owner))
        {
            return [];
        }
        byte[] result = settings.ToArray();
        BinaryPrimitives.WriteUInt32LittleEndian(result.AsSpan(ForcedEnemyOffset, 4), 0);
        return result;
    }

    /// <summary>
    /// 600바이트 인공지능 설정의 복사본에 난이도를 적용해 돌려준다. 입력은 바꾸지 않는다.
    /// 보통이면 입력과 같은 바이트의 복사본이다. 쉬움/어려움은 +0x220 다리 그리기 주기·+0x224 물건 갱신 주기·+0x228 이동 간격(float)에 배율을 곱하고,
    /// 쉬움은 +0x234 채취 유닛 수가 1 이상일 때만 절반(내림, 최소 1)으로 줄인다. 비유한/0 이하 주기는 그대로 둔다.
    /// </summary>
    /// <param name="settings">600바이트 인공지능 설정이다.</param>
    /// <param name="level">적용할 난이도다.</param>
    /// <returns>난이도를 적용한 복사본이다.</returns>
    /// <exception cref="ArgumentException">설정이 600바이트가 아니다.</exception>
    public static byte[] ApplyAiDifficulty(ReadOnlySpan<byte> settings, AiDifficultyLevel level)
    {
        CheckSize(settings);
        byte[] result = settings.ToArray();
        // 보통은 원본 값 그대로다
        if (level == AiDifficultyLevel.Normal)
        {
            return result;
        }
        float factor = level == AiDifficultyLevel.Easy ? EasyCycleFactor : HardCycleFactor;
        // 세 주기 필드를 차례로 곱한다. 0 이하/비유한 값(예: 다리 주기 0)은 배율로 바꿀 수 없어 그대로 둔다.
        foreach (int offset in CycleOffsets)
        {
            float value = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(result.AsSpan(offset, 4)));
            if (float.IsFinite(value) && value > 0.0f)
            {
                BinaryPrimitives.WriteUInt32LittleEndian(result.AsSpan(offset, 4), BitConverter.SingleToUInt32Bits(value * factor));
            }
        }
        // 쉬움만 채취 유닛 수를 줄이며 원래 0이면(채취 유닛 없음 설계) 건드리지 않는다
        if (level == AiDifficultyLevel.Easy)
        {
            int collectors = BinaryPrimitives.ReadInt32LittleEndian(result.AsSpan(CollectorsOffset, 4));
            if (collectors > 0)
            {
                BinaryPrimitives.WriteInt32LittleEndian(result.AsSpan(CollectorsOffset, 4), Math.Max(1, collectors / 2));
            }
        }
        return result;
    }
}
