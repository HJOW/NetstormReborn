using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>
/// 시작할 때 여는 "Did You Know?" 팁 목록. 원본 tell.english 의 [TipList] 섹션(tip0~tip39)을 읽고,
/// 원본 [Tips] 창 규칙(팁 번호 {tipNumber}, 번호가 0 이 아니면 Prior Tip 버튼, 없는 번호면 "You're out of Tips!")을 따른다.
/// 2026-10-01 네 번째 녹화 시작 화면에서 tip15 본문과 Prior Tip / Next Tip / No More Tips / OK 버튼을 확인했다.
/// </summary>
public sealed partial class StartupTips
{
    /// <summary>팁 목록이 들어 있는 원본 섹션 이름</summary>
    public const string SectionName = "TipList";

    /// <summary>표시할 팁이 없을 때의 원본 문구 ([Tips] 의 {@tell.tip{tipNumber}|You're out of Tips!})</summary>
    public const string OutOfTipsEnglish = "You're out of Tips!";

    /// <summary>표시할 팁이 없을 때의 한국어 문구</summary>
    public const string OutOfTipsKorean = "더 이상 팁이 없습니다!";

    /// <summary>
    /// 원본 tip0~tip39 의 한국어 번역 (UTF-8). 원본 영어 문장의 뜻을 그대로 옮기고, 키 이름·메뉴 이름은 원문을 유지한다.
    /// </summary>
    private static readonly string[] Korean =
    [
        "NetstormHQ를 방문해 업데이트를 확인하고, 지원을 받고, 새 친구를 사귀고, 좋아하는 전략을 토론해 보세요. 토너먼트에도 참가할 수 있습니다! 후원사도 방문해 주세요.",
        "F2 키를 쓰면 다리를 더 쉽게 연결할 수 있습니다.",
        "무엇이든 오른쪽 클릭하면 그에 대한 도움말을 볼 수 있습니다. 게임 창도 마찬가지입니다!",
        "처음에 실패해도 다시 해 보세요! NetStorm을 익히는 유일한 방법은 연습입니다. 몇 년째 플레이하면서도 아직 나아질 여지가 있는 사람들도 있습니다. 열심히 훈련하면 곧 그들을 이길 것입니다!",
        "F5 키로 언제든지 사제를 찾을 수 있습니다. P 또는 R 키를 누르면 사제를 선택하고, 두 번 누르면 사제를 찾아가 선택합니다!",
        "적 유닛을 클릭하면 그 유닛의 사거리를 확인할 수 있습니다.",
        "다리 밖에 유닛을 배치하면 그 유닛의 섬에서 다리를 더 놓을 수 있습니다!",
        "가이저로 다리를 곧장 놓으면 한쪽 옆에서 다가갈 때보다 연결하기 어렵습니다.",
        "골렘이 채집을 멈추지 않도록 Storm 가이저까지 계속 다리를 놓아야 합니다.",
        "단축키를 쓰면 전투에서 귀중한 시간을 아낄 수 있습니다. Q W A S Z X 키로 마우스를 움직이지 않고 다리를 꺼낼 수 있습니다. 또 U 키를 누르면 마지막으로 잃은 유닛 위치로 갑니다.",
        "공격용 다리 줄기를 만들 때 가지를 안팎으로 번갈아 뻗으면 기동성·공격·방어 위치를 모두 살릴 수 있습니다!",
        "탑으로 약한 공격 유닛을 보호할 수 있습니다!",
        "적 유닛을 파괴할 때마다 Storm Power를 얻습니다!",
        "가이저에 연결하면 그 섬을 소유하게 되고, 그 섬에서 다리를 놓을 수 있습니다!",
        "업그레이드보다 워크샵을 더 짓는 편이 효율적입니다!",
        "한 종류에만 집중하기보다 여러 무기를 섞는 편이 대개 더 좋습니다.",
        "모든 유닛은 언제나 가장 가까운 목표를 고릅니다. 그 목표가 자신의 공격에 무적이어도 마찬가지입니다!",
        "수송 유닛이 빈 가이저에 도착하면 다음에는 언제나 템플에서 가장 가까운 가이저를 고릅니다. 적 영토에 있어도 마찬가지입니다!",
        "사격 유닛을 한데 모아 두지 마세요. 하나가 폭발하면 모두 피해를 입습니다!",
        "적 다리 바로 옆에 값싼 유닛을 짓고 그 유닛을 회수하면 적 다리를 뚫을 수 있습니다. 그 파괴력에 보통 다리는 금이 가고, 금 간 다리는 무너집니다!",
        "멀티플레이에서는 적 다리 옆에 발전기를 짓고 오른쪽 클릭해 'Meltdown'을 고르면 적 다리를 폭파할 수 있습니다!",
        "그래픽 카드가 빠르다면 Direct Draw 모드에서 Options / Adjust Direct Draw / Parallax Clouds를 쓸 수 있습니다. 정말 멋집니다!!",
        "적 유닛을 쏠 때는 땅과 맞닿은 부분을 노리세요. 높은 곳을 노리면 탄이 그냥 지나칠 수 있습니다!",
        "다리가 이어져 있으면 동맹의 섬과 다리 위에도 지을 수 있습니다.",
        "Storm Power가 더 필요할 때는 쓰지 않는 것을 회수(Salvage)할 수 있습니다.",
        "유닛은 언제나 가장 가까운 목표를 고르므로, 탑으로 적의 사격을 끄는 동안 다른 방향에서 공격할 수 있습니다!",
        "끊어진 다리를 조심하세요! 골렘은 움직이기 시작할 때 있던 다리라면 끊어진 끝에서 걸어 나가 떨어질 수 있습니다!",
        "멀티플레이 게임에서는 사제가 기도해 강력한 주문을 얻을 수 있습니다!",
        "멀티플레이 모드에서는 전초기지(Outpost)를 지어 더 많은 섬을 차지할 수 있습니다!",
        "공중 유닛은 다른 공중 유닛을 공격할 수 없습니다!",
        "멀티플레이 Challenge Ring에서 어느 '슬롯'을 고르는지에 따라 전투에서 내 섬의 위치가 정해집니다.",
        "Whirligig, Dust Devil, Man o' War와 싸우는 요령은 기지를 부수는 것입니다. 비행체 자체에 시간을 낭비하지 마세요.",
        "Storm Power가 바닥날 걱정을 하고 싶지 않다면 Sun 유닛을 쓰세요. 값이 쌉니다!",
        "수송 유닛마다 중요한 차이가 있습니다. 창의적으로 쓰는 법을 안다면 말이죠!",
        "무기를 서로 보완하도록 묶을 수 있습니다. 한쪽의 약점이 다른 쪽의 강점이 될 수 있습니다!",
        "멀티플레이에서 템플을 지을 때는 가이저가 가장 많은 곳 가까이에 두세요. 그래야 골렘이 멀리 걷지 않습니다!",
        "연결하려는 다리가 아직 어두워도 다리를 더 놓을 수 있습니다.",
        "멀티플레이 채팅 창에 다른 플레이어의 이름을 치다가 TAB을 누르면 나머지 이름이 채워집니다.",
        "동맹의 돈이 부족하면 동맹이 가진 유닛을 오른쪽 클릭해 Player에서 Send Money를 누르고 보낼 금액을 고를 수 있습니다.",
        "팁을 모두 보았습니다! 더 많은 팁은 NetstormHQ에서 찾을 수 있습니다.",
    ];

    /// <summary>팁 번호 → 영어 본문(태그·색 코드를 뺀 표시 문자열)</summary>
    private readonly SortedDictionary<int, string> _english = [];

    /// <summary>보여 주면 팁 번호를 0 으로 되돌리는 팁 번호 (원본 tip39 의 &lt;$Config,tipNumber=0&gt;)</summary>
    private readonly SortedSet<int> _resetting = [];

    /// <summary>[TipList] 섹션 본문에서 tipN="..." 줄을 읽는다.</summary>
    /// <param name="sectionBody">tell.english 의 [TipList] 본문 (없으면 빈 목록)</param>
    public StartupTips(string? sectionBody)
    {
        if (sectionBody == null) return;
        // 한 줄에 팁 하나씩 들어 있다
        foreach (string line in sectionBody.Split('\n'))
        {
            Match match = TipLine().Match(line.Trim());
            if (!match.Success) continue;
            int number = int.Parse(match.Groups[1].Value, System.Globalization.CultureInfo.InvariantCulture);
            string raw = match.Groups[2].Value.Replace("`\"", "\"", StringComparison.Ordinal);
            if (raw.Contains("tipNumber=0", StringComparison.OrdinalIgnoreCase)) _resetting.Add(number);
            _english[number] = Clean(raw);
        }
    }

    /// <summary>읽은 팁 수</summary>
    public int Count => _english.Count;

    /// <summary>팁 번호의 본문. 없는 번호면 원본처럼 "You're out of Tips!"(한국어는 번역)를 돌려준다.</summary>
    /// <param name="number">팁 번호</param>
    /// <param name="korean">한국어로 표시할지</param>
    public string Text(int number, bool korean)
    {
        if (!_english.TryGetValue(number, out string? english)) return korean ? OutOfTipsKorean : OutOfTipsEnglish;
        return korean && number >= 0 && number < Korean.Length ? Korean[number] : english;
    }

    /// <summary>이 번호의 팁을 보여 주면 다음 팁 번호를 0 으로 되돌리는지 (마지막 팁)</summary>
    /// <param name="number">팁 번호</param>
    public bool ResetsNumber(int number) => _resetting.Contains(number);

    /// <summary>원본 문자열의 색 코드({CText}·{Text})·링크 태그·스크립트 명령을 빼고 보이는 글자만 남긴다.</summary>
    /// <param name="raw">따옴표 안의 원본 문자열</param>
    public static string Clean(string raw)
    {
        string text = Command().Replace(raw, "");
        text = Tag().Replace(text, "");
        text = ColorCode().Replace(text, "");
        return Spaces().Replace(text, " ").Trim();
    }

    /// <summary>tipN="본문" 한 줄</summary>
    [GeneratedRegex("^tip(\\d+)=\"(.*)\"$")]
    private static partial Regex TipLine();

    /// <summary>&lt;$Config,…&gt; 같은 스크립트 명령</summary>
    [GeneratedRegex("<\\$[^>]*>")]
    private static partial Regex Command();

    /// <summary>&lt;a href=…&gt;·&lt;/a&gt; 같은 HTML 태그</summary>
    [GeneratedRegex("<[^>]*>")]
    private static partial Regex Tag();

    /// <summary>{CText}·{Text} 같은 글자색 코드</summary>
    [GeneratedRegex("\\{[A-Za-z0-9]+\\}")]
    private static partial Regex ColorCode();

    /// <summary>연속된 공백</summary>
    [GeneratedRegex("\\s+")]
    private static partial Regex Spaces();
}
