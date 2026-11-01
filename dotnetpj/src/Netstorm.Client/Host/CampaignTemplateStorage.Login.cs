using System.Globalization;
using Netstorm.Core.Original;
using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Client.Host;

public sealed partial class CampaignTemplateStorage
{
    /// <summary>실제 Client보다 짧은 Template 수명이 빌리는 현재 진단 출력이다. 파일 읽기와 일반 생성에는 요구하지 않는다.</summary>
    private DataManagerTrace? _diagnosticLog;

    /// <summary>
    /// 호출한 manager 핸들로 소유 이미지를 찾는다. 없으면 <c>std::map::at</c>의 out_of_range에 해당하는 범위 오류다.
    /// </summary>
    /// <param name="token">호스트 manager 핸들.</param>
    /// <exception cref="ArgumentOutOfRangeException">그 핸들의 소유 이미지가 없다.</exception>
    private Image Owned(uint token)
    {
        return _images.TryGetValue(token, out Image? image)
            ? image
            : throw new ArgumentOutOfRangeException(nameof(token), token, "Template 소유 manager 핸들 범위");
    }

    /// <summary>
    /// 원문 <c>new 276</c>만 실행한다: 아직 생성하지 않은 실제 빈 이미지를 소유 표에 더하고 불투명 핸들을 돌려준다.
    /// 칸 헤더는 쓰지 않으며 생성자·이름표·헤더 공개는 바깥 부모가 이어서 맡는다.
    /// </summary>
    /// <param name="bytes">요청 크기. 원문과 같은 276이어야 한다.</param>
    /// <returns>새 manager 핸들(비영).</returns>
    /// <exception cref="ArgumentException">크기가 276이 아니거나 호스트 핸들이 소진됐다.</exception>
    /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
    public uint AllocateEmptyManager(uint bytes)
    {
        if (bytes != 276 || _nextHandle == 0)
        {
            throw new ArgumentException("Template 실제빈Image276할당계약");
        }
        uint token = _nextHandle++;
        _images.Add(token, new Image(_sectionNames));
        return token;
    }

    /// <summary>
    /// 존재하는 같은 할당만 원문 생성자로 전환한다: 공유 직렬화 버전을 0으로 쓰고 이름을 기록한다. 헤더 공개는 바깥 부모가 맡는다.
    /// </summary>
    /// <param name="token"><see cref="AllocateEmptyManager"/>가 돌려준 핸들.</param>
    /// <param name="name">파일 이름(255바이트 이하, NUL 없음).</param>
    /// <returns>같은 핸들.</returns>
    /// <exception cref="ArgumentOutOfRangeException">그 핸들의 이미지가 없다.</exception>
    /// <exception cref="ArgumentException">이미 생성했거나 이름 계약을 어겼다.</exception>
    /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
    public uint ConstructEmptyManager(uint token, string name)
    {
        Owned(token).ConstructName(name, _serializationGlobals);
        return token;
    }

    /// <summary>
    /// 고정 핸들의 manager 276바이트를 빌린다. 칸 헤더를 교체한 뒤에도 같은 이미지의 물리이며, 현재 헤더를 다시 읽지 않는다.
    /// </summary>
    /// <param name="token">manager 핸들.</param>
    /// <exception cref="ArgumentOutOfRangeException">그 핸들의 이미지가 없다.</exception>
    /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
    public Span<byte> ManagerBytes(uint token) => Owned(token).Manager;

    /// <summary>
    /// 고정 manager의 실제 35개 Create를 공유 변경 가능 이름표로 순회한다. 이름표는 진단 뒤 한 번 받고 같은 이미지의 기존 raw Create를 이름마다 실행한다.
    /// 선택 진단(root가 비어 있지 않음 등)은 생략하지 않는다.
    /// </summary>
    /// <param name="token">원문 생성자를 마친 manager 핸들.</param>
    /// <param name="assertion">원본 진단 장치. 반환하면 원문대로 계속한다.</param>
    /// <exception cref="ArgumentOutOfRangeException">그 핸들의 이미지가 없거나 이름표 항목에 NUL이 없다.</exception>
    /// <exception cref="InvalidOperationException">원문 생성자 이전이거나 진단 장치가 필요한데 연결되지 않았다.</exception>
    /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
    public void InitializeEmptyManager(uint token, Action<DataManagerInitializeAssertion>? assertion = null)
    {
        Image image = Owned(token);
        if (!image.Constructed)
        {
            throw new InvalidOperationException("Template 실제ctor이전초기화");
        }
        DataManagerInitializeHooks hooks = new()
        {
            Assertion = assertion,
            Names = () => _sectionNames.Names(),
            Create = name =>
            {
                int end = name.IndexOf((byte)0);
                if (end < 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(name), "Template 실제Create이름NUL범위");
                }
                image.CreateSection(Latin1.GetString(name[..end]));
            },
        };
        RawDataManagerInitialize.Initialize(image.Manager, hooks);
    }

    /// <summary>같은 실제 이미지의 소멸/할당 반납을 원문 바깥 부모(NewManager)에 빌려 준다.</summary>
    /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
    public TemplateLifetimeHooks ManagerLifetime() => Effects();

    /// <summary>
    /// 실제 Client의 level 29 로그 장치를 빌린다. 지역 송신 이미지의 덤프가 같은 공유 formatter/FILE/Chat을 사용하게 하며,
    /// 장치는 실제 덤프 시점에 호출한다. 연결하지 않으면 로그인 이미지 읽기가 덤프 경계 누락으로 거부된다.
    /// </summary>
    /// <param name="log">level 29 출력 장치.</param>
    /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
    public void BindDiagnosticLog(DataManagerTrace? log)
    {
        _diagnosticLog = log;
    }

    /// <summary>
    /// 원문 004b63d0의 지역 Template 읽기와 004bf3e0의 5섹션 복제·직렬화·반납을 실행해 로그인 송신용 이미지를 만든다.
    /// 송신용 지역 Template와 strip용 NONAME는 같은 Client 전역(직렬화 버전·이름표)을 빌리는 임시 저장소에 두며, 완료와 예외 모두 target부터 source 순서로 반납한다.
    /// 전역 9칸 헤더와 이 저장소의 manager·커서는 건드리지 않는다. 이미지 안에 남기는 섹션은 Subscriber·Territory·TypeNames·CoreData와 현재 영역의 Terr 하나뿐이다.
    /// 근거: <c>docs/exe/cpp-login-template-lifetime-20261101.md</c>, <c>docs/exe/cpp-datamanager-dump-20261101.md</c>.
    /// </summary>
    /// <param name="name">지역 Template 파일 이름.</param>
    /// <param name="file">파일 읽기·타입 수 장치. 권한·현재 슬롯·제목은 쓰지 않도록 이 호출이 막는다.</param>
    /// <param name="territory">현재 영역 번호(DWORD 비트를 그대로 signed 서식으로 쓴다).</param>
    /// <returns>strip한 0x8000 고정 할당의 이미지.</returns>
    /// <exception cref="InvalidOperationException">지역 Template 읽기가 정상 false를 돌려줬다(파일 없음·서명 거부), 현재 영역의 Terr 섹션이 원문에 없다, 덤프 장치가 없다 등.</exception>
    /// <remarks>2026-11-02 0.134(TS4)에서 추가했다. cpppj의 <c>ReadLoginImage</c>를 옮겼다.</remarks>
    public FortImageData ReadLoginImage(string name, TemplateFileLoadHooks file, uint territory)
    {
        ArgumentNullException.ThrowIfNull(name);
        ArgumentNullException.ThrowIfNull(file);
        CampaignTemplateStorage temporary = new(_serializationGlobals, _sectionNames);
        // 지역 Template 주소는 전역 현재 슬롯과 항상 다르며 readonly 사전 설정은 파일 권한 조회를 생략한다
        TemplateFileLoadHooks load = file with { CurrentSlot = () => 9u, RefreshTitle = null };
        Put(temporary.Header(0), 8, 1);
        try
        {
            temporary.LoadFile(0, new FortTemplate(), name, load);
            if (Read(temporary.Header(0), 0) == 0)
            {
                throw new InvalidOperationException("로그인송신의지역Template읽기실패진단");
            }
            temporary.Install(1, new FortTemplate(), "NONAME");
            Image source = temporary.Current(0);
            Image target = temporary.Current(1);
            // 서로 다른 실제 노드를 같은 raw 복제 장치에서 식별하도록 토큰 범위를 이어 쓴다
            target.NextStreamHandle = source.NextStreamHandle;
            temporary.InitializeEmptyManager(Read(temporary.Header(1), 0), _ => throw new InvalidOperationException("로그인이미지의NONAME초기화진단"));
            // 원문 004bf3e0의 네 고정 이름을 순서대로 복제하고 현재 Terr만 마지막에 복제한다
            foreach (string section in LoginKeptSections)
            {
                target.CloneSectionFrom(source, section);
            }
            target.CloneSectionFrom(source, TerritoryName(territory));
            // 복제가 끝난 같은 NONAME를 SaveImage 전에 검색/로그하며 필터가 닫혀도 current 검색은 생략하지 않는다
            target.Dump("Stripped Image:", _diagnosticLog);
            FortImageHooks image = new()
            {
                TypeCount = () => unchecked((uint)NeedFile(file.TypeCount)()),
                Assertion = _ => throw new InvalidOperationException("로그인이미지의직렬화진단"),
            };
            FortImageData result = target.SaveImage(image);
            temporary.Clear(1);
            temporary.Clear(0);
            return result;
        }
        catch
        {
            // 부분 읽기/복제 예외에서도 지역 노드/할당을 유지한 채 전역 owner로 넘기지 않는다
            temporary.Clear(1);
            temporary.Clear(0);
            throw;
        }
    }

    /// <summary>strip한 로그인 이미지에 항상 남기는 네 고정 섹션 이름이다(원문 004bf3e0의 복제 순서).</summary>
    private static readonly string[] LoginKeptSections = ["Subscriber", "Territory", "TypeNames", "CoreData"];

    /// <summary>
    /// 원문 서식 <c>"Terr%02d"</c>의 이름을 만든다. 값은 DWORD 비트를 그대로 signed로 보며, 음수는 부호 뒤에 폭 2를 채워 그대로 쓴다.
    /// </summary>
    /// <param name="territory">현재 영역 번호의 DWORD 표현.</param>
    private static string TerritoryName(uint territory)
    {
        int value = unchecked((int)territory);
        string digits = Math.Abs((long)value).ToString(CultureInfo.InvariantCulture);
        return "Terr" + (value < 0 ? "-" + digits : digits.PadLeft(2, '0'));
    }
}
