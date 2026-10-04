using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>미션 스크립트의 섹션 하나</summary>
/// <param name="Names">섹션 이름들 ("[Succeeded][BadTeamDead]" 이면 두 개)</param>
/// <param name="HeaderLine">섹션 머리 줄 번호 (0부터)</param>
/// <param name="Body">본문 (머리 다음 줄부터 0열이 '[' 인 다음 줄 앞까지, 줄바꿈은 LF)</param>
public sealed record MissionSection(IReadOnlyList<string> Names, int HeaderLine, string Body);

/// <summary>본문에서 찾은 $명령 하나 (조건 태그를 평가하기 전의 어휘 단위)</summary>
/// <param name="Name">명령 이름 (원본 표기 그대로, 예: "Button")</param>
/// <param name="Arguments">'=' 뒤 인자 문자열 (다음 $명령 또는 줄 끝까지, 끝 공백 제거)</param>
/// <param name="Fields">인자를 ',' 로 나눈 값</param>
/// <param name="Line">본문 안의 줄 번호 (0부터)</param>
public sealed record MissionCommand(string Name, string Arguments, IReadOnlyList<string> Fields, int Line);

/// <summary>변수 치환·조건 평가를 마친 본문과 줄 명령. HTML·인라인 명령의 실행은 포함하지 않는다.</summary>
public sealed record PreparedMissionSection(string Body, IReadOnlyList<MissionCommand> Commands);

/// <summary>
/// 미션·메뉴 스크립트 (&lt;이름&gt;.&lt;언어&gt;). 문법: docs/formats/mission-script.md
/// <list type="bullet">
/// <item>머리 값: 원본은 미션 파일을 설정 파일로 읽는다(missionSpec → Config). 따라서 파일 전체에서 처음 나온 "키 = 값" 을 쓴다.</item>
/// <item>섹션 찾기: 원본 FUN_0043fe60 / FUN_00440570 규칙. 앞 공백을 뺀 첫 글자가 '[' 인 줄에서 "[이름]" 을 대소문자 무시로 찾고
/// (한 줄의 두 번째 '[' 도 검사), 본문은 다음 줄부터 0열이 '[' 인 줄 앞까지다.</item>
/// <item>팬 제작 파일의 오타·대소문자 혼용을 받아들이기 위해 해석은 오류 없이 관대하게 한다.</item>
/// </list>
/// </summary>
public sealed partial class MissionScript
{
    /// <summary>줄바꿈을 LF 로 통일한 전체 텍스트</summary>
    public string Text { get; }

    /// <summary>줄 목록</summary>
    private readonly string[] _lines;

    /// <summary>머리 값 조회용 설정 텍스트 (파일 전체)</summary>
    public ConfigText Header { get; }

    /// <summary>파일에 나오는 순서대로 나열한 섹션</summary>
    public IReadOnlyList<MissionSection> Sections { get; }

    /// <summary>텍스트로부터 만든다</summary>
    /// <param name="text">스크립트 텍스트</param>
    public MissionScript(string text)
    {
        Text = text.Replace("\r\n", "\n", StringComparison.Ordinal).Replace('\r', '\n');
        _lines = Text.Split('\n');
        Header = new ConfigText(Text);
        Sections = ScanSections();
    }

    /// <summary>파일 바이트를 읽는다 (Windows-1252 또는 UTF-8 자동 판별)</summary>
    /// <param name="data">파일 내용</param>
    public static MissionScript FromFileBytes(byte[] data) => new(OriginalText.Decode(data));

    /// <summary>머리 값(원시 값)을 읽는다. 없으면 null</summary>
    /// <param name="key">키 (대소문자 무시, 예: "missionType", "ai2Name")</param>
    public string? GetHeader(string key) => Header.GetRaw(key);

    /// <summary>섹션 본문을 찾는다 (원본 규칙). 이름이 여러 개인 머리 줄의 두 번째 이름으로도 찾을 수 있다</summary>
    /// <param name="name">섹션 이름 (대괄호 없이, 예: "Succeeded", "@1", "A.")</param>
    /// <param name="body">본문</param>
    public bool TryGetSection(string name, out string body)
    {
        string target = name.StartsWith('[') ? name : $"[{name}]";
        // 머리 줄 후보마다 줄 안의 모든 '[' 위치에서 비교한다
        for (int i = 0; i < _lines.Length; i++)
        {
            string line = _lines[i];
            int first = FirstNonBlank(line);
            if (first < 0 || line[first] != '[')
            {
                continue;
            }
            for (int pos = first; pos >= 0; pos = line.IndexOf('[', pos + 1))
            {
                if (pos + target.Length <= line.Length
                    && string.Compare(line, pos, target, 0, target.Length, StringComparison.OrdinalIgnoreCase) == 0)
                {
                    body = BodyAfter(i);
                    return true;
                }
            }
        }
        body = "";
        return false;
    }

    /// <summary>섹션 본문을 돌려준다. 없으면 null</summary>
    /// <param name="name">섹션 이름 (대괄호 없이)</param>
    public string? GetSection(string name) => TryGetSection(name, out string body) ? body : null;

    /// <summary>섹션의 본문을 치환·조건 평가하고 표시되는 줄 명령을 추출한다. 섹션이 없으면 null.</summary>
    public PreparedMissionSection? PrepareSection(string name, ConfigStore settings)
    {
        string? raw = GetSection(name);
        if (raw == null)
        {
            return null;
        }
        string body = MissionConditions.Filter(raw, settings);
        return new PreparedMissionSection(body, FindCommands(body));
    }

    /// <summary>
    /// 본문에서 $명령=인자 를 모두 찾는다. 조건 태그(&lt;?조건&gt;…&lt;/?&gt;)는 평가하지 않으므로
    /// 인자 끝에 닫는 태그가 섞일 수 있다 — 정확한 해석은 스크립트 인터프리터(9단계)에서 한다.
    /// </summary>
    /// <param name="body">섹션 본문</param>
    public static IReadOnlyList<MissionCommand> FindCommands(string body)
    {
        var result = new List<MissionCommand>();
        string[] lines = body.Split('\n');
        // 줄마다 $이름= 위치를 찾아 다음 명령 전까지를 인자로 삼는다
        for (int lineNo = 0; lineNo < lines.Length; lineNo++)
        {
            string line = lines[lineNo].TrimEnd('\r');
            MatchCollection matches = CommandRegex().Matches(line);
            for (int k = 0; k < matches.Count; k++)
            {
                Match m = matches[k];
                int argStart = m.Index + m.Length;
                int argEnd = k + 1 < matches.Count ? matches[k + 1].Index : line.Length;
                string args = line[argStart..argEnd].TrimEnd(' ', '\t');
                result.Add(new MissionCommand(m.Groups[1].Value, args, args.Split(','), lineNo));
            }
        }
        return result;
    }

    /// <summary>섹션 머리 줄을 모두 찾는다</summary>
    private List<MissionSection> ScanSections()
    {
        var sections = new List<MissionSection>();
        // 앞 공백을 뺀 첫 글자가 '[' 인 줄을 머리 줄로 본다
        for (int i = 0; i < _lines.Length; i++)
        {
            string line = _lines[i];
            int first = FirstNonBlank(line);
            if (first < 0 || line[first] != '[')
            {
                continue;
            }
            var names = new List<string>();
            // 줄 안의 [이름] 을 모두 모은다
            foreach (Match m in SectionNameRegex().Matches(line, first))
            {
                names.Add(m.Groups[1].Value);
            }
            if (names.Count > 0)
            {
                sections.Add(new MissionSection(names, i, BodyAfter(i)));
            }
        }
        return sections;
    }

    /// <summary>머리 줄 다음 줄부터 0열이 '[' 인 줄 앞까지의 본문 (원본 FUN_00440570)</summary>
    /// <param name="headerLine">머리 줄 번호</param>
    private string BodyAfter(int headerLine)
    {
        int end = headerLine + 1;
        // 0열이 '[' 인 줄이 나올 때까지 진행
        while (end < _lines.Length && !_lines[end].StartsWith('['))
        {
            end++;
        }
        return string.Join('\n', _lines, headerLine + 1, end - headerLine - 1);
    }

    /// <summary>첫 공백·탭이 아닌 글자 위치, 없으면 -1</summary>
    /// <param name="line">검사할 줄</param>
    private static int FirstNonBlank(string line)
    {
        // 공백·탭을 건너뛴다
        for (int i = 0; i < line.Length; i++)
        {
            if (line[i] is not (' ' or '\t'))
            {
                return i;
            }
        }
        return -1;
    }

    /// <summary>섹션 이름: [ 와 ] 사이 (대괄호가 겹치지 않는 가장 짧은 것)</summary>
    [GeneratedRegex(@"\[([^\[\]]+)\]")]
    private static partial Regex SectionNameRegex();

    /// <summary>줄 명령: $이름= (이름은 영문자)</summary>
    [GeneratedRegex(@"\$([A-Za-z]+)=")]
    private static partial Regex CommandRegex();
}
