using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 수명 관측과 실제 생성/트리/삭제 통합을 검증하는 영역이다.

/// <summary>Gump 지연 삭제·소멸자·큐·등록 재사용의 원본 전체 바이트/콜백 순서를 검증한다.
/// 근거: cpppj/tests/GumpLifetimeTests.cpp.</summary>
public sealed class GumpLifetimeTests
{
    /// <summary>각 판본173행의 전체 노드·다섯 표·전역·반환·경계 당시 상태를 실제 PE 관측과 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeLifetime(string edition)
    {
        string[][] rows = X86Fixture.Read("gumplifetime", edition); Assert.Equal(173, rows.Length);
        // 한 행의 연속 연산만 현재 상태를 공유한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(13, row.Length); var scene = new GumpLifetimeFixture(edition, row, reset: false); GumpLifetimeHooks hooks = scene.Hooks(); GumpLifetime? current = null;
            if (row[6] == "1") hooks.DeletingDestructor = (token, flags) => current!.DeleteScalar(token, flags);
            current = new(scene.Create, scene.Life, hooks); var results = new List<string>();
            // 명시 입력 함수만 호출하며 예상 삭제/소멸 결과를 계산하지 않는다.
            foreach (string item in row[4].Split(';'))
            {
                string[] p = item.Split(','); uint token = X86Fixture.UInt(p[1]); string result = "-";
                switch (p[0])
                {
                    case "Kill": current.Kill(token); break; case "Cleanup": current.Cleanup(); break; case "Destroy": current.Destroy(token); break;
                    case "Base": current.DestructBase(token); break; case "Scalar": result = current.DeleteScalar(token, X86Fixture.UInt(p[2])).ToString(CultureInfo.InvariantCulture); break;
                    case "Idle": current.SetIdle(token, X86Fixture.UInt(p[2]) != 0); break; case "Overlay": current.SetOverlay(token, X86Fixture.UInt(p[2]) != 0); break;
                    case "Dialog": result = current.RemoveDialog(token) ? "1" : "0"; break; default: throw new InvalidDataException("수명 연산 오류");
                }
                results.Add(result);
            }
            scene.Check(row, results);
        }
    }

    /// <summary>실제 생성/부착/Push/기본 소멸자/해제/첫 빈 등록 칸 재사용까지 연결한다.</summary>
    [Fact]
    public void CreateTreeKillCleanupAndReuse()
    {
        var rig = new GumpLifetimeRig(); rig.Tree.Attach(2, 2, 3, 1, 0); rig.Tree.Attach(3, 1, 1, 2, 0); rig.Tree.Attach(4, 2, 2, 2, 0);
        rig.Lifetime.SetIdle(3, true); rig.Lifetime.SetOverlay(4, true); rig.LifeState.Focus = 3; rig.State.Collection.OverrideHead = 4;
        rig.LifeState.Dialogs[0] = 3; rig.LifeState.Dialogs[1] = 4; rig.LifeState.DialogCount = 2; rig.LifeState.CurrentDialog = 4;
        rig.Lifetime.Kill(2); Assert.Equal(3U, rig.LifeState.KillCount); Assert.Equal(new uint[] { 4, 3, 2 }, rig.LifeState.Kill[..3]);
        Assert.Equal(0U, rig.Nodes[1].Child); Assert.Equal(0U, rig.Nodes[2].Child); Assert.Equal(0U, rig.LifeState.Focus); Assert.Equal(0U, rig.State.Collection.OverrideHead); Assert.Equal(0U, rig.LifeState.DialogCount);
        var collection = new GumpCollection(rig.State.Collection, rig.Create.CollectionHooks()); var work = new RenderCollectionEntry[4];
        Assert.Equal(0, collection.Collect(new(0, 0, 64, 48), work, 4)); Assert.Equal(4U, rig.State.Count);
        rig.Lifetime.Kill(3); Assert.Equal(4U, rig.LifeState.KillCount); rig.Lifetime.Cleanup(); Assert.Equal(new uint[] { 4, 3, 2 }, rig.Freed);
        Assert.Equal(1U, rig.State.Count); Assert.Equal(1U, rig.State.Entries[0]); Assert.Equal(1U, rig.State.Collection.Root); Assert.Equal(0U, rig.LifeState.KillCount);
        Assert.False(rig.Nodes.ContainsKey(2)); Assert.False(rig.Nodes.ContainsKey(3)); Assert.False(rig.Nodes.ContainsKey(4));
        rig.Create.Construct(5, 27, 1, 1, 1); Assert.Equal(1U, rig.Nodes[5].Id); Assert.Equal(5U, rig.Nodes[5].Serial); Assert.Equal(2U, rig.State.Count);
        rig.Tree.Attach(5, 1, 2, 1, 0); Assert.Equal(1, collection.Collect(new(0, 0, 64, 48), work, 4)); Assert.Equal(5U, work[0].Token);
    }

    /// <summary>0x4000 부착 교체가 실제 Kill로 전달되어 이전 형제를 분리하고 큐에만 넣는다.</summary>
    [Fact]
    public void TreeReplacementDefersDestruction()
    {
        var rig = new GumpLifetimeRig(); rig.Nodes[2].Flags |= 0x4000; rig.Nodes[3].Flags |= 0x4000; rig.Tree.Attach(2, 0, 0, 1, 0); rig.Tree.Attach(3, 0, 0, 1, 0);
        Assert.Equal(3U, rig.Nodes[1].Child); Assert.Equal(1U, rig.LifeState.KillCount); Assert.Equal(2U, rig.LifeState.Kill[0]); Assert.Equal(4U, rig.State.Count);
        Assert.NotEqual(0U, rig.Nodes[2].Flags & 0x10000); Assert.Empty(rig.Freed);
    }

    /// <summary>기본 소멸자만 직접 실행하면 등록/dialog/flags/개수는 유지하고 각 표의 모든 일치만 제거한다.</summary>
    [Fact]
    public void BaseDoesNotUnregisterOrRemoveDialogs()
    {
        var rig = new GumpLifetimeRig(); uint flags = rig.Nodes[2].Flags; rig.LifeState.Idle[1] = rig.LifeState.Idle[499] = 2;
        rig.LifeState.Overlay[0] = rig.LifeState.Overlay[99] = 2; rig.LifeState.Kill[0] = rig.LifeState.Kill[999] = 2; rig.LifeState.KillCount = 1;
        rig.LifeState.Dialogs[0] = 2; rig.LifeState.DialogCount = 1; rig.LifeState.CurrentDialog = 2; rig.Lifetime.DestructBase(2);
        Assert.Equal(0U, rig.LifeState.Idle[1]); Assert.Equal(0U, rig.LifeState.Idle[499]); Assert.Equal(0U, rig.LifeState.Overlay[99]); Assert.Equal(0U, rig.LifeState.Kill[999]);
        Assert.Equal(4U, rig.State.Count); Assert.Equal(2U, rig.State.Entries[1]); Assert.Equal(1U, rig.LifeState.KillCount); Assert.Equal(1U, rig.LifeState.DialogCount); Assert.Equal(flags, rig.Nodes[2].Flags);
    }

    /// <summary>CD의 실제 idle80칸 밖에 보관한 호스트 값은 기본 소멸자가 건드리지 않는다.</summary>
    [Fact]
    public void CdIdleCapacity()
    {
        var rig = new GumpLifetimeRig(OriginalEdition.Cd1072); Array.Fill(rig.LifeState.Idle, 3U); rig.LifeState.Idle[79] = 0; rig.LifeState.Idle[80] = 2;
        rig.Lifetime.SetIdle(2, true); Assert.Equal(2U, rig.LifeState.Idle[79]); rig.Lifetime.DestructBase(2);
        Assert.Equal(0U, rig.LifeState.Idle[79]); Assert.Equal(2U, rig.LifeState.Idle[80]);
    }

    /// <summary>원본 유효 입력 밖의 토큰/표/플래그/순환/큐 한도는 명시 오류로 거부한다.</summary>
    [Fact]
    public void ContractsAndCycles()
    {
        var rig = new GumpLifetimeRig(); Assert.Throws<ArgumentException>(() => new GumpLifetime(rig.State, rig.LifeState, new()));
        Assert.Throws<InvalidOperationException>(() => rig.Lifetime.Kill(99)); rig.Nodes[2].Flags |= 0x80;
        Assert.Throws<ArgumentException>(() => rig.Lifetime.SetIdle(2, false)); rig.Nodes[2].Flags &= ~0x80U;
        rig.LifeState.KillCount = 999; Assert.Throws<ArgumentException>(() => rig.Lifetime.Kill(2)); rig.LifeState.KillCount = 0;
        rig.LifeState.DialogCount = 17; Assert.Throws<ArgumentException>(() => rig.Lifetime.RemoveDialog(2)); rig.LifeState.DialogCount = 0;
        rig.Nodes[2].Child = 3; rig.Nodes[3].Child = 2; Assert.Throws<InvalidOperationException>(() => rig.Lifetime.Kill(2)); rig.Nodes[2].Child = rig.Nodes[3].Child = 0;
        rig.Nodes[2].Id = 1000; Assert.Throws<ArgumentOutOfRangeException>(() => rig.Lifetime.Destroy(2)); rig.Nodes[2].Id = 1;
        rig.LifeState.KillCount = 1; rig.LifeState.Kill[0] = 2; Assert.Throws<InvalidOperationException>(() => rig.Lifetime.Cleanup()); rig.LifeState.KillCount = 0;
        rig.Lifetime.SetOverlay(2, true); Assert.Throws<ArgumentException>(() => rig.Lifetime.SetOverlay(2, true));
    }

    /// <summary>외부 소멸자/텍스트 해제의 예외가 발생해도 이미 수행한 등록/flags/기본 가상 표 쓰기는 남긴다.</summary>
    [Fact]
    public void CallbackExceptionKeepsWrittenState()
    {
        var node = new GumpCreateNode { Id = 1, Flags = 0x10000 }; var state = new GumpCreateState { Count = 4 }; state.Entries[1] = 2;
        var hooks = new GumpLifetimeHooks { Node = token => token == 2 ? node : null, DeletingDestructor = (_, _) => throw new InvalidOperationException("가상 소멸자 실패") };
        var life = new GumpLifetime(state, new(), hooks); Assert.Throws<InvalidOperationException>(() => life.Destroy(2));
        Assert.Equal(3U, state.Count); Assert.Equal(0U, state.Entries[1]); Assert.NotEqual(0U, node.Flags & 4); Assert.Equal(0U, node.Flags & 0x10000);
        hooks.Release = _ => throw new InvalidOperationException("텍스트 해제 실패"); node.Vtable = 123; node.TextPointer = 456;
        Assert.Throws<InvalidOperationException>(() => life.DestructBase(2)); Assert.Equal(0x50ac70U, node.Vtable); Assert.Equal(456U, node.TextPointer);
    }
}
