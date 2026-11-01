using Netstorm.Core.Original;
using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Client.Host;

/// <summary>
/// Client와 Session이 같은 Template 9칸·실제 manager 이미지·State 커서 수명을 공유하는 저장소다(cpppj <c>client/CampaignTemplateStorage</c>).
/// 9칸 헤더(20바이트×9)는 원본 전역 배열이고, 헤더의 첫 DWORD가 가리키는 manager는 이 저장소가 소유하는 <c>Image</c>다.
/// 빌린 헤더(<see cref="Borrow"/>)는 같은 manager의 State·지형·Money·Badges·Technology 커서를 같은 객체로 본다.
/// 호스트 포인터를 DWORD로 자르지 않고 불투명 핸들을 쓰며, 핸들 0은 항상 "없음"이다.
/// </summary>
/// <remarks>
/// 2026-11-01 0.131(TS1)에서 설치·선택 범위를 옮겼다: 9칸 수명(<see cref="Install"/>·<see cref="Borrow"/>·<see cref="Clear"/>·<see cref="ClearAll"/>),
/// 커서 조회·선택(State·Territory·Money·Badges·Technology), <see cref="SectionOrder"/>·<see cref="CurrentSection"/>·<see cref="ReadSubscriberName"/>·
/// <see cref="FileHeader"/>·<see cref="Name"/>, State 확장(<see cref="GrowState"/>). 같은 날 0.132(TS2)에서 동적 섹션(<see cref="CreateSection"/>·
/// <see cref="FindSection"/>·<see cref="SelectSection"/>·<see cref="RenameSection"/>·<see cref="CurrentWriteStream"/>·<see cref="WriteSection"/>·<see cref="AccessSection"/>)과
/// <see cref="InitializeState"/>·<see cref="EnsureStateSize"/>를 더했다. 0.133(TS3)에서 파일 읽기/저장(<see cref="InstallFile"/>·<see cref="LoadFile"/>·<see cref="SaveFile"/>)과
/// <see cref="BindCurrentName"/>을 더했다. 0.134(TS4)에서 빈 manager(<see cref="AllocateEmptyManager"/>·<see cref="ConstructEmptyManager"/>·<see cref="InitializeEmptyManager"/>·
/// <see cref="ManagerBytes"/>·<see cref="ManagerLifetime"/>)와 로그인 송신 이미지(<see cref="BindDiagnosticLog"/>·<see cref="ReadLoginImage"/>)를 <c>CampaignTemplateStorage.Login.cs</c>에 더했다.
/// 계획: <c>docs/dotnet-template-storage-plan-20261101.md</c>.
/// C++의 참조 반환은 같은 객체를 돌려주는 클래스 인스턴스로 옮겼으므로 반환된 커서는 저장소가 manager를 반납할 때까지 같은 참조다.
/// </remarks>
public sealed partial class CampaignTemplateStorage
{
    /// <summary>원본 전역 배열의 Template 칸 수(9)다.</summary>
    private const int TemplateCount = RawTemplateLifetime.TemplateCount;

    /// <summary>Template 헤더 한 칸의 실제 폭(20바이트)이다.</summary>
    private const int HeaderBytes = RawTemplateLifetime.HeaderBytes;

    /// <summary>모든 Image/외부 Session보다 먼저 확보하는 실제 공유 DWORD(005c89c0) 소유권이다.</summary>
    private readonly FortSerializationGlobals _serializationGlobals;

    /// <summary>모든 Image보다 오래 사는 동일 512바이트 lazy 이름표 owner다. 독립 Storage는 새 Client 상태를 한 개 소유한다.</summary>
    private readonly ClientTemplateSectionNames _sectionNames;

    /// <summary>Client보다 짧은 Session이 사라져도 유지되는 실제 9개 헤더(180바이트)다.</summary>
    private readonly byte[] _headers = new byte[TemplateCount * HeaderBytes];

    /// <summary>호스트 포인터를 DWORD로 자르지 않는 실제 소유 객체 표다.</summary>
    private readonly Dictionary<uint, Image> _images = [];

    /// <summary>서로 살아 있는 manager를 구별하는 비영 호스트 핸들이다.</summary>
    private uint _nextHandle = 1;

    /// <summary>
    /// 원본 9개 헤더의 처음 20바이트를 원본 생성 몸체로 초기화하고 같은 Client 전역을 빌린다. 생략하면 독립 전역 한 개를 소유한다.
    /// 시작 초기화는 기존 manager를 반납하는 전역 정리(<see cref="ClearAll"/>)와 다르다.
    /// </summary>
    /// <param name="serializationGlobals">같은 Client의 직렬화 버전 전역. 생략하면 새로 만든다.</param>
    /// <param name="sectionNames">같은 Client의 이름표 owner. 생략하면 새로 만든다.</param>
    public CampaignTemplateStorage(FortSerializationGlobals? serializationGlobals = null, ClientTemplateSectionNames? sectionNames = null)
    {
        _serializationGlobals = serializationGlobals ?? new FortSerializationGlobals();
        _sectionNames = sectionNames ?? new ClientTemplateSectionNames();
        // 아홉 개의 처음 20바이트만 원본 생성 몸체로 초기화한다
        for (uint slot = 0; slot < TemplateCount; slot++)
        {
            RawTemplateLifetime.InitializeHeader(Header(slot));
        }
    }

    /// <summary>실제 180바이트 헤더를 같은 Player/로더에 공급한다. 사본이 아니다.</summary>
    public Memory<byte> Headers => _headers;

    /// <summary>reader와 신규 초기화가 공유하는 Client 이름표 owner다.</summary>
    public ClientTemplateSectionNames SectionNamesOwner => _sectionNames;

    /// <summary>생성자·객체 로더·저장 부모가 직접 같이 쓰는 실제 005c89c0 DWORD를 가진 공유 객체다. 복사본이 아니다.</summary>
    public FortSerializationGlobals SerializationGlobals => _serializationGlobals;

    /// <summary>실제 공유 직렬화 버전의 현재 값이다(cpppj의 <c>SerializationVersion()</c> 참조 반환에 해당). 읽고 쓰는 모두 같은 DWORD다.</summary>
    public uint SerializationVersion
    {
        get => _serializationGlobals.Version;
        set => _serializationGlobals.Version = value;
    }

    /// <summary>현재 실제 manager 할당 수를 읽기만 한다.</summary>
    public int OwnedImages => _images.Count;

    /// <summary>유효 슬롯의 현재 기록 20바이트를 빌린다.</summary>
    /// <param name="slot">0~8.</param>
    /// <exception cref="ArgumentOutOfRangeException">슬롯이 9칸 밖이다.</exception>
    private Span<byte> Header(uint slot)
    {
        if (slot >= TemplateCount)
        {
            throw new ArgumentOutOfRangeException(nameof(slot), slot, "Template 실제9칸 범위");
        }
        return _headers.AsSpan((int)slot * HeaderBytes, HeaderBytes);
    }

    /// <summary>헤더가 빌린 같은 핸들도 같은 이미지/State 커서를 찾는다.</summary>
    /// <param name="slot">0~8.</param>
    /// <exception cref="InvalidOperationException">현재 manager가 없거나 소유자보다 오래 산 빌림이다.</exception>
    private Image Current(uint slot)
    {
        uint handle = Read(Header(slot), 0);
        if (handle == 0 || !_images.TryGetValue(handle, out Image? image))
        {
            throw new InvalidOperationException("Template 현재 manager가 없거나 소유자보다 오래 산 빌림");
        }
        return image;
    }

    /// <summary>실제 소멸은 안쪽 버퍼를 비우고, 반납은 manager 객체 할당을 해제한다. 소멸 뒤 요새·State·지형 커서를 비운다.</summary>
    private TemplateLifetimeHooks Effects()
    {
        return new TemplateLifetimeHooks
        {
            Destroy = handle =>
            {
                Image image = _images[handle];
                image.Destroy();
                image.Fort = new FortTemplate();
                image.ResetState();
                image.Territory.Reset();
                image.RetiredState.Release();
            },
            Release = handle =>
            {
                if (!_images.Remove(handle))
                {
                    throw new InvalidOperationException("Template 같은 manager 중복 반납");
                }
            },
        };
    }

    /// <summary>
    /// 기존 한 manager를 정확히 해제한 뒤 새 실제 객체를 소유한다.
    /// </summary>
    /// <param name="slot">설치할 칸(0~8).</param>
    /// <param name="fort">요새. 소유권이 넘어가므로 호출자가 계속 쓰려면 <see cref="FortTemplate.Clone"/>한 사본을 넘긴다.</param>
    /// <param name="name">파일 이름.</param>
    /// <exception cref="OverflowException">호스트 manager 핸들이 소진됐다.</exception>
    public void Install(uint slot, FortTemplate fort, string name)
    {
        Clear(slot);
        if (_nextHandle == 0)
        {
            throw new OverflowException("Template 호스트 manager 핸들 소진");
        }
        uint handle = _nextHandle++;
        _images.Add(handle, new Image(fort, name, _serializationGlobals, _sectionNames));
        Put(Header(slot), 0, handle);
    }

    /// <summary>
    /// 같은 manager를 빌린다. 빌린 헤더를 반납해도 원래 소유자의 실제 버퍼/커서는 유지된다.
    /// 빌림 헤더는 첫 DWORD(핸들)와 +4(빌림 값 1)만 쓴다.
    /// </summary>
    /// <param name="slot">빌릴 칸.</param>
    /// <param name="source">소유자 칸.</param>
    /// <exception cref="ArgumentException">자기 자신을 빌리거나, 원래 소유자를 반납하게 되는 같은 manager 빌림이다.</exception>
    public void Borrow(uint slot, uint source)
    {
        if (slot == source)
        {
            throw new ArgumentException("Template 자기 manager 빌림 계약");
        }
        // 소유자가 실제로 manager를 가졌는지 먼저 확인한다
        _ = Current(source);
        uint handle = Read(Header(source), 0);
        Span<byte> target = Header(slot);
        if (Read(target, 0) == handle && Read(target, 4) == 0)
        {
            throw new ArgumentException("Template 원래 소유자를 반납하는 같은 manager 빌림");
        }
        Clear(slot);
        Put(Header(slot), 0, handle);
        Put(Header(slot), 4, 1);
    }

    /// <summary>비영 포인터만 정리하는 원본 전역 몸체를 그대로 사용한다. 포인터 0인 칸의 stale 필드는 보존한다.</summary>
    public void ClearAll()
    {
        RawTemplateLifetime.ClearHeaders(_headers, Effects());
    }

    /// <summary>원본 단일 반납이다. 포인터 0이어도 한 칸 20바이트는 0으로 비운다.</summary>
    /// <param name="slot">반납할 칸.</param>
    public void Clear(uint slot)
    {
        RawTemplateLifetime.ReleaseHeader(Header(slot), Effects());
    }

    /// <summary>모든 소비자는 같은 현재 manager의 요새를 다시 조회한다.</summary>
    /// <param name="slot">칸.</param>
    public FortTemplate Template(uint slot) => Current(slot).Fort;

    /// <summary>한 manager를 빌린 헤더끼리도 같은 State 커서 객체를 돌려준다.</summary>
    /// <param name="slot">칸.</param>
    public PlayerTemplateStateStream State(uint slot) => Current(slot).State;

    /// <summary>실제 Player State getter가 같은 목록/current를 선택(쓰기 허용)한 뒤 같은 커서를 돌려받는다.</summary>
    /// <param name="slot">칸.</param>
    public PlayerTemplateStateStream SelectState(uint slot)
    {
        Image image = Current(slot);
        image.SelectState();
        return image.State;
    }

    /// <summary>파싱 시점의 편의 사본이 아니라 실제 현재 Subscriber 섹션/노드를 읽어 이름을 돌려준다.</summary>
    /// <param name="slot">칸.</param>
    public string ReadSubscriberName(uint slot) => Current(slot).ReadSubscriberName();

    /// <summary>같은 manager의 지형 커서를 선택 효과 없이 빌린다.</summary>
    /// <param name="slot">칸.</param>
    public FortTerritoryStream Territory(uint slot) => Current(slot).Territory;

    /// <summary>월드 로더가 같은 manager에서 원본 읽기 선택을 한 뒤 같은 지형 커서를 사용한다(current와 pos가 갱신된다).</summary>
    /// <param name="slot">칸.</param>
    public FortTerritoryStream SelectTerritory(uint slot)
    {
        Image image = Current(slot);
        image.SelectTerritory();
        return image.Territory;
    }

    /// <summary>같은 manager의 현재 Money 노드 커서를 선택 효과 없이 빌린다.</summary>
    /// <param name="slot">칸.</param>
    public FortMoneyCursor Money(uint slot)
    {
        Image image = Current(slot);
        image.LoadCursor("Money");
        return image.Money;
    }

    /// <summary>Money는 원본 wrapper의 읽기 전용 선택(004bf340의 쓰기 불가)을 적용하고 pos를 0으로 되돌린다.</summary>
    /// <param name="slot">칸.</param>
    public FortMoneyCursor SelectMoney(uint slot)
    {
        Image image = Current(slot);
        image.SelectLoadSection("Money", 0);
        return image.Money;
    }

    /// <summary>같은 manager의 Badges 노드와 실제 버퍼를 선택 효과 없이 빌린다.</summary>
    /// <param name="slot">칸.</param>
    public FortMetaStream Badges(uint slot)
    {
        Image image = Current(slot);
        image.LoadCursor("Badges");
        return image.Badges;
    }

    /// <summary>빈 Badges도 허용하는 원본 선택 모드(004be8e0의 쓰기 허용 1)를 유지한다.</summary>
    /// <param name="slot">칸.</param>
    public FortMetaStream SelectBadges(uint slot)
    {
        Image image = Current(slot);
        image.SelectLoadSection("Badges", 1);
        return image.Badges;
    }

    /// <summary>같은 manager의 Technology 노드와 실제 버퍼를 선택 효과 없이 빌린다.</summary>
    /// <param name="slot">칸.</param>
    public FortMetaStream Technology(uint slot)
    {
        Image image = Current(slot);
        image.LoadCursor("Technology");
        return image.Technology;
    }

    /// <summary>원본 기술 로더가 빈 섹션의 부모/UI 처리 여부를 직접 결정하도록 쓰기 허용(004beee0의 1)으로 선택한다.</summary>
    /// <param name="slot">칸.</param>
    public FortMetaStream SelectTechnology(uint slot)
    {
        Image image = Current(slot);
        image.SelectLoadSection("Technology", 1);
        return image.Technology;
    }

    /// <summary>원문 선택 때 쓴 같은 manager+c의 파일 이름을 돌려준다.</summary>
    /// <param name="slot">칸.</param>
    public string Name(uint slot) => Current(slot).CurrentName();

    /// <summary>빌린 헤더도 원래 소유자의 같은 manager 꼬리 DWORD(+10c 서명, +110 FE 판정)를 읽는다. 읽기/저장 전에는 0/0xffffffff다.</summary>
    /// <param name="slot">칸.</param>
    public (uint Signature, uint FeFlag) FileHeader(uint slot)
    {
        byte[] manager = Current(slot).Manager;
        return (Read(manager, 0x10c), Read(manager, 0x110));
    }

    /// <summary>빌린 헤더도 같은 manager의 실제 연결 순서를 확인한다.</summary>
    /// <param name="slot">칸.</param>
    public IReadOnlyList<string> SectionOrder(uint slot) => Current(slot).SectionOrder();

    /// <summary>빌린 헤더도 같은 manager가 마지막으로 선택한 실제 섹션 이름을 읽는다. 선택이 없으면 빈 문자열이다.</summary>
    /// <param name="slot">칸.</param>
    public string CurrentSection(uint slot) => Current(slot).CurrentSection();

    /// <summary>
    /// 원본을 옮겨 보존한 뒤 같은 Fort State 섹션에 크기만큼의 대체 할당을 만든다. 이전 할당은 <see cref="CommitStateGrowth"/>까지
    /// 호출자가 복사할 수 있게 유지한다. 반환한 <see cref="Memory{T}"/>는 이 호출이 새로 연 할당이며 크기가 <paramref name="size"/>다.
    /// </summary>
    /// <param name="slot">칸.</param>
    /// <param name="size">새 크기.</param>
    /// <exception cref="InvalidOperationException">앞선 확장의 복사가 아직 끝나지 않았다.</exception>
    public Memory<byte> GrowState(uint slot, uint size)
    {
        Image image = Current(slot);
        if (image.RetiredState.Count != 0)
        {
            throw new InvalidOperationException("Template State 확장 복사 미완료");
        }
        int at = Image.StateSection();
        // 표준 State 위치가 아직 없으면 빈 섹션으로 늘린다
        while (image.Fort.Sections.Count <= at)
        {
            image.Fort.Sections.Add(new RawSectionBuffer());
        }
        RawSectionBuffer bytes = image.Fort.Sections[at];
        image.RetiredState.TakeFrom(bytes);
        bytes.Resize(checked((int)size));
        return bytes.Memory;
    }

    /// <summary>Get이 복사를 끝낸 뒤에만 이전 State 실제 버퍼를 반납한다.</summary>
    /// <param name="slot">칸.</param>
    public void CommitStateGrowth(uint slot)
    {
        Current(slot).RetiredState.Release();
    }

    /// <summary>
    /// 미정 바이트를 포함한 완성 28바이트를 실제 새 State 섹션에 쓰고 같은 현재 커서를 돌려준다. 기존 State는 반납되고
    /// 새 섹션이 목록 끝에 붙으며(이미 마지막이면 같은 자리), 앞의 마지막 섹션은 end 크기로 압축된다.
    /// </summary>
    /// <param name="slot">칸.</param>
    /// <param name="bytes">State 28바이트.</param>
    /// <exception cref="ArgumentException">입력이 28바이트가 아니다.</exception>
    public PlayerTemplateStateStream InitializeState(uint slot, ReadOnlySpan<byte> bytes)
    {
        Image image = Current(slot);
        image.InitializeState(bytes);
        return image.State;
    }

    /// <summary>생성한 섹션의 manager·노드·생성 수명을 한 참조로 고정해 이후 current 변경과 분리한다.</summary>
    /// <param name="manager">manager 핸들.</param>
    /// <param name="image">manager의 이미지.</param>
    /// <param name="section">노드 핸들.</param>
    private static TemplateWriteStream WriteStreamOf(uint manager, Image image, uint section)
    {
        return new TemplateWriteStream(manager, section, image.Node(section).Generation);
    }

    /// <summary>
    /// 원본 생성 부모로 이름의 기존 노드를 교체하고 새 마지막 노드의 참조를 돌려준다. Temp 같은 동적 이름도 허용한다.
    /// </summary>
    /// <param name="slot">칸.</param>
    /// <param name="name">섹션 이름.</param>
    public TemplateWriteStream CreateSection(uint slot, string name)
    {
        uint manager = Read(Header(slot), 0);
        Image image = Current(slot);
        return WriteStreamOf(manager, image, image.CreateSection(name));
    }

    /// <summary>
    /// 원문이 이미 선택한 현재 manager를 고정해 섹션을 만든다. 뒤 콜백에서 헤더가 교체돼도 다른 manager를 재선택하지 않는다.
    /// </summary>
    /// <param name="manager">manager 핸들.</param>
    /// <param name="name">섹션 이름.</param>
    /// <exception cref="ArgumentOutOfRangeException">그 manager가 이미 반납됐거나 없다.</exception>
    public TemplateWriteStream CreateManagerSection(uint manager, string name)
    {
        if (manager == 0 || !_images.TryGetValue(manager, out Image? image))
        {
            throw new ArgumentOutOfRangeException(nameof(manager), manager, "섹션 생성의 실제 manager 수명 누락");
        }
        return WriteStreamOf(manager, image, image.CreateSection(name));
    }

    /// <summary>검색 실패도 원문 current 0을 유지하며 참조 유효성과 빈 결과(<c>default</c>)를 구별한다.</summary>
    /// <param name="slot">칸.</param>
    /// <param name="name">섹션 이름.</param>
    public TemplateWriteStream FindSection(uint slot, string name)
    {
        uint manager = Read(Header(slot), 0);
        Image image = Current(slot);
        uint section = image.FindSection(name);
        return section != 0 ? WriteStreamOf(manager, image, section) : default;
    }

    /// <summary>선택 부모의 실제 pos 0/읽기 진단을 끝낸 같은 노드의 생성 수명을 돌려준다.</summary>
    /// <param name="slot">칸.</param>
    /// <param name="name">섹션 이름.</param>
    /// <param name="writable">쓰기 허용 여부(0이면 빈 섹션을 진단으로 거부).</param>
    public TemplateWriteStream SelectSection(uint slot, string name, int writable)
    {
        uint manager = Read(Header(slot), 0);
        Image image = Current(slot);
        return WriteStreamOf(manager, image, image.SelectLoadSection(name, writable));
    }

    /// <summary>
    /// 실제 Rename 부모가 대상을 반납한 뒤 원래 source 노드의 이름만 바꾼 결과를 돌려준다. 노드 핸들·생성 수명·버퍼 할당을 보존하며
    /// current/head/next를 편의로 다시 쓰지 않는다.
    /// </summary>
    /// <param name="slot">칸.</param>
    /// <param name="destination">새 이름.</param>
    /// <param name="source">현재 이름.</param>
    public TemplateWriteStream RenameSection(uint slot, string destination, string source)
    {
        uint manager = Read(Header(slot), 0);
        Image image = Current(slot);
        return WriteStreamOf(manager, image, image.RenameSection(destination, source));
    }

    /// <summary>
    /// Create 반환값과 독립적으로 현재 선택을 보관한다. 과거 파일에서 읽은 노드에도 첫 빌림 수명을 부여한다.
    /// </summary>
    /// <param name="slot">칸.</param>
    /// <exception cref="InvalidOperationException">현재 선택된 노드가 없다.</exception>
    public TemplateWriteStream CurrentWriteStream(uint slot)
    {
        uint manager = Read(Header(slot), 0);
        Image image = Current(slot);
        uint section = Read(image.Manager, 8);
        if (section == 0 || !image.Streams.ContainsKey(section))
        {
            throw new InvalidOperationException("Template 현재 쓰기 stream의 실제 노드 누락");
        }
        image.SyncCursorPosition(section);
        return WriteStreamOf(manager, image, section);
    }

    /// <summary>반납된 manager와 재생성된 동일 이름의 새 노드를 이전 쓰기 대상으로 취급하지 않는다.</summary>
    /// <param name="stream">보관한 쓰기 참조.</param>
    /// <exception cref="InvalidOperationException">manager나 섹션이 반납됐거나 생성 수명이 다르다.</exception>
    private Image Writable(TemplateWriteStream stream)
    {
        if (!_images.TryGetValue(stream.Manager, out Image? image)
            || stream.Section == 0
            || stream.Generation == 0
            || !image.Streams.TryGetValue(stream.Section, out Image.Stream? node)
            || node.Generation != stream.Generation)
        {
            throw new InvalidOperationException("Template 쓰기 참조의 해제된 manager/섹션 수명");
        }
        return image;
    }

    /// <summary>원본 일반 MemStream 쓰기를 보관한 노드의 현재 커서에 적용한다. 현재 선택과 무관하다.</summary>
    /// <param name="stream">보관한 쓰기 참조.</param>
    /// <param name="bytes">쓸 바이트.</param>
    public void WriteSection(TemplateWriteStream stream, ReadOnlySpan<byte> bytes)
    {
        Writable(stream).WriteStream(stream.Section, bytes);
    }

    /// <summary>
    /// TypeNames처럼 current가 다른 섹션으로 바뀐 경우에도 보관한 실제 노드만 빌려 인라인 쓰기를 적용한다.
    /// 연산 동안 같은 노드의 중첩 빌림과 섹션 구조 변경은 허용하지 않는다.
    /// </summary>
    /// <param name="stream">보관한 쓰기 참조.</param>
    /// <param name="operation">같은 16바이트·버퍼 장치를 빌리는 연산.</param>
    public void AccessSection(TemplateWriteStream stream, TemplateSectionAccess operation)
    {
        Writable(stream).AccessSection(stream.Section, operation);
    }

    /// <summary>
    /// 파싱된 보조 자료와 같은 실제 파일 원문을 새 manager에 읽는다(임의의 빈 섹션 목록을 먼저 만들지 않는다). 실제 파일/서명/섹션 읽기 부모를 실행하며,
    /// 읽기가 거부되면 예외를 던지고 칸에는 아무 manager도 남지 않는다. 읽기 중 원문이 칸 헤더를 공개하지 않으므로 성공 뒤에만 헤더에 쓴다.
    /// </summary>
    /// <param name="slot">설치할 칸.</param>
    /// <param name="fort">파싱된 보조 자료. 실제 섹션은 파일 reader가 만든 할당이 대체한다. 소유권이 넘어간다.</param>
    /// <param name="image">파일 전체 바이트.</param>
    /// <param name="name">파일 이름.</param>
    /// <param name="typeCount">서명 검사가 도달한 순간의 현재 signed TYPE 개수. 기본 188은 10.78 독립 호출용이며 Session은 현재 타입 수를 전달한다.</param>
    /// <exception cref="OverflowException">호스트 manager 핸들이 소진됐다.</exception>
    /// <exception cref="InvalidOperationException">파일 열기/서명이 거부됐다.</exception>
    /// <exception cref="InvalidDataException">잘못된 서명의 원본 파일 제거 요청이다(보호 원문은 삭제하지 않는다).</exception>
    public void InstallFile(uint slot, FortTemplate fort, ReadOnlySpan<byte> image, string name, int typeCount = 188)
    {
        Clear(slot);
        if (_nextHandle == 0)
        {
            throw new OverflowException("Template 파일 manager 핸들 소진");
        }
        Image created = new(fort, name, _serializationGlobals, _sectionNames, fileImage: true);
        created.ReadFile(image, typeCount);
        uint handle = _nextHandle++;
        _images.Add(handle, created);
        Put(Header(slot), 0, handle);
    }

    /// <summary>
    /// 원문 Load(004bf550)를 실제 소유자에 연결한다: 기존 같은 파일은 그대로 두고 빈 칸에만 manager를 만들고 읽기 전에 헤더를 공개하며,
    /// 정상 false(파일 없음·서명 거부)는 현재 manager만 반납하고, 예외는 이미 공개한 manager를 되돌리지 않는다.
    /// </summary>
    /// <param name="slot">읽을 칸.</param>
    /// <param name="fort">파싱된 보조 자료. 실제 섹션은 reader의 할당을 쓴다. 소유권이 넘어간다.</param>
    /// <param name="name">요청 파일 이름.</param>
    /// <param name="file">실제 파일·CRT·현재 슬롯·제목 경계. 선택한 분기가 쓰는 항목만 연결하면 된다.</param>
    public void LoadFile(uint slot, FortTemplate fort, string name, TemplateFileLoadHooks file)
    {
        ArgumentNullException.ThrowIfNull(file);
        TemplateLoadHooks hooks = new()
        {
            // 항상 전달받은 현재 handle의 실제 +12를 빌린다
            FileName = manager => RawTemplateLoad.FileName(_images[manager].Manager),
            SameName = (first, second) => NeedFile(file.SameName)(first, second),
            WrongName = () => NeedFile(file.WrongName)(),
            Allocate = bytes =>
            {
                if (bytes != 276 || _nextHandle == 0)
                {
                    throw new OverflowException("Template Load의 실제276BYTE/호스트handle 소진");
                }
                return _nextHandle++;
            },
            // 생성자의 공유 version 0과 실제 Image 소유를 끝낸 뒤 원문 부모가 header.ptr를 공개한다
            Construct = (manager, fileName) =>
            {
                Image created = new(fort, fileName, _serializationGlobals, _sectionNames, fileImage: true);
                created.Fort.Sections.Clear();
                if (!_images.TryAdd(manager, created))
                {
                    throw new InvalidOperationException("Template Load manager의 같은 할당 중복");
                }
                return manager;
            },
            ReadOnly = fileName => NeedFile(file.ReadOnly)(fileName),
            // readonly callback 뒤 header에서 재조회한 manager를 그대로 읽는다
            ReadFile = manager => _images[manager].ReadFile(file),
            IsCurrent = () => slot == NeedFile(file.CurrentSlot)(),
            RefreshTitle = () => NeedFile(file.RefreshTitle)(),
            Lifetime = Effects(),
        };
        RawTemplateLoad.Load(Header(slot), name, hooks);
    }

    /// <summary>
    /// 같은 manager의 실제 서명/검색/current와 저장 부모를 통해 현재 섹션을 파일에 쓴다. 빌린 헤더도 원래 소유자의 같은 현재 manager와 원시 섹션을 저장한다.
    /// </summary>
    /// <param name="slot">저장할 칸.</param>
    /// <param name="state">저장 억제·로컬 소유자·숨김 전역의 현재 값.</param>
    /// <param name="image">타입 개수·진단. 서명 flag와 섹션 조회는 이 호출이 채운다(호출자의 객체는 바꾸지 않는다).</param>
    /// <param name="file">권한 변경(0x180)·쓰기 장치.</param>
    public void SaveFile(uint slot, DataManagerFileSaveState state, FortImageHooks image, TemplateFileWriteHooks file)
    {
        Current(slot).SaveFile(state, image, file);
    }

    /// <summary>
    /// 원문 이름 부모에 같은 헤더/manager+c 256바이트만 연결한다. Notify·저장·삭제 등은 실제 호출자가 공급한다.
    /// C++은 값으로 받은 장치 묶음에 채워 돌려주지만 C#은 받은 객체에 채워 같은 객체를 돌려준다.
    /// </summary>
    /// <param name="hooks">호출자가 공급한 장치 묶음.</param>
    /// <exception cref="ArgumentOutOfRangeException">그 manager가 이미 반납됐거나 없다(이름 변경의 현재 manager 없음).</exception>
    public FortCurrentNameHooks BindCurrentName(FortCurrentNameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        hooks.Header = slot => Header(slot);
        hooks.ManagerName = token =>
        {
            if (!_images.TryGetValue(token, out Image? image))
            {
                throw new ArgumentOutOfRangeException(nameof(token), token, "Fort 이름 변경의 현재 manager 없음");
            }
            return image.Manager.AsSpan(12, 256);
        };
        return hooks;
    }

    /// <summary>
    /// 보관한 노드가 현재 가진 실제 raw 할당(<see cref="RawSectionBuffer"/>)을 돌려준다. 검사가 이름 교체·압축 뒤에도 같은 할당(배열)인지 구별하는 용도이며
    /// C++은 같은 일을 <c>data()</c> 포인터 비교로 했다. 호출자는 이 버퍼를 직접 바꾸지 않는다.
    /// </summary>
    /// <param name="stream">보관한 쓰기 참조.</param>
    internal RawSectionBuffer SectionBuffer(TemplateWriteStream stream)
    {
        return Writable(stream).Buffer(stream.Section);
    }

    /// <summary>같은 manager의 전체 최소 확장 부모가 만든 실제 현재 State를 돌려준다. 이미 충분하면 할당·순서·커서를 보존한다.</summary>
    /// <param name="slot">칸.</param>
    /// <param name="minimum">필요한 최소 end(signed).</param>
    public PlayerTemplateStateStream EnsureStateSize(uint slot, int minimum)
    {
        Image image = Current(slot);
        image.EnsureStateSize(minimum);
        return image.State;
    }
}
