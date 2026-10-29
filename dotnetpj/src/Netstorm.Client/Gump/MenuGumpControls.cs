using System.Globalization;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 controlled 메뉴의 항목·체크·제목·자식 문맥 작성 영역이다.

/// <summary>원본 ReadInt의 현재 signed DWORD를 참조로 갱신한다.누락 키는 값을 보존한다.</summary>
public delegate void MenuReadInt(string key, ref int value);
/// <summary>같은 실제 기본/메뉴 저장소와 현재 설정 읽기 경계.</summary>
public sealed class MenuGumpControlHooks
{
    /// <summary>현재 실제 메뉴.</summary>
    public Func<uint, MenuGumpNode?>? Node { get; set; }
    /// <summary>현재 같은 기본/제목/자식.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>설정 조회는 controlled 현재 칸도 변경할 수 있다.</summary>
    public MenuReadInt? ReadInt { get; set; }
}
/// <summary>cpppj/src/client/MenuGumpControls.cpp의 판본별 서식·색 조회·현재 칸 재조회와 실제 항목/제목 호출.</summary>
public sealed class MenuGumpControls
{
    /// <summary>원본 항목의 본문 비트.</summary>
    private const uint Text = 0x10000;
    /// <summary>원본 소유 StyleText 비트.</summary>
    private const uint Styled = 0x400;
    /// <summary>명시 매개변수 쓰기 비트.</summary>
    private const uint Parameter = 0x80;
    /// <summary>체크 표시 비트.</summary>
    private const uint Checked = 4;
    /// <summary>비활성 항목 비트.</summary>
    private const uint Disabled = 0x200;
    /// <summary>등록 ID로 연결한 하위 메뉴 비트.</summary>
    private const uint Child = 0x20;
    /// <summary>현재 강조 비트.</summary>
    private const uint Selected = 8;
    /// <summary>장식/별도 제목 메뉴 비트.</summary>
    private const uint Fancy = 0x800000;
    /// <summary>일반 항목의 원본 기본 플래그.</summary>
    private const uint NormalItem = Text | Styled | Parameter;
    /// <summary>자식 항목의 원본 기본 플래그.</summary>
    private const uint SubmenuItem = Text | Styled;
    /// <summary>구분선 플래그.</summary>
    private const uint SeparatorItem = 0x10010;
    /// <summary>원본 지역 서식 버퍼의 NUL 포함 한도.</summary>
    private const int FormatBytes = 256;
    /// <summary>현재 실제 등록/판본.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 controlled 칸/생성 개수.</summary>
    private readonly MenuGumpState _state;
    /// <summary>같은 실제 메뉴 본체.</summary>
    private readonly MenuGump _menu;
    /// <summary>같은 실제 항목/레이아웃.</summary>
    private readonly MenuGumpLayout _layout;
    /// <summary>같은 실제 다중 줄 제목 파서.</summary>
    private readonly ScrollingTextGumpMarkup _title;
    /// <summary>현재 기본/메뉴/설정 경계.</summary>
    private readonly MenuGumpControlHooks _hooks;
    /// <summary>같은 저장소/처리기를 빌린다.설정 경계는 실제 색 조회 시에만 요구한다.</summary>
    public MenuGumpControls(GumpCreateState create, MenuGumpState state, MenuGump menu, MenuGumpLayout layout, ScrollingTextGumpMarkup title, MenuGumpControlHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(menu); ArgumentNullException.ThrowIfNull(layout); ArgumentNullException.ThrowIfNull(title); ArgumentNullException.ThrowIfNull(hooks); if (hooks.Node is null || hooks.Base is null) throw new ArgumentException("controlled 메뉴 공유 저장소 필요"); _create = create; _state = state; _menu = menu; _layout = layout; _title = title; _hooks = hooks; }
    /// <summary>현재 실제 제목/자식.메뉴 아닌 기본 파생도 허용한다.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("controlled 기본 노드 없음");
    /// <summary>현재 메뉴와 기본 객체가 같은지 확인한다.</summary>
    private MenuGumpNode Node(uint token) { var node = token != 0 ? _hooks.Node!(token) : null; if (node is null || !ReferenceEquals(node, Base(token))) throw new InvalidOperationException("controlled 메뉴 공유 노드 없음"); return node; }
    /// <summary>항목 수와 별개로 실제40칸 접근 범위를 확인한다.</summary>
    private MenuGumpItem Item(uint token, int index) => index >= 0 && index < MenuGump.ItemCapacity ? Node(token).Items[index] : throw new ArgumentOutOfRangeException(nameof(index));
    /// <summary>등록0은 선택 조회에서 허용한다.도달한 배열 범위만 검사한다.</summary>
    private uint Registered(uint id) => id < _create.Entries.Length ? _create.Entries[id] : throw new ArgumentOutOfRangeException(nameof(id));
    /// <summary>현재 판본의 호출 규약을 고른다.</summary>
    private bool Patch() => _create.Collection.Edition == OriginalEdition.Patch1078;
    /// <summary>패치는 include 검사 전에도 현재 메뉴를 요구한다.</summary>
    private uint CurrentRequired() { uint token = _menu.Current(); Node(token); return token; }
    /// <summary>CD 전용 참고 몸체는 패치에서 변경 전에 거부한다.</summary>
    private void RequireCd() { if (Patch()) throw new ArgumentException("CD 전용 메뉴 helper"); }
    /// <summary>첫NUL 앞 원본 BYTE만 사용한다.호스트 문자열 인코딩으로 바꾸지 않는다.</summary>
    private static ReadOnlySpan<byte> CString(ReadOnlySpan<byte> source) { int end = source.IndexOf((byte)0); return end < 0 ? source : source[..end]; }
    /// <summary>원본 signed DWORD를 현재 문화권과 무관한 십진수로 쓴다.</summary>
    private static string Decimal(int value) => value.ToString(CultureInfo.InvariantCulture);
    /// <summary>고정 ASCII 접두/접미와 원본 입력 BYTE를 지역256바이트 결과로 결합한다.</summary>
    private static byte[] Format(string prefix, ReadOnlySpan<byte> source, string suffix = "")
    { source = CString(source); if (prefix.Length + source.Length + suffix.Length >= FormatBytes) throw new ArgumentException("메뉴 원본 포맷 버퍼 한도 초과"); byte[] result = new byte[prefix.Length + source.Length + suffix.Length + 1]; Encoding.ASCII.GetBytes(prefix, result); source.CopyTo(result.AsSpan(prefix.Length)); Encoding.ASCII.GetBytes(suffix, result.AsSpan(prefix.Length + source.Length)); return result; }
    /// <summary>256바이트 한도를 추가하지 않는 CD 원문 전달.원본 string_view처럼 첫NUL 뒤를 제외한다.</summary>
    private static byte[] Terminated(ReadOnlySpan<byte> source) => [.. CString(source), 0];
    /// <summary>추가 인자가 없는 printf 경로의 %%만 처리한다.다른 변환은 원본의 무효 스택 입력이다.</summary>
    private static byte[] NoArguments(ReadOnlySpan<byte> source)
    {
        source = CString(source); var result = new List<byte>();
        // %%를 한BYTE로 소비하며 원본 버퍼 밖 결과는 쓰기 전에 거부한다.
        for (int index = 0; index < source.Length; index++) { if (source[index] == '%') { if (index + 1 >= source.Length || source[index + 1] != '%') throw new ArgumentException("무인자 메뉴 서식에 변환 인자 없음"); index++; } if (result.Count + 1 >= FormatBytes) throw new ArgumentException("메뉴 원본 포맷 버퍼 한도 초과"); result.Add(source[index]); } result.Add(0); return result.ToArray();
    }
    /// <summary>현재 전경→배경을 각0 초기값으로 조회한다.두 조회의 상태 변경을 순서대로 반영한다.</summary>
    private int[] Colors(string foreground, string background)
    { if (_hooks.ReadInt is null) throw new ArgumentException("메뉴 색 설정 ReadInt 경계 필요"); int[] colors = new int[2]; _hooks.ReadInt(foreground, ref colors[0]); _hooks.ReadInt(background, ref colors[1]); return colors; }
    /// <summary>고정 원본 ~[C.%d]~[B.%d]~E%s.입력%는 다시 서식으로 해석하지 않는다.</summary>
    private static byte[] Colored(ReadOnlySpan<byte> source, int[] colors) => Format("~[C." + Decimal(colors[0]) + "]~[B." + Decimal(colors[1]) + "]~E", source);
    /// <summary>CD는 null 현재 칸/include false에서 생략한다.패치는 선검사/색 조회 뒤 현재 칸을 다시 읽는다.</summary>
    public void AddText(ReadOnlySpan<byte> text, uint command = 0, uint parameter = 0, bool include = true, bool enabled = true, bool forceColor = false)
    {
        if (!Patch()) { uint token = _menu.Current(); if (token == 0 || !include) return; _layout.Append(token, Terminated(text), NormalItem | (command != 0 ? 1U : 0) | (enabled ? 0 : Disabled), command, parameter); return; }
        CurrentRequired(); if (!include) return; int[] colors = Colors("MenuColor", "MenuBGColor"); byte[] source = enabled || forceColor ? Colored(text, colors) : NoArguments(text); _layout.Append(CurrentRequired(), source, NormalItem | (command != 0 ? 1U : 0) | (enabled ? 0 : Disabled), command, parameter);
    }
    /// <summary>패치 체크 항목은 command0에도 쓰기 비트를 세운다.CD는 원문과 미작성 명령 필드를 보존한다.</summary>
    public void AddChecked(bool check, ReadOnlySpan<byte> text, uint command, uint parameter = 0, bool include = true, bool enabled = true)
    {
        if (!Patch()) { uint token = _menu.Current(); if (token == 0 || !include) return; _layout.Append(token, Terminated(text), NormalItem | (command != 0 ? 1U : 0) | (check ? Checked : 0) | (enabled ? 0 : Disabled), command, parameter); return; }
        CurrentRequired(); if (!include) return; int[] colors = Colors("MenuCheckedColor", "MenuCheckedBGColor"); byte[] source = enabled ? Colored(text, colors) : NoArguments(text); _layout.Append(CurrentRequired(), source, NormalItem | 1U | (check ? Checked : 0) | (enabled ? 0 : Disabled), command, parameter);
    }
    /// <summary>판본별 제목/머리 서식 뒤 현재 fancy는 실제 다중 줄 Parse만,일반은 disabled 항목을 추가한다.</summary>
    private void Heading(ReadOnlySpan<byte> text, bool title)
    {
        byte[] source;
        if (Patch()) { int[] colors = title ? Colors("MenuTitleColor", "MenuTitleBGColor") : Colors("MenuHeaderColor", "MenuHeaderBGColor"); source = Format((title ? "~3~E" : "~E") + "~[C." + Decimal(colors[0]) + "]~[B." + Decimal(colors[1]) + "]", text, "~."); }
        else source = title ? Format("~3", text, "~.") : NoArguments(text);
        uint token = CurrentRequired(); if ((Node(token).MenuFlags & Fancy) != 0) { uint target = Node(token).Title; if (target == 0) throw new InvalidOperationException("fancy 메뉴 제목 없음"); _title.Parse(target, source); return; }
        if (!Patch()) { AddText(source, 0, 0, true, false); return; } int[] normal = Colors("MenuColor", "MenuBGColor"); source = Colored(source, normal); _layout.Append(CurrentRequired(), source, NormalItem | Disabled);
    }
    /// <summary>패치 ~3~E/색과 CD ~3을 구분하는 제목 추가.</summary>
    public void AddTitle(ReadOnlySpan<byte> text) => Heading(text, true);
    /// <summary>패치 ~E/색과 CD 무인자 서식의 머리 추가.</summary>
    public void AddHeader(ReadOnlySpan<byte> text) => Heading(text, false);
    /// <summary>실제 자식 부착은 Append에 맡긴다.패치는 항상 styled,CD는 첫NUL 앞~ 유무로 결정한다.</summary>
    public void AddSubmenu(ReadOnlySpan<byte> text, uint child, bool enabled = true, bool check = false)
    {
        if (!Patch()) { uint token = _menu.Current(); if (token == 0) return; text = CString(text); _layout.Append(token, Terminated(text), Text | (text.Contains((byte)'~') ? Styled : 0) | (enabled ? 0 : Disabled) | (check ? Checked : 0), 0, 0, child); return; }
        CurrentRequired(); int[] colors = Colors("MenuSubColor", "MenuSubBGColor"); byte[] source = enabled ? Colored(text, colors) : NoArguments(text); _layout.Append(CurrentRequired(), source, SubmenuItem | (enabled ? 0 : Disabled) | (check ? Checked : 0), 0, 0, child);
    }
    /// <summary>선두/연속/40개 한도는 실제 Append가 처리한다.미작성 명령/인자는 유지한다.</summary>
    public void AddSeparator() => _layout.Append(CurrentRequired(), [0], SeparatorItem, 1, 1);
    /// <summary>지연 Kill 뒤 실제 소멸 전까지 생성 개수에 남는다.표/표시 상태는 검사하지 않는다.</summary>
    public bool NoMenus() => _state.Instances == 0;
    /// <summary>실제 재귀 경로의 순환만 막고 같은 입력 문맥 참조를 자식별 복사→재귀에 유지한다.</summary>
    private void CopyChildren(uint token, uint[] context, List<uint> active)
    {
        if (active.Contains(token)) throw new InvalidOperationException("CD 메뉴 문맥 재귀 순환"); active.Add(token);
        // 매 반복 끝에 현재 signed 항목 수/자식 등록 ID를 읽는다.숨김/dead 자식도 방문한다.
        for (int index = 0; index < unchecked((int)Node(token).ItemCount); index++)
        {
            if ((Item(token, index).Flags & Child) == 0) continue; uint child = Registered(Item(token, index).ChildId); uint[] destination = Base(child).TextContext;
            // REP MOVSD처럼 순서대로 복사하며 앞선 방문이 바꾼 현재 입력 문맥을 다시 읽는다.
            for (int word = 0; word < context.Length; word++) destination[word] = context[word]; CopyChildren(child, context, active);
        }
        active.RemoveAt(active.Count - 1);
    }
    /// <summary>CD 자식의 기본 문맥8DWORD만 복사한다.자신/저장 StyleText/재그리기/재배치는 바꾸지 않는다.</summary>
    public void CopyChildTextContextCd(uint token, uint[] context)
    { RequireCd(); ArgumentNullException.ThrowIfNull(context); if (context.Length != 8) throw new ArgumentException("CD 메뉴 문맥8DWORD 필요"); CopyChildren(token, context, []); }
    /// <summary>CD는 방문 항목의 강조를 지우다가 첫 비영 등록/visible 자식에서 즉시true를 반환한다.</summary>
    public bool ClearSelectionUntilVisibleChildCd(uint token)
    {
        RequireCd();
        // 뒤 항목의 강조를 지우지 않는 원본 부분 갱신을 유지한다.
        for (int index = 0; index < unchecked((int)Node(token).ItemCount); index++) { var item = Item(token, index); uint flags = item.Flags; item.Flags &= ~Selected; if ((flags & Child) != 0) { uint child = Registered(item.ChildId); if (child != 0 && (Base(child).Flags & 1) != 0) return true; } } return false;
    }
    /// <summary>기존 실제 ConfigStore 조회/치환과 CRT 숫자 판독을 ReadInt 경계에 연결한다.전체 ConfigInterface 시작/저장은 후속이다.</summary>
    public static void BindMenuGumpControlConfig(MenuGumpControlHooks hooks, ConfigStore config)
    { ArgumentNullException.ThrowIfNull(config); hooks.ReadInt = (string key, ref int value) => { if (config.Get(key) is { } source) value = ConfigNumbers.ParseLong(source); }; }
}
