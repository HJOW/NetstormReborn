using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using PlayerParticipationHooks = Netstorm.Core.Simulation.PlayerParticipationHooks;
using PlayerParticipationState = Netstorm.Core.Simulation.PlayerParticipationState;

namespace Netstorm.Core.Tests;

/// <summary>
/// 11차 난이도 배율과 10차 NB10 강제 적 정리의 계약을 검사한다
/// (cpppj <c>o/AiDifficulty.h/.cpp</c>와 <c>tests/</c>의 NB10·난이도 검사를 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.347에서 11차·NB10 재대조와 함께 추가했다. 원본 x86 fixture로 대조하지 않으므로 PORT_MAP 상태는 port다.</remarks>
public sealed class AiDifficultyTests
{
    /// <summary>600바이트 설정을 만든다.</summary>
    /// <returns>0으로 채운 설정이다.</returns>
    private static byte[] Settings() => new byte[RawMissionAiSettings.Size];

    /// <summary>설정의 DWORD를 쓴다.</summary>
    /// <param name="settings">쓸 설정이다.</param><param name="offset">쓸 위치다.</param><param name="value">쓸 값이다.</param>
    private static void Put(byte[] settings, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(settings.AsSpan(offset, 4), value);

    /// <summary>설정의 float을 쓴다.</summary>
    /// <param name="settings">쓸 설정이다.</param><param name="offset">쓸 위치다.</param><param name="value">쓸 값이다.</param>
    private static void PutFloat(byte[] settings, int offset, float value) => BinaryPrimitives.WriteUInt32LittleEndian(settings.AsSpan(offset, 4), BitConverter.SingleToUInt32Bits(value));

    /// <summary>설정의 float을 읽는다.</summary>
    /// <param name="settings">읽을 설정이다.</param><param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static float GetFloat(byte[] settings, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(settings.AsSpan(offset, 4)));

    /// <summary>설정 파일 정수를 난이도로 바꾼다. 1은 쉬움, 3은 어려움이고 그 밖은 보통이다.</summary>
    /// <param name="value">저장된 값이다.</param><param name="expected">기대 난이도다.</param>
    [Theory]
    [InlineData(1u, AiDifficultyLevel.Easy)]
    [InlineData(3u, AiDifficultyLevel.Hard)]
    [InlineData(0u, AiDifficultyLevel.Normal)]
    [InlineData(2u, AiDifficultyLevel.Normal)]
    [InlineData(99u, AiDifficultyLevel.Normal)]
    public void FromValueMapsStoredIntegers(uint value, AiDifficultyLevel expected)
    {
        // 0과 모르는 값은 보통이다
        Assert.Equal(expected, AiDifficulty.FromValue(value));
        Assert.Equal(value is 1u or 3u ? value : 2u, AiDifficulty.ToValue(AiDifficulty.FromValue(value)));
    }

    /// <summary>보통은 입력과 같은 바이트의 복사본을 돌려준다.</summary>
    [Fact]
    public void ApplyNormalKeepsBytes()
    {
        byte[] settings = Settings();
        PutFloat(settings, 0x220, 10.0f);
        Put(settings, 0x234, 5u);
        byte[] actual = AiDifficulty.ApplyAiDifficulty(settings, AiDifficultyLevel.Normal);
        Assert.Equal(settings, actual);
    }

    /// <summary>쉬움은 주기를 4배하고 채취 유닛을 절반(최소 1)으로 줄인다.</summary>
    [Fact]
    public void ApplyEasyScalesCyclesAndHalvesCollectors()
    {
        byte[] settings = Settings();
        PutFloat(settings, 0x220, 10.0f);
        PutFloat(settings, 0x224, 20.0f);
        PutFloat(settings, 0x228, 30.0f);
        Put(settings, 0x234, 5u);
        byte[] actual = AiDifficulty.ApplyAiDifficulty(settings, AiDifficultyLevel.Easy);
        Assert.Equal(40.0f, GetFloat(actual, 0x220));
        Assert.Equal(80.0f, GetFloat(actual, 0x224));
        Assert.Equal(120.0f, GetFloat(actual, 0x228));
        Assert.Equal(2, BinaryPrimitives.ReadInt32LittleEndian(actual.AsSpan(0x234, 4)));
        // 입력은 바뀌지 않는다
        Assert.Equal(10.0f, GetFloat(settings, 0x220));
        Assert.Equal(5, BinaryPrimitives.ReadInt32LittleEndian(settings.AsSpan(0x234, 4)));
    }

    /// <summary>쉬움의 채취 유닛 하한·0 주기·비유한 주기 경계를 검사한다.</summary>
    [Fact]
    public void ApplyEasyKeepsCollectorFloorAndUnscalableCycles()
    {
        byte[] settings = Settings();
        PutFloat(settings, 0x220, 0.0f);
        PutFloat(settings, 0x224, float.NaN);
        PutFloat(settings, 0x228, 4.0f);
        Put(settings, 0x234, 1u);
        byte[] actual = AiDifficulty.ApplyAiDifficulty(settings, AiDifficultyLevel.Easy);
        // 0 이하·비유한 주기는 그대로 두고 1은 절반 내림의 하한 1이다
        Assert.Equal(0.0f, GetFloat(actual, 0x220));
        Assert.Equal(float.NaN, GetFloat(actual, 0x224));
        Assert.Equal(16.0f, GetFloat(actual, 0x228));
        Assert.Equal(1, BinaryPrimitives.ReadInt32LittleEndian(actual.AsSpan(0x234, 4)));
        byte[] empty = Settings();
        Assert.Equal(0, BinaryPrimitives.ReadInt32LittleEndian(AiDifficulty.ApplyAiDifficulty(empty, AiDifficultyLevel.Easy).AsSpan(0x234, 4)));
    }

    /// <summary>어려움은 주기를 절반으로 하고 채취 유닛은 그대로 둔다.</summary>
    [Fact]
    public void ApplyHardHalvesCyclesKeepsCollectors()
    {
        byte[] settings = Settings();
        PutFloat(settings, 0x220, 10.0f);
        Put(settings, 0x234, 5u);
        byte[] actual = AiDifficulty.ApplyAiDifficulty(settings, AiDifficultyLevel.Hard);
        Assert.Equal(5.0f, GetFloat(actual, 0x220));
        Assert.Equal(5, BinaryPrimitives.ReadInt32LittleEndian(actual.AsSpan(0x234, 4)));
    }

    /// <summary>600바이트가 아니면 두 계산 모두 거부한다.</summary>
    [Fact]
    public void WrongSizeSettingsAreRejected()
    {
        Assert.Throws<ArgumentException>(() => AiDifficulty.ApplyAiDifficulty(new byte[599], AiDifficultyLevel.Easy));
        Assert.Throws<ArgumentException>(() => AiDifficulty.SanitizeAiForcedEnemy(new byte[599], 1));
    }

    /// <summary>범위 밖·자기 자신 강제 적은 0으로 정리하고 정상·자동 선택은 그대로 둔다.</summary>
    /// <param name="forced">설정의 강제 적이다.</param><param name="owner">소유자다.</param><param name="changed">바뀌는지 여부다.</param>
    [Theory]
    [InlineData(0u, 1u, false)]
    [InlineData(3u, 1u, false)]
    [InlineData(9u, 1u, true)]
    [InlineData(1u, 1u, true)]
    public void SanitizeAiForcedEnemyCases(uint forced, uint owner, bool changed)
    {
        byte[] settings = Settings();
        Put(settings, 0x240, forced);
        byte[] actual = AiDifficulty.SanitizeAiForcedEnemy(settings, owner);
        // 입력은 바뀌지 않는다
        Assert.Equal(forced, BinaryPrimitives.ReadUInt32LittleEndian(settings.AsSpan(0x240, 4)));
        if (!changed)
        {
            Assert.Empty(actual);
            return;
        }
        Assert.Equal(600, actual.Length);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(actual.AsSpan(0x240, 4)));
    }

    /// <summary>생성 테스트용 Player 참여 모듈을 만든다.</summary>
    /// <returns>빈 참여 모듈이다.</returns>
    private static PlayerParticipation Players()
    {
        byte[] raw = new byte[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerStride];
        return new PlayerParticipation(raw, new uint[81], new byte[9], new PlayerParticipationState(), new PlayerParticipationHooks());
    }

    /// <summary>생성은 입력이 아니라 난이도 복사본을 생성자·초기 갱신에 넘긴다.</summary>
    [Fact]
    public void CreateAppliesDifficultyCopyWithoutChangingInput()
    {
        byte[] settings = Settings();
        PutFloat(settings, 0x220, 8.0f);
        Put(settings, 0x234, 4u);
        byte[]? built = null;
        byte[]? updated = null;
        var hooks = new AiCreateHooks
        {
            Construct = (_, body, _) => { built = body.ToArray(); return 77u; },
            InitialUpdate = (_, body) => { updated = body.ToArray(); },
            Difficulty = _ => 1u,
        };
        var create = new RawAiCreate(OriginalEdition.Patch1078, Players(), hooks);
        Assert.Equal(77u, create.Create(1, settings));
        Assert.NotNull(built);
        Assert.NotNull(updated);
        // 생성자와 초기 갱신은 같은 쉬움 복사본을 받는다
        Assert.Equal(32.0f, GetFloat(built, 0x220));
        Assert.Equal(2, BinaryPrimitives.ReadInt32LittleEndian(built.AsSpan(0x234, 4)));
        Assert.Equal(built, updated);
        // 입력 설정은 바뀌지 않는다
        Assert.Equal(8.0f, GetFloat(settings, 0x220));
        Assert.Equal(4, BinaryPrimitives.ReadInt32LittleEndian(settings.AsSpan(0x234, 4)));
    }

    /// <summary>보통·장치 없음이면 입력을 그대로 넘긴다.</summary>
    [Fact]
    public void CreateKeepsInputForNormalOrMissingDifficulty()
    {
        foreach (uint? value in new uint?[] { null, 0u, 2u })
        {
            byte[] settings = Settings();
            PutFloat(settings, 0x220, 8.0f);
            byte[]? built = null;
            AiCreateHooks hooks = value is null
                ? new AiCreateHooks
                {
                    Construct = (_, body, _) => { built = body.ToArray(); return 78u; },
                    InitialUpdate = (_, _) => { },
                }
                : new AiCreateHooks
                {
                    Construct = (_, body, _) => { built = body.ToArray(); return 78u; },
                    InitialUpdate = (_, _) => { },
                    Difficulty = _ => value.Value,
                };
            var create = new RawAiCreate(OriginalEdition.Patch1078, Players(), hooks);
            Assert.Equal(78u, create.Create(1, settings));
            Assert.NotNull(built);
            Assert.Equal(8.0f, GetFloat(built, 0x220));
        }
    }

    /// <summary>잘못된 강제 적은 생성 전에 0으로 정리된다.</summary>
    [Fact]
    public void CreateSanitizesInvalidForcedEnemy()
    {
        byte[] settings = Settings();
        Put(settings, 0x240, 9u);
        byte[]? built = null;
        var hooks = new AiCreateHooks
        {
            Construct = (_, body, _) => { built = body.ToArray(); return 79u; },
            InitialUpdate = (_, _) => { },
        };
        var create = new RawAiCreate(OriginalEdition.Patch1078, Players(), hooks);
        Assert.Equal(79u, create.Create(1, settings));
        Assert.NotNull(built);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(built.AsSpan(0x240, 4)));
        Assert.Equal(9u, BinaryPrimitives.ReadUInt32LittleEndian(settings.AsSpan(0x240, 4)));
    }
}
