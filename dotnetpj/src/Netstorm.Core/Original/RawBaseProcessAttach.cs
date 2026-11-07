// BaseProcess의 로컬 신규 생성과 지정 Form 분기를 같은 원문 부모로 표현한다.
namespace Netstorm.Core.Original;

// 부착 부모 모듈을 담는 네임스페이스다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.

// C# 객체의 실제 두 SID 필드이며 callback 뒤에도 현재 값을 읽는다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.
public sealed class BaseProcessAttachFields
{
    /// <summary>원문 this+4의 호스트 SID 표현이다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public int Form;

    /// <summary>원문 this+8의 호스트 SID 표현이다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public int Parent;
}

// 실패 시 이미 쓴 필드·등록·슬롯을 복구하지 않는다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.
public sealed class BaseProcessAttachHooks
{
    /// <summary>최초 타입 진단이 읽는 현재 signed 상한 DWORD다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Func<uint>? TypeEnd { get; set; }

    /// <summary>신규 생성·타입 변경이 읽는 현재 번호다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Func<uint>? ProcessFormType { get; set; }

    /// <summary>필드·부모 상태 뒤 커널에 등록하고 PID를 반환한다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Func<uint>? RegisterProcess { get; set; }

    /// <summary>원문 할당 플래그 0·2로 새 Form을 만든다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Func<uint, int>? Create { get; set; }

    /// <summary>지정 빈 슬롯의 현재 공통 Take다. 반환 객체는 부모가 사용하지 않는다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Action<int>? Take { get; set; }

    /// <summary>지정 슬롯의 현재 부모 getter다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Func<int, int>? Parent { get; set; }

    /// <summary>현재 가상 +74를 선택한다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Action<int, int, uint>? Attach { get; set; }

    /// <summary>생성·지정 검증 뒤의 실제 프로세스 가상 통지다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Action? OnAttached { get; set; }

    /// <summary>진단이 정상 복귀하면 원문 다음 위치로 진행한다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    /// <returns>복사된 훅 묶음이다.</returns>
    public BaseProcessAttachHooks Clone() => new()
    {
        TypeEnd = TypeEnd,
        ProcessFormType = ProcessFormType,
        RegisterProcess = RegisterProcess,
        Create = Create,
        Take = Take,
        Parent = Parent,
        Attach = Attach,
        OnAttached = OnAttached,
        Assertion = Assertion,
    };
}

// 원문 필드·현재 번호와 선택 장치를 빌리는 부착 부모다. C++ 소유권의 최종 반납은 Host·커널에 남긴다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.
public static class RawBaseProcessAttach
{
    /// <summary>선택된 실제 장치가 없으면 명시 실패한다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    /// <param name="device">쓸 장치다.</param>
    private static T Need<T>(T? device) where T : class
    {
        if (device is null)
        {
            throw new InvalidOperationException("BaseProcess 부착의 선택된 실제 장치 미연결");
        }

        return device;
    }

    /// <summary>비정렬 PID DWORD 전체다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    private static uint GetPid(ReadOnlySpan<byte> raw) =>
        (uint)raw[18] | ((uint)raw[19] << 8) | ((uint)raw[20] << 16) | ((uint)raw[21] << 24);

    /// <summary>PID 밖 payload는 보존한다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="pid">쓸 PID다.</param>
    private static void PutPid(Span<byte> raw, uint pid)
    {
        // 낮은 바이트부터 지정 폭만 쓰는 반복문이다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            raw[18 + index] = unchecked((byte)(pid >> (index * 8)));
        }
    }

    /// <summary>타입 진단→필드 쓰기→부모 상태→커널→Form→가상 통지 순서와 부분 실패를 유지한다. 패치 비활성 부모 조기 반환만 거짓이다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="fields">호스트 SID 필드다. 실패해도 쓴 값은 둔다.</param>
    /// <param name="processType">부착할 프로세스 타입 번호다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="form">고를 Form SID다. 0이면 신규 생성이다.</param>
    /// <param name="flags">원문 플래그다.</param>
    /// <param name="hooks">선택 장치 묶음이다.</param>
    /// <returns>부착했으면 참이다.</returns>
    public static bool Attach(RawSidPool pool, BaseProcessAttachFields fields, uint processType, int parent, int form, uint flags, BaseProcessAttachHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(fields);
        ArgumentNullException.ThrowIfNull(hooks);
        bool patch = pool.Edition == OriginalEdition.Patch1078;
        void Diagnose(string expression, uint patchLine, uint cdLine) =>
            Need(hooks.Assertion)(expression, patch ? "\\Ns\\O\\Baseprocess.cpp" : "\\ns\\o\\BaseProcess.cpp", patch ? patchLine : cdLine);
        int signedType = unchecked((int)processType);
        if (signedType < 10 || unchecked((int)Need(hooks.TypeEnd)()) <= signedType)
        {
            Diagnose("isProcessType(_processType)", 0x1cu, 0x1cu);
        }

        fields.Parent = parent;
        fields.Form = form;
        Span<byte> parentRaw = pool.RawSlot(parent);
        if (patch)
        {
            if ((parentRaw[11] & 3u) != 0u)
            {
                return false;
            }
        }
        else
        {
            if ((parentRaw[11] & 1u) != 0u)
            {
                Diagnose("!base.isFree()", 0u, 0x38u);
            }

            if ((parentRaw[11] & 2u) != 0u)
            {
                Diagnose("!base.isDead()", 0u, 0x39u);
            }
        }

        uint pid = Need(hooks.RegisterProcess)();
        if (fields.Form == 0)
        {
            int created = Need(hooks.Create)((flags & 0x10u) != 0u ? 2u : 0u);
            fields.Form = created;
            Span<byte> raw = pool.RawSlot(created);
            void TypeDiagnose(string expression, uint patchLine, uint cdLine) =>
                Need(hooks.Assertion)(expression, patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp", patch ? patchLine : cdLine);
            if (signedType < 0 || signedType > 0xfe)
            {
                TypeDiagnose("0 <= pType && pType < MAX_TYPES", 0xab7u, 0xa6eu);
            }

            if (raw[10] != Need(hooks.ProcessFormType)())
            {
                TypeDiagnose("typeNum == processFormType", 0xab8u, 0xa6fu);
            }

            raw[10] = unchecked((byte)processType);
            PutPid(raw, pid);
            Need(hooks.Attach)(created, fields.Parent, flags | 0x80u);
            Need(hooks.OnAttached)();
            return true;
        }

        int selected = fields.Form;
        Span<byte> selectedRaw = pool.RawSlot(selected);
        if ((selectedRaw[11] & 1u) != 0u)
        {
            Need(hooks.Take)(selected);
            selectedRaw[10] = unchecked((byte)processType);
            PutPid(selectedRaw, pid);
            Need(hooks.Attach)(selected, fields.Parent, flags | 0x80u);
        }

        // getter가 복귀한 뒤 현재 프로세스 부모 필드를 읽는다
        int currentParent = Need(hooks.Parent)(selected);
        if (currentParent != fields.Parent)
        {
            Diagnose("s.getParentSid() == attachToSid", 0x5eu, 0x59u);
        }

        if (GetPid(selectedRaw) != pid)
        {
            Diagnose("s.getPid() == pid", 0x5fu, 0x5au);
        }

        Need(hooks.OnAttached)();
        return true;
    }
}
