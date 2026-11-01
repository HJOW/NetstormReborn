using System.Buffers.Binary;
using Netstorm.Client.Input;

namespace Netstorm.Client.Mission;

/// <summary>현재 맵 선택 모드와 같은 State·Player 전역이다.</summary>
public sealed class FortSelectionGlobals
{
    public uint Mode { get; set; }
    public uint[] Next { get; }
    /// <summary>Client가 이미 소유한 144바이트 다음 State를 그대로 빌린다.</summary>
    public FortSelectionGlobals(uint[]? next = null)
    {
        Next = next ?? new uint[36];
        if (Next.Length != 36) throw new ArgumentException("Fort 선택의 실제 State144BYTE 크기 오류", nameof(next));
    }
    public uint InBattle { get; set; }
    public uint CurrentState { get; set; }
    public uint TemplateOwner { get; set; }
    public uint Owner { get; set; }
}

/// <summary>실제 맵 파일·목록·State 및 선택 장치다.</summary>
public sealed class FortSelectionHooks
{
    public Func<uint, string>? ResolveString { get; init; }
    public Func<string>? LastEditPath { get; init; }
    public Action<string>? Pending { get; init; }
    public Action<int>? Refresh { get; init; }
    public Action<string>? Tell { get; init; }
    public Func<string>? CurrentFortPath { get; init; }
    public Action<uint, string>? LoadFort { get; init; }
    public Func<uint, Memory<byte>>? Core { get; init; }
    public Func<double>? GameSeconds { get; init; }
    public Func<uint, int>? OwnedIslands { get; init; }
    public Action<uint>? Notify { get; init; }
}

/// <summary>편집 선택의 현재 입력과 지연 State 예약이다. 근거: cpppj FortSelection.cpp.</summary>
public static class FortSelection
{
    public const uint AskEditFort = 0x020000c9, ReadFort = 0x020000ca;
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Fort 선택의 실제 효과 미연결");
    private static Memory<byte> Core(FortSelectionHooks hooks, uint slot) { Memory<byte> bytes = Need(hooks.Core)(slot); return bytes.Length >= 16 ? bytes : throw new ArgumentOutOfRangeException(nameof(slot)); }
    private static uint TimeInteger(double value) => !double.IsFinite(value) || value >= 9223372036854775808.0 || value < -9223372036854775808.0 ? 0u : unchecked((uint)(long)value);

    /// <summary>첫 NUL 앞의 점을 진단하고 복귀하면 현재 이름을 반환한다.</summary>
    public static string PendingName(string name, Action? assertion = null)
    {
        int nul = name.IndexOf('\0'); if (nul >= 0) name = name[..nul];
        if (name.Contains('.')) Need(assertion)(); return name;
    }

    /// <summary>일반 맵 읽기 State6을 같은 다음 블록에 예약한다.</summary>
    public static void RequestRead(FortSelectionGlobals state) => state.Next[0] = 6;

    /// <summary>Editor State1을 예약하고 전투 복귀일 때 현재 파일과 State를 원본 순서로 갱신한다.</summary>
    public static void RequestEdit(FortSelectionGlobals state, FortSelectionHooks hooks)
    {
        state.Next[0] = 1;
        if (state.InBattle == 0 || state.CurrentState != 14) return;
        string path = Need(hooks.CurrentFortPath)(); Need(hooks.LoadFort)(state.TemplateOwner, path);
        Core(hooks, state.TemplateOwner).Span[0] &= 0xfd;
        uint stamp = TimeInteger(Need(hooks.GameSeconds)()); BinaryPrimitives.WriteUInt32LittleEndian(Core(hooks, state.TemplateOwner).Span[12..], stamp);
        if (Need(hooks.OwnedIslands)(state.Owner) < 1) { Core(hooks, state.TemplateOwner).Span[0] &= 0xfb; Need(hooks.Notify)(state.TemplateOwner); }
    }

    /// <summary>AskEditFort·ReadFort를 현재 전역과 실제 선택 장치에 분배한다.</summary>
    public static bool Dispatch(InputEvent input, FortSelectionGlobals state, FortSelectionHooks hooks)
    {
        if ((input.Code & InputCode.Release) != 0) return false;
        switch (input.Code & 0x03ffffff)
        {
            case AskEditFort:
                state.Mode = 2; string path = Need(hooks.LastEditPath)(); Need(hooks.Pending)(path); Need(hooks.Refresh)(2); Need(hooks.Tell)("LoadEditable"); return true;
            case ReadFort:
                Need(hooks.Pending)(Need(hooks.ResolveString)(unchecked((uint)input.Y)));
                if (state.Mode == 0) RequestRead(state); else RequestEdit(state, hooks); return true;
            default: return false;
        }
    }
}
