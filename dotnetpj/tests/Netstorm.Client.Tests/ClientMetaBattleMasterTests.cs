// 0x7c·메타 연결이 공유하는 현재 전투의 첫 master 참가자 조회를 대조한다.
// cpppj/src/client/NativeMetaConnectedState.cpp 의 FindMetaBattleMaster(004b2c60)를 대조한다.
// 0.945에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 004b2c60 master 조회의 실제 CS 기록을 준비한다.
public sealed class ClientMetaBattleMasterTests
{
    /// <summary>같은 실제 CS와 전투·참가자 기록을 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>실제 참가자의 전투 번호를 쓴다.</summary>
        public void BattleOf(uint participant, uint battle) =>
            BinaryPrimitives.WriteUInt32LittleEndian(Challenge.Participant(participant).Span.Slice(0xa4, 4), battle);

        /// <summary>실제 참가자의 flags BYTE를 쓴다.</summary>
        public void FlagsOf(uint participant, byte flags) => Challenge.Participant(participant).Span[0x64] = flags;

        /// <summary>실제 전투 기록의 8칸에 후보를 쓴다.</summary>
        public void CandidatesOf(uint battle, params uint[] candidates)
        {
            Span<byte> span = Challenge.Battle(battle).Span;
            for (int slot = 0; slot < candidates.Length && slot < 8; slot++)
            {
                BinaryPrimitives.WriteUInt32LittleEndian(span.Slice(0x38 + slot * 4, 4), candidates[slot]);
            }
        }
    }

    /// <summary>미연결 CS는 일반 실패로 꾸미지 않는다. 0.945에서 추가됐다.</summary>
    [Fact]
    public void NullChallengeThrows()
    {
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleMaster.Find(null!, 1));
    }

    /// <summary>전투 번호가 0이면 전투 기록을 읽지 않고 -1이다. 0.945에서 추가됐다.</summary>
    [Fact]
    public void ZeroBattleReturnsNegativeOne()
    {
        var scene = new Scene();
        scene.BattleOf(1, 0);
        Assert.Equal(-1, ClientMetaBattleMaster.Find(scene.Challenge, 1));
    }

    /// <summary>flags bit4가 없으면 -1이다. 0.945에서 추가됐다.</summary>
    [Fact]
    public void NoFlaggedCandidateReturnsNegativeOne()
    {
        var scene = new Scene();
        scene.BattleOf(1, 2);
        scene.CandidatesOf(2, 3, 4);
        scene.FlagsOf(3, 0);
        scene.FlagsOf(4, 3);
        Assert.Equal(-1, ClientMetaBattleMaster.Find(scene.Challenge, 1));
    }

    /// <summary>0 후보를 건너뛰고 bit4가 선 첫 후보를 반환한다. 0.945에서 추가됐다.</summary>
    [Fact]
    public void FirstFlaggedCandidateWins()
    {
        var scene = new Scene();
        scene.BattleOf(1, 2);
        scene.CandidatesOf(2, 0, 5, 7);
        scene.FlagsOf(5, 4);
        scene.FlagsOf(7, 4);
        Assert.Equal(5, ClientMetaBattleMaster.Find(scene.Challenge, 1));
    }

    /// <summary>뒤 칸의 bit4는 앞 칸이 비었을 때만 선택된다. 0.945에서 추가됐다.</summary>
    [Fact]
    public void LaterFlaggedCandidateSelectedWhenEarlierEmpty()
    {
        var scene = new Scene();
        scene.BattleOf(1, 3);
        scene.CandidatesOf(3, 0, 0, 6);
        scene.FlagsOf(6, 7);
        Assert.Equal(6, ClientMetaBattleMaster.Find(scene.Challenge, 1));
    }

    /// <summary>범위 밖 전투·참가자는 원문 상한에서 거부한다. 0.945에서 추가됐다.</summary>
    [Fact]
    public void OutOfRangeRecordsThrow()
    {
        var scene = new Scene();
        scene.BattleOf(1, 10);
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaBattleMaster.Find(scene.Challenge, 1));
        scene.BattleOf(1, 2);
        scene.CandidatesOf(2, 40);
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaBattleMaster.Find(scene.Challenge, 1));
    }
}
