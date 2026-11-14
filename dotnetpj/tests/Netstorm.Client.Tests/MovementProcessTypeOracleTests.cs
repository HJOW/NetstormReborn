using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 실제 원본 선택 관찰과 장기TYPE 주소/명시 사본·별칭 대입을 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>기본718·전역/프로세스변경2872입력과 실제 현재 장기 owner 검사다.</summary>
public sealed class MovementProcessTypeOracleTests
{
    /// <summary>같은 판본의 타입 번호 표다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition) => Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
    /// <summary>전체TYPE×balloon의 실제 원본 결과/미지원CD진단을 재생한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 376)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 342)]
    public void DefaultSelectionMatchesNative(string name, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read("pathprocesstype", name); Assert.Equal(expectedRows, rows.Length); var types = Types(edition); var state = new RawMovementProcessTypes();
        X86Fixture.CheckRows(rows, row => { Assert.Equal(5, row.Length); uint type = X86Fixture.UInt(row[1]); types[type].Flags2 = X86Fixture.UInt(row[2]); if (row[4] != "0") Assert.Throws<ArgumentException>(() => RawMovementProcessTypes.Select(edition, types, state, type)); else Assert.Equal(X86Fixture.UInt(row[3]), RawMovementProcessTypes.Select(edition, types, state, type)); return true; });
    }
    /// <summary>사제/Walker별칭·현재프로세스변경·balloon우선순위를 실제 원본2872입력에 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 1504)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 1368)]
    public void AliasAndProcessShiftMatchNative(string name, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read("priestworldevents", name); Assert.Equal(expectedRows, rows.Length); var types = Types(edition);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(7, row.Length); var state = new RawMovementProcessTypes(); if (row[1] != "0") state.Priest = state.WindWalker; uint shift = X86Fixture.UInt(row[2]); state.WormProcess += shift; state.WalkerProcess += shift; state.BalloonProcess += shift; state.TeleportProcess += shift; state.BulfProcess += shift; state.DudeProcess += shift; state.PriestProcess += shift; state.FlyProcess += shift; uint type = X86Fixture.UInt(row[4]); types[type].Flags2 = X86Fixture.UInt(row[3]); if (row[6] != "0") Assert.Throws<ArgumentException>(() => RawMovementProcessTypes.Select(edition, types, state, type)); else Assert.Equal(X86Fixture.UInt(row[5]), RawMovementProcessTypes.Select(edition, types, state, type)); return true;
        });
    }
    /// <summary>장기 owner의 같은 상자를 빌리고 일반 월드 준비 뒤에도 현재값을 유지한다.</summary>
    [Fact]
    public void BorrowedCurrentRolesKeepOwnerAndWorldResetIdentity()
    {
        var owner = new RawGameAuxState(); owner.RainWalkerTypeBox.Value = 99; owner.BulfTypeBox.Value = 100; owner.TempleProductTypeBox.Value = 96; owner.EmptyCarrierTypeBox.Value = 97; owner.DudePathTypeBox.Value = 29; owner.PriestTypeBox.Value = 98; var first = new RawMovementProcessTypes(owner.PriestTypeBox, owner); var types = Types(OriginalEdition.Patch1078); types[99].Flags2 = 0x20000;
        Assert.Equal(first.WalkerProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, first, 99)); Assert.Equal(first.BulfProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, first, 100)); owner.RainWalkerTypeBox.Value = 101; owner.BulfTypeBox.Value = 102; owner.ResetForGeneralWorld(); var second = new RawMovementProcessTypes(owner.PriestTypeBox, owner);
        Assert.Equal(first.BalloonProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, first, 99)); Assert.Equal(second.WalkerProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, second, 101)); Assert.Equal(second.BulfProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, second, 102)); Assert.Same(owner.TempleProductTypeBox, first.SunWalkerBox); Assert.Same(owner.EmptyCarrierTypeBox, first.EmptyBox); Assert.Same(first.RainWalkerBox, second.RainWalkerBox); Assert.Same(owner.BulfTypeBox, first.BulfBox); Assert.Same(owner.DudePathTypeBox, second.DudeProcessBox); Assert.Same(owner.PriestTypeBox, first.PriestBox); Assert.Equal((96u, 97u, 29u, 98u), (second.SunWalker, second.Empty, second.DudeProcess, second.Priest));
    }
    /// <summary>명시 사본은 독립 주소이며 Assign은 목적지 주소/원문 순차 읽기·쓰기를 유지한다.</summary>
    [Fact]
    public void ExplicitCopyAndAssignmentKeepCorrectAddressSemantics()
    {
        var owner = new RawGameAuxState(); owner.RainWalkerTypeBox.Value = 99; var borrowed = new RawMovementProcessTypes(owner.PriestTypeBox, owner); var copied = new RawMovementProcessTypes(borrowed); owner.RainWalkerTypeBox.Value = 100; Assert.Equal(99u, copied.RainWalker); Assert.NotSame(copied.RainWalkerBox, borrowed.RainWalkerBox); Assert.NotSame(copied.PriestBox, borrowed.PriestBox); var independent = new RawMovementProcessTypes { RainWalker = 101, Priest = 70, DudeProcess = 31, FlyProcess = 88 }; borrowed.Assign(independent); Assert.Same(owner.RainWalkerTypeBox, borrowed.RainWalkerBox); Assert.Equal((101u, 70u, 31u, 88u), (owner.RainWalkerTypeBox.Value, owner.PriestTypeBox.Value, owner.DudePathTypeBox.Value, borrowed.FlyProcess)); Assert.Equal(99u, copied.RainWalker);
        var left = new RawGameAuxState(); var right = new RawGameAuxState(); left.TempleProductTypeBox.Value = 96; right.TempleProductTypeBox.Value = 133; var source = new RawMovementProcessTypes(right.TempleProductTypeBox, left); var destination = new RawMovementProcessTypes(left.TempleProductTypeBox, right); destination.Assign(source); Assert.Equal((96u, 96u), (left.TempleProductTypeBox.Value, right.TempleProductTypeBox.Value)); destination.Assign(destination); Assert.Equal(96u, destination.Priest);
    }
    /// <summary>CD의선행sunWalker·PATCH선행타입검사와 우선순위·전체DWORD 역할을 구별한다.</summary>
    [Fact]
    public void EditionLookupOrderAndFullCurrentTypeArePreserved()
    {
        var selection = new RawMovementProcessTypes { SunWalker = uint.MaxValue, WormProcess = 0x80000000 }; Assert.Equal(0x80000000u, RawMovementProcessTypes.Select(OriginalEdition.Cd1072, [], selection, uint.MaxValue)); Assert.Throws<ArgumentOutOfRangeException>(() => RawMovementProcessTypes.Select(OriginalEdition.Patch1078, [], selection, uint.MaxValue)); var types = Types(OriginalEdition.Patch1078); selection.SunWalker = 133; selection.Priest = selection.WindWalker; types[137].Flags2 = 0x20000; Assert.Equal(selection.WalkerProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, selection, 137)); selection.WindWalker = 99; Assert.Equal(selection.BalloonProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, selection, 137)); types[137].Flags2 = 0; Assert.Equal(selection.PriestProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, selection, 137)); selection.Priest = 0x1000009e; Assert.Equal(selection.WormProcess, RawMovementProcessTypes.Select(OriginalEdition.Patch1078, types, selection, 158)); Assert.Throws<ArgumentException>(() => RawMovementProcessTypes.Select(OriginalEdition.Cd1072, types, selection, 158));
    }
}
