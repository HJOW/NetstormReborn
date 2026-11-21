using Netstorm.Core.Original;
using Netstorm.Assets;

// 같은 초기 점 명령 등록과 현재 공유 치트 DWORD의 실제 효과를 처리한다.
namespace Netstorm.Client.Scripting;

/// <summary>선택된 flash/음향/설명만 호출 위치에서 요구하는 실제 장치다.</summary>
public sealed class IssueCommandHooks
{
    /// <summary>원문 atol 밝기 또는 인자 부재0을 실제 번쩍임에 전달한다.</summary>
    public Action<int>? Flash { get; set; }
    /// <summary>번쩍임 뒤 원문 thunderQuietDistant.wav를 재생한다.</summary>
    public Action<string>? Sound { get; set; }
    /// <summary>원문0042c040(-1,arg,0)의 실제 설명 요청이다.</summary>
    public Action<int, string?, uint>? Description { get; set; }
}
/// <summary>004d53a0의 네 등록과 flash/desc/cheatorama 본체다.</summary>
public static class RawIssueCommands
{
    /// <summary>원문이 유일하게 허용하는 치트 DWORD다.</summary>
    private const uint CheatValue = 0x845fed;
    /// <summary>실제로 선택한 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("초기 점 명령의 선택 장치 미연결");
    /// <summary>중복 flash 두 번을 포함한 원문 네 등록을 같은 순서로 실행한다.</summary>
    public static void RegisterIssueCommands(Action<string, uint, uint>? registration)
    {
        Need(registration)("flash", 0x4d5300, 0); Need(registration)("flash", 0x4d5300, 0); Need(registration)("desc", 0x4d5340, 1); Need(registration)("cheatorama", 0x4d5360, 1);
    }
    /// <summary>알려진 명령은0, 알 수 없는 callback은 null을 반환한다. 선택 효과 실패 전 쓰기는 보존한다.</summary>
    public static uint? InvokeIssueCommand(uint callback, string? argument, ref uint cheat, IssueCommandHooks hooks)
    {
        if (callback == 0x4d5300) { Need(hooks.Flash)(argument is null ? 0 : ConfigNumbers.ParseLong(argument)); Need(hooks.Sound)("thunderQuietDistant.wav"); return 0; }
        if (callback == 0x4d5340) { Need(hooks.Description)(-1, argument, 0); return 0; }
        if (callback == 0x4d5360)
        {
            if (argument is null) throw new ArgumentException("cheatorama 원문의 실제 인자 포인터 없음", nameof(argument));
            if (unchecked((uint)ConfigNumbers.ParseLong(RawTextToken.Read(argument).Text)) == CheatValue) cheat = CheatValue;
            return 0;
        }
        return null;
    }
}
