// 메타 NSInfo의 실제 참가자 등록 명부 정렬·행 작성을 같은 CS/Gump 저장소에 수행한다.
// cpppj/src/client/HudNsInfoMeta.h·cpp 의 HudNsInfoMetaGlobals/HudNsInfoMetaHooks/RefreshNsInfoRoster(0047e630) 를 이식한다.
// 0.906에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
// LayoutMetaNsInfo(0047e9d0 일반 앞단/관문 뒤 전체 본문)는 0.907에서 이었다.
using System.Text;
using Netstorm.Client.Display;

namespace Netstorm.Client.Gump;

// 메타 NSInfo 모듈을 담는 네임스페이스다. 0.906에서 추가됐다.

// 두 원문 몸체가 현재 위치에서 읽는 실제 전역이다. 0.906에서 추가됐다.
public sealed class HudNsInfoMetaGlobals
{
    /// <summary>같은 CS 참가자40·전투10 기록을 소유한다. 0.906에서 추가됐다.</summary>
    public required ClientChallengeGlobals Challenge { get; init; }

    /// <summary>같은 005930b0 정렬 표/현재 NSInfo를 소유한다. 0.906에서 추가됐다.</summary>
    public required ClientNsInfoGlobals Info { get; init; }

    /// <summary>같은 594fc8의 현재 메타맵 모드를 빌린다. 0.906에서 추가됐다.</summary>
    public required Func<uint> Metamap { get; init; }

    /// <summary>같은 595344의 현재 플레이어 인식 값을 빌린다. 0.906에서 추가됐다.</summary>
    public required Func<uint> Known { get; init; }

    /// <summary>같은 540c70의 현재 로컬 소유자를 빌린다. 0.906에서 추가됐다.</summary>
    public required Func<uint> Owner { get; init; }
}

// 실제 UI 처리기·현재 색/기술 값만 선택된 위치에서 호출한다. 0.906에서 추가됐다.
public sealed class HudNsInfoMetaHooks
{
    /// <summary>같은 0xd8 NSInfo 노드다. 0.906에서 추가됐다.</summary>
    public Func<uint, HudNsInfoNode?>? Root { get; set; }

    /// <summary>004bc250 모드0의 실제 문자열 처리다. 0.906에서 추가됐다.</summary>
    public Action<uint, string>? Replace { get; set; }

    /// <summary>004ba210의 같은 버퍼 noWrap 쓰기다. 0.906에서 추가됐다.</summary>
    public Action<uint, uint>? Wrap { get; set; }

    /// <summary>현재 팔레트색/metadataMode의 원문 DWORD를 읽는다. 0.906에서 추가됐다.</summary>
    public Func<uint, uint>? Read { get; set; }

    /// <summary>같은 Pool의 실제 SID+22 BYTE다. 0.906에서 추가됐다.</summary>
    public Func<uint, uint>? ObjectOwner { get; set; }

    /// <summary>0042b210의 현재 TYPE 기술 합계다. 0.906에서 추가됐다.</summary>
    public Func<ReadOnlyMemory<byte>, uint>? Weight { get; set; }

    /// <summary>004ba0f0의 길이-1/실제 정렬행 metadata다. 0.906에서 추가됐다.</summary>
    public Action<uint, string, uint[], uint>? Append { get; set; }

    /// <summary>현재 실제 공통 노드/크기/위치를 빌린다. 0.907에서 추가됐다.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }

    /// <summary>같은 0xbc PlayerInfo 노드다. 0.907에서 추가됐다.</summary>
    public Func<uint, HudPlayerInfoNode?>? Player { get; set; }

    /// <summary>같은 0x16c BattleInfo 노드다. 0.907에서 추가됐다.</summary>
    public Func<uint, HudBattleInfoNode?>? Battle { get; set; }

    /// <summary>가상+18의 실제 Push다. 0.907에서 추가됐다.</summary>
    public Action<uint>? Push { get; set; }

    /// <summary>004b9f80의 같은 실제 내용 맞춤이다. 0.907에서 추가됐다.</summary>
    public Action<uint>? FitHeight { get; set; }

    /// <summary>각 파생의 실제 가상+20이다. 0.907에서 추가됐다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }

    /// <summary>004ba8f0의 같은 실제 문자열 처리다. 0.907에서 추가됐다.</summary>
    public Action<uint, string>? Parse { get; set; }

    /// <summary>header의 번역→zone 캡처→현재 server 조회→256BYTE 서식이다. 0.907에서 추가됐다.</summary>
    public Func<bool, string>? Header { get; set; }

    /// <summary>00464ed0의 실제 소유 문자열 교체다. 0.907에서 추가됐다.</summary>
    public Action<uint, string>? HelpText { get; set; }

    /// <summary>00464f80의 실제 소유 단어 교체다. 0.907에서 추가됐다.</summary>
    public Action<uint, string>? HelpWord { get; set; }

    /// <summary>현재 표시 바이트 언어 번역이다. 0.907에서 추가됐다.</summary>
    public Func<string, string>? Translate { get; set; }
}

// 0047e630 전체의 39명 초기 순서·원문 쌍교환 정렬·이름/문맥/행 metadata다. 0.906에서 추가됐다.
public static class HudNsInfoMetaRoster
{
    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.906에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("NSInfo 메타의 실제 선택 장치 미연결");

    /// <summary>지정한 실제 폭만 읽고 원문 필드 밖 접근은 그 위치에서 거부한다. 0.906에서 추가됐다.</summary>
    private static uint ReadField(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 낮은 BYTE부터 원문 폭만 조립한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "NSInfo 메타의 실제 CS 필드 부족");
        }

        uint value = 0;
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>현재 레코드의 실제 첫 NUL을 찾으며 임의 길이로 이름을 잘라 성공시키지 않는다. 0.906에서 추가됐다.</summary>
    private static string ParticipantName(ReadOnlySpan<byte> record)
    {
        ReadOnlySpan<byte> text = record.Slice(0x26);
        int end = text.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(record), "NSInfo 이름의 실제 NUL 없음");
        }

        return Encoding.Latin1.GetString(text[..end]);
    }

    /// <summary>원문은 정렬되지 않은 유효표를 표준 sort가 아닌 전체 39×39 쌍교환으로 재배치한다. 0.906에서 추가됐다.</summary>
    /// <param name="token">같은 0xd8 NSInfo 노드 토큰이다.</param>
    /// <param name="globals">두 원문 몸체가 현재 위치에서 읽는 실제 전역이다.</param>
    /// <param name="hooks">실제 UI 처리기·현재 색/기술 값이다.</param>
    public static void Refresh(uint token, HudNsInfoMetaGlobals globals, HudNsInfoMetaHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(globals.Challenge);
        ArgumentNullException.ThrowIfNull(globals.Info);
        if (Need(globals.Metamap)() == 0 || Need(globals.Known)() == 0)
        {
            return;
        }

        uint shade = Need(hooks.Read)(0x5c78d0u);
        uint yellow = Need(hooks.Read)(0x59afd8u);
        uint[] ownContext = [2, 0, yellow, 0, 0, shade, 1, 1];
        HudNsInfoNode root = Need(hooks.Root)(token) ?? throw new InvalidOperationException("NSInfo 메타의 실제 Root 없음");
        Need(hooks.Replace)(root.Roster, "");
        Need(hooks.Wrap)(root.Roster, 0);
        // 0번을 보존하며 005930b4..314f의 39DWORD만 초기 순서로 쓴다.
        for (uint index = 1; index < 40; index++)
        {
            globals.Info.RosterOrder[index] = index;
        }

        // 바깥 현재 후보는 교환 직후 새 참가자로 재조회한다.
        for (uint outer = 1; outer < 40; outer++)
        {
            ReadOnlyMemory<byte> current = globals.Challenge.Participant(globals.Info.RosterOrder[outer]);
            if ((ReadField(current.Span, 0x64, 1) & 1u) == 0)
            {
                continue;
            }

            // 유효한 상대의 레벨 BYTE를 먼저 비교하고 동률 때만 상대→현재 순서로 기술 합계를 읽는다.
            for (uint inner = 1; inner < 40; inner++)
            {
                ReadOnlyMemory<byte> other = globals.Challenge.Participant(globals.Info.RosterOrder[inner]);
                if ((ReadField(other.Span, 0x64, 1) & 1u) == 0)
                {
                    continue;
                }

                uint level = ReadField(current.Span, 0x58, 1);
                uint otherLevel = ReadField(other.Span, 0x58, 1);
                bool exchange = level < otherLevel;
                if (level == otherLevel)
                {
                    uint otherWeight = Need(hooks.Weight)(other.Slice(0x54, 10));
                    uint currentWeight = Need(hooks.Weight)(current.Slice(0x54, 10));
                    exchange = currentWeight < otherWeight;
                }

                if (exchange)
                {
                    (globals.Info.RosterOrder[outer], globals.Info.RosterOrder[inner]) = (globals.Info.RosterOrder[inner], globals.Info.RosterOrder[outer]);
                    current = globals.Challenge.Participant(globals.Info.RosterOrder[outer]);
                }
            }
        }

        // 유효행만 추가하되 metadata는 실제 참가자 번호가 아닌 정렬 배열의 1..39행 번호다.
        for (uint row = 1; row < 40; row++)
        {
            ReadOnlyMemory<byte> record = globals.Challenge.Participant(globals.Info.RosterOrder[row]);
            if ((ReadField(record.Span, 0x64, 1) & 1u) == 0)
            {
                continue;
            }

            bool own = Need(hooks.ObjectOwner)(ReadField(record.Span, 0xb8)) == Need(globals.Owner)();
            uint rank = Need(hooks.Weight)(record.Slice(0x54, 10));
            string text = $" {ReadField(record.Span, 0x58, 1) + 1u}.{rank} " + ParticipantName(record.Span) + '\n';
            if (Encoding.Latin1.GetByteCount(text) >= 120)
            {
                throw new ArgumentOutOfRangeException(nameof(record), "NSInfo 참가자행의 실제120BYTE 범위");
            }

            uint minor = ReadField(record.Span, 0xb2, 2);
            uint[] context = [0, 0, minor == 78 ? 0xc4u : 0xdfu, 0, 0, 0, 1, 1];
            if (minor > 78)
            {
                context = [0, 0, 0x9du, 0, 0, 0, 1, 1];
            }

            context[0] |= 2u;
            context[5] = Need(hooks.Read)(0x5c78d0u);
            context[6] = 1;
            context[7] = 1;
            uint[] selected = own && Need(hooks.Read)(0x5c85c8u) != 0 ? ownContext : context;
            Need(hooks.Append)(root.Roster, text, selected, row);
        }
    }
}

// 일반 부착/모드 관문은 HudNsInfo가 실행했다. 여기서는 그 뒤 원문 큰 본문만 수행한다.
// cpppj/src/client/HudNsInfoMeta.cpp 의 LayoutMetaNsInfo(0047e9d0) 를 이식한다.
// 0.907에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
public static class HudNsInfoMetaLayout
{
    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.907에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 선택 장치 미연결");

    /// <summary>지정한 실제 폭만 읽고 원문 필드 밖 접근은 그 위치에서 거부한다. 0.907에서 추가됐다.</summary>
    private static uint ReadField(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 낮은 BYTE부터 원문 폭만 조립한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "NSInfo 메타 배치의 실제 CS 필드 부족");
        }

        uint value = 0;
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>원문 현재 자식의 기본폭을 쓰뒤 같은 배율의 float 결과만 저장한다. 0.907에서 추가됐다.</summary>
    private static void SetWidth(uint token, double width, HudNsInfoMetaHooks hooks)
    {
        GumpCreateNode node = Need(hooks.Node)(token) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 자식 없음");
        node.BaseWidth = (float)width;
        node.Width = (float)(width * node.Scale);
    }

    /// <summary>원문 현재 자식의 기본높이를 쓰뒤 같은 배율의 float 결과만 저장한다. 0.907에서 추가됐다.</summary>
    private static void SetHeight(uint token, double height, HudNsInfoMetaHooks hooks)
    {
        GumpCreateNode node = Need(hooks.Node)(token) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 자식 없음");
        node.BaseHeight = (float)height;
        node.Height = (float)(height * node.Scale);
    }

    /// <summary>다른 Gump 플래그는 보존하고 표시 비트만 바꾼다. 0.907에서 추가됐다.</summary>
    private static void SetVisible(uint token, bool visible, HudNsInfoMetaHooks hooks)
    {
        GumpCreateNode node = Need(hooks.Node)(token) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 자식 없음");
        if (visible)
        {
            node.Flags |= 1u;
        }
        else
        {
            node.Flags &= ~1u;
        }
    }

    /// <summary>바뀐 대상 또는 0대상에서만 +b8을 지우며 고정 PlayerInfo의 현재 +b0을 그대로 둔다. 0.907에서 추가됐다.</summary>
    private static void Select(uint token, uint participant, HudNsInfoMetaHooks hooks)
    {
        HudPlayerInfoNode node = Need(hooks.Player)(token) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 PlayerInfo 없음");
        if (node.Player != participant || participant == 0)
        {
            node.Reserved = 0;
        }

        node.Player = participant;
    }

    /// <summary>실제 현재 도움말 제목/키를 각각 빈 문자열 교체 뒤에 쓴다. 0.907에서 추가됐다.</summary>
    private static void Help(uint token, bool battle, HudNsInfoMetaHooks hooks)
    {
        Need(hooks.HelpText)(token, "");
        string title = Need(hooks.Translate)(battle ? "Battle Status Window" : "Player List Window");
        Need(hooks.HelpText)(token, title);
        Need(hooks.HelpWord)(token, "");
        Need(hooks.HelpWord)(token, battle ? "battleStatusView" : "playerListView");
    }

    /// <summary>일반 앞단/관문 뒤 전체 본문이다. 실제 자식 Attach가 Player/Battle 후속을 선택한다. 0.907에서 추가됐다.</summary>
    /// <param name="token">같은 0xd8 NSInfo 노드 토큰이다.</param>
    /// <param name="globals">두 원문 몸체가 현재 위치에서 읽는 실제 전역이다.</param>
    /// <param name="hooks">실제 UI 처리기·현재 색/기술 값이다.</param>
    public static void Layout(uint token, HudNsInfoMetaGlobals globals, HudNsInfoMetaHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(globals.Challenge);
        HudNsInfoNode root = Need(hooks.Root)(token) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 Root 없음");
        ReadOnlyMemory<byte> own = globals.Challenge.Participant(Need(globals.Owner)());
        if (ReadField(own.Span, 0xa4) == 0)
        {
            HudBattleInfoNode noBattle = Need(hooks.Battle)(root.Battle) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 BattleInfo 없음");
            noBattle.Bytes[0] = 0;
            Need(hooks.Push)(root.Title);
            SetVisible(root.Title, true, hooks);
            Need(hooks.Replace)(root.Title, "");
            string header = Need(hooks.Header)(false);
            Need(hooks.Parse)(root.Title, header);
            SetWidth(root.Title, root.BaseWidth, hooks);
            Need(hooks.FitHeight)(root.Title);
            Need(hooks.Push)(root.UpperDivider);
            SetVisible(root.UpperDivider, true, hooks);
            SetWidth(root.UpperDivider, root.BaseWidth, hooks);
            Need(hooks.Push)(root.Roster);
            SetVisible(root.Roster, true, hooks);
            SetWidth(root.Roster, root.BaseWidth, hooks);
            GumpCreateNode titleNode = Need(hooks.Node)(root.Title) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 제목 없음");
            GumpCreateNode dividerNode = Need(hooks.Node)(root.UpperDivider) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 구분선 없음");
            SetHeight(root.Roster, (double)root.BaseHeight - titleNode.BaseHeight - dividerNode.BaseHeight, hooks);
            HudNsInfoMetaRoster.Refresh(token, globals, hooks);
            Need(hooks.Attach)(root.Title, 0, 0, root.Background, 0);
            Need(hooks.Attach)(root.UpperDivider, 0, titleNode.BaseHeight, root.Background, 0);
            Need(hooks.Attach)(root.Roster, 0, titleNode.BaseHeight + dividerNode.BaseHeight, root.Background, 0);
            Need(hooks.Push)(root.LowerDivider);
            SetVisible(root.LowerDivider, false, hooks);
            SetWidth(root.LowerDivider, root.BaseWidth, hooks);
            GumpCreateNode backgroundNode = Need(hooks.Node)(root.Background) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 배경 없음");
            Need(hooks.Attach)(root.LowerDivider, 0, backgroundNode.BaseHeight * 0.5f, root.Background, 0);
            Need(hooks.Push)(root.Battle);
            SetVisible(root.Battle, false, hooks);
            HudBattleInfoNode hiddenBattle = Need(hooks.Battle)(root.Battle) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 BattleInfo 없음");
            hiddenBattle.Battle = ReadField(own.Span, 0xa4);
            hiddenBattle.Changed = 0;
            SetWidth(root.Battle, root.BaseWidth, hooks);
            GumpCreateNode lowerDividerNode = Need(hooks.Node)(root.LowerDivider) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 아래 구분선 없음");
            Need(hooks.Attach)(root.Battle, 0, backgroundNode.BaseHeight * 0.5f + lowerDividerNode.BaseHeight, root.Background, 0);
            Help(token, false, hooks);
            return;
        }

        SetVisible(root.Roster, false, hooks);
        Need(hooks.Push)(root.Title);
        Need(hooks.Replace)(root.Title, "");
        string battleHeader = Need(hooks.Header)(true);
        Need(hooks.Parse)(root.Title, battleHeader);
        SetVisible(root.Title, true, hooks);
        SetWidth(root.Title, root.BaseWidth, hooks);
        Need(hooks.FitHeight)(root.Title);
        Need(hooks.Push)(root.UpperDivider);
        SetVisible(root.UpperDivider, true, hooks);
        SetWidth(root.UpperDivider, root.BaseWidth, hooks);
        Need(hooks.Attach)(root.Title, 0, 0, root.Background, 0);
        GumpCreateNode battleTitleNode = Need(hooks.Node)(root.Title) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 제목 없음");
        Need(hooks.Attach)(root.UpperDivider, 0, battleTitleNode.BaseHeight, root.Background, 0);
        ReadOnlyMemory<byte> battle = globals.Challenge.Battle(ReadField(own.Span, 0xa4));
        GumpCreateNode battleUpperDivider = Need(hooks.Node)(root.UpperDivider) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 위 구분선 없음");
        int y = RawCameraView.FtolLow((double)battleTitleNode.BaseHeight + battleUpperDivider.BaseHeight);
        uint used = 0;
        bool master = false;
        // 같은 전투 8슬롯에서 첫 마스터 하나만 별도 0번 패널에 놓는다. 마스터가 없으면 used 0을 유지한다.
        for (uint slot = 0; slot < 8; slot++)
        {
            uint participant = ReadField(battle.Span, 0x38 + (int)slot * 4);
            if (participant == 0 || (ReadField(globals.Challenge.Participant(participant).Span, 0x64, 1) & 4u) == 0)
            {
                continue;
            }

            Need(hooks.Push)(root.Players[0]);
            SetWidth(root.Players[0], root.BaseWidth, hooks);
            Select(root.Players[0], participant, hooks);
            SetVisible(root.Players[0], true, hooks);
            Need(hooks.Attach)(root.Players[0], 3, y, root.Background, 0);
            GumpCreateNode firstPanel = Need(hooks.Node)(root.Players[0]) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 0번 패널 없음");
            y = RawCameraView.FtolLow((double)firstPanel.Y + firstPanel.BaseHeight + 2.0);
            used = 1;
            master = true;
            break;
        }

        // 원문 슬롯 순서로 비마스터만 추가하고 다음 Y는 저장된 현재 정수 Y·실제 후행 높이+2를 ftol한다.
        for (uint slot = 0; slot < 8; slot++)
        {
            uint participant = ReadField(battle.Span, 0x38 + (int)slot * 4);
            if (participant == 0 || (ReadField(globals.Challenge.Participant(participant).Span, 0x64, 1) & 4u) != 0)
            {
                continue;
            }

            if (used >= root.Players.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(slot), "NSInfo PlayerInfo 실제8패널 범위");
            }

            uint panel = root.Players[(int)used++];
            Need(hooks.Push)(panel);
            SetWidth(panel, root.BaseWidth, hooks);
            Select(panel, participant, hooks);
            SetVisible(panel, true, hooks);
            float point = y;
            Need(hooks.Attach)(panel, 3, point, root.Background, 0);
            GumpCreateNode panelNode = Need(hooks.Node)(panel) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 패널 없음");
            y = RawCameraView.FtolLow(point + panelNode.BaseHeight + 2.0f);
        }

        Need(hooks.Push)(root.LowerDivider);
        SetVisible(root.LowerDivider, master, hooks);
        SetWidth(root.LowerDivider, root.BaseWidth, hooks);
        GumpCreateNode backgroundNode2 = Need(hooks.Node)(root.Background) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 배경 없음");
        Need(hooks.Attach)(root.LowerDivider, 3, backgroundNode2.BaseHeight * 0.5f, root.Background, 0);
        Need(hooks.Push)(root.Battle);
        SetVisible(root.Battle, master, hooks);
        HudBattleInfoNode info = Need(hooks.Battle)(root.Battle) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 BattleInfo 없음");
        info.Battle = ReadField(own.Span, 0xa4);
        info.Changed = ReadField(own.Span, 0x64) & 4u;
        SetWidth(root.Battle, root.BaseWidth, hooks);
        GumpCreateNode lowerDividerNode2 = Need(hooks.Node)(root.LowerDivider) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 아래 구분선 없음");
        SetHeight(root.Battle, (double)backgroundNode2.BaseHeight * 0.5f - lowerDividerNode2.BaseHeight, hooks);
        Need(hooks.Attach)(root.Battle, 3, backgroundNode2.BaseHeight * 0.5f + lowerDividerNode2.BaseHeight, root.Background, 0);
        Help(token, true, hooks);
        // 남은 패널도 숨긴 뒤 실제 Attach를 호출한다. 미복원 자식 본문을 빈 성공으로 우회하지 않는다.
        for (uint index = used; index < 8; index++)
        {
            uint panel = root.Players[(int)index];
            Need(hooks.Push)(panel);
            SetWidth(panel, root.BaseWidth, hooks);
            HudPlayerInfoNode player = Need(hooks.Player)(panel) ?? throw new InvalidOperationException("NSInfo 메타 배치의 실제 PlayerInfo 없음");
            player.Reserved = 0;
            player.Player = 0;
            SetVisible(panel, false, hooks);
            Need(hooks.Attach)(panel, 0, 0, root.Background, 0);
        }
    }
}
