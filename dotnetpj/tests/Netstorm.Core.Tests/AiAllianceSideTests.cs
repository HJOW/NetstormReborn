using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using PlayerParticipationHooks = Netstorm.Core.Simulation.PlayerParticipationHooks;
using PlayerParticipationState = Netstorm.Core.Simulation.PlayerParticipationState;

// 생성 전의 비대칭 동맹 선언과 실제 설정 정리/난이도 전달 순서를 검증한다.
namespace Netstorm.Tests;

/// <summary>cpppj1312의 동맹 AI 오분류 보정을 현재 생성 API와 함께 검사한다.</summary>
public sealed class AiAllianceSideTests
{
    /// <summary>선언 동맹/로컬 마스크의 서로 다른 방향과0·8경계를 명시 입력으로 대조한다.</summary>
    [Theory]
    [InlineData(2u, 1u, 0u, 2u, false)]
    [InlineData(2u, 1u, 0u, 0u, true)]
    [InlineData(2u, 1u, 4u, 0u, false)]
    [InlineData(1u, 1u, 0u, 0u, false)]
    [InlineData(2u, 0u, 0u, 1u, false)]
    [InlineData(0u, 8u, 0u, 256u, false)]
    [InlineData(8u, 1u, 256u, 0u, false)]
    [InlineData(8u, 1u, 2u, 0u, true)]
    [InlineData(2u, 1u, 0x80000000u, 0x80000000u, true)]
    [InlineData(2u, 1u, 0u, 4u, true)]
    public void DeclaredAndRecordedAlliancesAreRecognized(uint owner, uint local, uint allies, uint declared, bool enemy)
    {
        var settings = new byte[RawMissionAiSettings.Size];
        BinaryPrimitives.WriteUInt32LittleEndian(settings.AsSpan(0x23c), declared);
        Assert.Equal(enemy, AiAllianceSide.IsAiEnemyOfLocal(owner, local, allies, settings));
    }

    /// <summary>실제 DWORD 끝까지의 자료를 허용하며 짧은 자료 검사를 owner 검사보다 먼저 실행한다.</summary>
    [Fact]
    public void OriginalBoundsAndValidationOrderArePreserved()
    {
        var exact = new byte[0x240];
        Assert.True(AiAllianceSide.IsAiEnemyOfLocal(2, 1, 0, exact));
        Assert.Throws<ArgumentException>(() => AiAllianceSide.IsAiEnemyOfLocal(9, 9, 0, exact.AsSpan(0, 0x23f)));
        Assert.Throws<ArgumentOutOfRangeException>(() => AiAllianceSide.IsAiEnemyOfLocal(9, 1, 0, exact));
        Assert.Throws<ArgumentOutOfRangeException>(() => AiAllianceSide.IsAiEnemyOfLocal(2, uint.MaxValue, 0, exact));
    }

    /// <summary>NB10 정리 뒤의 설정을 판정에 넘기고 동맹이면 보통, 적이면 쉬움 복사본을 실제 생성·초기 갱신에 전달한다.</summary>
    [Theory]
    [InlineData(2u, 0u, 8f, 4u)]
    [InlineData(0u, 4u, 8f, 4u)]
    [InlineData(0u, 0u, 32f, 2u)]
    public void ActualCreationPassesSanitizedSettingsBeforeApplyingEnemyDifficulty(uint declared, uint allies, float cycle, uint collectors)
    {
        var records = new byte[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerStride];
        var players = new PlayerParticipation(records, new uint[81], new byte[9], new PlayerParticipationState(), new PlayerParticipationHooks());
        var settings = new byte[RawMissionAiSettings.Size];
        BinaryPrimitives.WriteUInt32LittleEndian(settings.AsSpan(0x23c), declared);
        BinaryPrimitives.WriteUInt32LittleEndian(settings.AsSpan(0x240), 0xff);
        BinaryPrimitives.WriteSingleLittleEndian(settings.AsSpan(0x220), 8f);
        BinaryPrimitives.WriteUInt32LittleEndian(settings.AsSpan(0x234), 4);
        byte[] before = settings.ToArray(); ReadOnlyMemory<byte> selected = default, built = default, updated = default;
        var create = new RawAiCreate(OriginalEdition.Patch1078, players, new AiCreateHooks
        {
            Difficulty = (owner, active) =>
            {
                Assert.Equal(2u, owner); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(active.Span[0x240..]));
                Assert.Equal(8f, BinaryPrimitives.ReadSingleLittleEndian(active.Span[0x220..]));
                selected = active;
                return AiAllianceSide.IsAiEnemyOfLocal(owner, 1, allies, active.Span) ? 1u : 2u;
            },
            Construct = (_, active, size) => { Assert.Equal(0xb08, size); built = active; return 77; },
            InitialUpdate = (_, active) => updated = active,
        });
        Assert.Equal(77u, create.Create(2, settings)); Assert.Equal(77u, players.Brain(2));
        Assert.Equal(built, updated); Assert.NotEqual((ReadOnlyMemory<byte>)settings, selected);
        Assert.Equal(cycle, BinaryPrimitives.ReadSingleLittleEndian(built.Span[0x220..]));
        Assert.Equal(collectors, BinaryPrimitives.ReadUInt32LittleEndian(built.Span[0x234..]));
        Assert.Equal(before, settings);
    }
}
