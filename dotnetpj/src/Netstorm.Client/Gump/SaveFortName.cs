using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Client.Gump;

/// <summary>현재 문자 입력의 원본 256바이트 복사 장치다.</summary>
public delegate void CopyFortNameText(Span<byte> destination);

/// <summary>이름 확인이 사용하는 실제 입력·파일·현재 Template 장치다.</summary>
public sealed class SaveFortNameHooks
{
    public CopyFortNameText? CopyText { get; init; }
    public Func<sbyte, bool>? Alnum { get; init; }
    public Func<string, string>? Path { get; init; }
    public Func<string, int>? Exists { get; init; }
    public Func<Memory<byte>>? Header { get; init; }
    public Action<string>? SetName { get; init; }
    public Action<string>? ReadFort { get; init; }
    public Action<string>? Tell { get; init; }
}

/// <summary>Fort 이름 입력의 확인·취소 본문이다. 근거: cpppj SaveFortNameDialog.cpp.</summary>
public static class SaveFortName
{
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Save as의 실제 장치 미연결");
    /// <summary>복사한 현재 입력을 정리하고 존재 검사 뒤 실제 이름 변경 또는 읽기를 요청한다.</summary>
    public static uint Confirm(uint fortMode, SaveFortNameHooks hooks) => Confirm(() => fortMode, hooks);
    /// <summary>파일 존재 검사 뒤 현재 Fort 모드를 다시 읽어 편집·읽기 경로를 선택한다.</summary>
    public static uint Confirm(Func<uint> fortMode, SaveFortNameHooks hooks)
    {
        Span<byte> text = stackalloc byte[256]; Need(hooks.CopyText)(text);
        int end = text.IndexOf((byte)0); if (end < 0) throw new InvalidOperationException("Save as 실제 편집문자열 NUL 누락");
        int newline = text[..end].IndexOf((byte)'\n'); if (newline >= 0) { text[newline] = 0; end = newline; }
        if (end == 0) return 0;
        for (int i = 0; i < end; i++) if (!Need(hooks.Alnum)(unchecked((sbyte)text[i])) && text[i] != '_' && text[i] != ' ') text[i] = (byte)' ';
        string name = Encoding.Latin1.GetString(text[..end]); string path = Need(hooks.Path)(name);
        if (Need(hooks.Exists)(path) != 0) { Need(hooks.Tell)("NameRequired"); return 0; }
        if (fortMode() != 0)
        {
            Memory<byte> header = Need(hooks.Header)(); if (header.Length < 12) throw new ArgumentOutOfRangeException(nameof(hooks));
            if (BinaryPrimitives.ReadUInt32LittleEndian(header.Span) != 0) BinaryPrimitives.WriteUInt32LittleEndian(header.Span[8..], 0);
            Need(hooks.SetName)(name); return 1;
        }
        Need(hooks.ReadFort)(name); return 1;
    }
    /// <summary>취소가 허용되지 않으면 원본의 이름 필요 안내를 연다.</summary>
    public static uint Cancel(uint canCancel, Action<string>? tell)
    { if (canCancel != 0) return 1; Need(tell)("NameRequired"); return 0; }
}
