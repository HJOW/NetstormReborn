using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 원본 AI 생성과 Player 소유 수명을 검증한다.
namespace Netstorm.Tests;

/// <summary>AI 생성의 Player 바이트와 외부 효과 순서를 독립 원본 결과에 대조한다.</summary>
public sealed class RawAiCreateTests
{
    /// <summary>원본 Player 저장소의 DWORD를 쓴다.</summary>
    /// <param name="raw">Player 저장소.</param><param name="offset">필드 위치.</param><param name="value">쓸 값.</param>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset, 4), value);

    /// <summary>한 원본 입력으로 실제 Player 참여 모듈과 AI 생성기를 실행한다.</summary>
    /// <param name="row">독립 원본 결과 행.</param>
    private static bool Matches(string[] row)
    {
        long[] numbers = row[1].Split('|')[0].Split(',').Select(long.Parse).ToArray();
        uint owner = unchecked((uint)numbers[0]);
        var players = new byte[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerStride];
        int start = (int)owner * PlayerParticipation.PlayerStride;
        Put(players, start + 0xb0, unchecked((uint)numbers[1]));
        Put(players, start + 0x78, 127);
        Put(players, start + 0x50, unchecked((uint)numbers[5]));
        Put(players, start + 0x2c, unchecked((uint)numbers[6]));
        var participation = new PlayerParticipation(players, new uint[81], new byte[9], new PlayerParticipationState(), new PlayerParticipationHooks());
        var settings = new byte[RawMissionAiSettings.Size];
        Put(settings, 0x244, unchecked((uint)numbers[2]));
        Put(settings, 0x250, unchecked((uint)numbers[3]));
        var trace = new List<string>();
        var hooks = new AiCreateHooks
        {
            ChangeColor = (id, color) => trace.Add("O," + id + "," + color),
            FirstRegion = _ => 127,
            IslandTemple = region => { trace.Add("V," + region); return unchecked((uint)numbers[4]); },
            DestroyTemple = (_, flags) => trace.Add("D," + flags),
            Construct = (id, _, size) => { trace.Add("B," + size); trace.Add("K," + id); return 0x12310000; },
            InitialUpdate = (brain, _) => trace.Add("U," + brain),
        };
        var create = new RawAiCreate(OriginalEdition.Patch1078, participation, hooks);
        uint actual = create.Create(owner, settings);
        string player = Convert.ToHexString(players.AsSpan(start, PlayerParticipation.PlayerStride)).ToLowerInvariant();
        return player == row[2] && actual.ToString() == row[3] && (trace.Count == 0 ? "-" : string.Join(';', trace)) == row[4];
    }

    /// <summary>기존 brain·NoTemple·정상 생성의 원본 32행을 Player 전체 바이트와 효과 순서로 검사한다.</summary>
    [Fact]
    public void CreateMatchesNative32()
    {
        string[][] rows = X86Fixture.Read("mission-start", "create");
        Assert.Equal(32, rows.Length);
        X86Fixture.CheckRows(rows, Matches);
    }

    /// <summary>생성 본체나 초기 갱신이 없으면 Player를 부착하지 않고 기존 brain은 재사용한다.</summary>
    [Fact]
    public void MissingActualBodiesNeverAttachSyntheticBrain()
    {
        var raw = new byte[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerStride];
        var players = new PlayerParticipation(raw, new uint[81], new byte[9], new PlayerParticipationState(), new PlayerParticipationHooks());
        var create = new RawAiCreate(OriginalEdition.Patch1078, players, new AiCreateHooks { Construct = (_, _, _) => 99 });
        Assert.Throws<InvalidOperationException>(() => create.Create(1, new byte[600]));
        Assert.Equal(0u, players.Brain(1));
        Assert.Equal(0u, players.Flags(1) & PlayerParticipation.BrainFlag);
        players.AttachBrain(1, 73);
        Assert.Equal(73u, create.Create(1, new byte[600]));
    }

    /// <summary>정상 reset 증빙은 같은 모듈에 한 번만 소비하며 반납 뒤 재생성을 차단한다.</summary>
    [Fact]
    public void ReturnedBrainCannotBeReinstalledOrForgedAcrossOwners()
    {
        var raw = new byte[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerStride];
        var hooks = new PlayerParticipationHooks { DestructBrain = _ => { }, FreeBrain = _ => { } };
        var players = new PlayerParticipation(raw, new uint[81], new byte[9], new PlayerParticipationState(), hooks);
        var other = new PlayerParticipation(new byte[raw.Length], new uint[81], new byte[9], new PlayerParticipationState(), hooks);
        players.AttachBrain(1, 73);
        var pass = players.RemoveBrainForState(1);
        Assert.Throws<ArgumentException>(() => other.ReturnBrainForState(pass));
        players.ReturnBrainForState(pass);
        Assert.True(players.BrainReturnedForState(1));
        Assert.Throws<ArgumentException>(() => players.ReturnBrainForState(pass));
        Assert.Throws<InvalidOperationException>(() => players.AttachBrain(1, 74));
        var create = new RawAiCreate(OriginalEdition.Patch1078, players, new AiCreateHooks());
        Assert.Throws<InvalidOperationException>(() => create.Create(1, new byte[600]));
        Put(raw, PlayerParticipation.PlayerStride + PlayerParticipation.BrainOffset, 88);
        Assert.Throws<InvalidOperationException>(() => players.BrainReturnedForState(1));
    }

    /// <summary>파괴 중 재진입한 증빙은 콜백이 끝나기 전 소비할 수 없고 기존 brain은 유지된다.</summary>
    [Fact]
    public void ResetProofCannotBeConsumedDuringDestruction()
    {
        var raw = new byte[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerStride];
        PlayerParticipation? players = null;
        PlayerParticipation.BrainResetPass? pass = null;
        var hooks = new PlayerParticipationHooks
        {
            DestructBrain = _ => { Assert.Equal(73u, players!.Brain(1)); Assert.Throws<InvalidOperationException>(() => players.ReturnBrainForState(pass!)); },
            FreeBrain = _ => Assert.Equal(73u, players!.Brain(1)),
        };
        players = new PlayerParticipation(raw, new uint[81], new byte[9], new PlayerParticipationState(), hooks);
        pass = players.RemoveBrainForState(2);
        players.AttachBrain(1, 73);
        players.RemoveBrain(1);
        players.ReturnBrainForState(pass);
        Assert.True(players.BrainReturnedForState(2));
    }
}
