using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 이미 월드에 놓여 있던(void 가 아닌) 오브젝트를 Take 가 다시 받을 때 먼저 공간 등록을 푸는 원본 Unpop 의 연결 지점.
/// 원본 월드 코어의 Unpop 모듈(<see cref="RawUnpop"/>, W02 Unpop 조각)이 구현해 <see cref="RawSquidFactory"/> 에 넘긴다.
/// </summary>
public interface IRawUnpop
{
    /// <summary>이 모듈이 쓰는 슬롯 풀. 팩토리는 자기 풀과 같은 풀의 모듈만 받아 서로 다른 수명 상태의 혼용을 막는다.</summary>
    RawSidPool Pool { get; }

    /// <summary>오브젝트를 공간(해시·spot·표면 그래프)에서 푼다. 끝나면 슬롯은 void 상태여야 한다.</summary>
    /// <param name="sid">풀 슬롯 번호.</param>
    /// <param name="type">그 오브젝트의 타입 레코드.</param>
    void Unpop(int sid, OriginalTypeRecord type);
}

/// <summary>
/// 같은 월드·세션의 현재 타입 표와 정상 복귀 가능한 원본 진단을 빌린다
/// (cpppj <c>o/SquidFactory.h</c>의 <c>SquidFactoryDiagnostics</c>).
/// </summary>
/// <remarks>2026-11-07 F2d 공장 진단 묶음 이식에서 추가했다. 현재 표만 소비 위치에서 읽고, 진단 장치는 F3·R6 소비자가 연결한다.</remarks>
public sealed class SquidFactoryDiagnostics
{
    /// <summary>빌린 현재 타입 표다. 비어 있으면 기존 독립 생성자의 타입 사본을 쓴다. 2026-11-07 F2d 공장 진단 묶음 이식에서 추가됐다.</summary>
    public IReadOnlyList<OriginalTypeRecord>? Types { get; set; }

    /// <summary>Take·PostTake의 원문 식·파일·줄을 실제 진단 장치에 전달한다. 2026-11-07 F2d 공장 진단 묶음 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>타입 불일치 문구를 같은 Client 소유자에 복사한다. 2026-11-07 F2d 공장 진단 묶음 이식에서 추가됐다.</summary>
    public Action<string>? Detail { get; set; }

    /// <summary>타입 HP의 선택된 진단에서 현재값을 읽는다. R6 HP 경로에서 소비한다. 2026-11-07 F2d 공장 진단 묶음 이식에서 추가됐다.</summary>
    public RawCheckingFlag? Checking { get; set; }

    /// <summary>현재 진단 묶음의 얕은 사본을 만든다. 표·장치는 공유하고 검사 표시는 복사하지 않는다. 2026-11-07 F2d 공장 진단 묶음 이식에서 추가됐다.</summary>
    /// <returns>복사된 진단 묶음이다.</returns>
    public SquidFactoryDiagnostics Clone() => new()
    {
        Types = Types,
        Assertion = Assertion,
        Detail = Detail,
        Checking = Checking is null ? null : new RawCheckingFlag(Checking),
    };
}

/// <summary>
/// 오브젝트 슬롯을 할당해 타입별 생성자 필드를 채우는 원본 팩토리(Squid.cpp 의 생성·Take·공통 초기화, 10.78).
/// 순서는 원본 그대로 "슬롯 할당 → 생성자 쓰기 → 타입 바이트 → 공통 postCreate" 이고, Take 는 풀 목록·카운터를 건드리지 않고
/// free/dead 를 지운 뒤 생성자 필드만 다시 쓴다(<c>004af530</c> 생성, <c>004af610</c> Take, <c>004b0c60</c> postCreate, <c>004ad4a0</c> postTake,
/// <c>004ad470</c> 최초 Pop 플래그). 미복원 생성자(표에 없는 주소)·null/assert 생성자·잘못된 슬롯은 풀을 바꾸기 전에 예외로 거부하며,
/// 기본 객체로 조용히 대신하지 않는다.
/// 슬롯 필드 위치(10.78): 가상 표 +0, 섬 번호 +8, 타입 +10, 상태 +11, HP +26(4바이트), 소유자 +34, zOrder +35, 추가 비트 +40.
/// 생산 칸 contained Form 과 GumpForm 의 생성은 Form 모듈(W03)에서 옮긴다.
/// 근거: docs/exe/cpp-creation-reconstruction.md, docs/exe/cpp-derived-reconstruction.md, cpppj/src/o/SquidFactory.cpp (W02 첫 조각, 2026-10-16; Retype·생성자 표 명시 오류는 R0-4 재대조, 2026-10-28).
/// 최종 가상 표 판별(SupportsConstructedVtable)은 0.224, 약화 전역 공유는 0.223, 현재 TYPE HP와 공통 초기화는 0.528에서 연결했다.
/// 생성·타입 변경·최초 Pop 플래그의 판본별 표·위치는 F2c-11에서 이었다. Take·공통 postCreate의 나머지 CD 분기는 별도 범위다.
/// </summary>
public sealed partial class RawSquidFactory
{
    /// <summary>슬롯 안 가상 표(클래스 식별 값, 4바이트)의 위치.</summary>
    private const int VtableOffset = 0;

    /// <summary>슬롯 안 HP(4바이트)의 위치.</summary>
    private const int HpOffset = 26;

    /// <summary>슬롯 안 소유자 바이트의 위치.</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 zOrder 바이트의 위치.</summary>
    private const int ZOrderOffset = 35;

    /// <summary>슬롯 안 추가 비트 바이트의 위치. 받침의 abstract 비트(1)와 최초 Pop 의 매립 비트(8)를 담는다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(CD 10.72). (F2c-11, 2026-11-04)</summary>
    private const int CdExtraOffset = 35;

    /// <summary>상태 바이트의 contained 비트(생산 칸에 들어 있는 내용물).</summary>
    private const byte ContainedBit = 8;

    /// <summary>mana 타입 번호. 약화 옵션이 켜지면 최대 HP 를 4 로 나눈다.</summary>
    private const int ManaType = 158;

    /// <summary>받침(dais) 기본 타입 번호. HP 바인딩 미연결 때 쓴다. 2026-11-07 F2d 공장 HP·표면 바인딩 이식에서 추가됐다.</summary>
    private const int DaisType = 165;

    /// <summary>패치판에서 생성이 금지된 fakeThreeByThreeSurface 타입 번호.</summary>
    private const int FakeSurfaceType = 162;

    /// <summary>내장 Form 타입 번호 네 개. 일반 Create·Take도 원문 생성자·가상 표를 쓴다. 2026-11-07 F2d 공장 폼 생성 이식에서 추가됐다.</summary>
    private static readonly uint[] FormTypes = [1u, 2u, 3u, 6u];

    /// <summary>원문 NULL 생성자 주소. 반환 객체가 필요하면 생성을 거부한다. 2026-11-07 F2d 공장 폼 생성 이식에서 추가됐다.</summary>
    private const uint NullConstructorAddress = 0x4429E0u;

    /// <summary>원문 받침 보조 생성자 주소(패치판). 진단 정상 복귀 뒤 가상 표만 쓴다. 2026-11-07 F2d 공장 현재 생성 이식에서 추가됐다.</summary>
    private const uint DaisAuxAddress = 0x44a420u;

    /// <summary>받침 보조 생성자가 진단 복귀 뒤 쓰는 가상 표 값이다. 2026-11-07 F2d 공장 현재 생성 이식에서 추가됐다.</summary>
    private const uint DaisAuxVtable = 0x507950u;

    /// <summary>슬롯을 할당받을 풀. 팩토리는 풀의 목록·카운터를 Allocate 로만 바꾼다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 호출자의 배열 내용이 바뀌면 그대로 반영된다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>mana 약화 옵션. 켜지면 mana 의 최대 HP 를 0 쪽으로 절삭한 정수 1/4 로 쓴다.</summary>
    private readonly bool _weakenedMana;

    /// <summary>void 가 아닌 오브젝트를 Take 할 때 쓰는 공간 해제. 연결 전이면 null 이며 그 경우 Take 는 거부한다.</summary>
    private readonly IRawUnpop? _unpop;

    /// <summary>선택된 Form 해제 때 실제 공통 종속 분리만 실행하는 경계다. 없으면 공통 순서를 쓴다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    private Action<int>? _formDetach;

    /// <summary>같은 월드·세션의 현재 타입 표와 정상 복귀 가능한 원본 진단 묶음이다. 비어 있으면 기존 독립 생성자의 타입 사본을 쓴다. 2026-11-07 F2d 공장 진단 묶음 이식에서 추가됐다.</summary>
    private SquidFactoryDiagnostics? _diagnostics;

    /// <summary>죽은 Session 주소를 저장하지 않는 Form 전송 경계다. 없으면 서버 전송 경로에서 예외다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    private Action<int, uint>? _formTransmit;

    /// <summary>Pop 뒤 현재 전체 contentFormType DWORD다. 없으면 6을 쓴다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    private Func<uint>? _formContentType;

    /// <summary>호출 중 같은 World 저장소를 바꾸지 않는 공유 깊이 상자다. 없으면 지역 깊이를 쓴다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    private UIntBox? _formPostPopDepth;

    /// <summary>공통 초기화·선택 HP가 읽는 같은 장기 사제 타입 DWORD다. 미연결이면 158을 쓴다.</summary>
    private UIntBox? _sharedPriestType;

    /// <summary>공통 초기화·선택 HP가 읽는 같은 장기 받침 타입 DWORD다. 미연결이면 165를 쓴다.</summary>
    private UIntBox? _sharedDaisType;

    /// <summary>Create가 읽는 같은 장기 fakeThreeByThreeSurface 타입 DWORD다. 미연결이면 162를 쓴다. 2026-11-07 F2d 공장 HP·표면 바인딩 이식에서 추가됐다.</summary>
    private UIntBox? _sharedFakeSurfaceType;

    /// <summary>생성 HP가 읽는 같은 장기 약화 전역이다. 미연결이면 생성자 값을 쓴다. 2026-11-07 F2d 공장 약화 공유 이식에서 추가됐다.</summary>
    private BoolBox? _sharedWeakenedMana;

    /// <summary>팩토리를 만든다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 개수는 <see cref="OriginalConstructors.TypeCount"/> 와 같아야 한다.</param>
    /// <param name="weakenedMana">mana 약화 옵션.</param>
    /// <param name="unpop">non-void Take 의 공간 해제. 없으면 null.</param>
    public RawSquidFactory(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, bool weakenedMana = false, IRawUnpop? unpop = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        if (unpop is not null && !ReferenceEquals(unpop.Pool, pool))
        {
            throw new ArgumentException("Unpop과 Factory의 SID 풀이 다릅니다", nameof(unpop));
        }
        _pool = pool;
        _types = types;
        _weakenedMana = weakenedMana;
        _unpop = unpop;
    }

    /// <summary>이 팩토리가 쓰는 슬롯 풀. 생성 효과 어댑터가 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>같은 판본 길이의 현재 타입 표와 실제 진단 장치를 연결한다. 빈 표는 기존 독립 사본을 유지한다. 2026-11-07 F2d 공장 진단 묶음 이식에서 추가됐다.</summary>
    /// <param name="diagnostics">연결할 진단 묶음이다.</param>
    public void BindDiagnostics(SquidFactoryDiagnostics diagnostics)
    {
        ArgumentNullException.ThrowIfNull(diagnostics);
        if (diagnostics.Types is not null && diagnostics.Types.Count != 0 && diagnostics.Types.Count != _types.Count)
        {
            throw new ArgumentException("Factory 현재 TYPE의 판본/길이 오류", nameof(diagnostics));
        }

        _diagnostics = diagnostics.Clone();
    }

    /// <summary>새 대여는 이후 선택된 Form 해제부터 분리 경계를 사용한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    /// <param name="detach">void 설정·부모 free 관문 뒤 실행할 분리 경계다. null이면 공통 순서를 쓴다.</param>
    public void BindFormDetach(Action<int>? detach)
    {
        _formDetach = detach;
    }

    /// <summary>죽은 Session 주소를 공장에 저장하지 않고 전송·종류 경계만 둔다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    /// <param name="contentType">Pop 뒤 현재 전체 contentFormType DWORD다. null이면 6을 쓴다.</param>
    /// <param name="transmit">서버 Pop 전송 경계다. null이면 서버 전송 경로에서 예외다.</param>
    public void BindFormAttach(Func<uint>? contentType, Action<int, uint>? transmit)
    {
        _formContentType = contentType;
        _formTransmit = transmit;
    }

    /// <summary>호출 중 같은 World 저장소를 바꾸지 않고 실제 동일 DWORD를 읽고 쓴다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    /// <param name="current">공유할 깊이 상자다.</param>
    public void BindFormPopDepth(UIntBox current)
    {
        ArgumentNullException.ThrowIfNull(current);
        if (_formPostPopDepth is not null && !ReferenceEquals(_formPostPopDepth, current))
        {
            throw new ArgumentException("Form Pop의 다른 공유 깊이 주소 재연결");
        }

        _formPostPopDepth = current;
    }

    /// <summary>타입 HP가 읽는 같은 장기 사제·받침 DWORD 두 주소를 빌린다. 둘 다 null이면 거부하고 다른 주소 재연결도 거부한다. R6 TypeHitPoints에서 소비하며 그 전까지는 보관만 한다. 2026-11-07 F2d 공장 HP·표면 바인딩 이식에서 추가됐다.</summary>
    /// <param name="priest">빌릴 사제 타입 상자다.</param>
    /// <param name="dais">빌릴 받침 타입 상자다.</param>
    public void BindHitPointTypes(UIntBox? priest, UIntBox? dais)
    {
        if (priest is null || dais is null)
        {
            throw new ArgumentException("생성 HP의 특별 TYPE 주소 연결 오류");
        }

        if ((_sharedPriestType is not null && !ReferenceEquals(_sharedPriestType, priest))
            || (_sharedDaisType is not null && !ReferenceEquals(_sharedDaisType, dais)))
        {
            throw new ArgumentException("생성 HP의 특별 TYPE 주소 연결 오류");
        }

        _sharedPriestType = priest;
        _sharedDaisType = dais;
    }

    /// <summary>Create가 읽는 같은 장기 fakeThreeByThreeSurface DWORD를 빌린다. 다른 주소 재연결은 거부한다. 2026-11-07 F2d 공장 HP·표면 바인딩 이식에서 추가됐다.</summary>
    /// <param name="current">빌릴 fake 표면 타입 상자다.</param>
    public void BindFakeSurfaceType(UIntBox current)
    {
        ArgumentNullException.ThrowIfNull(current);
        if (_sharedFakeSurfaceType is not null && !ReferenceEquals(_sharedFakeSurfaceType, current))
        {
            throw new ArgumentException("Create fakeSurface TYPE의 중복 다른 주소 연결");
        }

        _sharedFakeSurfaceType = current;
    }

    /// <summary>현재 사제 HP 타입 번호다. 미연결이면 158을 쓴다. R6 소비 전까지 보관값 확인용이다. 2026-11-07 F2d 공장 HP·표면 바인딩 이식에서 추가됐다.</summary>
    /// <returns>사제 타입 번호다.</returns>
    public uint CurrentPriestType() => _sharedPriestType?.Value ?? (uint)ManaType;

    /// <summary>현재 받침 HP 타입 번호다. 미연결이면 165를 쓴다. R6 소비 전까지 보관값 확인용이다. 2026-11-07 F2d 공장 HP·표면 바인딩 이식에서 추가됐다.</summary>
    /// <returns>받침 타입 번호다.</returns>
    public uint CurrentDaisType() => _sharedDaisType?.Value ?? (uint)DaisType;

    /// <summary>현재 fakeThreeByThreeSurface 타입 번호다. 미연결이면 162를 쓴다. 2026-11-07 F2d 공장 HP·표면 바인딩 이식에서 추가됐다.</summary>
    /// <returns>fake 표면 타입 번호다.</returns>
    public uint CurrentFakeSurfaceType() => _sharedFakeSurfaceType?.Value ?? (uint)FakeSurfaceType;

    /// <summary>생성 HP가 읽는 같은 장기 약화 전역을 빌린다. 다른 주소 재연결은 거부하며 대상은 이 공장보다 오래 살아야 한다. 2026-11-07 F2d 공장 약화 공유 이식에서 추가됐다.</summary>
    /// <param name="current">빌릴 약화 상자다.</param>
    public void BindWeakenedMana(BoolBox current)
    {
        ArgumentNullException.ThrowIfNull(current);
        if (_sharedWeakenedMana is not null && !ReferenceEquals(_sharedWeakenedMana, current))
        {
            throw new ArgumentException("생성 HP의 약화 전역 중복 다른 주소 연결");
        }

        _sharedWeakenedMana = current;
    }

    /// <summary>현재 약화 여부다. 묶은 상자가 있으면 그 값을 읽고 없으면 생성자 값을 쓴다. 2026-11-07 F2d 공장 약화 공유 이식에서 추가됐다.</summary>
    /// <returns>약화 여부다.</returns>
    public bool CurrentWeakenedMana() => _sharedWeakenedMana?.Value ?? _weakenedMana;

    /// <summary>같은 공장의 현재 진단·공유 깊이를 부착·직접 Pop에 함께 연결한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    /// <param name="forms">연결할 Form 몸체다. 같은 풀을 써야 한다.</param>
    public void BindCurrentFormPop(RawSquidForms forms)
    {
        ArgumentNullException.ThrowIfNull(forms);
        if (!ReferenceEquals(forms.Pool, _pool))
        {
            throw new ArgumentException("Form Pop 연결의 실제 Pool 불일치", nameof(forms));
        }

        var hooks = new FormPopHooks();
        if (_formPostPopDepth is not null)
        {
            hooks.Depth = () => _formPostPopDepth.Value;
            hooks.SetDepth = value => _formPostPopDepth.Value = value;
        }

        forms.BindPop(hooks);
    }

    /// <summary>파생 직렬화 접두 뒤에만 공통 현재 genus·링크·Pop 진단을 실행한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    /// <param name="form">부를 Form SID다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="flags">Pop 플래그다.</param>
    /// <param name="transmit">서버 Pop 전송 경계다. null이면 바인딩된 경계를 쓴다.</param>
    public void AttachForm(int form, int parent, uint flags, Action<int, uint>? transmit = null)
    {
        var forms = new RawSquidForms(_pool, null, transmit ?? _formTransmit);
        BindCurrentFormPop(forms);
        var hooks = new FormAttachHooks
        {
            // 각 호출에서 현재 빌린 TYPE 표를 읽는다
            Genus = number =>
            {
                IReadOnlyList<OriginalTypeRecord> current = CurrentTypes();
                return number >= current.Count
                    ? throw new ArgumentOutOfRangeException(nameof(number), number, "Form 부착의 현재 TYPE 범위 오류")
                    : current[number].Flags2;
            },
            ContentType = _formContentType ?? (() => 6u),
        };
        forms.BindAttach(hooks);
        forms.Attach(form, parent, flags);
    }

    /// <summary>바인딩된 분리 경계가 있으면 실제 공통 분리만 실행하고, 없으면 공통 순서를 쓴다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐고 0.218에서 분리 종단 재작업을 이었다.</summary>
    /// <param name="sid">해제할 Form 번호다.</param>
    public void UnpopForm(int sid)
    {
        var forms = new RawSquidForms(_pool);
        if (_formDetach is not null)
        {
            Action<int> detach = _formDetach;
            forms.BindUnpop(new FormUnpopHooks { Detach = detach });
        }

        forms.Unpop(sid);
    }

    /// <summary>실제 현재 진단·깊이를 같은 호출 수명으로 빌려 직접 Pop 한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    /// <param name="form">Pop 할 Form 번호.</param>
    /// <param name="flags">Pop 플래그다.</param>
    /// <param name="transmit">서버 Pop 전송 경계다. null이면 바인딩된 경계를 쓴다.</param>
    public void PopForm(int form, uint flags, Action<int, uint>? transmit = null)
    {
        var forms = new RawSquidForms(_pool, null, transmit ?? _formTransmit);
        BindCurrentFormPop(forms);
        forms.Pop(form, flags);
    }

    /// <summary>
    /// 오브젝트를 만든다. 슬롯 할당 → 생성자 쓰기 → 타입 바이트 → postCreate 순서이며 새 슬롯 번호를 돌려준다.
    /// 내장 Form(1·2·3·6)은 가상 표와 타입만 쓰고 자산 후처리를 건너뛴다.
    /// 지원하지 않는 타입(미복원 생성자, 현재 fake 표면 타입, 자산·Form 밖)은 슬롯을 할당하기 전에 예외로 거부한다.
    /// 근거: cpppj/src/o/SquidFactory.cpp Create·RawSquidCreation.cpp CreateRawSquid (F2d 공장 폼 생성, 2026-11-07; fake 연결 F2d 0.221, 2026-11-07).
    /// </summary>
    /// <param name="type">타입 번호(내장 Form 1·2·3·6 또는 70 이상).</param>
    /// <param name="flags">할당 영역 비트(1 예측, 2 클라이언트).</param>
    public int Create(uint type, SidAllocation flags = SidAllocation.Server)
    {
        OriginalTypeRecord record = TypeForCreate(type);
        if (type == CurrentFakeSurfaceType())
        {
            throw new InvalidOperationException("패치판 fakeThreeByThreeSurface 는 생성할 수 없습니다");
        }
        int sid = _pool.Allocate(flags);
        ConstructRaw(record.ConstructorAddress, sid, true);
        uint table = ReadVtable(_pool.AllocatedBytes(sid));
        _pool.AllocatedBytes(sid)[RawSidPool.TypeOffset] = (byte)type;
        VirtualPostCreate(sid, table);
        return sid;
    }

    /// <summary>
    /// 생산 칸에 넣는 contained Form(타입 6)을 만든다(원본 SquidFactory::CreateContained, 004af530 계열).
    /// 자산 생성자 표가 아니라 내장 Form 생성자를 쓰므로 슬롯에는 가상 표와 타입만 쓰고 postCreate 는 부르지 않는다.
    /// 생성 영역은 contained 생성기가 넘기는 원본 플래그(1 예측, 2 클라이언트, 0 서버)를 그대로 따른다.
    /// 추가: W03 내용물 생성 조각(2026-10-23). 근거: cpppj/src/o/SquidFactory.cpp CreateContained.
    /// </summary>
    /// <param name="flags">원본 생성 플래그(1 예측, 2 클라이언트, 0 서버).</param>
    /// <returns>새 contained Form 번호.</returns>
    public int CreateContained(uint flags)
    {
        // 예측 비트가 클라이언트 비트보다 우선하는 원본 할당 순서를 그대로 쓴다
        SidAllocation allocation = (flags & 1) != 0 ? SidAllocation.Predictable
            : (flags & 2) != 0 ? SidAllocation.Client : SidAllocation.Server;
        return new RawSquidForms(_pool).Create(6, allocation);
    }

    /// <summary>
    /// 메뉴 GumpForm(타입 3)을 만든다(원본 SquidFactory::CreateGumpForm, 004af530 계열).
    /// contained 생성과 같이 자산 생성자 표가 아니라 내장 Form 생성자를 쓰므로 postCreate 는 부르지 않는다.
    /// 추가: W03 삭제·수명 열다섯째 조각(2026-10-23). 근거: cpppj/src/o/SquidFactory.cpp CreateGumpForm.
    /// </summary>
    /// <param name="flags">원본 생성 플래그(1 예측, 2 클라이언트, 0 서버).</param>
    /// <returns>새 GumpForm 번호.</returns>
    public int CreateGumpForm(uint flags = 2)
    {
        // 예측 비트가 클라이언트 비트보다 우선하는 원본 할당 순서를 그대로 쓴다
        SidAllocation allocation = (flags & 1) != 0 ? SidAllocation.Predictable
            : (flags & 2) != 0 ? SidAllocation.Client : SidAllocation.Server;
        return new RawSquidForms(_pool).Create(RawGumpFormHost.GumpFormType, allocation);
    }

    /// <summary>
    /// 이미 확보된 슬롯에 생성자 쓰기만 적용한다. 타입 바이트·상태·풀 목록·HP·소유자는 덮지 않는다(독립 검사와 재초기화용).
    /// 5 번 미만·빈·contained 슬롯은 거부한다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    /// <param name="sid">대상 슬롯 번호.</param>
    public int Construct(uint type, int sid)
    {
        OriginalTypeRecord record = Type(type, requireSupported: true);
        Span<byte> bytes = _pool.RawSlot(sid);
        if (sid < RawSidPool.FirstOrdinary || (bytes[RawSidPool.StateOffset] & (RawSidPool.FreeBit | ContainedBit)) != 0)
        {
            throw new InvalidOperationException("생성자 슬롯 상태 오류");
        }
        ApplyConstructor(bytes, record);
        return sid;
    }

    /// <summary>
    /// 같은 SID의 타입을 바꾼다(원본 004b1620). void/foot 진단은 선택된 위치에서만 요구하고, 진단 복귀 뒤에도 계속한다.
    /// free/dead/예약 SID를 추가로 제외하지 않으며 HP/소유자/할당은 보존한다. 타입 바이트 쓰기 뒤 현재 타입의 실제 생성자를 다시 조회한다.
    /// 10.78만 복원했으며 C# 풀은 패치판 범위이므로 판본 검사는 생략한다.
    /// 근거: cpppj/src/o/SquidFactory.cpp Retype (R0-4 재대조, 2026-10-28).
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="type">바꿀 타입 번호.</param>
    /// <param name="assertion">원문 진단(0x22d·0x22e·0x22f)을 받는 콜백. 진단이 필요한데 null이면 거부한다.</param>
    public void Retype(int sid, uint type, Action<uint>? assertion = null)
    {
        Span<byte> bytes = _pool.RawSlot(sid);
        if ((bytes[RawSidPool.StateOffset] & RawSidPool.VoidBit) == 0)
        {
            Diagnose(assertion, 0x22d);
        }
        if (Type(bytes[RawSidPool.TypeOffset], requireSupported: false).FootX != Type(type, requireSupported: false).FootX)
        {
            Diagnose(assertion, 0x22e);
        }
        if (Type(bytes[RawSidPool.TypeOffset], requireSupported: false).FootY != Type(type, requireSupported: false).FootY)
        {
            Diagnose(assertion, 0x22f);
        }
        bytes[RawSidPool.TypeOffset] = (byte)type;
        ApplyConstructor(bytes, Type(bytes[RawSidPool.TypeOffset], requireSupported: true));
    }

    /// <summary>원문 진단을 선택된 위치에서만 요구한다. 콜백이 없으면 명시 경계 오류로 멈춘다.</summary>
    /// <param name="assertion">원문 진단을 받는 콜백.</param>
    /// <param name="line">원문 진단 줄 번호.</param>
    private static void Diagnose(Action<uint>? assertion, uint line)
    {
        if (assertion is null)
        {
            throw new InvalidOperationException("타입 변경의 실제 원문 진단 미연결");
        }
        assertion(line);
    }

    /// <summary>공장 Take·생성의 원문 식·줄을 빌린 진단 장치에 전달한다. 미연결이면 명시 경계 오류로 멈추고, 연결되면 정상 복귀 뒤 계속한다. 2026-11-07 F2d 공장 진단 소비 이식에서 추가됐다.</summary>
    /// <param name="expression">원문 식이다.</param>
    /// <param name="line">원문 줄 번호다.</param>
    private void Assert(string expression, uint line)
    {
        Action<string, string, uint>? assertion = _diagnostics?.Assertion;
        if (assertion is null)
        {
            throw new InvalidOperationException("Factory의 선택된 원문 진단 장치 미연결");
        }

        // 원문은 판본별 파일 경로를 넘긴다
        assertion(expression, _pool.Edition == OriginalEdition.Patch1078 ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp", line);
    }

    /// <summary>Take 타입 불일치 상세를 현재 두 이름으로 복사한다. 256바이트를 넘기면 거부하고 장치 미연결이면 거부한다. 2026-11-07 F2d 공장 진단 소비 이식에서 추가됐다.</summary>
    /// <param name="serverType">서버가 정한 타입 번호다.</param>
    /// <param name="clientType">슬롯에 있던 기존 타입 번호다.</param>
    private void WriteTakeDetail(uint serverType, uint clientType)
    {
        // TYPE+4의 첫 NUL까지의 이름이다
        static string BaseName(OriginalTypeRecord record)
        {
            string name = record.Name ?? "";
            int cut = name.IndexOf('\0');
            return cut < 0 ? name : name.Substring(0, cut);
        }

        IReadOnlyList<OriginalTypeRecord> current = CurrentTypes();
        string serverName = serverType < (uint)current.Count ? BaseName(current[(int)serverType]) : "";
        string clientName = clientType < (uint)current.Count ? BaseName(current[(int)clientType]) : "";
        string detail = "Invalid take. Server type " + serverName + ", client type " + clientName;
        if (System.Text.Encoding.Latin1.GetByteCount(detail) >= 256)
        {
            throw new InvalidOperationException("Take 원문 상세256BYTE 범위");
        }

        Action<string>? writer = _diagnostics?.Detail;
        if (writer is null)
        {
            throw new InvalidOperationException("Take의 실제 오류 상세 장치 미연결");
        }

        writer(detail);
    }

    /// <summary>
    /// 서버가 정한 번호의 슬롯을 받는다(원본 Take). 풀 목록·빈 번호 카운터는 바꾸지 않는다.
    /// 이미 쓰는 슬롯이면 같은 타입·같은 클래스(가상 표)여야 하고, void 가 아니면 공간 해제(Form은 <see cref="UnpopForm"/>, 자산은 <see cref="IRawUnpop"/>)를 먼저 한다.
    /// 내장 Form(1·2·3·6)은 가상 표·타입만 쓰고 자산 후처리를 건너뛴다.
    /// 그다음 free/dead 를 지우고 void 를 켠 뒤 생성자 필드와 타입 바이트를 쓴다. 그 밖의 필드는 보존한다.
    /// 근거: cpppj/src/o/SquidFactory.cpp Take (F2d 공장 폼 생성, 2026-11-07).
    /// </summary>
    /// <param name="type">타입 번호(내장 Form 1·2·3·6 또는 70 이상).</param>
    /// <param name="sid">서버 영역의 슬롯 번호.</param>
    public int Take(uint type, int sid)
    {
        OriginalTypeRecord record = TypeForCreate(type);
        if (sid < RawSidPool.ServerFirst)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "Take 는 서버 영역 번호만 받습니다");
        }
        bool isForm = IsFormTypeNumber(type);
        Span<byte> bytes = _pool.RawSlot(sid);
        if (!isForm && (bytes[RawSidPool.StateOffset] & ContainedBit) != 0)
        {
            throw new InvalidOperationException("contained 오브젝트의 base Take 는 지원하지 않습니다");
        }
        if ((bytes[RawSidPool.StateOffset] & RawSidPool.FreeBit) == 0)
        {
            if (bytes[RawSidPool.TypeOffset] != type)
            {
                // 원문 Take 부모는 상세 복사 뒤 진단을 거쳐 정상 복귀하면 새 타입으로 덮어쓴다. 2026-11-07 F2d 공장 진단 소비 이식에서 연결했다.
                uint previous = bytes[RawSidPool.TypeOffset];
                WriteTakeDetail(type, previous);
                Assert("0", _pool.Edition == OriginalEdition.Patch1078 ? 0x1bau : 0x1b1u);
            }
            uint beforeTable = ReadVtable(bytes);
            if (beforeTable != ConstructorVtable(record))
            {
                throw new InvalidOperationException("Take 의 미복원 가상/공간 해제 효과가 필요합니다");
            }
            if ((bytes[RawSidPool.StateOffset] & RawSidPool.VoidBit) == 0)
            {
                if (isForm)
                {
                    UnpopForm(sid);
                    bytes = _pool.RawSlot(sid);
                }
                else
                {
                    if (_unpop is null)
                    {
                        throw new InvalidOperationException("Take 의 non-void 공간 해제 연결이 없습니다");
                    }
                    _unpop.Unpop(sid, record);
                }
            }
            // 이미 void 인 오브젝트의 기본 Unpop 은 쓰기 없이 돌아온다
        }
        bytes[RawSidPool.StateOffset] = (byte)((bytes[RawSidPool.StateOffset] & 0xFC) | RawSidPool.VoidBit);
        ConstructRaw(record.ConstructorAddress, sid, true);
        _pool.RawSlot(sid)[RawSidPool.TypeOffset] = (byte)type;
        if (!isForm)
        {
            PostTake(sid);
        }
        return sid;
    }

    /// <summary>
    /// 최초 Pop 의 공통 플래그를 추가 비트에서 읽는다(원본 004ad470). 추가 비트(패치 +40·CD +35)의 1·8 이 모두 꺼져 있으면 1,
    /// 아니면 1 비트(abstract)가 2, 8 비트(매립)가 4 를 더한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public uint FirstPopFlags(int sid)
    {
        ReadOnlySpan<byte> bytes = LiveBytes(sid);
        OriginalTypeRecord type = Type(bytes[RawSidPool.TypeOffset], requireSupported: true);
        if (ReadVtable(bytes) != ConstructorVtable(type))
        {
            throw new InvalidOperationException("firstPop 가상 표 오류");
        }
        byte extra = bytes[_pool.Edition == OriginalEdition.Patch1078 ? ExtraOffset : CdExtraOffset];
        return (extra & 9) == 0 ? 1U : (((extra & 1) != 0 ? 2U : 0U) | ((extra & 8) != 0 ? 4U : 0U));
    }

    /// <summary>
    /// 공통 postCreate(원본 004b0c60). zOrder 하위 바이트를 복사하고, 서버이면 소유자를 0 으로 만들며 필요한 타입은 HP 를 최대치로 채운다.
    /// HP 가 필요한 타입은 일반 타입이면 플래그1 의 hit points 비트, 받침(dais)이면 추가 비트의 abstract 비트로 정한다.
    /// 파생 클래스가 base 초기화를 부를 때도 이 메서드를 쓴다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public void PostCreate(int sid)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        Span<byte> bytes = _pool.RawSlot(sid);
        if (bytes[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType)
            Assert("!isForm()", patch ? 0x7d4u : 0x7a6u);
        if ((bytes[RawSidPool.StateOffset] & ContainedBit) != 0)
            Assert("!isContained()", patch ? 0x7d5u : 0x7a7u);
        IReadOnlyList<OriginalTypeRecord> selectedBase = CurrentTypes();
        OriginalTypeRecord initial = TypeRecord(bytes[RawSidPool.TypeOffset]);
        bool cdServer = !patch && _pool.IsServer;
        bytes[patch ? 35 : 33] = unchecked((byte)initial.ZOrder);
        if (patch ? _pool.IsServer : cdServer)
        {
            bytes[patch ? 34 : 32] = 0;
            OriginalTypeRecord current = TypeRecord(bytes[RawSidPool.TypeOffset]);
            bool needsHp = (current.Flags2 & OriginalTypeRecord.Dais) != 0
                ? (bytes[patch ? 40 : 35] & 1) != 0 : (current.Flags1 & OriginalTypeRecord.HasHitPoints) != 0;
            if (needsHp)
                Put(bytes, HpOffset, unchecked((uint)ReadSelectedHitPoints(initial, selectedBase)), patch ? 4 : 2);
        }
        if ((bytes[RawSidPool.StateOffset] & ContainedBit) != 0)
            Assert("!isContained()", patch ? 0x7dfu : 0x7b1u);
    }

    /// <summary>공통 postTake(원본 004ad4a0). zOrder 하위 바이트만 복사하고 소유자·HP 는 보존한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public void PostTake(int sid)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        Span<byte> bytes = _pool.RawSlot(sid);
        if (bytes[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType)
            Assert("!isForm()", patch ? 0x7f6u : 0x7c8u);
        bytes[patch ? 35 : 33] = unchecked((byte)TypeRecord(bytes[RawSidPool.TypeOffset]).ZOrder);
        if ((bytes[RawSidPool.StateOffset] & ContainedBit) != 0)
            Assert("!isContained()", patch ? 0x7fau : 0x7ccu);
    }

    /// <summary>
    /// 타입 번호의 레코드를 찾는다. 자산 타입 밖 번호는 예외이고, <paramref name="requireSupported"/> 이면
    /// 생성자 주소가 0 이 아닌데 표에 없는(미복원·null·assert) 타입도 거부한다.
    /// </summary>
    /// <param name="number">타입 번호.</param>
    /// <param name="requireSupported">생성자 지원 여부까지 확인할지.</param>
    private OriginalTypeRecord Type(uint number, bool requireSupported)
    {
        IReadOnlyList<OriginalTypeRecord> current = CurrentTypes();
        if (number < OriginalConstructors.FirstAssetType || number >= current.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(number), number, "base 오브젝트 타입 번호 오류");
        }
        OriginalTypeRecord type = current[(int)number];
        if (requireSupported && type.ConstructorAddress != 0 && OriginalConstructors.Recipe(_pool.Edition, type.ConstructorAddress) is null)
        {
            throw new InvalidOperationException("null/assert 또는 미복원 생성자는 지원하지 않습니다");
        }
        return type;
    }

    /// <summary>
    /// 사용 중인 슬롯의 바이트를 돌려준다. Take 로 받은 예측 머리 같은 슬롯도 허용하며, 5 번 미만·빈·contained 슬롯과
    /// 타입 바이트가 자산 타입 밖인 슬롯은 거부한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private Span<byte> LiveBytes(int sid)
    {
        Span<byte> bytes = _pool.RawSlot(sid);
        if (sid < RawSidPool.FirstOrdinary || (bytes[RawSidPool.StateOffset] & (RawSidPool.FreeBit | ContainedBit)) != 0)
        {
            throw new InvalidOperationException("base 초기화의 번호 상태 오류");
        }
        Type(bytes[RawSidPool.TypeOffset], requireSupported: false);
        return bytes;
    }

    /// <summary>
    /// 타입의 최종 가상 표 값. 생성자 주소가 0 이면 판본별 기본 Squid, 내장 Form 주소이면 Form 가상 표, 아니면 판본별 표의 파생 생성자가 마지막에 쓰는 값이다.
    /// 표에 없는 생성자 주소는 원문처럼 명시 오류로 거부한다(호스트 역참조 전에 검사한다).
    /// 근거: cpppj/src/o/SquidFactory.cpp ConstructorVtable (R0-4 재대조, 2026-10-28; 판본별 표 F2c-11, 2026-11-04; Form 분기 F2d 0.215, 2026-11-07).
    /// </summary>
    /// <param name="type">타입 레코드.</param>
    private uint ConstructorVtable(OriginalTypeRecord type)
    {
        if (type.ConstructorAddress == 0)
        {
            return OriginalConstructors.BaseVtableFor(_pool.Edition);
        }
        // 내장 Form 네 종류는 일반 Create·Take도 원문 생성자·가상 표를 쓴다
        foreach (uint formType in FormTypes)
        {
            if (type.ConstructorAddress == OriginalConstructors.Address(_pool.Edition, (int)formType))
            {
                return FormVtable(formType);
            }
        }
        ConstructorRecipe? recipe = OriginalConstructors.Recipe(_pool.Edition, type.ConstructorAddress);
        if (recipe is null)
        {
            throw new InvalidOperationException("현재 TYPE의 미등록 생성자 가상 표 조회");
        }
        return recipe.Vtable;
    }

    /// <summary>
    /// 타입의 생성자 쓰기를 슬롯에 순서대로 적용한다. 주소가 0 이면 판본별 기본 가상 표 하나만 쓴다.
    /// OR 쓰기는 같은 폭의 기존 바이트를 읽어 합치며 이웃 필드는 건드리지 않는다. 호출 전에 지원 여부를 검사해야 한다.
    /// </summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="type">타입 레코드.</param>
    private void ApplyConstructor(Span<byte> bytes, OriginalTypeRecord type)
    {
        if (type.ConstructorAddress == 0)
        {
            Put(bytes, VtableOffset, OriginalConstructors.BaseVtableFor(_pool.Edition), 4);
            return;
        }
        // 내장 Form은 가상 표 DWORD만 남긴다
        foreach (uint formType in FormTypes)
        {
            if (type.ConstructorAddress == OriginalConstructors.Address(_pool.Edition, (int)formType))
            {
                Put(bytes, VtableOffset, FormVtable(formType), 4);
                return;
            }
        }
        // 쓰기 순서와 폭을 유지해 섬 번호·플래그·anim HP 처리를 원본대로 한다
        foreach (ConstructorWrite write in OriginalConstructors.Recipe(_pool.Edition, type.ConstructorAddress)!.Writes)
        {
            uint value = write.Value;
            if (write.OrBits)
            {
                // OR 에 필요한 기존 바이트만 읽는다
                for (int i = 0; i < write.Width; i++)
                {
                    value |= (uint)bytes[write.Offset + i] << (i * 8);
                }
            }
            Put(bytes, write.Offset, value, write.Width);
        }
    }

    /// <summary>내장 Form 타입 번호인지 판단한다.</summary>
    /// <param name="number">확인할 타입 번호.</param>
    private static bool IsFormTypeNumber(uint number)
    {
        // 고정된 네 종류만 Form 경로를 쓴다
        foreach (uint formType in FormTypes)
        {
            if (number == formType)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>원문 생성자 주소가 내장 Form 네 종류 중 하나인지 판단한다.</summary>
    /// <param name="address">확인할 생성자 주소.</param>
    private bool IsFormAddress(uint address)
    {
        // 판본별 주소 표와 직접 대조한다
        foreach (uint formType in FormTypes)
        {
            if (address == OriginalConstructors.Address(_pool.Edition, (int)formType))
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>내장 Form 타입의 실제 가상 표 기록값을 돌려준다. CD판은 명시 경계로 둔다.</summary>
    /// <param name="formType">내장 Form 타입 번호.</param>
    private uint FormVtable(uint formType)
    {
        if (_pool.Edition != OriginalEdition.Patch1078)
        {
            throw new InvalidOperationException("CD Form 가상 표 미연결");
        }

        return RawSquidForms.VtableFor(formType);
    }

    /// <summary>가상 표 값이 내장 Form 네 종류 중 하나인지 판단한다.</summary>
    /// <param name="table">확인할 가상 표 값.</param>
    private bool IsFormVtable(uint table)
    {
        if (_pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        // 네 종류의 패치 가상 표와 대조한다
        foreach (uint formType in FormTypes)
        {
            if (table == RawSquidForms.VtableFor(formType))
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>같은 판본의 기본·검증된 생성자 표에 있는 현재 가상 표인지 확인한다. 타입과 가상 표의 쌍 일치는 강제하지 않는다. 2026-11-07 F2d 공장 현재 생성 이식에서 추가됐다.</summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="table">확인할 가상 표 값이다.</param>
    /// <returns>같은 판본의 실제 최종 가상 표이면 참이다.</returns>
    public static bool SupportsConstructedVtable(OriginalEdition edition, uint table)
    {
        if (table == OriginalConstructors.BaseVtableFor(edition))
        {
            return true;
        }

        // 현재 판본의 실제 최종 가상 표만 허용한다
        IReadOnlyList<ConstructorRecipe> recipes = edition == OriginalEdition.Cd1072
            ? OriginalConstructors.CdRecipes
            : OriginalConstructors.Recipes;
        foreach (ConstructorRecipe recipe in recipes)
        {
            if (recipe.Vtable == table)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>진단 뒤에도 같은 현재 표를 조회한다. 빌린 표가 없으면 기존 독립 소유 표를 복사 없이 읽는다. 2026-11-07 F2d 공장 진단 묶음 이식에서 추가됐다.</summary>
    /// <returns>현재 타입 표다.</returns>
    private IReadOnlyList<OriginalTypeRecord> CurrentTypes()
    {
        if (_diagnostics?.Types is { Count: > 0 } current)
        {
            return current;
        }

        return _types;
    }

    /// <summary>타입 번호의 레코드를 범위만 확인해 찾는다. Form도 허용한다.</summary>
    /// <param name="number">타입 번호.</param>
    private OriginalTypeRecord TypeRecord(uint number)
    {
        IReadOnlyList<OriginalTypeRecord> current = CurrentTypes();
        if (number >= (uint)current.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(number), number, "공장 현재 TYPE 번호 오류");
        }

        return current[(int)number];
    }

    /// <summary>Create·Take가 쓰는 타입 조회다. 내장 Form(1·2·3·6) 또는 자산(70 이상)만 받고 지원 여부를 미리 검사한다.</summary>
    /// <param name="type">타입 번호.</param>
    private OriginalTypeRecord TypeForCreate(uint type)
    {
        IReadOnlyList<OriginalTypeRecord> current = CurrentTypes();
        if (type >= (uint)current.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "공장 현재 TYPE 번호 오류");
        }

        if (!IsFormTypeNumber(type) && type < (uint)OriginalConstructors.FirstAssetType)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "base 오브젝트 타입 번호 오류");
        }

        OriginalTypeRecord record = current[(int)type];
        if (record.ConstructorAddress != 0 && !IsFormAddress(record.ConstructorAddress)
            && OriginalConstructors.Recipe(_pool.Edition, record.ConstructorAddress) is null)
        {
            throw new InvalidOperationException("null/assert 또는 미복원 생성자는 지원하지 않습니다");
        }

        return record;
    }

    /// <summary>현재 슬롯의 타입이 가리키는 생성자만 실행하고 반환 객체는 무시한다. 타입·상태 선행 검사를 하지 않는다. 2026-11-07 F2d 공장 현재 생성 이식에서 추가됐다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <returns>같은 슬롯 번호다.</returns>
    public int ConstructCurrent(int sid)
    {
        uint address = TypeRecord(_pool.RawSlot(sid)[RawSidPool.TypeOffset]).ConstructorAddress;
        return ConstructRaw(address, sid, false);
    }

    /// <summary>원문 cdecl 주소를 타입·상태 선행 검사 없이 같은 물리 슬롯에 적용한다. Form은 가상 표 DWORD만 남긴다.</summary>
    /// <param name="address">원문 생성자 주소.</param>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="requireObject">참이면 void NULL 생성자의 객체 반환을 거부한다.</param>
    public int ConstructRaw(uint address, int sid, bool requireObject)
    {
        if (address == 0)
        {
            Span<byte> baseBytes = _pool.RawSlot(sid);
            Put(baseBytes, VtableOffset, OriginalConstructors.BaseVtableFor(_pool.Edition), 4);
            return sid;
        }

        // 네 Form 생성자는 TYPE·부모·목록을 쓰지 않고 가상 표 DWORD만 남긴다
        foreach (uint formType in FormTypes)
        {
            if (address == OriginalConstructors.Address(_pool.Edition, (int)formType))
            {
                Span<byte> formBytes = _pool.RawSlot(sid);
                Put(formBytes, VtableOffset, FormVtable(formType), 4);
                return sid;
            }
        }

        if (_pool.Edition == OriginalEdition.Patch1078 && address == NullConstructorAddress)
        {
            if (requireObject)
            {
                throw new InvalidOperationException("void NULL 생성자는 Create의 반환 객체를 제공하지 않습니다");
            }

            return sid;
        }

        // 패치판 받침 보조 생성자는 진단 정상 복귀 뒤 같은 슬롯에 가상 표만 쓴다
        if (_pool.Edition == OriginalEdition.Patch1078 && address == DaisAuxAddress)
        {
            Action<string, string, uint>? assertion = _diagnostics?.Assertion;
            if (assertion is null)
            {
                throw new InvalidOperationException("Dais 보조 생성자의 실제 진단 장치 미연결");
            }

            assertion("false", ".\\Dais.cpp", 0x484);
            Span<byte> daisBytes = _pool.RawSlot(sid);
            Put(daisBytes, VtableOffset, DaisAuxVtable, 4);
            return sid;
        }

        ConstructorRecipe? recipe = OriginalConstructors.Recipe(_pool.Edition, address);
        if (recipe is null)
        {
            throw new InvalidOperationException("Create/현재 TYPE의 선택된 cdecl 생성자 미연결");
        }

        Span<byte> bytes = _pool.RawSlot(sid);
        // 쓰기 순서와 폭을 유지해 원본대로 적용한다
        foreach (ConstructorWrite write in recipe.Writes)
        {
            uint value = write.Value;
            if (write.OrBits)
            {
                // OR 에 필요한 기존 바이트만 읽는다
                for (int i = 0; i < write.Width; i++)
                {
                    value |= (uint)bytes[write.Offset + i] << (i * 8);
                }
            }

            Put(bytes, write.Offset, value, write.Width);
        }

        return sid;
    }

    /// <summary>실제 가상 표를 보고 Form이면 빈 후처리로 끝내고 자산이면 공통 후처리를 실행한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="table">생성 직후 읽은 가상 표 값.</param>
    private void VirtualPostCreate(int sid, uint table)
    {
        // 네 Form의 후처리는 빈 함수다
        if (IsFormVtable(table))
        {
            return;
        }

        PostCreate(sid);
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 4 의 배수가 아닌 위치(+26 의 HP 등)에도 안전하다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>슬롯 +0 의 가상 표 값(4바이트 리틀 엔디언)을 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    private static uint ReadVtable(ReadOnlySpan<byte> bytes) => (uint)(bytes[VtableOffset] | (bytes[VtableOffset + 1] << 8)
        | (bytes[VtableOffset + 2] << 16) | (bytes[VtableOffset + 3] << 24));
}
