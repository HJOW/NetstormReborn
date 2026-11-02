using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Mission;

/// <summary>빈 State를 만들 때 원본이 보존하는 미정 스택 입력 28바이트를 현재 외부 입력으로 채운다.</summary>
/// <param name="bytes">채울 28바이트.</param>
/// <remarks>2026-11-02 TS6에서 추가했다. <see cref="Span{T}"/>를 받으므로 <see cref="Action{T}"/> 대신 전용 대리자를 쓴다.</remarks>
public delegate void SoloMapAllocationBytes(Span<byte> bytes);

/// <summary>
/// 맵 목록이 게임 규칙 대신 현재 호스트·언어·사용자 자료만 빌리는 경계다(원본 <c>SoloMapCatalogEffects</c>).
/// 생략한 항목 중 <see cref="TypeCount"/>·<see cref="AllocationBytes"/>·<see cref="UserName"/>·<see cref="Translate"/>는 그 지점에 이르면 명시 오류로 멈춘다.
/// </summary>
/// <remarks>2026-11-02 TS6에서 추가했다.</remarks>
public sealed class SoloMapCatalogEffects
{
    /// <summary>파일 서명 검사가 읽는 현재 TYPE 개수다.</summary>
    public Func<int>? TypeCount { get; init; }

    /// <summary>빈 State가 보존하는 현재 미정 바이트 입력이다.</summary>
    public SoloMapAllocationBytes? AllocationBytes { get; init; }

    /// <summary>원본 00540ca8의 현재 사용자 이름이다.</summary>
    public Func<string>? UserName { get; init; }

    /// <summary>손상 행 형식의 현재 번역이다.</summary>
    public Func<string, string>? Translate { get; init; }

    /// <summary>각 PathSpec 직전의 현재 키다. 생략하면 이 목록 수명의 기본 키(<see cref="SoloMapCatalog.FileSpec"/>)를 쓴다.</summary>
    public Func<string>? FileSpecKey { get; init; }

    /// <summary>분류·메타 두 임시 저장소도 같은 Client의 005c89c0을 쓴다. 생략하면 목록 수명에 한 개를 만든다.</summary>
    public FortSerializationGlobals? SerializationGlobals { get; init; }

    /// <summary>실제 Client가 제공하는 같은 목록·dirty 상태다. 독립 호출은 생략하면 자체 저장소를 쓴다.</summary>
    public SoloMapListState? ListState { get; init; }

    /// <summary>선택적 실제 파일 열거 장치다. 현재 PathSpec 결과를 받아 경로를 돌려주며 생략하면 기본 디스크만 열거한다.</summary>
    public Func<string, IReadOnlyList<string>>? EnumeratePaths { get; init; }

    /// <summary>선택적 현재 파일 reader다. 분류와 메타마다 다시 호출하며 생략하면 기본 파일 시스템의 우선순위로 읽는다.</summary>
    public Func<string, byte[]?>? ReadFile { get; init; }

    /// <summary>두 MapProbe도 주 저장소와 같은 005c8aa4 이름표를 빌린다. 생략하면 목록 수명에 한 개를 소유한다.</summary>
    public ClientTemplateSectionNames? SectionNames { get; init; }
}

/// <summary>
/// 파일마다 분류와 목록 메타 조회를 서로 다른 실제 임시 Template(<see cref="CampaignTemplateStorage"/>)에서 수행하는 편집 맵 목록이다.
/// 전체 지형 파싱 없이 같은 원본 파일·섹션 읽기 부모에서 종류(State+17)·Subscriber·첫 State 바이트만 읽는다.
/// cpppj <c>client/SoloMapCatalog</c>를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 직접 구현한 임시 섹션 판독기(<c>Probe</c>)를 실제 Template 저장소로 교체했다. 그 전에는 서명·섹션 길이 검사를 따로 구현해
/// 저장소의 읽기 부모(서명·TYPE 수 진단·파일 제거 요청)와 달랐다. 생성자는 cpppj처럼 <see cref="SoloMapCatalogEffects"/> 하나로 장치를 받는다.
/// </remarks>
public sealed class SoloMapCatalog
{
    /// <summary>fortSpec·mayEditMissions의 현재 설정이다.</summary>
    private readonly ConfigInterface _config;

    /// <summary>언어·사용자·미정 입력의 실제 장치다.</summary>
    private readonly SoloMapCatalogEffects _effects;

    /// <summary>패턴의 파일 목록을 돌려주는 열거 장치다. 편집 작업본 장치를 주지 않으면 기본 디스크만 OS 순서로 열거한다.</summary>
    private readonly Func<string, IReadOnlyList<string>> _enumerate;

    /// <summary>분류와 메타마다 현재 파일을 읽는 장치다. 편집 작업본 장치를 주지 않으면 기본 파일 시스템의 우선순위로 읽는다.</summary>
    private readonly Func<string, byte[]?> _read;

    /// <summary>분류·메타 임시 저장소가 공유하는 직렬화 버전 전역이다. 저장소를 만들 때마다 복제하지 않는다.</summary>
    private readonly FortSerializationGlobals _serializationGlobals;

    /// <summary>목록의 모든 임시 manager가 공유하는 이름표 owner다.</summary>
    private readonly ClientTemplateSectionNames _sectionNames;

    /// <summary>마지막 construct/load가 선택한 실제 임시 Template이다.</summary>
    private MapProbe? _current;

    /// <summary>별도 Client 전역을 공급하지 않은 호출도 이 목록보다 짧지 않은 기본 키 버퍼를 빌린다.</summary>
    public SoloMapFileSpec FileSpec { get; } = new();

    /// <summary>같은 Client 수명의 50칸 캐시다. 실제 Client가 제공한 상태가 있으면 그것을 빌린다.</summary>
    public SoloMapListState State { get; }

    /// <summary>같은 실제 캐시와 파일 장치를 사용하는 원본 목록 본체다.</summary>
    public RawSoloMapList List { get; }

    /// <summary>
    /// 현재 파일·설정과 <paramref name="effects"/>의 장치를 빌린다. <see cref="SoloMapCatalogEffects.EnumeratePaths"/>와 <see cref="SoloMapCatalogEffects.ReadFile"/>을 주면
    /// 목록 열거와 파일 읽기가 기본 디스크 대신 편집 작업본 장치(<c>EditorCatalogWorkspace.Enumerate</c>/<c>Read</c>)를 거친다.
    /// 파일 시스템과 설정은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="files">기본 디스크 열거와 디스크·TAFF·보조 경로의 내용 읽기를 빌릴 파일 시스템.</param>
    /// <param name="config">fortSpec과 mayEditMissions의 현재 설정.</param>
    /// <param name="effects">언어·사용자·미정 입력 등 실제 장치.</param>
    public SoloMapCatalog(GameFileSystem files, ConfigInterface config, SoloMapCatalogEffects effects)
    {
        ArgumentNullException.ThrowIfNull(files);
        ArgumentNullException.ThrowIfNull(config);
        ArgumentNullException.ThrowIfNull(effects);
        _config = config;
        _effects = effects;
        _enumerate = effects.EnumeratePaths ?? files.MatchDisk;
        _read = effects.ReadFile ?? files.TryReadAllBytes;
        _serializationGlobals = effects.SerializationGlobals ?? new FortSerializationGlobals();
        _sectionNames = effects.SectionNames ?? new ClientTemplateSectionNames();
        State = effects.ListState ?? new SoloMapListState();
        List = new RawSoloMapList(State, Hooks());
    }

    /// <summary>선택한 외부 경계만 요구하고, 연결되지 않았으면 그 지점에서 명시 오류로 멈춘다.</summary>
    /// <typeparam name="T">장치 대리자 형태.</typeparam>
    /// <param name="device">요구할 장치.</param>
    private static T Need<T>(T? device) where T : class
    {
        return device ?? throw new InvalidOperationException("단독 맵 파일 목록의 현재 장치 미연결");
    }

    /// <summary>각 PathSpec 직전의 현재 키를 읽는다. 외부 키를 주지 않으면 이 목록의 기본 키를 쓴다.</summary>
    private string FileSpecKey() => _effects.FileSpecKey is { } key ? key() : FileSpec.Current();

    /// <summary>원본 004595a0은 같은 파일명에 있는 모든 점 뒤를 지운다. 마지막 확장자만 제거하지 않는다.</summary>
    /// <param name="path">열거된 경로.</param>
    private static string MapName(string path)
    {
        string normalized = path.Replace('\\', '/');
        int slash = normalized.LastIndexOf('/');
        string name = normalized[(slash + 1)..];
        int dot = name.IndexOf('.');
        return dot < 0 ? name : name[..dot];
    }

    /// <summary>선택된 실제 임시 Template이 없으면 원본의 잘못된 메모리 경계를 명시 거부한다.</summary>
    private MapProbe Current() => _current ?? throw new InvalidOperationException("단독 맵 목록의 임시 Template 생성 누락");

    /// <summary>실제 파일 읽기와 현재 자료만 원본 호출 순서의 각 경계에 공급한다.</summary>
    private SoloMapListHooks Hooks() => new()
    {
        MayEditMissions = () => _config.GetInt("mayEditMissions"),
        // 실제 열거 직전의 현재 키·설정만 읽으며 분류와 메타 읽기까지 캡처하지 않는다
        Enumerate = () => _enumerate(_config.PathSpec(FileSpecKey(), "*")).Select(MapName).ToArray(),
        Kind = Kind,
        Construct = () => _current = new MapProbe(this),
        Load = name => Current().Load(name, mine: true),
        Present = () => Current().Present(),
        Subscriber = () => Current().Storage.ReadSubscriberName(0),
        StateByte = () => Current().State.Get(0)[0],
        UserName = () => Need(_effects.UserName)(),
        Reserved = () => Current().Present() ? 0u : throw new InvalidOperationException("단독 맵 예약 조회의 누락 Template 진단"),
        Translate = text => Need(_effects.Translate)(text),
    };

    /// <summary>
    /// 동일 파일 키 버퍼의 현재 내용을 단독/일반 명세로 바꾼다. 이 목록의 기본 키(<see cref="FileSpec"/>)만 바꾸며,
    /// <see cref="SoloMapCatalogEffects.FileSpecKey"/>로 외부 키를 빌린 경우에는 그 키의 소유자를 직접 바꾼다.
    /// </summary>
    /// <param name="solo">참이면 "SOLOSpec", 거짓이면 "fortSpec".</param>
    public void SelectFileSpec(bool solo)
    {
        if (solo)
        {
            FileSpec.SelectSolo();
        }
        else
        {
            FileSpec.SelectFort();
        }
    }

    /// <summary>다음 목록 조회에서 캐시를 다시 만든다. 즉시 캐시를 바꾸지 않는다.</summary>
    public void Invalidate() => State.Dirty = 1;

    /// <summary>분류는 별도의 새 임시 Template으로 실제 파일을 읽고 State+17을 돌려준 뒤 반납한다. 빈 State는 분류 쪽에서도 따로 만든다.</summary>
    /// <param name="name">맵 이름.</param>
    public byte Kind(string name)
    {
        MapProbe probe = new(this);
        probe.Load(name, mine: false);
        return probe.State.Get(0)[17];
    }

    /// <summary>
    /// 파일마다 실제 임시 Template 한 개를 소유한다. 상태 getter는 같은 저장소의 State를 읽으며 원본의 기존·빈·짧은 State 부모를 모두 같은 저장소에 연결한다.
    /// 이 객체를 버리면 임시 저장소의 모든 manager가 함께 사라진다.
    /// </summary>
    private sealed class MapProbe
    {
        /// <summary>목록 수명이 정한 장치와 공유 전역을 빌리는 소유 목록.</summary>
        private readonly SoloMapCatalog _owner;

        /// <summary>원본 getter가 빌리는 로딩 깊이다.</summary>
        private uint _depth;

        /// <summary>실제 파일 읽기·현재 섹션·빈 State 생성의 소유자다.</summary>
        public CampaignTemplateStorage Storage { get; }

        /// <summary>같은 현재 manager의 State를 읽는 실제 getter다.</summary>
        public RawPlayerTemplateState State { get; }

        /// <summary>임시 저장소와 State getter를 같은 공유 전역·이름표 위에 만든다.</summary>
        /// <param name="owner">목록.</param>
        public MapProbe(SoloMapCatalog owner)
        {
            _owner = owner;
            Storage = new CampaignTemplateStorage(owner._serializationGlobals, owner._sectionNames);
            State = new RawPlayerTemplateState(
                OriginalEdition.Patch1078,
                Storage.Headers,
                slot => Storage.SelectState(slot),
                slot => Storage.Name(slot),
                () => _depth,
                value => _depth = value,
                () => false,
                _ => throw new InvalidOperationException("단독 맵 메타 조회의 실제 Template State 진단"),
                _ => throw new InvalidOperationException("10.78 단독 맵 조회의 예상하지 않은 CD 로딩 깊이 변경"));
            State.BindInitialization(new PlayerTemplateStateInitialization
            {
                Bytes = (_, bytes) => Need(owner._effects.AllocationBytes)(bytes),
                Replace = (slot, bytes) => Storage.InitializeState(slot, bytes),
            });
            State.BindMinimumSize((slot, size) => Storage.EnsureStateSize(slot, size));
        }

        /// <summary>
        /// 현재 키로 경로를 만들어 파일을 읽고 실제 저장소에 설치한다. 파일이 없어지면 빈 Template을 유지한다.
        /// 서명·본문 오류를 성공으로 대신하지 않는다. 목록 임시 Template의 isMine=1은 실제 파일 쓰기를 발생시키지 않는다.
        /// </summary>
        /// <param name="name">맵 이름.</param>
        /// <param name="mine">참이면 헤더 isMine을 1로 쓴다(목록용). 분류용은 0이다.</param>
        public void Load(string name, bool mine)
        {
            string path = _owner._config.PathSpec(_owner.FileSpecKey(), name);
            byte[]? bytes = _owner._read(path);
            if (bytes is null)
            {
                return;
            }
            Storage.InstallFile(0, new FortTemplate(), bytes, path, Need(_owner._effects.TypeCount)());
            Storage.Headers.Span[8] = (byte)(mine ? 1 : 0);
        }

        /// <summary>같은 현재 헤더의 첫 DWORD(manager)가 비어 있지 않은지 조회한다.</summary>
        public bool Present()
        {
            ReadOnlySpan<byte> bytes = Storage.Headers.Span;
            return bytes[0] != 0 || bytes[1] != 0 || bytes[2] != 0 || bytes[3] != 0;
        }
    }
}
