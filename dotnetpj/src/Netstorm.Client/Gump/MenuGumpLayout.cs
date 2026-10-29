using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 메뉴 항목의 추가·소유 문자열·측정·두 방향 레이아웃 영역이다.

/// <summary>같은 항목 소유 head에 BYTE 스타일 문자열/문맥을 파싱한다.</summary>
public delegate void MenuParseStyle(uint token, int index, ReadOnlySpan<byte> text, uint[] context);
/// <summary>현재 실제 메뉴와 일반/스타일/자식 표시 측정·제목·부착 경계.</summary>
public sealed class MenuGumpLayoutHooks
{
    /// <summary>현재 메뉴.</summary>
    public Func<uint, MenuGumpNode?>? Node { get; set; }
    /// <summary>같은 기본 노드와 제목/자식.</summary>
    public Func<uint, GumpCreateNode?>? BaseNode { get; set; }
    /// <summary>현재 일반 항목 크기.</summary>
    public Func<uint, int, uint[], (int Width, int Height)>? Plain { get; set; }
    /// <summary>현재 소유 스타일 조각 크기.</summary>
    public Func<uint, int, (int Width, int Height)>? Styled { get; set; }
    /// <summary>현재 문맥의 자식 표시 두BYTE 측정.</summary>
    public Func<uint, uint[], (int Width, int Height)>? Marker { get; set; }
    /// <summary>이전 실제 소유 head를 비운다.</summary>
    public Action<uint, int>? ClearStyle { get; set; }
    /// <summary>새 소유 조각을 파싱한다.</summary>
    public MenuParseStyle? ParseStyle { get; set; }
    /// <summary>현재 제목 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>현재 제목 가상 Fit.</summary>
    public Action<uint, uint>? TitleFit { get; set; }
    /// <summary>현재 제목 가상 Layout.</summary>
    public Action<uint, uint, uint>? TitleLayout { get; set; }
    /// <summary>현재 실제 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
}
/// <summary>cpppj/src/client/MenuGumpLayout.cpp의 현재 항목 재조회·부분 쓰기·DWORD/float 저장 순서.</summary>
public sealed class MenuGumpLayout
{
    /// <summary>현재 등록/판본.</summary>
    private readonly GumpCreateState _create;
    /// <summary>같은 실제 크기/장식 처리기.</summary>
    private readonly MenuGump _menu;
    /// <summary>현재 측정/소유/제목/부착.</summary>
    private readonly MenuGumpLayoutHooks _hooks;
    /// <summary>실제 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("메뉴 레이아웃 경계 필요");
    /// <summary>원본 signed 항목 수.</summary>
    private static int Count(MenuGumpNode n) => unchecked((int)n.ItemCount);
    /// <summary>같은 실제 파생/기본 조회를 빌린다.</summary>
    public MenuGumpLayout(GumpCreateState create, MenuGump menu, MenuGumpLayoutHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(menu); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.BaseNode); _create = create; _menu = menu; _hooks = hooks; }
    /// <summary>기본/파생 메뉴가 같은 객체인지 검사한다.</summary>
    private MenuGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.BaseNode!(token))) throw new InvalidOperationException("메뉴 레이아웃 공유 노드 없음"); return n; }
    /// <summary>도달한 현재 자식/제목.</summary>
    private GumpCreateNode BaseNode(uint token) => (token != 0 ? _hooks.BaseNode!(token) : null) ?? throw new InvalidOperationException("메뉴 자식/제목 없음");
    /// <summary>손상된 개수도 고정 항목 배열 밖은 접근하지 않는다.</summary>
    private MenuGumpItem Item(uint token, int index) { var n = Node(token); return index >= 0 && index < n.Items.Count ? n.Items[index] : throw new ArgumentOutOfRangeException(nameof(index)); }
    /// <summary>원본 C문자열의 알려진 BYTE 범위.종료가 없으면 포인터 계약 밖이다.</summary>
    private static ReadOnlySpan<byte> CString(byte[] text) { int end = text.AsSpan().IndexOf((byte)0); if (end < 0) throw new ArgumentException("메뉴 문자열 NUL 범위 필요"); return text.AsSpan(0, end); }
    /// <summary>40개 한도/구분선 생략과 조건부 필드/자식 숨김·부착 뒤 현재 항목 문자열을 쓰고 전체를 다시 배치한다.</summary>
    public void Append(uint token, byte[]? text, uint flags, uint command = 0, uint parameter = 0, uint child = 0)
    {
        var n = Node(token); if (Count(n) >= MenuGump.ItemCapacity) return;
        if ((flags & 0x10) != 0) { if (child != 0 || (text is { Length: > 0 } && text[0] != 0)) throw new ArgumentException("메뉴 구분선의 빈 문자열/자식 없음 필요"); if (Count(n) < 1 || (Item(token, unchecked((int)n.ItemCount - 1)).Flags & 0x10) != 0) return; }
        if ((flags & 1) != 0) Item(token, unchecked((int)n.ItemCount)).Command = command; if ((flags & 0x80) != 0) Item(token, unchecked((int)n.ItemCount)).Parameter = parameter;
        if (child != 0)
        {
            var below = BaseNode(child);
            if ((flags & 0x800) != 0) Item(token, unchecked((int)n.ItemCount)).ChildId = below.Id;
            else { if (_create.Collection.Edition == OriginalEdition.Patch1078) { Item(token, unchecked((int)n.ItemCount)).ChildId = below.Id; below.Flags &= ~1U; } else { below.Flags &= ~1U; Item(token, unchecked((int)n.ItemCount)).ChildId = below.Id; } Require(_hooks.Attach)(child, 0, 0, token, 0); flags |= 0x20; }
        }
        Item(token, unchecked((int)n.ItemCount)).Flags = flags | 0x10000; int index = unchecked((int)n.ItemCount); uint[] context = (uint[])n.TextContext.Clone(); SetText(token, index, text, context); n.ItemCount = unchecked(n.ItemCount + 1); if (Count(n) > MenuGump.ItemCapacity) throw new ArgumentException("메뉴 추가 후 항목 수 한도 초과"); Reflow(token);
    }
    /// <summary>일반은 strncpy40,스타일은 문맥font0 복사→Clear→현재 disabled 접두→현재 입력 파싱.</summary>
    public void SetText(uint token, int index, byte[]? text, uint[] context)
    {
        var item = Item(token, index); if (text is null) throw new ArgumentException("메뉴 항목 문자열 필요");
        if ((item.Flags & 0x400) == 0)
        {
            bool ended = false;
            // 종료 뒤는0을 채우며40바이트 입력에는 추가NUL을 쓰지 않는다.알려진 범위 밖 원본 포인터 읽기는 거부한다.
            for (int i = 0; i < item.Label.Length; i++) { if (!ended && i >= text.Length) throw new ArgumentException("메뉴 strncpy 입력 범위 부족"); byte value = ended ? (byte)0 : text[i]; item.Label[i] = value; if (value == 0) ended = true; } return;
        }
        uint[] copy = (uint[])context.Clone(); copy[1] = 0; Require(_hooks.ClearStyle)(token, index); ReadOnlySpan<byte> prefix = (item.Flags & 0x200) != 0 ? "~*~x"u8 : []; ReadOnlySpan<byte> input = CString(text); if (prefix.Length + input.Length >= 256) throw new ArgumentException("메뉴 StyleText 원본256바이트 한도 초과"); byte[] source = new byte[prefix.Length + input.Length + 1]; prefix.CopyTo(source); input.CopyTo(source.AsSpan(prefix.Length)); Require(_hooks.ParseStyle)(token, index, source, copy);
    }
    /// <summary>측정 시점의 현재 플래그로 실제 스타일/일반 경계를 선택한다.</summary>
    public (int Width, int Height) MeasureItem(uint token, int index, uint[] context) => (Item(token, index).Flags & 0x400) != 0 ? Require(_hooks.Styled)(token, index) : Require(_hooks.Plain)(token, index, context);
    /// <summary>현재 항목 수/문맥/방향을 매 콜백 뒤 다시 읽어 크기·높이·offset을 쓴다.남는 칸은 보존한다.</summary>
    public void Reflow(uint token)
    {
        var n = Node(token); n.Margins[0] = (n.MenuFlags & 0x80000) != 0 ? 15 : 4; n.Margins[1] = (n.MenuFlags & 0x80000) != 0 ? 15 : 4; n.Margins[2] = n.Margins[3] = 1; if ((n.MenuFlags & 0x1000000) != 0) n.Margins[0] = n.Margins[1] = 3; int width = 0, height = 0, total = 0;
        // 현재 signed 항목 수를 다시 읽고 각 항목의 실제32바이트 문맥을 복사한다.
        for (int i = 0; i < Count(n); i++)
        {
            uint[] context = (uint[])n.TextContext.Clone(); var size = MeasureItem(token, i, context);
            if ((n.MenuFlags & 0x80000) != 0 && (Item(token, i).Flags & (0x20 | 0x800)) != 0) { var marker = Require(_hooks.Marker)(token, n.TextContext); size.Width = unchecked(size.Width + marker.Width); if (size.Height <= marker.Height) size.Height = marker.Height; }
            size.Width = GumpCollection.Ftol((double)size.Width + n.Margins[0] + n.Margins[1]); size.Height = GumpCollection.Ftol((double)size.Height + n.Margins[2] + n.Margins[3]); width = Math.Max(width, size.Width); height = Math.Max(height, size.Height); total = unchecked(total + size.Height); n.Heights[i] = size.Height;
        }
        if ((n.MenuFlags & 0x800000) != 0)
        {
            if ((n.MenuFlags & 0x40000) != 0) throw new ArgumentException("장식 메뉴의 세로 방향 필요"); Require(_hooks.Push)(n.Title); Require(_hooks.TitleFit)(n.Title, 0); Require(_hooks.TitleLayout)(n.Title, 0, 0); uint title = n.Title; var above = BaseNode(title); double widest = (double)width > above.BaseWidth ? (double)width : above.BaseWidth; width = GumpCollection.Ftol(widest); Require(_hooks.Attach)(title, above.X, above.Y, 0, 0);
        }
        if ((n.MenuFlags & 0x80000) != 0)
        {
            if ((n.MenuFlags & 0x40000) != 0) throw new ArgumentException("세로 메뉴의 가로 비트 없음 필요"); n.TargetWidth = width; n.TargetHeight = total; _menu.SetWidth(token, n.TargetWidth); _menu.SetHeight(token, n.TargetHeight); int offset = 0;
            // 크기 변경 콜백 뒤 현재 개수/측정 높이로 누적 위치를 쓴다.
            for (int i = 0; i < Count(n); i++) { Item(token, i); n.Offsets[i] = offset; offset = GumpCollection.Ftol((double)offset + n.Heights[i]); }
        }
        else if ((n.MenuFlags & 0x40000) != 0)
        {
            n.TargetWidth = unchecked((int)(n.ItemCount * unchecked((uint)width))); n.TargetHeight = height; float commonHeight = height; _menu.SetWidth(token, n.TargetWidth); _menu.SetHeight(token, n.TargetHeight);
            // 현재 항목 높이를 같게 맞추고 세로offset은0을 쓴다.
            for (int i = 0; i < Count(n); i++) { Item(token, i); n.Heights[i] = commonHeight; n.Offsets[i] = 0; }
        }
        n.ItemWidth = width;
    }
    /// <summary>추가NUL 없이 라벨 뒤 다섯 DWORD를 실제 순서의 little endian으로 직렬화한다.</summary>
    public static byte[] MenuGumpItemBytes(MenuGumpItem item)
    {
        byte[] result = new byte[60]; item.Label.CopyTo(result, 0); uint[] words = [item.ChildId, item.Command, item.Parameter, item.Styled, item.Flags];
        // 측정기가 읽을 수 있는 실제 공통 필드까지 포함한다.
        for (int i = 0; i < words.Length; i++) BinaryPrimitives.WriteUInt32LittleEndian(result.AsSpan(40 + i * 4), words[i]); return result;
    }
    /// <summary>실제60바이트 항목과 자식 표시를 같은 FontStore/GumpText로 측정한다.</summary>
    public static void BindMenuLayoutText(MenuGumpLayoutHooks hooks, GumpText text)
    {
        var lookup = Require(hooks.Node); hooks.Plain = (token, index, context) => { var n = lookup(token); if (n is null || index < 0 || index >= n.Items.Count) throw new ArgumentOutOfRangeException(nameof(index)); int width = 0, height = 0; text.Measure(MenuGumpItemBytes(n.Items[index]), GumpText.DecodeGumpTextContext(context), ref width, ref height); return (width, height); }; hooks.Marker = (_, context) => { int width = 0, height = 0; text.Measure(" >\0"u8, GumpText.DecodeGumpTextContext(context), ref width, ref height); return (width, height); };
    }
}
