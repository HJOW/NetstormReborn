using Netstorm.Assets;

namespace Netstorm.Core.Original;

// 미션의 플레이어 한 명이다. 번호·색·활성·지식과 저장 기술·덱을 둔다.
public sealed class MissionPlayer
{
    /// <summary>소유 번호다.</summary>
    public int Number { get; set; }

    /// <summary>그림 색 번호다.</summary>
    public int Color { get; set; }

    /// <summary>사람 입력이나 이름이 있어 활성인 참가자다.</summary>
    public bool Active { get; set; }

    /// <summary>인공지능 참가자다.</summary>
    public bool Computer { get; set; }

    /// <summary>스톰파워다.</summary>
    public double StormPower { get; set; }

    /// <summary>이름이다.</summary>
    public string Name { get; set; } = "";

    /// <summary>시작 기술 원문이다.</summary>
    public string StartingTech { get; set; } = "";

    /// <summary>능력 원문이다.</summary>
    public string Ability { get; set; } = "";

    /// <summary>동맹 비트다.</summary>
    public ushort Allies { get; set; }

    /// <summary>지식 타입 번호 목록이다.</summary>
    public List<int> Knowledge { get; set; } = [];

    /// <summary>저장한 기술 목록이다.</summary>
    public FortContentList StoredTechnology { get; set; } = new();

    /// <summary>저장한 덱이다.</summary>
    public List<FortDeckEntry> StoredDeck { get; set; } = [];
}

/// <summary>미션 플레이어 목록과 기술·회수·승급·AI 허용 상태다.
/// 근거: cpppj/src/o/Player.cpp의 <c>MissionPlayers::Load</c>·<c>Allied</c>.
/// (2026-11-12 0.394에서 R4 미션 값 채우기의 첫 조각으로 추가했다)</summary>
public sealed class MissionPlayers
{
    /// <summary>원본 플레이어 수다. 0번은 중립이며 1~8번이 참가자다.</summary>
    public const int PlayerCount = 8;

    /// <summary>원본 색 이름의 번호 순서다(00452660). 0번 none은 색으로 쓰지 않는다.</summary>
    private static readonly string[] ColorNames = ["none", "blue", "red", "white", "green", "purple", "yellow", "lightblue", "orange"];

    /// <summary>플레이어다. 0번은 비워 둔다.</summary>
    public MissionPlayer[] Players { get; } = new MissionPlayer[PlayerCount + 1];

    /// <summary>기술 허용 표다.</summary>
    public bool[] TechAllowed { get; set; } = [];

    /// <summary>회수 금지 여부다.</summary>
    public bool DenySalvage { get; set; }

    /// <summary>승급 금지 여부다.</summary>
    public bool DenyAscend { get; set; }

    /// <summary>AI 끄기 여부다.</summary>
    public bool AiOff { get; set; }

    /// <summary>빈 목록을 만든다.</summary>
    public MissionPlayers()
    {
        // 0번부터 끝번까지 기본 참가자를 둔다.
        for (int index = 0; index < Players.Length; index++) Players[index] = new MissionPlayer();
    }

    /// <summary>숫자 머리 값은 원본 atof 계열처럼 앞의 숫자를 읽는다.</summary>
    private static double Number(string? value, double fallback)
    {
        if (value is null) return fallback;
        int position = 0;
        // 앞의 공백류를 건너뛴다.
        while (position < value.Length && char.IsWhiteSpace(value[position])) position++;
        int start = position;
        // 부호와 숫자·소수점·지수 형태를 앞에서부터 읽는다.
        if (position < value.Length && value[position] is '-' or '+') position++;
        bool dot = false;
        while (position < value.Length && (char.IsAsciiDigit(value[position]) || (!dot && value[position] == '.')))
        {
            if (value[position] == '.') dot = true;
            position++;
        }
        if (position < value.Length && value[position] is 'e' or 'E')
        {
            int tail = position + 1;
            if (tail < value.Length && value[tail] is '-' or '+') tail++;
            int digits = tail;
            while (digits < value.Length && char.IsAsciiDigit(value[digits])) digits++;
            if (digits > tail) position = digits;
        }
        if (position == start) return 0;
        if (double.TryParse(value[start..position], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out double number)) return number;
        return 0;
    }

    /// <summary>세미콜론 목록의 타입 이름을 현재 타입 번호로 바꾼다. all은 생산 그룹의 자산 타입이다.</summary>
    private static List<int> Knowledge(string text, RiftTypeTable types)
    {
        var result = new List<int>();
        // 원본 목록 순서를 보존하며 중복을 제거한다.
        for (int index = 0; ; index++)
        {
            string token = ConfigNumbers.ListItem(text, index);
            if (token.Length == 0) break;
            if (string.Equals(token, "all", StringComparison.OrdinalIgnoreCase))
            {
                // NO_GROUP을 제외한 생산 그룹의 타입만 전체 지식에 넣는다.
                for (int type = RiftTypeTable.FirstAssetTypeNumber; type < types.Types.Count; type++)
                    if (types.Types[type].FromAsset && types.Types[type].Group < 9) result.Add(type);
            }
            else
            {
                int type = types.Find(token);
                if (type > 0) result.Add(type);
            }
        }
        result.Sort();
        // 정렬 뒤 이웃 중복을 지운다.
        for (int index = result.Count - 1; index > 0; index--)
            if (result[index] == result[index - 1]) result.RemoveAt(index);
        return result;
    }

    /// <summary>기술 허용 표의 deny/allow/all 순서와 시작 SP 덮어쓰기를 적용한다.</summary>
    /// <param name="get">미션 설정 조회. 없으면 null이다.</param>
    /// <param name="fort">저장 기술·덱을 빌릴 요새 Template.</param>
    /// <param name="types">지식 번호를 매기는 타입 표.</param>
    /// <param name="fallbackMoney">시작 SP가 없을 때 사람 플레이어에 쓰는 값.</param>
    public static MissionPlayers Load(Func<string, string?> get, FortTemplate fort, RiftTypeTable types, double fallbackMoney)
    {
        ArgumentNullException.ThrowIfNull(get);
        ArgumentNullException.ThrowIfNull(fort);
        ArgumentNullException.ThrowIfNull(types);
        var result = new MissionPlayers();
        result.TechAllowed = new bool[types.Types.Count];
        Array.Fill(result.TechAllowed, true);
        result.DenySalvage = Number(get("denySalvage"), 0) != 0;
        result.DenyAscend = Number(get("denyAscend"), 0) != 0;
        result.AiOff = Number(get("aiOff"), 0) != 0;
        string permissions = get("techAllowed") ?? "";
        bool allow = true;
        // 00482eb0의 상태 변경 토큰을 앞에서부터 적용한다.
        for (int index = 0; ; index++)
        {
            string token = ConfigNumbers.ListItem(permissions, index);
            if (token.Length == 0) break;
            if (string.Equals(token, "allow", StringComparison.OrdinalIgnoreCase)) allow = true;
            else if (string.Equals(token, "deny", StringComparison.OrdinalIgnoreCase)) allow = false;
            else if (string.Equals(token, "all", StringComparison.OrdinalIgnoreCase)) Array.Fill(result.TechAllowed, allow);
            else
            {
                int type = types.Find(token);
                if (type > 0) result.TechAllowed[type] = allow;
            }
        }
        // 소유 번호를 색 번호의 기본값으로 사용한다.
        for (int number = 1; number <= PlayerCount; number++)
        {
            MissionPlayer player = result.Players[number];
            player.Number = number;
            player.Color = number;
            player.Computer = number != 1;
            player.Allies = unchecked((ushort)(1u << number));
            string prefix = "ai" + number.ToString(System.Globalization.CultureInfo.InvariantCulture);
            // 번호가 없는 구식 ai 설정은 플레이어 2의 기본 입력이다.
            string? AiGet(string suffix)
            {
                string? value = get(prefix + suffix);
                return value ?? (number == 2 ? get("ai" + suffix) : null);
            }
            player.Name = number == 1 ? "Player" : AiGet("Name") ?? "";
            player.Active = number == 1 || player.Name.Length != 0;
            player.StormPower = number == 1 ? Number(get("myStartMoney"), fallbackMoney) : Number(AiGet("StartMoney"), 0);
            player.StartingTech = number == 1 ? get("myTech") ?? "" : AiGet("Tech") ?? "";
            player.Knowledge = Knowledge(player.StartingTech, types);
            player.Ability = AiGet("Ability") ?? "";
            string? color = AiGet("Color");
            if (color is not null)
            {
                int found = Array.FindIndex(ColorNames, name => string.Equals(name, color, StringComparison.OrdinalIgnoreCase));
                if (found > 0) player.Color = found;
            }
            string allyText = number == 1 ? get("myAllyList") ?? "" : AiGet("AllyList") ?? "";
            // 자기 자신은 항상 동맹이며 유효한 목록 번호만 추가한다.
            for (int index = 0; ; index++)
            {
                string token = ConfigNumbers.ListItem(allyText, index);
                if (token.Length == 0) break;
                int ally = ConfigNumbers.ParseLong(token);
                if (ally > 0 && ally <= PlayerCount) player.Allies |= unchecked((ushort)(1u << ally));
            }
        }
        // 저장된 기술/덱은 시작 지식과 별도로 보존한다. 생산 덱의 재구성은 다음 단계다.
        result.Players[1].StoredTechnology = fort.Technology;
        result.Players[1].StoredDeck = fort.Deck;
        // 경로 조회에서도 동맹을 대칭으로 사용한다.
        for (int first = 1; first <= PlayerCount; first++)
        {
            // 두 플레이어 중 한쪽의 지정으로 양쪽 비트를 켠다.
            for (int second = first + 1; second <= PlayerCount; second++)
                if (result.Allied(first, second))
                {
                    result.Players[first].Allies |= unchecked((ushort)(1u << second));
                    result.Players[second].Allies |= unchecked((ushort)(1u << first));
                }
        }
        return result;
    }

    /// <summary>중립·범위 밖 번호는 어느 플레이어와도 동맹이 아니다.</summary>
    public bool Allied(int first, int second)
    {
        if (first <= 0 || second <= 0 || first > PlayerCount || second > PlayerCount) return false;
        return (Players[first].Allies & (1u << second)) != 0 || (Players[second].Allies & (1u << first)) != 0;
    }
}
