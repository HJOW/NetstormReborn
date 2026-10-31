using Netstorm.Core.Original;

namespace Netstorm.Client.Mission;

/// <summary>
/// 미션 시작(FUN_00482fb0 ↔ CD 00481020) 가운데 미션 스크립트를 읽고 새 파일 이름·돕는 부분을 만든다.
/// UberGump가 원본 브리핑 본문을 표시한다. 미션 종류는 실제 객체·게임 명령에 속한다.
/// 근거: cpppj/src/client/Mission.h/.cpp (00482fb0·0594fa0·00483210·00483260). (R3k-12, 2026-10-31)
/// Fort 바이트 읽기(ReadFortImage·FortReader)와 Guide 생성·반납은 미션 실행 조립 때 잇는다.
/// </summary>
public sealed class MissionScript : IDisposable
{
    /// <summary>원본이 조회하는 말의 접두부. 설정 객체 이름 "mission"과 구분하는 '.'이 붙은 것이다.</summary>
    private const string MissionPrefix = "mission.";

    /// <summary>현재 언어와 실행 설정을 조회하는 인터페이스. 수명은 호출자가 관리한다.</summary>
    private readonly ConfigInterface _configuration;

    /// <summary>이름으로 조회·표시할 때 쓰는 문자열.</summary>
    private readonly string _name;

    /// <summary>스크립트 경로. `missionSpec`을 이름으로 푼 값이며 바뀐 뒤에는 다시 읽지 않는다.</summary>
    private readonly string _scriptPath;

    /// <summary>호출자가 직접 고른 읽기 경계. 없으면 전역 Config reader를 그대로 쓴다.</summary>
    private readonly Func<string, byte[]?>? _source;

    /// <summary>명시 source가 있으면 참. Guide 교체·세션 로그에 같은 읽기를 보존한다.</summary>
    private readonly bool _explicitSource;

    /// <summary>안내·명령·섹션을 조회하는 미션 스크립트.</summary>
    private readonly Config _script;

/// <summary>
    /// `mission` 이름의 설정 객체를 만들고 추가 설정 글(원본 끝의 두 번째 인자)을 `missionSpec` 경로의 스크립트로 읽는다.
    /// 이 객체가 살아 있는 동안 다른 설정에서 `{mission.??}`는 스크립트 값을 읽는다.
    /// 명시 source가 없으면 추가 설정·파일·이어 script/Fort를 같은 이름으로 쓰고 전역 Config reader를 바꾸지 않는다.
    /// 버리면 등록을 푼다(원본 소멸자와 같다).
    /// </summary>
    /// <param name="configuration">현재 언어와 실행 설정을 조회하는 인터페이스.</param>
    /// <param name="name">미션 이름.</param>
    /// <param name="extraSettings">추가 설정 글. 세미콜론으로 나눈 `키=값` 목록이다.</param>
    /// <param name="source">명시 읽기 경계. 없으면 전역 Config reader를 쓴다.</param>
    public MissionScript(ConfigInterface configuration, string name, string extraSettings = "", Func<string, byte[]?>? source = null)
    {
        ArgumentNullException.ThrowIfNull(configuration);
        ArgumentNullException.ThrowIfNull(name);
        ArgumentNullException.ThrowIfNull(extraSettings);
        _configuration = configuration;
        _name = name;
        _explicitSource = source != null;
        _source = source ?? configuration.Reader;
        _script = new Config(configuration.Registry, "mission");
        _script.AppendArguments(extraSettings, _source);
        _scriptPath = _configuration.PathSpec("missionSpec", _name);
        if (_source != null)
        {
            byte[]? bytes = _source(_scriptPath);
            if (bytes != null)
            {
                _script.LoadBytes(bytes);
            }
        }
        if (_script.IsValid())
        {
            _script.Set("fileName", _name);
        }
    }

    /// <summary>원본처럼 `mission.??`로 조회한다. 스크립트를 찾지 못했으면 찾지 못한다.</summary>
    /// <param name="key">찾을 키.</param>
    public string? Get(string key) => _script.Get(MissionPrefix + key);

    /// <summary>미션 이름(스크립트 파일 이름).</summary>
    public string Name => _name;

    /// <summary>스크립트 파일의 경로(`missionSpec`).</summary>
    public string ScriptPath => _scriptPath;

    /// <summary>버퍼가 만들어졌는지. 스크립트를 찾지 못했으면 거짓이다.</summary>
    public bool Loaded => _script.IsValid();

    /// <summary>값을 비우거나 찾지 못하면 미션 이름이다(원본은 출력 버퍼 첫 글자로 판단한다).</summary>
    public string FortName()
    {
        string fort = Get("loadFort") ?? "";
        return fort.Length == 0 ? _name : fort;
    }

    /// <summary>새 파일의 경로(`fortSpec`, 원본 FUN_00460650).</summary>
    public string FortPath() => _configuration.PathSpec("fortSpec", FortName());

    /// <summary>선택한 Fort 파일 원문과 실제 설치 경로를 함께 돌려준다. 경로 매퍼와 reader가 수명을 둔다.</summary>
    /// <param name="fileName">경로 매퍼. 없으면 읽은 경로를 그대로 쓴다.</param>
    public MissionFortFileReader FortFileReader(Func<string, string>? fileName = null) =>
        selectedFort =>
        {
            if (_source == null)
            {
                throw new InvalidOperationException("미션 Fort 읽기에 쓸 읽기 경계가 없다");
            }
            string path = _configuration.PathSpec("fortSpec", selectedFort);
            byte[]? bytes = _source(path);
            if (bytes == null)
            {
                throw new InvalidOperationException("게임 파일을 찾을 수 없다: " + path);
            }
            return new MissionFortSourceImage { Image = bytes, FileName = fileName != null ? fileName(path) : path };
        };

    /// <summary>같은 source를 공유 복사한 Guide 교체용 읽기 경계. 기본 파일 reader를 같은 이름으로 둔다.</summary>
    public Func<string, byte[]?>? FileSource() => _explicitSource ? _source : null;

    /// <summary>`mission.missionType`: 원본은 빈 문자열로 등록한 미션 종류를 찾아 객체를 만든다.</summary>
    public string MissionType() => Get("missionType") ?? "";

    /// <summary>미션 파일의 원본 브리핑 본문을 설정 치환 없이 찾는다.</summary>
    /// <param name="name">섹션 이름.</param>
    public string? Section(string name) => _script.Section(name);

    /// <summary>현재 미션 Config에 00440640(접두부)으로 실행한다. Start의 tellAtList 작성과 같은 객체를 쓴다.</summary>
    /// <param name="prefix">섹션 이름 접두부.</param>
    public string SectionNames(string prefix) => _script.SectionNames(prefix);

    /// <summary>객체 접두부를 붙이지 않고 현재 미션 Config에 직접 기록한다. 전역 설정이 아닌 현재 mission 객체에 값을 기록한다.</summary>
    /// <param name="key">쓸 키.</param>
    /// <param name="value">쓸 값.</param>
    public bool Set(string key, string value) => _script.Set(key, value);

    /// <summary>버리면 등록을 푼다(원본 소멸자와 같다). 등록 해제를 잊은 미션 객체가 전역 조회를 오염시키지 않게 한다.</summary>
    public void Dispose()
    {
        _script.Dispose();
        GC.SuppressFinalize(this);
    }
}
