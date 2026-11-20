using System.Globalization;
using Netstorm.Client.Input;
using Netstorm.Tests;

// 기존 PE 기대값의 사건 인자와 전체 전역을 독립 입력마다 재생한다.
namespace Netstorm.Client.Tests;

/// <summary>강조 모드 본체와 장기 선택 owner의 실제 필드/배열 수명을 검증한다.</summary>
public sealed class UserHighlightTests
{
    /// <summary>십진수 DWORD 목록을 원문 순서로 읽는다.</summary>
    private static uint[] Numbers(string value) => value.Split(',').Select(X86Fixture.UInt).ToArray();

    /// <summary>원본 178입력의 모든 사건과 다섯 최종 전역을 전수 대조한다.</summary>
    [Fact]
    public void EveryNativeHighlightSequenceMatches()
    {
        string[][] rows = X86Fixture.Read("user-highlight"); Assert.Equal(178, rows.Length); X86Fixture.CheckRows(rows, Replay);
    }

    /// <summary>관찰에 지정된 경계 변경만 공급하며 기대 분기나 출력은 만들지 않는다.</summary>
    private static bool Replay(string[] row)
    {
        uint[] input = Numbers(row[0]); var state = new ClientUserSelectionGlobals(); var events = new List<string>();
        state.Mode.Value = input[1]; state.Selected.Value = input[3]; state.MouseType.Value = input[4];
        state.MouseX.Value = unchecked((int)input[5]); state.MouseY.Value = unchecked((int)input[6]);
        // 원본 관찰과 같은 전체 DWORD 사건 셀을 기록한다.
        void Event(string name, params uint[] values) => events.Add(name + ":" + string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture))));
        // 명시된 한 외부 경계가 현재 전역을 바꾼다. 부모의 재조회는 제품 본체가 담당한다.
        void Mutate(uint boundary)
        {
            if (input[7] != boundary) return;
            state.Mode.Value = 7; state.Selected.Value = 202; state.MouseType.Value = 77; state.MouseX.Value = -33; state.MouseY.Value = 65;
        }
        var highlight = new RawUserHighlight(state.HighlightState(), new()
        {
            SelectCursor = cursor => { Event("K", cursor, state.MouseType.Value); Mutate(1); },
            Project = (x, y) => { Event("P", unchecked((uint)x), unchecked((uint)y), state.MouseType.Value); Mutate(2); return (input[8], input[9]); },
            Move = (sid, x, y, force) => { Event("M", sid, x, y, force); Mutate(3); },
        });
        if (input[0] == 0) highlight.ResetMouse(); else highlight.Highlight(input[2]);
        uint[] current = [state.Mode.Value, state.Selected.Value, state.MouseType.Value, unchecked((uint)state.MouseX.Value), unchecked((uint)state.MouseY.Value)];
        return (events.Count == 0 ? "-" : string.Join(';', events)) == row[1] && current.SequenceEqual(Numbers(row[2]));
    }

    /// <summary>같은 모드는 외부 장치를 요구하지 않고 Reset은 마우스 TYPE만 지운다.</summary>
    [Fact]
    public void SameModeAndResetKeepOtherCurrentFields()
    {
        var state = new ClientUserSelectionGlobals(); state.Mode.Value = 5; state.Selected.Value = uint.MaxValue; state.MouseType.Value = 71; state.MouseX.Value = -33; state.MouseY.Value = 65;
        var highlight = new RawUserHighlight(state.HighlightState(), new()); highlight.Highlight(5); Assert.Equal(71u, state.MouseType.Value);
        highlight.ResetMouse(); Assert.Equal(0u, state.MouseType.Value); Assert.Equal(5u, state.Mode.Value); Assert.Equal(uint.MaxValue, state.Selected.Value); Assert.Equal(-33, state.MouseX.Value); Assert.Equal(65, state.MouseY.Value);
    }

    /// <summary>선택된 커서/역투영 누락은 mode/TYPE 선행 쓰기를 보존한다.</summary>
    [Theory]
    [InlineData(0u)]
    [InlineData(5u)]
    public void MissingSelectedDeviceKeepsWrittenPrefix(uint mode)
    {
        var state = new ClientUserSelectionGlobals(); state.Mode.Value = 9; state.Selected.Value = 73; state.MouseType.Value = 81;
        var highlight = new RawUserHighlight(state.HighlightState(), new()); Assert.Throws<InvalidOperationException>(() => highlight.Highlight(mode));
        Assert.Equal(mode, state.Mode.Value); Assert.Equal(0u, state.MouseType.Value); Assert.Equal(73u, state.Selected.Value);
    }

    /// <summary>역투영이 바꾼 현재 전역은 이동 장치 실패에서도 되돌리지 않는다.</summary>
    [Fact]
    public void MissingMoveKeepsProjectionEffects()
    {
        var state = new ClientUserSelectionGlobals(); state.Mode.Value = 4; state.Selected.Value = 73; state.MouseType.Value = 81; state.MouseX.Value = -1; state.MouseY.Value = 2; int calls = 0;
        var highlight = new RawUserHighlight(state.HighlightState(), new()
        {
            Project = (x, y) => { Assert.Equal(-1, x); Assert.Equal(2, y); Assert.Equal(0u, state.MouseType.Value); calls++; state.Selected.Value = 99; state.Mode.Value = 6; return (0x80000000u, 0x7fc01234u); },
        });
        Assert.Throws<InvalidOperationException>(() => highlight.Highlight(5)); Assert.Equal(1, calls); Assert.Equal(6u, state.Mode.Value); Assert.Equal(99u, state.Selected.Value); Assert.Equal(0u, state.MouseType.Value);
    }

    /// <summary>다중 배열의 안정 압축·꼬리 보존·중복·가득참과 음수 실패 경계를 확인한다.</summary>
    [Fact]
    public void CurrentMultipleArrayKeepsNativeCapacityAndTail()
    {
        var state = new ClientUserSelectionGlobals { Multiple = [10, 20, 10, 30, 777] }; uint[] physical = state.Multiple; state.MultipleCount.Value = 4;
        state.RemoveMultiple(10); Assert.Equal(2, state.MultipleCount.Value); Assert.Equal(new uint[] { 20, 30, 10, 30, 777 }, physical);
        state.AppendMultiple(20); Assert.Equal(2, state.MultipleCount.Value); state.AppendMultiple(99); Assert.Equal(99u, physical[2]); Assert.Equal(3, state.MultipleCount.Value);
        state.MultipleCount.Value = physical.Length; uint[] before = physical.ToArray(); state.AppendMultiple(123); Assert.Same(physical, state.Multiple); Assert.Equal(before, physical);
        state.MultipleCount.Value = -1; state.RemoveMultiple(20); Assert.Equal(-1, state.MultipleCount.Value); Assert.Throws<ArgumentOutOfRangeException>(() => state.AppendMultiple(20)); Assert.Equal(before, physical);
        state.MultipleCount.Value = physical.Length + 1; Assert.Throws<ArgumentOutOfRangeException>(() => state.RemoveMultiple(20)); Assert.Equal(before, physical);
    }

    /// <summary>실제 signed 필드와 비정규 DWORD를 보존하며 fresh reset도 P/R 누적값을 유지한다.</summary>
    [Fact]
    public void DwordFieldsAndGeneralResetKeepNativeOwnership()
    {
        var state = new ClientUserSelectionGlobals(); Assert.Equal(1u, state.Enabled.Value); uint[] physical = state.Multiple;
        state.WriteDword(ClientUserSelectionGlobals.Field.Enabled, 0x80000000); state.WriteDword(ClientUserSelectionGlobals.Field.MouseX, uint.MaxValue); state.WriteDword(ClientUserSelectionGlobals.Field.MultipleCount, uint.MaxValue);
        Assert.Equal(0x80000000u, state.Enabled.Value); Assert.Equal(-1, state.MouseX.Value); Assert.Equal(-1, state.MultipleCount.Value); Assert.Equal(uint.MaxValue, state.ReadDword(ClientUserSelectionGlobals.Field.MultipleCount));
        state.PriestSelections.Value = 0xdeadbeef; state.Selected.Value = uint.MaxValue; state.Remembered[0] = 0x7fc01234; physical[29] = 91;
        UserHighlightState view = state.HighlightState(); state.ResetForGeneralWorld(); Assert.Same(physical, state.Multiple); Assert.Same(state.Selected, view.SelectedSid);
        Assert.Equal(0u, view.SelectedSid.Value); Assert.Equal(1u, state.Enabled.Value); Assert.Equal(0, state.MouseX.Value); Assert.Equal(0, state.MultipleCount.Value); Assert.Equal(0u, state.Remembered[0]); Assert.Equal(0u, physical[29]); Assert.Equal(0xdeadbeefu, state.PriestSelections.Value);
    }

    /// <summary>같은 Gameplay의 준비 대여/반납이 장기 선택 값을 암묵 초기화하지 않는다.</summary>
    [Fact]
    public void PreparedGameplayUsePreservesActualSelectionOwner()
    {
        var game = new ClientGameplayGlobals(); var state = game.Selection; state.Mode.Value = 5; state.Selected.Value = 0xabcdef12; state.MultipleCount.Value = 3; state.PriestSelections.Value = 71;
        using (game.AcquirePrepared()) { Assert.Same(state, game.Selection); Assert.Equal(3, state.MultipleCount.Value); }
        Assert.Same(state, game.Selection); Assert.Equal(5u, state.Mode.Value); Assert.Equal(0xabcdef12u, state.Selected.Value); Assert.Equal(3, state.MultipleCount.Value); Assert.Equal(71u, state.PriestSelections.Value);
    }
}
