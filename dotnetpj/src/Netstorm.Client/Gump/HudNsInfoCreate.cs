using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 NSInfo/PlayerInfo/BattleInfo 생성과 같은 현재 자식을 소유한다.

/// <summary>PlayerInfo0xbc의 미작성 필드도 보존하는 실제 확장 주소다.</summary>
public sealed class HudPlayerInfoNode : GumpCreateNode
{
    /// <summary>+a0의 실제 스크롤 본문이다.</summary>
    public uint Text { get; set; }
    /// <summary>+a4의 실제 마스터 문자다.</summary>
    public uint Master { get; set; }
    /// <summary>+a8의 실제 특수 버튼이다.</summary>
    public uint Button { get; set; }
    /// <summary>+ac의 실제 Start 돌 버튼이다.</summary>
    public uint Start { get; set; }
    /// <summary>+b0의 현재 메타 플레이어 번호다.</summary>
    public uint Player { get; set; }
    /// <summary>+b4의 현재 실제 TYPE 주소 토큰이다.</summary>
    public uint ButtonType { get; set; }
    /// <summary>+b8의 원문 초기화 상태다.</summary>
    public uint Reserved { get; set; }
}

/// <summary>BattleInfo0x16c의 실제 설정과 자식 포인터다.</summary>
public sealed class HudBattleInfoNode : GumpCreateNode
{
    /// <summary>+a0의 실제 제목이다.</summary>
    public uint Title { get; set; }
    /// <summary>+a4의 실제 요약이다.</summary>
    public uint Summary { get; set; }
    /// <summary>+a8의 실제 옵션 본문이다.</summary>
    public uint Body { get; set; }
    /// <summary>+ac의 현재 전투 번호다.</summary>
    public uint Battle { get; set; }
    /// <summary>+b0의 현재 변경 표식이다.</summary>
    public uint Changed { get; set; }
    /// <summary>+b4..163의22개 실제 값/활성 쌍이다.</summary>
    public uint[,] Settings { get; } = new uint[22, 2];
    /// <summary>+164..167이며 생성자는 첫BYTE만 변경한다.</summary>
    public byte[] Bytes { get; } = new byte[4];
    /// <summary>+168의 최초값2다.</summary>
    public uint Mode { get; set; }
}

/// <summary>004256d0의 기본 버튼 기반 특수 파생이다.</summary>
public sealed class HudPlayerButtonNode : ButtonGumpNode
{
    /// <summary>+c0는 생성자가 쓰지 않으므로 현재 값을 보존한다.</summary>
    public uint UntouchedC0 { get; set; }
    /// <summary>+c4만 생성자가0으로 초기화한다.</summary>
    public uint Entry { get; set; }
}

/// <summary>일반 관문 뒤 실제 메타 배치가 선택하는 자식 종류다.</summary>
public enum HudNsInfoChild
{
    /// <summary>00479fb0의 PlayerInfo다.</summary>
    Player,
    /// <summary>0047b480의 BattleInfo다.</summary>
    Battle,
}

/// <summary>전체 생성자는 같은 실제 기존 자식 생성기와 현재 주소/문자열을 사용한다.</summary>
public sealed class HudNsInfoCreateHooks
{
    /// <summary>고정 NSInfo this다.</summary>
    public Func<uint, HudNsInfoNode?>? Node { get; set; }
    /// <summary>고정 PlayerInfo this다.</summary>
    public Func<uint, HudPlayerInfoNode?>? Player { get; set; }
    /// <summary>고정 BattleInfo this다.</summary>
    public Func<uint, HudBattleInfoNode?>? Battle { get; set; }
    /// <summary>고정 특수 버튼 this다.</summary>
    public Func<uint, HudPlayerButtonNode?>? Button { get; set; }
    /// <summary>같은 실제 기본 주소다.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>현재 Scrolling+d0의 같은 값을 비트 OR한다. Source의 참조 반환을 한 실제 쓰기 장치로 표현한다.</summary>
    public Action<uint, uint>? AddTextFlags { get; set; }
    /// <summary>현재 Button+ac의 같은 값을 비트 OR한다.</summary>
    public Action<uint, uint>? AddButtonFlags { get; set; }
    /// <summary>원문 전역 주소를 실제 사용 위치에서 읽는다.</summary>
    public Func<uint, uint>? Read { get; set; }
    /// <summary>원문 new 폭의 실제 파생을 할당하며0 실패도 보존한다.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>같은 기본 Gump 생성자다.</summary>
    public Action<uint, uint, float, float, uint>? BaseConstruct { get; set; }
    /// <summary>같은 실제 배경 생성자다.</summary>
    public Func<uint, float, float, uint, uint, uint, uint, uint>? Background { get; set; }
    /// <summary>같은 실제 Scrolling 생성자다.</summary>
    public Func<uint, int, int, uint, uint, uint>? Scroll { get; set; }
    /// <summary>같은 실제 Text 생성자다.</summary>
    public Func<uint, int, int, uint, uint, uint>? Text { get; set; }
    /// <summary>기존 실제 기본 Button 생성자다.</summary>
    public Action<uint, int, int, uint>? ButtonBase { get; set; }
    /// <summary>문자열/색/문맥 토큰을 보존하는 기존 돌 버튼 생성자다.</summary>
    public Func<uint, int, int, uint, uint, uint, uint>? Stone { get; set; }
    /// <summary>현재 실제 스크롤 가능 설정이다.</summary>
    public Action<uint, uint>? Scrollable { get; set; }
    /// <summary>현재 실제 슬라이더 설정이다.</summary>
    public Action<uint, uint>? Slider { get; set; }
    /// <summary>현재 실제 줄 나눔 설정이다.</summary>
    public Action<uint, uint>? Wrap { get; set; }
    /// <summary>같은 실제 스크롤 마크업 파싱이다.</summary>
    public Action<uint, uint>? ParseScroll { get; set; }
    /// <summary>같은 실제 Text 스타일 파싱이다.</summary>
    public Action<uint, uint>? ParseText { get; set; }
    /// <summary>실제 Text의 두 원시DWORD 여백으로 크기를 맞춘다.</summary>
    public Action<uint, uint, uint>? FitText { get; set; }
    /// <summary>실제 돌 버튼의 문자를 측정해 크기를 맞춘다.</summary>
    public Action<uint>? FitStone { get; set; }
    /// <summary>같은 현재 자식의 실제 가상 부착이다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>같은 현재 자식의 공통 기본 부착이다.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; set; }
    /// <summary>원본 포인터 입력/반환을 유지하는 실제 번역이다.</summary>
    public Func<uint, uint>? Translate { get; set; }
    /// <summary>현재 실제 TYPE 주소를 조회한다.</summary>
    public Func<uint, uint>? Type { get; set; }
    /// <summary>인자 없는 현재 서버 이름 고리다.</summary>
    public Func<uint>? ServerName { get; set; }
    /// <summary>캡처한 format/server/zone으로 현재 형식 문자열을 작성한다.</summary>
    public Func<uint, uint, int, uint>? FormatZone { get; set; }
    /// <summary>실제 진단이며 복귀하면 원문처럼 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>일반 관문 뒤에만 선택되는 실제 메타맵 자식 배치다.</summary>
    public Action<HudNsInfoChild, uint>? MetamapLayout { get; set; }
    /// <summary>Source의 값 전달처럼 함수 슬롯만 복사한다. 현재 callback 저장소는 공유한다.</summary>
    internal HudNsInfoCreateHooks CopyDevices() => (HudNsInfoCreateHooks)MemberwiseClone();
}

/// <summary>cpppj/src/client/HudNsInfoCreate.cpp의 네 부모 생성자와 전체 숨김 wrapper다.</summary>
public sealed class HudNsInfoCreate
{
    /// <summary>00593150의 같은 장기 현재값이다.</summary>
    private readonly UIntBox _current;
    /// <summary>원문 접근 위치까지 하위 선택을 미루는 실제 함수 슬롯이다.</summary>
    private readonly HudNsInfoCreateHooks _hooks;
    /// <summary>상태를 생성/초기화하지 않고 같은 현재 owner와 함수 슬롯을 빌린다.</summary>
    public HudNsInfoCreate(UIntBox current, HudNsInfoCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(current); ArgumentNullException.ThrowIfNull(hooks); _current = current; _hooks = hooks.CopyDevices();
    }
    /// <summary>실제로 도달한 하위만 요구하며 빈 성공으로 바꾸지 않는다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("NSInfo의 실제 생성/자식 효과 미연결");
    /// <summary>같은 실제 파생 주소를 원문 접근 위치에서 요구한다.</summary>
    private static T Node<T>(Func<uint, T?>? hook, uint token) where T : class => (token != 0 ? Need(hook)(token) : null) ?? throw new InvalidOperationException("NSInfo의 실제 파생 저장소 없음");
    /// <summary>현재 같은 기본 주소다.</summary>
    private GumpCreateNode Base(uint token) => Node(_hooks.Base, token);
    /// <summary>현재 종류BYTE를 signed 확장한 실제DWORD다.</summary>
    private static uint Kind(uint value) => unchecked((uint)(int)(sbyte)(byte)value);
    /// <summary>new 뒤마다 현재frame→TYPE를 다시 읽고 실제 배경을 생성한다.</summary>
    private uint NewBackground(uint flags)
    {
        uint token = Need(_hooks.Allocate)(0xb8); if (token == 0) return 0; uint frame = Need(_hooks.Read)(0x5ca924), type = _hooks.Read!(0x5ca920);
        return Need(_hooks.Background)(token, 1, 1, flags, 1, type, frame);
    }
    /// <summary>선택한 실제 Scrolling/Text의 네 생성인자를 모두0으로 전달한다.</summary>
    private uint NewText(bool scrolling)
    {
        uint token = Need(_hooks.Allocate)(scrolling ? 0xd8 : 0xa4); if (token == 0) return 0;
        return scrolling ? Need(_hooks.Scroll)(token, 0, 0, 0, 0) : Need(_hooks.Text)(token, 0, 0, 0, 0);
    }
    /// <summary>같은 현재 자식을0,0,parent,mode0으로 실제 가상 부착한다.</summary>
    private void Attach(uint token, uint parent) { Base(token); Need(_hooks.Attach)(token, 0, 0, parent, 0); }
    /// <summary>0047ca50. 전체 생성이 반환한 뒤에만 현재값과 실제 visible을 변경한다.</summary>
    public uint CreateHidden()
    {
        uint token = Need(_hooks.Allocate)(0xd8); uint result = token != 0 ? Construct(token) : 0; _current.Value = result;
        if (result != 0) Base(result).Flags &= ~1U; return result;
    }
    /// <summary>0047c4b0. 같은 실제 배경·목록·제목·구분선·BattleInfo·PlayerInfo8개를 항상 전체 생성한다.</summary>
    public uint Construct(uint token)
    {
        var node = Node(_hooks.Node, token); Need(_hooks.BaseConstruct)(token, 0, 0, 0, 1); node.Vtable = 0x50d074; node.Command = Kind(Need(_hooks.Read)(0x540421));
        node.Background = NewBackground(0x02000000); Attach(node.Background, token);
        node.Roster = NewText(true); Base(node.Roster).Flags &= ~1U; Need(_hooks.AddTextFlags)(node.Roster, 8);
        Need(_hooks.Scrollable)(node.Roster, 1); Need(_hooks.Slider)(node.Roster, 1); Attach(node.Roster, node.Background);
        node.Title = NewText(true); uint format = Need(_hooks.Translate)(Need(_hooks.Read)(0x540bac)); int zone = unchecked((int)_hooks.Read!(0x5467b8)); uint server = Need(_hooks.ServerName)();
        uint text = Need(_hooks.FormatZone)(format, server, zone); Need(_hooks.ParseScroll)(node.Title, text); Base(node.Title).Flags &= ~1U; Attach(node.Title, node.Background);
        node.UpperDivider = NewBackground(0x02a00000); Base(node.UpperDivider).Flags &= ~1U; Attach(node.UpperDivider, node.Background);
        node.LowerDivider = NewBackground(0x02a00000); Base(node.LowerDivider).Flags &= ~1U; Attach(node.LowerDivider, node.Background);
        uint battle = Need(_hooks.Allocate)(0x16c); node.Battle = battle != 0 ? ConstructBattle(battle) : 0; Base(node.Battle).Flags &= ~1U; Attach(node.Battle, node.Background);
        // 배열 주소는 고정하며 각 생성/부착 뒤 배경 포인터를 다시 읽는다.
        for (int i = 0; i < node.Players.Length; i++) { uint next = Need(_hooks.Allocate)(0xbc); node.Players[i] = next != 0 ? ConstructPlayer(next) : 0; Attach(node.Players[i], node.Background); }
        node.Flags |= 0x800; return token;
    }
    /// <summary>00479d40. 실제 스크롤/특수버튼/마스터 문자/Start 돌 버튼을 같은 부모에 생성한다.</summary>
    public uint ConstructPlayer(uint token)
    {
        var node = Node(_hooks.Player, token); Need(_hooks.BaseConstruct)(token, 0, 0, 0, 1); node.Vtable = 0x50cd64; node.Reserved = 0;
        node.Command = Kind(Need(_hooks.Read)(0x540440)); node.Player = 0; node.Text = NewText(true); Need(_hooks.Scrollable)(node.Text, 0);
        node.ButtonType = Need(_hooks.Type)(_hooks.Read!(0x5411c8)); if (node.ButtonType == 0) Need(_hooks.Assertion)(1912);
        uint button = Need(_hooks.Allocate)(0xc8); node.Button = button != 0 ? ConstructButton(button, 16, 15) : 0; Need(_hooks.AddButtonFlags)(node.Button, 6);
        node.Master = NewText(false); uint master = Need(_hooks.Translate)(0x50cd3c); Need(_hooks.ParseText)(node.Master, master); Need(_hooks.FitText)(node.Master, 0, 0);
        uint stone = Need(_hooks.Allocate)(0x170);
        if (stone != 0) { uint label = _hooks.Translate!(0x50cd2c); uint color = _hooks.Read!(0x5bec3c); node.Start = Need(_hooks.Stone)(stone, 1, 1, label, color, 0); } else node.Start = 0;
        Need(_hooks.FitStone)(node.Start); Attach(node.Text, token); Attach(node.Button, token); Attach(node.Start, token); Attach(node.Master, token); return token;
    }
    /// <summary>0047afa0. 실제 설정22쌍과 텍스트 자식3개를 원문 순서로 초기화한다.</summary>
    public uint ConstructBattle(uint token)
    {
        var node = Node(_hooks.Battle, token); Need(_hooks.BaseConstruct)(token, 0, 0, 0, 1); node.Vtable = 0x50ce00;
        // 원문은 각 쌍에서 뒤 활성값1을 먼저 쓰고 앞 값0을 쓴다.
        for (int i = 0; i < 22; i++) { node.Settings[i, 1] = 1; node.Settings[i, 0] = 0; }
        node.Mode = 2; node.Command = Kind(Need(_hooks.Read)(0x540457)); node.Bytes[0] = 0;
        node.Title = NewText(true); Need(_hooks.Scrollable)(node.Title, 0); Need(_hooks.Wrap)(node.Title, 0); Attach(node.Title, token);
        node.Summary = NewText(false); Need(_hooks.FitText)(node.Summary, 0, 0); Attach(node.Summary, token);
        node.Body = NewText(true); Need(_hooks.Scrollable)(node.Body, 0); Need(_hooks.Wrap)(node.Body, 0); Attach(node.Body, token);
        node.Flags |= 0x800; node.Battle = 0; node.Changed = 0; return token;
    }
    /// <summary>004256d0. 실제 기본 Button 생성 후 가상표와+c4만 변경하고+c0는 보존한다.</summary>
    public uint ConstructButton(uint token, int width, int height)
    {
        var node = Node(_hooks.Button, token); Need(_hooks.ButtonBase)(token, width, height, 0); node.Vtable = 0x5038e8; node.Entry = 0; return token;
    }
    /// <summary>공통 부착 뒤 현재 mode/known을 다시 읽고 선택된 실제 메타 후속만 요구한다.</summary>
    private void AttachChild(HudNsInfoChild kind, uint token, float x, float y, uint parent, uint mode)
    {
        Need(_hooks.BaseAttach)(token, x, y, parent, mode); if (Need(_hooks.Read)(0x594fc8) == 0 || _hooks.Read!(0x595344) == 0) return; Need(_hooks.MetamapLayout)(kind, token);
    }
    /// <summary>00479fb0의 일반 앞단과 Player 후속 관문이다.</summary>
    public void AttachPlayer(uint token, float x, float y, uint parent, uint mode) => AttachChild(HudNsInfoChild.Player, token, x, y, parent, mode);
    /// <summary>0047b480의 일반 앞단과 Battle 후속 관문이다.</summary>
    public void AttachBattle(uint token, float x, float y, uint parent, uint mode) => AttachChild(HudNsInfoChild.Battle, token, x, y, parent, mode);
}
