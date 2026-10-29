using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 현재 제외 타입·루트 교체의 전체 수명 연결을 대조하는 영역이다.

/// <summary>원본 reset 관측과 실제 생성/트리/기본 소멸자/생존 자식 이동·등록 재사용을 검증한다.
/// 근거: cpppj/tests/GumpResetTests.cpp.</summary>
public sealed class GumpResetTests
{
    /// <summary>판본마다121행의 현재 공유 제외 BYTE·등록 표 순회·루트 교체를 전체 상태로 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void NativeReset(string edition)
    {
        string[][] rows = X86Fixture.Read("uireset", edition); Assert.Equal(121, rows.Length);
        // 각 행의 연속 명령만 같은 현재 전역과 표를 공유한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(13, row.Length); var scene = new GumpLifetimeFixture(edition, row, reset: true); GumpLifetimeHooks hooks = scene.Hooks(); GumpLifetime? current = null;
            if (row[6] == "1") hooks.DeletingDestructor = (token, flags) => current!.DeleteScalar(token, flags);
            current = new(scene.Create, scene.Life, hooks); var results = new List<string>();
            // 실제 API만 호출하고 기대 필터나 삭제 결과는 만들지 않는다.
            foreach (string item in row[4].Split(';'))
            {
                string[] action = item.Split(','); uint token = X86Fixture.UInt(action[1]);
                switch (action[0])
                {
                    case "KillExceptType": current.KillExceptType(ref scene.Excluded); break;
                    case "ReplaceRoot": current.ReplaceRoot(token); break; case "Cleanup": current.Cleanup(); break;
                    default: throw new InvalidDataException("Gump reset 연산 오류");
                }
                results.Add("-");
            }
            scene.Check(row, results);
        }
    }

    /// <summary>두 판본에서 부모 Kill·생존 직접 자식 이동·루트 교체·등록 칸 재사용을 실제 수명으로 연결한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void TransferAndReuse(OriginalEdition edition)
    {
        var rig = new GumpLifetimeRig(edition); rig.Nodes[2].Command = 21; rig.Nodes[3].Command = rig.Nodes[4].Command = 11;
        rig.Tree.Attach(3, 2, 3, 1, 0); rig.Tree.Attach(2, 4, 5, 1, 0); rig.Tree.Attach(4, 1, 1, 2, 0);
        sbyte excluded = 11; rig.Lifetime.KillExceptType(ref excluded); rig.Lifetime.Cleanup();
        Assert.Equal(new uint[] { 4, 2 }, rig.Freed); Assert.Equal(2U, rig.State.Count); Assert.Equal(4U, rig.State.Serial); Assert.Equal(3U, rig.Nodes[1].Child);
        rig.Create.Construct(5, 54, 64, 48, 1); uint temporaryId = rig.Nodes[5].Id; rig.State.Entries[temporaryId] = 0; rig.Nodes[5].Id = 0;
        float x = rig.Nodes[3].X; float y = rig.Nodes[3].Y; rig.Tree.Push(3); rig.Tree.Attach(3, x, y, 5, 0); rig.Lifetime.ReplaceRoot(5);
        Assert.Equal(new uint[] { 4, 2, 1 }, rig.Freed); Assert.Equal(5U, rig.State.Entries[0]); Assert.Equal(5U, rig.State.Collection.Root);
        Assert.Equal(2U, rig.State.Count); Assert.Equal(5U, rig.State.Serial); Assert.Equal(0U, rig.Nodes[5].Id); Assert.Equal(3U, rig.Nodes[5].Child); Assert.Equal(5U, rig.Nodes[3].Parent);
        Assert.Equal(0U, rig.LifeState.KillCount); Assert.Equal(0U, rig.State.Entries[temporaryId]);
        rig.Create.Construct(6, 21, 4, 3, 1); Assert.Equal(temporaryId, rig.Nodes[6].Id); Assert.Equal(6U, rig.Nodes[6].Serial); Assert.Equal(3U, rig.State.Count);
    }
}
